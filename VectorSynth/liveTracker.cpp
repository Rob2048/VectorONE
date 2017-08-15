#include "liveTracker.h"

LiveTracker::LiveTracker() :
	frames(0),
	data(0),
	fps(0),
	dataRecv(0),
	selected(false),
	active(true),
	connected(false)
{
	for (int i = 0; i < 128 * 88; ++i)
		maskData[i] = { 0, 0, 0, 0 };
	
	maskData[128 * 62 + 5] = { 255, 0, 0, 128 };
	maskData[128 * 62 + 6] = { 255, 0, 0, 128 };
	maskData[128 * 62 + 7] = { 255, 0, 0, 128 };
	maskData[128 * 62 + 8] = { 255, 0, 0, 128 };
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