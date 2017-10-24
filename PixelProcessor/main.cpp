#include <iostream>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>

#include "precomp.h"

typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;
typedef char			i8;
typedef short			i16;
typedef int				i32;
typedef float			f32;
typedef double			f64;

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
};

struct segment 
{
	u8 id;
	i32 x;
	i32 y;
	i32 lowMark;
	i32 highMark;
	i32 pixelCount;
	regionPixel pixels[1024 * 706];
};

struct centroid
{
	float x;
	float y;
};

u8			regionMarks[1024 * 706];
regionPixel regionPixels[1024 * 706];
i32			regionPixelCount;
region		regions[1024 * 1024];
i32 		regionCount;
region		regionCurrent;

floodNode	floodList[1024 * 1024];
i32			floodListCount;

u8			tempData[100 * 100];
i32			tempDataWidth;
i32			tempDataHeight;
u8			tempMarks[100 * 100];

u8*			mapData;
i32			mapWidth;
i32			mapHeight;

segment		segments[8];
i32			segmentCount;

centroid	centroids[1024];
i32			centroidCount;

// Debug image.
uint8_t* 	rgbImg;

//------------------------------------------------------------------------------------------------------------
// Utility functions.
//------------------------------------------------------------------------------------------------------------
inline int64_t PlatformGetMicrosecond()
{
	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

inline int32_t PlatformGetMS()
{
	return (PlatformGetMicrosecond() / 1000);
}

float distSq(float X1, float Y1, float X2, float Y2)
{
	return (X2 - X1) * (X2 - X1) + (Y2 - Y1) * (Y2 - Y1);
}

void saveGrayscale(char* Filename, i32 Width, i32 Height, u8* Data)
{
	if (stbi_write_tga(Filename, Width, Height, 1, Data) == 0)
		std::cout << "Failed STB write\n";
}

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

	for (int iY = 0; iY < regionCurrent.height; ++iY)
	{
		for (int iX = 0; iX < regionCurrent.width; ++iX)
		{
			i32 idx = iY * regionCurrent.width + iX;

			if (tempData[idx] >= 200 && tempMarks[idx] == 0)
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
					watershedFloodUpdate(s, 200);
				}

				segmentCount++;
			}
		}
	}

	// Fill the segments
	for (int iS = 0; iS < segmentCount; ++iS)
	{
		segments[iS].lowMark = 0;
		segments[iS].highMark = segments[iS].pixelCount;
	}

	i32 floodLevel = 0;
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
inline void addFloodNode(i32 X, i32 Y)
{
	floodNode fn = {};
	fn.x = X;
	fn.y = Y;
	floodList[floodListCount++] = fn;
}

void flood(i32 X, i32 Y)
{
	i32 idx = Y * mapWidth + X;
	regionMarks[idx] = 255;

	// Add pixel to region.
	regionPixel rp = {};
	rp.x = X;
	rp.y = Y;
	rp.value = mapData[idx];
	regionPixels[regionPixelCount++] = rp;
	regionCurrent.pixelCount++;

	if (X < regionCurrent.minX) regionCurrent.minX = X;
	else if (X > regionCurrent.maxX) regionCurrent.maxX = X;

	if (Y < regionCurrent.minY) regionCurrent.minY = Y;
	else if (Y > regionCurrent.maxY) regionCurrent.maxY = Y;

	if (rp.value > regionCurrent.maxLum) regionCurrent.maxLum = rp.value;

	// Remove this node from the process queue.
	floodListCount--;

	// Add next nodes to process.
	i32 idx0 = (Y - 1) * mapWidth + (X - 1);
	i32 idx1 = (Y - 1) * mapWidth + (X - 0);
	i32 idx2 = (Y - 1) * mapWidth + (X + 1);
	i32 idx3 = (Y - 0) * mapWidth + (X - 1);
	i32 idx4 = (Y - 0) * mapWidth + (X + 1);
	i32 idx5 = (Y + 1) * mapWidth + (X - 1);
	i32 idx6 = (Y + 1) * mapWidth + (X - 0);
	i32 idx7 = (Y + 1) * mapWidth + (X + 1);

	if (regionMarks[idx0] == 0 && mapData[idx0] > 0) addFloodNode(X - 1, Y - 1);
	if (regionMarks[idx1] == 0 && mapData[idx1] > 0) addFloodNode(X - 0, Y - 1);
	if (regionMarks[idx2] == 0 && mapData[idx2] > 0) addFloodNode(X + 1, Y - 1);
	if (regionMarks[idx3] == 0 && mapData[idx3] > 0) addFloodNode(X - 1, Y - 0);
	if (regionMarks[idx4] == 0 && mapData[idx4] > 0) addFloodNode(X + 1, Y - 0);
	if (regionMarks[idx5] == 0 && mapData[idx5] > 0) addFloodNode(X - 1, Y + 1);
	if (regionMarks[idx6] == 0 && mapData[idx6] > 0) addFloodNode(X - 0, Y + 1);
	if (regionMarks[idx7] == 0 && mapData[idx7] > 0) addFloodNode(X + 1, Y + 1);
}

//------------------------------------------------------------------------------------------------------------
// Entire algo for locating centroid positions.
//------------------------------------------------------------------------------------------------------------
void process(i32 Pixels, i32 Width, i32 Height, u8* Data, const char* Filename) 
{
	// NOTE: There is strange ringing on the YUV output from the Pi camera. Values are supposed to be clamped to
	// 16 to 235, but in practice we get values outside of this range. Values below 16 should be around 20 and
	// values above 235 should be around 220. 

	mapData = Data;
	mapWidth = Width;
	mapHeight = Height;

	centroidCount = 0;

	// Rescale data.
	// Remove everything below 32
	i32 startLevel = 32;
	f32 scale = 255.0f / (255 - startLevel);
	
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

			if (p < 0)
				p = 0;

			Data[i] = (u8)p;
		}
	}

	memset(tempMarks, 255, sizeof(tempMarks));

	memset(regionMarks, 0, sizeof(regionMarks));
	regionCount = 0;
	regionPixelCount = 0;

	// Create bounds border.
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

	// Flood fill to find regions above 0.
	for (int iY = 0; iY < Height; ++iY)
	{
		for (int iX = 0; iX < Width; ++iX)
		{
			int idx = iY * Width + iX;
			
			if (regionMarks[idx] == 0 && Data[idx] > 30)
			{
				//std::cout << "Flooding " << iX << ", " << iY << "\n";
				floodListCount = 1;
				flood(iX, iY);

				while (floodListCount)
				{
					flood(floodList[floodListCount - 1].x, floodList[floodListCount - 1].y);
				}

				//std::cout << "Added region " << (i32)regionCurrent.id << ": " << regionCurrent.pixelCount << "\n";

				regionCurrent.width = (regionCurrent.maxX + 1) - regionCurrent.minX;
				regionCurrent.height = (regionCurrent.maxY + 1) - regionCurrent.minY;

				// Reject small regions.
				// TODO: Only reject large regions after watershedding?
				if (regionCurrent.width >= 3 && regionCurrent.width <= 100 && regionCurrent.height >= 3 && regionCurrent.height <= 100)
				{
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
						centroids[centroidCount].x = cX;
						centroids[centroidCount++].y = cY;
					}

					for (int iS = 0; iS < segmentCount; ++iS)
					{
						segment* s = &segments[iS];
					
						srand(regionCurrent.id * 100 + iS + 10);
						u8 cR = rand() % 255;
						u8 cG = rand() % 255;
						u8 cB = rand() % 255;
						
						// Draw segs on image
						for (int iSP = 0; iSP < s->pixelCount; ++iSP)
						{
							int idx = (s->pixels[iSP].y + s->y) * 1024 + (s->pixels[iSP].x + s->x);
							float scale = rgbImg[idx * 3 + 0] / 255.0f;
							rgbImg[idx * 3 + 0] = (u8)(cR * scale);
							rgbImg[idx * 3 + 1] = (u8)(cG * scale);
							rgbImg[idx * 3 + 2] = (u8)(cB * scale);
						}
					}

					//char tempFileName[256];
					//sprintf(tempFileName, "reg%d_%s", regionCurrent.id, Filename);
					//saveGrayscale(tempFileName, regionCurrent.width, regionCurrent.height, tempData);
					
					regions[regionCount++] = regionCurrent;

					// Run distance field transform.

					// Multiply distance and image lum.

					// Apply threshold to find minima in each region.

					// Priority fill to build watersheds.

					// Reject large segments.

					// Gravity weight each segment to find centroid.
				}

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
	}
}

void processImage(const char* Filename)
{
	std::cout << "Process Pixels in " << Filename << "\n";

	int width = 1024;
	int height = 704;
	int pixelCount = width * height;

	u8 buffer[pixelCount];

	int x, y, n;
	unsigned char* imgData = stbi_load(Filename, &x, &y, &n, 0);
	
	for (int i = 0; i < pixelCount; ++i)
	{
		buffer[i] = imgData[i * 3];
	}

	stbi_image_free(imgData);

	rgbImg = (uint8_t*)malloc(pixelCount * 3);
	
	// Copy grayscale to rgb.
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int idx = y * width + x;
			u8 val = buffer[idx];
			
			//if (buffer[idx] > 0)
				//val = 255;
			
			rgbImg[idx * 3 + 0] = val;
			rgbImg[idx * 3 + 1] = val;
			rgbImg[idx * 3 + 2] = val;
		}
	}

	int brightPixels = 0;

	int64_t startTime = PlatformGetMicrosecond();

	process(pixelCount, width, height, buffer, Filename);

	startTime = PlatformGetMicrosecond() - startTime;

	std::cout << "Runtime: " << (startTime / 1000.0f) << " ms\n";

	for (int iR = 0; iR < regionCount; ++iR)
	{
		region* r = &regions[iR];

		srand(r->id + 10);
		u8 cR = rand() % 255;
		u8 cG = rand() % 255;
		u8 cB = rand() % 255;

		/*
		for (int iP = 0; iP < r->pixelCount; ++iP)
		{
			regionPixel* p = &regionPixels[r->pixelIdx + iP];

			int idx = p->y * width + p->x;
			rgbImg[idx * 3 + 0] = cR;
			rgbImg[idx * 3 + 1] = cG;
			rgbImg[idx * 3 + 2] = cB;
		}
		*/

		/*
		for (int iP = 0; iP < r->pixelCount; ++iP)
		{
			regionPixel* p = &regionPixels[r->pixelIdx + iP];

			int idx = p->y * width + p->x;
			rgbImg[idx * 3 + 0] = p->value;
			rgbImg[idx * 3 + 1] = p->value;
			rgbImg[idx * 3 + 2] = p->value;
		}
		*/
		
		//*
		// Region bounds.
		for (int y = r->minY - 1; y < r->maxY + 2; ++y)
		{
			for (int x = r->minX - 1; x < r->maxX + 2; ++x)
			{
				if (x == r->minX - 1 || x == r->maxX + 1 ||
					y == r->minY - 1 || y == r->maxY + 1)
				{
					int idx = y * width + x;

					rgbImg[idx * 3 + 0] = 255;
					rgbImg[idx * 3 + 1] = 0;
					rgbImg[idx * 3 + 2] = 0;
				}
			}
		}
		//*/
	}

	for (int iC = 0; iC < centroidCount; ++iC)
	{
		int idx = (int)centroids[iC].y * width + (int)centroids[iC].x;
		
		rgbImg[idx * 3 + 0] = 0;
		rgbImg[idx * 3 + 1] = 255;
		rgbImg[idx * 3 + 2] = 0;
	}

	// Save processed frame.
	char outFilename[256];
	sprintf(outFilename, "proc_%s", Filename);
	if (stbi_write_tga(outFilename, width, height, 3, rgbImg) == 0)
		std::cout << "Failed STB write\n";
}

//------------------------------------------------------------------------------------------------------------
// Application entry.
//------------------------------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	/*
	const char* fileList[] = 
	{
		"m1.tga",
		"m2.tga",
		"m2.tga",
		"m4.tga",
		"m5.tga",
		"close1.tga",
		"close2.tga",
		"blur1.tga",
		"blur2.tga",
		"desk_blobs.tga",
		"blobs.tga"
	};
	//*/
	//*
	const char* fileList[] = 
	{
		"desk_blobs_masked.tga"
	};
	//*/
	
	for (int i = 0; i < sizeof(fileList) / sizeof(char**); ++i)
	{
		processImage(fileList[i]);
	}

	return 0;
}