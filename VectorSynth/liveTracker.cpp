#include "liveTracker.h"

LiveTracker::LiveTracker() :
	frames(0),
	data(0),
	fps(0),
	dataRecv(0),
	selected(false),
	active(true),
	connected(false),
	loaded(false),
	decodeVideo(true),
	interactMode(0),
	markerDataSize(0)
{
	for (int i = 0; i < 64 * 44; ++i)
	{
		maskVisualData[i] = { 0, 0, 0, 0 };
		maskData[i] = 1;
	}
}

LiveTracker::~LiveTracker()
{
}

void LiveTracker::updateStats()
{
	// NOTE: Called at 1Hz
	fps = frames;
	dataRecv = data / 1024.0f;
	frames = 0;
	data = 0;
}

void LiveTracker::setMask(uint8_t* Data)
{
	memcpy(maskData, Data, sizeof(maskData));

	for (int i = 0; i < 64 * 44; ++i)
	{
		if (maskData[i] == 0)
			maskVisualData[i] = { 255, 0, 0, 128 };
		else
			maskVisualData[i] = { 0, 0, 0, 0 };
	}
}

void LiveTracker::changeMask(int X, int Y, bool Value)
{
	if (Value)
	{
		maskData[Y * 64 + X] = 0;
		maskVisualData[Y * 64 + X] = { 255, 0, 0, 128 };
	}
	else
	{
		maskData[Y * 64 + X] = 1;
		maskVisualData[Y * 64 + X] = { 0, 0, 0, 0 };
	}
}

bool LiveTracker::getMask(int X, int Y)
{
	return (maskData[Y * 64 + X] == 0);
}

void LiveTracker::generateMask()
{
	for (int iY = 0; iY < 44; ++iY)
	{
		for (int iX = 0; iX < 64; ++iX)
		{
			bool masked = false;

			int kXStart = (iX * 16) - 16;
			int kXEnd = (iX * 16) + 32;
			int kYStart = (iY * 16) - 16;
			int kYEnd = (iY * 16) + 32;

			if (kXStart < 0) kXStart = 0;
			if (kXEnd > VID_W) kXEnd = VID_W;
			if (kYStart < 0) kYStart = 0;
			if (kYEnd > VID_H) kYEnd = VID_H;

			for (int kY = kYStart; kY < kYEnd; ++kY)
			{
				for (int kX = kXStart; kX < kXEnd; ++kX)
				{
					if (frameData[(kY * VID_W + kX) * 3] > 0)
					{
						masked = true;
						break;
					}
				}

				if (masked)
					break;
			}

			changeMask(iX, iY, masked);
		}
	}
}