#include <Python.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// objdump -S --disassemble blobdetectmodule.o > ass.dump

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;
typedef char				i8;
typedef short				i16;
typedef int					i32;
typedef long long			i64;
typedef float				f32;
typedef double				f64;

volatile char* 		masterIp = 0;
volatile bool 		timeSyncReset = false;
volatile bool 		timeSyncRunning = false;
pthread_mutex_t		timeSyncMutex;
pthread_cond_t		timeSyncSignal;

//------------------------------------------------------------------------------------------------------------
// Blob detect.
//------------------------------------------------------------------------------------------------------------
uint8_t _frameBuffer[1024 * 704];
uint8_t _maskBuffer[64 * 44];
i64 	_frameId = 0;
f32		_avgMasterOffset = 0.0f;

pthread_t 			_workerThread;
pthread_mutex_t		_queueMutex;
pthread_cond_t		_queueSignal;
volatile uint32_t	_frameStatus = 0;

typedef struct 
{
	float minX;
	float minY;
	float maxX;
	float maxY;
	float x;
	float y;

} blob;

int		frameTimeUs;
int 	blobCount = 0;
blob 	blobs[1024];

struct sendDataHeader
{
	i64	frameId;
	f32 avgMasterOffset;
	i32 blobCount;
	i32 regionCount;
	i32 foundRegionCount;
	i32 totalTime;
};

u8 sendDataBuffer[1024 * 1024];

//------------------------------------------------------------------------------------------------------------
// Time sync.
//------------------------------------------------------------------------------------------------------------
pthread_t 		timeSyncThread;

i64				rttBuffer[100];
i64				rttBufferSorted[100];
i32 			rttBufferIdx = 0;
i64 			rttMedian = 0;
float 			rttMedianFiltered = 0.0f;

i64 			offsetBuffer[100];
i64 			offsetBufferSorted[100];
i32 			offsetBufferIdx = 0;
i64				offsetMedian = 0;
volatile float	offsetMedianFiltered = 0.0f;

struct timeEntry
{
	i64 localTime;
	i64 masterTime;
};

volatile u64 startTimeUs = 0;
volatile i64 startMasterUs = 0;
volatile float avgHostOffset = 0.0f;

//------------------------------------------------------------------------------------------------------------
// Utility functions.
//------------------------------------------------------------------------------------------------------------
inline u64 GetUS()
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

inline int64_t PlatformGetMicrosecond(void)
{
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

inline int32_t PlatformGetMS(void)
{
	return (PlatformGetMicrosecond() / 1000);
}

float distSq(float X1, float Y1, float X2, float Y2)
{
	return (X2 - X1) * (X2 - X1) + (Y2 - Y1) * (Y2 - Y1);
}

//------------------------------------------------------------------------------------------------------------
// Marker processing.
//------------------------------------------------------------------------------------------------------------
struct region
{
	u8 	id;
	i32 minX;
	i32 minY;
	i32 maxX;
	i32 maxY;
	i32 width;
	i32 height;
	i32 pixelIdx;
	i32 pixelCount;
	u8 	maxLum;
};

region foundRegions[1024];
i32 foundRegionCount = 0;

struct regionPixel
{
	i16	x;
	i16	y;
	u8	value;
};

struct floodNode
{
	i16 x;
	i16 y;
	u8 lum;
};

struct segment 
{
	u8 id;
	i32 x;
	i32 y;
	i32 lowMark;
	i32 highMark;
	i32 pixelCount;
	regionPixel pixels[1024 * 704];
};

u8			regionMarks[1024 * 704];
regionPixel regionPixels[1024 * 704];
i32			regionPixelCount;
region		regions[1024 * 1024];
i32 		regionCount;
region		regionCurrent;

floodNode	floodList[1024 * 1024];
i32			floodListCount;

struct sclSegment
{
	i32 	startX;
	i32 	endX;
	i32 	y;
	i8		dir;
	bool 	scanLeft;
	bool 	scanRight;
};

sclSegment	sclSegments[10240];
i32			sclSegmentCount;

u8			tempData[100 * 100];
i32			tempDataWidth;
i32			tempDataHeight;
u8			tempMarks[100 * 100];

u8*			mapData;
i32			mapWidth;
i32			mapHeight;
f32			mapLumScale;
i32			mapLumOffset;

segment		segments[8];
i32			segmentCount;

//------------------------------------------------------------------------------------------------------------
// Flood to find minima regions.
//------------------------------------------------------------------------------------------------------------
inline void addSegmentPixel(segment* S, i32 X, i32 Y, i32 Idx)
{
	S->pixels[S->pixelCount].x = X;
	S->pixels[S->pixelCount].y = Y;
	S->pixels[S->pixelCount++].value = tempData[Idx];
	tempMarks[Idx] = 255;
}

// Process next low marker.
void watershedFloodUpdate(segment* S, u8 Threshold)
{
	regionPixel* p = &S->pixels[S->lowMark++];
	
	i32 X = p->x;
	i32 Y = p->y;

	i32 idx0 = (Y - 1) * tempDataWidth + (X - 1);
	i32 idx1 = (Y - 1) * tempDataWidth + (X - 0);
	i32 idx2 = (Y - 1) * tempDataWidth + (X + 1);
	i32 idx3 = (Y - 0) * tempDataWidth + (X - 1);
	i32 idx4 = (Y - 0) * tempDataWidth + (X + 1);
	i32 idx5 = (Y + 1) * tempDataWidth + (X - 1);
	i32 idx6 = (Y + 1) * tempDataWidth + (X - 0);
	i32 idx7 = (Y + 1) * tempDataWidth + (X + 1);

	if ((X > 0 && Y > 0) && (tempMarks[idx0] == 0 && tempData[idx0] >= Threshold)) addSegmentPixel(S, X - 1, Y - 1, idx0);
	if ((Y > 0) && (tempMarks[idx1] == 0 && tempData[idx1] >= Threshold)) addSegmentPixel(S, X - 0, Y - 1, idx1);
	if ((X < tempDataWidth - 1 && Y > 0) && (tempMarks[idx2] == 0 && tempData[idx2] >= Threshold)) addSegmentPixel(S, X + 1, Y - 1, idx2);
	if ((X > 0) && (tempMarks[idx3] == 0 && tempData[idx3] >= Threshold)) addSegmentPixel(S, X - 1, Y - 0, idx3);
	if ((X < tempDataWidth - 1) && (tempMarks[idx4] == 0 && tempData[idx4] >= Threshold)) addSegmentPixel(S, X + 1, Y - 0, idx4);
	if ((X > 0 && Y < tempDataHeight - 1) && (tempMarks[idx5] == 0 && tempData[idx5] >= Threshold)) addSegmentPixel(S, X - 1, Y + 1, idx5);
	if ((Y < tempDataHeight - 1) && (tempMarks[idx6] == 0 && tempData[idx6] >= Threshold)) addSegmentPixel(S, X - 0, Y + 1, idx6);
	if ((X < tempDataWidth - 1 && Y < tempDataHeight - 1) && (tempMarks[idx7] == 0 && tempData[idx7] >= Threshold)) addSegmentPixel(S, X + 1, Y + 1, idx7);
}

void watershedRegion()
{
	segmentCount = 0;

	i32 peak = 170;

	for (int iY = 0; iY < regionCurrent.height; ++iY)
	{
		for (int iX = 0; iX < regionCurrent.width; ++iX)
		{
			i32 idx = iY * regionCurrent.width + iX;

			if (tempData[idx] >= peak && tempMarks[idx] == 0)
			{
				segment* s = &segments[segmentCount];

				s->id = segmentCount;
				s->x = regionCurrent.minX;
				s->y = regionCurrent.minY;
				s->lowMark = 0;
				s->highMark = 0;
				s->pixelCount = 1;
				s->pixels[0].x = iX;
				s->pixels[0].y = iY;
				s->pixels[0].value = tempData[idx];
				tempMarks[idx] = 255;

				//std::cout << "New segment " << (i32)s->id << " " << iX << ", " << iY << "\n";

				while (s->lowMark != s->pixelCount)
				{
					//std::cout << "Itr " << s->lowMark << s->pixelCount
					s->highMark = s->pixelCount;
					watershedFloodUpdate(s, peak);
				}

				segmentCount++;
				if (segmentCount == 8)
				{
					printf("Max Segs\n");
					return;
				}
			}
		}
	}

	// Fill the segments
	for (int iS = 0; iS < segmentCount; ++iS)
	{
		segments[iS].lowMark = 0;
		segments[iS].highMark = segments[iS].pixelCount;
	}

	while (true)
	{
		bool completed = true;

		for (int iS = 0; iS < segmentCount; ++iS)
		{
			segment* s = &segments[iS];

			if (s->lowMark != s->highMark)
			{
				while (s->lowMark != s->highMark)
				{
					watershedFloodUpdate(s, 1);
				}

				s->highMark = s->pixelCount;
				completed = false;
			}
		}

		if (completed)
			break;
	}
}

//------------------------------------------------------------------------------------------------------------
// Flood to find initial connected regions.
//------------------------------------------------------------------------------------------------------------
inline void addFloodNode(i32 X, i32 Y, u8 Lum)
{
	floodNode fn = {};
	fn.x = X;
	fn.y = Y;
	fn.lum = Lum;
	floodList[floodListCount++] = fn;
}

//inline void floodAddRegionPixel(i32 X, i32 Y, i32 Idx)
inline void floodAddRegionPixel(i32 X, i32 Y, u8 Lum)
{
	// Add pixel to region.
	regionPixel rp = {};
	rp.x = X;
	rp.y = Y;
	rp.value = (u8)((Lum - 32) * mapLumScale);
	//rp.value = (u8)((mapData[Idx] - 32) * mapLumScale);
	//rp.value = mapData[Idx];
	regionPixels[regionPixelCount++] = rp;
	regionCurrent.pixelCount++;

	if (X < regionCurrent.minX) regionCurrent.minX = X;
	else if (X > regionCurrent.maxX) regionCurrent.maxX = X;

	if (Y < regionCurrent.minY) regionCurrent.minY = Y;
	else if (Y > regionCurrent.maxY) regionCurrent.maxY = Y;

	if (rp.value > regionCurrent.maxLum) regionCurrent.maxLum = rp.value;
}

void flood(i32 X, i32 Y, u8 Lum)
{
	// Add pixel to region.
	floodAddRegionPixel(X, Y, Lum);

	// Remove this node from the process queue.
	floodListCount--;

	// Add next nodes to process.
	i32 idx1 = (Y - 1) * mapWidth + (X - 0);
	i32 idx3 = (Y - 0) * mapWidth + (X - 1);
	i32 idx4 = (Y - 0) * mapWidth + (X + 1);
	i32 idx6 = (Y + 1) * mapWidth + (X - 0);
	
	if (mapData[idx1] > mapLumOffset) { addFloodNode(X - 0, Y - 1, mapData[idx1]); mapData[idx1] = 0; }
	if (mapData[idx3] > mapLumOffset) { addFloodNode(X - 1, Y - 0, mapData[idx3]); mapData[idx3] = 0; }
	if (mapData[idx4] > mapLumOffset) { addFloodNode(X + 1, Y - 0, mapData[idx4]); mapData[idx4] = 0; }
	if (mapData[idx6] > mapLumOffset) { addFloodNode(X - 0, Y + 1, mapData[idx6]); mapData[idx6] = 0; }
}

inline bool scanLinePixelFloodable(i32 X, i32 Y)
{
	i32 idx = Y * mapWidth + X;
	return (regionMarks[idx] == 0 && mapData[idx] > 0);
}

void scanLineAddLine(i32 StartX, i32 EndX, i32 Y, i32 IgnoreStart, i32 IgnoreEnd, i8 Dir, bool IsNextInDir)
{
	i32 regionStart = -1;
	i32 x;

	for (x = StartX; x < EndX; ++x)
	{
		if ((IsNextInDir || x < IgnoreStart || x >= IgnoreEnd) && scanLinePixelFloodable(x, Y))
		{
			i32 idx = Y * mapWidth + x;
			regionMarks[idx] = 255;
			floodAddRegionPixel(x, Y, idx);

			if (regionStart < 0)
				regionStart = x;
		}
		else if (regionStart >= 0)
		{
			sclSegment seg = {};
			seg.startX = regionStart;
			seg.endX = x;	
			seg.y = Y;
			seg.dir = Dir;
			seg.scanLeft = (regionStart == StartX);
			seg.scanRight = false;
			sclSegments[sclSegmentCount++] = seg;

			regionStart = -1;
		}

		if (!IsNextInDir && x < IgnoreEnd && x >= IgnoreStart)
			x = IgnoreEnd - 1;
	}

	if (regionStart >= 0)
	{
		sclSegment seg = {};
		seg.startX = regionStart;
		seg.endX = x;	
		seg.y = Y;
		seg.dir = Dir;
		seg.scanLeft = (regionStart == StartX);
		seg.scanRight = true;
		sclSegments[sclSegmentCount++] = seg;
	}
}

void scanLineFlood(i32 X, i32 Y)
{
	i32 idx = Y * mapWidth + X;
	regionMarks[idx] = 255;
	floodAddRegionPixel(X, Y, idx);

	sclSegmentCount = 0;

	sclSegment seg = {};
	seg.startX = X;
	seg.endX = X + 1;	
	seg.y = Y;
	seg.dir = 0;
	seg.scanLeft = true;
	seg.scanRight = true;
	sclSegments[sclSegmentCount++] = seg;

	do
	{
		sclSegment* r = &sclSegments[--sclSegmentCount];
		i32 startX = r->startX;
		i32 endX = r->endX;

		if (r->scanLeft)
		{
			while (startX > 0)
			{
				i32 idx = r->y * mapWidth + (startX - 1);

				if (regionMarks[idx] == 255 || mapData[idx] == 0)
					break;
				
				regionMarks[idx] = 255;
				floodAddRegionPixel(--startX, r->y, idx);
			}
		}

		if (r->scanRight)
		{
			while (endX < mapWidth)
			{
				i32 idx = r->y * mapWidth + endX;

				if (regionMarks[idx] == 255 || mapData[idx] == 0)
					break;

				regionMarks[idx] = 255;
				floodAddRegionPixel(endX++, r->y, idx);
			}
		}

		r->startX--;
		r->endX++;

		if (r->y > 0)
			scanLineAddLine(startX, endX, r->y - 1, r->startX, r->endX, -1, r->dir <= 0);
		
		if (r->y < mapHeight - 1)
			scanLineAddLine(startX, endX, r->y + 1, r->startX, r->endX, 1, r->dir >= 0);
		
	} while (sclSegmentCount != 0);
}

//------------------------------------------------------------------------------------------------------------
// Non-recursive scan line fill.
// https://www.codeproject.com/Articles/6017/QuickFill-An-efficient-flood-fill-algorithm
//------------------------------------------------------------------------------------------------------------
#define nMinX 0
#define nMaxX (mapWidth - 1)
#define nMinY 0
#define nMaxY (mapHeight - 1)

#define SLF_MAXDEPTH 10000
#define SLF_PUSH(XL, XR, Y, DY) \
	if( sp < stack + SLF_MAXDEPTH && Y+(DY) >= nMinX && Y+(DY) <= nMaxY ) \
	{ sp->x1 = XL; sp->x2 = XR; sp->y = Y; sp->dy = DY; ++sp; }

#define SLF_POP(XL, XR, Y, DY) \
	{ --sp; XL = sp->x1; XR = sp->x2; Y = sp->y+(DY = sp->dy); }

struct slfLine
{
	int x1;
	int x2;
	int y;
	int dy;
};

inline void slfAddRegionPixel(i32 X, i32 Y)
{
	i32 idx = Y * mapWidth + X;
	
	// Add pixel to region.
	regionPixel rp = {};
	rp.x = X;
	rp.y = Y;
	rp.value = (u8)((mapData[idx] - 32) * mapLumScale);
	regionPixels[regionPixelCount++] = rp;
	regionCurrent.pixelCount++;

	mapData[idx] = 0;

	if (X < regionCurrent.minX) regionCurrent.minX = X;
	else if (X > regionCurrent.maxX) regionCurrent.maxX = X;

	if (Y < regionCurrent.minY) regionCurrent.minY = Y;
	else if (Y > regionCurrent.maxY) regionCurrent.maxY = Y;

	if (rp.value > regionCurrent.maxLum) regionCurrent.maxLum = rp.value;
}

inline bool slfPixelFloodable(i32 X, i32 Y)
{
	i32 idx = Y * mapWidth + X;
	return (mapData[idx] > mapLumOffset);
}

// Fill background with given color
void slfFill(int x, int y)
{
    int left, x1, x2, dy;
    slfLine stack[SLF_MAXDEPTH];
	slfLine* sp = stack;
	
    SLF_PUSH(x, x, y, 1);        /* needed in some cases */
    SLF_PUSH(x, x, y + 1, -1);    /* seed segment (popped 1st) */

	while (sp > stack)
	{
        SLF_POP(x1, x2, y, dy);

		for (x = x1; x >= nMinX && slfPixelFloodable(x, y); --x)
		{
			slfAddRegionPixel(x, y);
		}

		if( x >= x1 )
		    goto SKIP;

        left = x + 1;
        if (left < x1)
			SLF_PUSH(y, left, x1 - 1, -dy);    /* leak on left? */

        x = x1 + 1;

		do 
		{
            for (; x <= nMaxX && slfPixelFloodable(x, y); ++x)
				slfAddRegionPixel(x, y);

			SLF_PUSH(left, x - 1, y, dy);

            if (x > x2+1)
				SLF_PUSH(x2 + 1, x - 1, y, -dy);    /* leak on right? */

SKIP:        for (++x; x <= x2 && !slfPixelFloodable(x, y); ++x) {;}

            left = x;
		} 
		while (x <= x2);
    }
}

//------------------------------------------------------------------------------------------------------------
// Scan line flood fill with stack
// http://lodev.org/cgtutor/floodfill.html#Scanline_Floodfill_Algorithm_With_Stack
//------------------------------------------------------------------------------------------------------------
void floodFillPush(int X, int Y)
{
	floodNode fn;
	fn.x = X;
	fn.y = Y;
	floodList[floodListCount++] = fn;
}

bool floodFillPop(int& X, int& Y)
{
	if (floodListCount > 0)
	{
		X = floodList[--floodListCount].x;
		Y = floodList[floodListCount].y;

		return true;
	}
	else 
	{
		return false;
	}
}

inline bool floodFillPixelFloodable(i32 X, i32 Y)
{
	i32 idx = Y * mapWidth + X;
	return (mapData[idx] > mapLumOffset);
}

inline void floodFillAddRegionPixel(i32 X, i32 Y)
{
	i32 idx = Y * mapWidth + X;
	
	// Add pixel to region.
	regionPixel rp = {};
	rp.x = X;
	rp.y = Y;
	rp.value = (u8)((mapData[idx] - 32) * mapLumScale);
	regionPixels[regionPixelCount++] = rp;
	regionCurrent.pixelCount++;

	mapData[idx] = 0;

	if (X < regionCurrent.minX) regionCurrent.minX = X;
	else if (X > regionCurrent.maxX) regionCurrent.maxX = X;

	if (Y < regionCurrent.minY) regionCurrent.minY = Y;
	else if (Y > regionCurrent.maxY) regionCurrent.maxY = Y;

	if (rp.value > regionCurrent.maxLum) regionCurrent.maxLum = rp.value;
}

void floodFillScanlineStack(int x, int y)
{
	int x1;
	bool spanAbove, spanBelow;
	
	floodListCount = 0;

	floodFillPush(x, y);
	
	while (floodFillPop(x, y))
	{
		x1 = x;
		while (x1 >= 0 && floodFillPixelFloodable(x1, y)) x1--;		
		x1++;		
		spanAbove = spanBelow = 0;
		
		while (x1 < mapWidth && floodFillPixelFloodable(x1, y))
		{
			floodFillAddRegionPixel(x1, y);
			
			if(!spanAbove && y > 0 && floodFillPixelFloodable(x1, y - 1))
			{
				floodFillPush(x1, y - 1);
				spanAbove = 1;
			}
			else if(spanAbove && y > 0 && !floodFillPixelFloodable(x1, y - 1))
			{
				spanAbove = 0;
			}
			
			if(!spanBelow && y < mapHeight - 1 && floodFillPixelFloodable(x1, y + 1))
			{
				floodFillPush(x1, y + 1);
				spanBelow = 1;
			}
			else if(spanBelow && y < mapHeight - 1 && !floodFillPixelFloodable(x1, y + 1))
			{
				spanBelow = 0;
			}

			x1++;
		}
	}
}

//------------------------------------------------------------------------------------------------------------
// Entire algo for locating centroid positions.
//------------------------------------------------------------------------------------------------------------
void process(i32 Pixels, i32 Width, i32 Height, u8* Data) 
{
	// NOTE: There is strange ringing on the YUV output from the Pi camera. Values are supposed to be clamped to
	// 16 to 235, but in practice we get values outside of this range. Values below 16 should be around 20 and
	// values above 235 should be around 220. 

	//int64_t t = PlatformGetMicrosecond();

	mapData = Data;
	mapWidth = Width;
	mapHeight = Height;
	blobCount = 0;
	foundRegionCount = 0;
	memset(tempMarks, 255, sizeof(tempMarks));
	//memset(regionMarks, 0, sizeof(regionMarks));
	regionCount = 0;
	regionPixelCount = 0;

	// Rescale data.
	// Remove everything below 32
	i32 startLevel = 32;
	f32 scale = 255.0f / (255 - startLevel);
	mapLumScale = scale;
	mapLumOffset = startLevel;

	//t = PlatformGetMicrosecond() - t;
	//printf("Setup %d\n", (int)t);

	//printf("Thershold and scale\n");
	
	
	//t = PlatformGetMicrosecond();
	
	/*
	for (int i = 0; i < Pixels; ++i)
	{
		if (Data[i] <= startLevel)
		{
			Data[i] = 0;
		}
		else
		{
			f32 p = Data[i];
			p = (p - startLevel) * scale;

			// NOTE: Magic branch.
			if (p < 0)
				p = 0;
			
			Data[i] = (u8)p;
		}
	}
	*/

	//t = PlatformGetMicrosecond() - t;
	//printf("Thresh time: %d\n", (int)t);
	
	//t = PlatformGetMicrosecond();

	//printf("Mask Buffer: %d\n ", _maskBuffer[0]);

	for (int mY = 0; mY < 44; ++mY)
	{
		for (int mX = 0; mX < 64; ++mX)
		{
			if (_maskBuffer[mY * 64 + mX] == 48)
			{
				for (int iY = mY * 16; iY < ((mY + 1) * 16); ++iY)
				{
					for (int iX = mX * 16; iX < ((mX + 1) * 16); ++iX)
					{
						Data[iY * Width + iX] = 0;
					}
				}				
			}
		}
	}
	
	// Create bounds border.
	// NOTE: Don't need this with new flood fill code.
	for (int i = 0; i < Width; ++i)
	{
		Data[i] = 0;
		Data[(Height - 1) * Width + i] = 0;
	}

	for (int i = 0; i < Height; ++i)
	{
		Data[i * Width + 0] = 0;
		Data[i * Width + Width - 1] = 0;
	}

	regionCurrent.minX = 2000;
	regionCurrent.minY = 2000;
	regionCurrent.maxX = 0;
	regionCurrent.maxY = 0;
	regionCurrent.pixelIdx = regionPixelCount;
	regionCurrent.pixelCount = 0;
	regionCurrent.maxLum = 0;
	regionCurrent.id = regionCount;

	//t = PlatformGetMicrosecond() - t;
	//printf("Border %d\n", (int)t);

	//printf("Region filling\n");

	int64_t tFlood = PlatformGetMicrosecond();

	i32 scaledThresh = (i32)(30.0 / scale) + startLevel;
	//i32 scaledThresh = 254;
	// Note: Flooding nothing takes 2.7ms

	// Flood fill to find regions above 0.
	for (int idx = 0; idx < Pixels; ++idx)
	{
		if (Data[idx] > scaledThresh)
		{
			int iY = idx / 1024;
			int iX = idx % 1024;
			
			//std::cout << "Flooding " << iX << ", " << iY << "\n";
			//printf("New flood region: %d, %d\n", iX, iY);

			int64_t t = PlatformGetMicrosecond();

			//scanLineFlood(iX, iY);
			//slfFill(iX, iY);
			floodFillScanlineStack(iX, iY);
			/*
			floodListCount = 1;
			flood(iX, iY, Data[idx]);
			Data[idx] = 0;

			while (floodListCount)
			{
				flood(floodList[floodListCount - 1].x, floodList[floodListCount - 1].y, floodList[floodListCount - 1].lum);
			}
			//*/

			//printf("Added region: %d Pixels: %d\n", regionCurrent.id, regionCurrent.pixelCount);

			//std::cout << "Added region " << (i32)regionCurrent.id << ": " << regionCurrent.pixelCount << "\n";

			regionCurrent.width = (regionCurrent.maxX + 1) - regionCurrent.minX;
			regionCurrent.height = (regionCurrent.maxY + 1) - regionCurrent.minY;
			foundRegions[foundRegionCount++] = regionCurrent;

			t = PlatformGetMicrosecond() - t;
			//printf("Region: %d,%d %d,%d (%d) %d us\n", regionCurrent.minX, regionCurrent.minY, regionCurrent.width, regionCurrent.height, regionCurrent.pixelCount, (int)t);

			// Reject small regions.
			if (regionCurrent.width >= 3 && regionCurrent.width < 100 && regionCurrent.height >= 3 && regionCurrent.height < 100)
			{
				//t = PlatformGetMicrosecond();

				tempDataWidth = regionCurrent.width;
				tempDataHeight = regionCurrent.height;
				memset(tempData, 0, regionCurrent.width * regionCurrent.height);
				memset(tempMarks, 0, regionCurrent.width * regionCurrent.height);

				// Apply auto contrast scale to region.
				float lumScale = 255.0f / regionCurrent.maxLum;
				
				// Copy region to temporary data buffer.
				for (int iP = 0; iP < regionCurrent.pixelCount; ++iP)
				{
					regionPixel* p = &regionPixels[regionCurrent.pixelIdx + iP];
					float pVal = (float)p->value * lumScale;

					i32 tempIdx = (p->y - regionCurrent.minY) * regionCurrent.width + (p->x - regionCurrent.minX);
					tempData[tempIdx] = (u8)pVal;
					p->value = (u8)pVal;
				}
				
				//std::cout << "Watershed Region " << (i32)regionCurrent.id << "\n";
				watershedRegion();
				//std::cout << "Region " << (i32)regionCurrent.id << " Segments " << segmentCount << "\n";

				// Calculate centroid from watershed segments.
				for (int iS = 0; iS < segmentCount; ++iS)
				{
					if (blobCount == 256)
						break;

					segment* s = &segments[iS];
					i32 totalGravity = 0;
					float cX = 0.0f;
					float cY = 0.0f;

					for (int iSP = 0; iSP < s->pixelCount; ++iSP)
					{
						totalGravity += s->pixels[iSP].value;
					}

					float tG = (float)totalGravity;

					for (int iSP = 0; iSP < s->pixelCount; ++iSP)
					{
						float gravity = (float)(s->pixels[iSP].value) / tG;
						cX += (float)s->pixels[iSP].x * gravity;
						cY += (float)s->pixels[iSP].y * gravity;
					}

					cX += s->x;
					cY += s->y;

					//std::cout << "Centroid " << cX << ", " << cY << "\n";
					blobs[blobCount].x = cX;
					blobs[blobCount++].y = cY;
				}
				
				regions[regionCount++] = regionCurrent;

				//t = PlatformGetMicrosecond() - t;
				//printf("Watershedding %d\n", (int)t);
			}
			
			// Reset region for next flood.
			regionCurrent.minX = 2000;
			regionCurrent.minY = 2000;
			regionCurrent.maxX = 0;
			regionCurrent.maxY = 0;
			regionCurrent.pixelIdx = regionPixelCount;
			regionCurrent.pixelCount = 0;
			regionCurrent.maxLum = 0;
			regionCurrent.id = regionCount;
		}
	}

	tFlood = PlatformGetMicrosecond() - tFlood;
	//printf("Total flood %d\n", (int)tFlood);

	//printf("Done %d\n", blobCount);
}

void ProcessPixels(void)
{
	int width = 1024;
	int height = 704;
	int pixelCount = width * height;

	int64_t startTime = PlatformGetMicrosecond();
	process(pixelCount, width, height, _frameBuffer);
	startTime = PlatformGetMicrosecond() - startTime;
	frameTimeUs = startTime;
	//printf("Total time %d\n", (int)startTime);
}

//------------------------------------------------------------------------------------------------------------
// Blob process threading.
//------------------------------------------------------------------------------------------------------------
void *runWorkerThread(void *Args)
{
	printf("IO thread running.\n");

	while (1)
	{
		pthread_mutex_lock(&_queueMutex);
		
		while (_frameStatus != 1)
			pthread_cond_wait(&_queueSignal, &_queueMutex);
		
		_frameStatus = 2;
		pthread_mutex_unlock(&_queueMutex);

		ProcessPixels();

		pthread_mutex_lock(&_queueMutex);
		_frameStatus = 3;
		pthread_mutex_unlock(&_queueMutex);
	}

	printf("IO thread done.\n");
	pthread_exit(0);
}

void enqueueWork()
{
	pthread_mutex_lock(&_queueMutex);
	_frameStatus = 1;
	pthread_mutex_unlock(&_queueMutex);
	pthread_cond_signal(&_queueSignal);
}

//------------------------------------------------------------------------------------------------------------
// Time sync threading.
//------------------------------------------------------------------------------------------------------------
void timeSyncStart()
{
	pthread_mutex_lock(&timeSyncMutex);
	timeSyncReset = true;
	timeSyncRunning = true;
	pthread_mutex_unlock(&timeSyncMutex);
	pthread_cond_signal(&timeSyncSignal);
}

void timeSyncStop()
{
	pthread_mutex_lock(&timeSyncMutex);
	timeSyncReset = true;
	timeSyncRunning = false;
	pthread_mutex_unlock(&timeSyncMutex);
	pthread_cond_signal(&timeSyncSignal);
}

int syncsortcmpfunc(const void* a, const void* b)
{
	return (*(i64*)a - *(i64*)b);
}

void *runTimeSyncThread(void *Args)
{
	printf("Time sync thread running.\n");

	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}

	printf("Time socket: %d\n", s);

	struct sockaddr_in hostAddr = {};
	memset((char *) &hostAddr, 0, sizeof(hostAddr));
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_port = htons(4894);

	while (1)
	{
		printf("Time sync pending\n");

		pthread_mutex_lock(&timeSyncMutex);
		
		while (!timeSyncRunning)
			pthread_cond_wait(&timeSyncSignal, &timeSyncMutex);

		printf("Time sync reset\n");

		timeSyncReset = false;

		// TOOD: Validate IP conversion.
		inet_aton((const char*)masterIp, &hostAddr.sin_addr);
		
		memset(rttBuffer, 0, sizeof(rttBuffer));		
		memset(rttBufferSorted, 0, sizeof(rttBuffer));
		rttBufferIdx = 0;		
		rttMedian = 0;
		rttMedianFiltered = 0.0f;

		memset(offsetBuffer, 0, sizeof(offsetBuffer));
		memset(offsetBufferSorted, 0, sizeof(offsetBufferSorted));
		offsetBufferIdx = 0;
		offsetMedian = 0;
		offsetMedianFiltered = 0.0f;
		
		startTimeUs = GetUS();
		startMasterUs = 0;
		avgHostOffset = 0.0f;
		
		pthread_mutex_unlock(&timeSyncMutex);

		while (1)
		{
			pthread_mutex_lock(&timeSyncMutex);
			bool leave = timeSyncReset;
			pthread_mutex_unlock(&timeSyncMutex);

			if (leave)
				break;

			char msg[1500];

			*(i64*)(msg + 8) = (GetUS() - startTimeUs) + offsetMedianFiltered + startMasterUs + (i64)(rttMedianFiltered * 0.5f);
			*(i64*)msg = GetUS() - startTimeUs;
			
			int res = sendto(s, msg, 16, 0, (struct sockaddr*)&hostAddr, sizeof(hostAddr));
			//std::cout << "Send " << res << "\n";
			
			struct sockaddr_in recvAddr;
			unsigned int recvAddrLen = sizeof(recvAddr);
			res = recvfrom(s, msg, sizeof(msg), 0, (struct sockaddr*)&recvAddr, &recvAddrLen);

			i64 recvTimeUs = GetUS() - startTimeUs;

			if (res == -1)
			{
				printf("TIME OUT BITCHES\n");
				continue;
			}

			i64 sendTimeUs = *(i64*)msg;
			i64 rttUs = recvTimeUs - sendTimeUs;
			i64 masterTimeUs = *(i64*)(msg + 8) - startMasterUs - (i64)(rttMedianFiltered * 0.5f);
			avgHostOffset = *(float*)(msg + 16);

			if (startMasterUs == 0)
				startMasterUs = masterTimeUs;

			//printf("Recv %d: %lld %lld %lld\n", res, (u64)rttUs, masterTimeUs, (masterTimeUs - recvTimeUs));
			
			rttBuffer[rttBufferIdx++] = rttUs;
			if (rttBufferIdx == 100)
				rttBufferIdx = 0;

			memcpy(rttBufferSorted, rttBuffer, sizeof(rttBuffer));
			qsort(rttBufferSorted, 100, sizeof(i64), syncsortcmpfunc);
			
			rttMedian = rttBufferSorted[49];
			rttMedianFiltered = rttMedianFiltered * 0.95f + rttMedian * 0.05f;
			//printf("Median rtt: %lld %f\n", rttMedian, rttMedianFiltered);

			offsetBuffer[offsetBufferIdx++] = masterTimeUs - recvTimeUs;
			if (offsetBufferIdx == 100)
				offsetBufferIdx = 0;

			memcpy(offsetBufferSorted, offsetBuffer, sizeof(offsetBuffer));
			qsort(offsetBufferSorted, 100, sizeof(i64), syncsortcmpfunc);

			offsetMedian = offsetBufferSorted[49];
			offsetMedianFiltered = offsetMedianFiltered * 0.95f + offsetMedian * 0.05f;
			//printf("Median offset: %lld %f\n", offsetMedian, offsetMedianFiltered);
			
			usleep(100000);
		}
	}

	close(s);
	printf("Sync time thread done.\n");
	pthread_exit(0);
}

//------------------------------------------------------------------------------------------------------------
// Python interface.
//------------------------------------------------------------------------------------------------------------
static PyObject* blobdetect_masterconnectionmade(PyObject* Self, PyObject* Args)
{
	if (!PyArg_ParseTuple(Args, "s", &masterIp))
		return NULL;

	timeSyncStart();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* blobdetect_masterconnectionlost(PyObject* Self, PyObject* Args)
{
	timeSyncStop();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* blobdetect_getstatus(PyObject* Self, PyObject* Args)
{
	pthread_mutex_lock(&_queueMutex);
	int b = _frameStatus;
	pthread_mutex_unlock(&_queueMutex);

	return Py_BuildValue("i", b);
}

static PyObject* blobdetect_getblobcount(PyObject* Self, PyObject* Args)
{
	return Py_BuildValue("i", blobCount);
}

static PyObject* blobdetect_getblobdata(PyObject* Self, PyObject* Args)
{
	sendDataHeader header = {};
	header.frameId = _frameId;
	header.avgMasterOffset = _avgMasterOffset;
	header.blobCount = blobCount;
	header.regionCount = regionCount;
	header.foundRegionCount = foundRegionCount;
	header.totalTime = frameTimeUs;
	u8* sendBuffer = sendDataBuffer;

	memcpy(sendBuffer, &header, sizeof(sendDataHeader));
	sendBuffer += sizeof(sendDataHeader);
	memcpy(sendBuffer, blobs, sizeof(blob) * blobCount);
	sendBuffer += sizeof(blob) * blobCount;
	memcpy(sendBuffer, foundRegions, sizeof(region) * foundRegionCount);
	sendBuffer += sizeof(region) * foundRegionCount;
	
	return PyBuffer_FromMemory(&sendDataBuffer, sendBuffer - sendDataBuffer);
}

static PyObject* blobdetect_getframetime(PyObject* Self, PyObject* Args)
{
	return Py_BuildValue("i", frameTimeUs);
}

static PyObject* blobdetect_startworkers(PyObject* Self, PyObject* Args)
{
	pthread_mutex_init(&_queueMutex, 0);
	pthread_cond_init(&_queueSignal, 0);

	pthread_create(&_workerThread, 0, runWorkerThread, 0);
	pthread_create(&timeSyncThread, 0, runTimeSyncThread, 0);

	//int intVal = data.len;
	//return Py_BuildValue("i", intVal);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* blobdetect_pushframe(PyObject* Self, PyObject* Args)
{
	Py_buffer data;

	if (!PyArg_ParseTuple(Args, "Lfs*", &_frameId, &_avgMasterOffset, &data))
		return NULL;

	// TODO: Do we need to memcpy here?
	memcpy(_frameBuffer, data.buf, 1024 * 704);
	enqueueWork();

	PyBuffer_Release(&data);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* blobdetect_setmask(PyObject* Self, PyObject* Args)
{
	Py_buffer data;

	if (!PyArg_ParseTuple(Args, "s*", &data))
		return NULL;

	memcpy(_maskBuffer, data.buf, sizeof(_maskBuffer));

	/*
	printf("Mask Data: ");
	for (int i = 0; i < 64 * 44; ++i)
	{
		printf("%d ", _maskBuffer[i]);
	}
	printf("\n");
	*/
	
	PyBuffer_Release(&data);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* blobdetect_getmask(PyObject* Self, PyObject* Args)
{
	return PyBuffer_FromMemory(&_maskBuffer, sizeof(_maskBuffer));
}

static PyObject* blobdetect_getmastertime(PyObject* Self, PyObject* Args)
{	
	// TODO: Indicate master time sync status.
	u64 mt = (GetUS() - startTimeUs) + (u64)offsetMedianFiltered + (u64)startMasterUs;
	return Py_BuildValue("Kf", mt, avgHostOffset);
}

static PyMethodDef BlobdetectMethods[] = 
{
	{"pushframe", blobdetect_pushframe, METH_VARARGS, "Push a YUV frame for processing."},
	{"setmask", blobdetect_setmask, METH_VARARGS, "Set the blocking mask."},
	{"getmask", blobdetect_getmask, METH_VARARGS, "Get the blocking mask."},
	{"startworkers", blobdetect_startworkers, METH_VARARGS, "Start threads to process frame data."},
	{"getblobcount", blobdetect_getblobcount, METH_VARARGS, "Get processed blob count."},
	{"getblobdata", blobdetect_getblobdata, METH_VARARGS, "Get processed blob data."},
	{"getframetime", blobdetect_getframetime, METH_VARARGS, "Get processed frame time."},
	{"getstatus", blobdetect_getstatus, METH_VARARGS, "Get processing state."},
	{"getmastertime", blobdetect_getmastertime, METH_VARARGS, "Get master time."},
	{"masterconnectionmade", blobdetect_masterconnectionmade, METH_VARARGS, "Indicate that a host has been connected to."},
	{"masterconnectionlost", blobdetect_masterconnectionlost, METH_VARARGS, "Indicate that a host has been disconnected from."},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initblobdetect(void)
{
	(void)Py_InitModule("blobdetect", BlobdetectMethods);
}