#pragma once

#include <QString>
#include "decoder.h"

struct MaskElement
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

class LiveTracker
{
public:

	LiveTracker();
	~LiveTracker();

	int			id;
	uint32_t	serial;
	QString		name;
	int			version;

	int			exposure;
	int			iso;
	int			threshold;
	int			sensitivity;
	int			targetFps;

	float		frames;
	float		data;
	float		fps;
	float		dataRecv;

	bool		selected;
	bool		active;
	bool		connected;
	bool		loaded;
	bool		decodeVideo;

	int			interactMode;

	uint8_t		frameData[VID_W * VID_H * 3];
	MaskElement	maskVisualData[128 * 88];
	uint8_t		maskData[128 * 88];

	void updateStats();

	void changeMask(int X, int Y, bool Value);
	bool getMask(int X, int Y);
	void generateMask();
	void setMask(uint8_t* Data);

private:

};