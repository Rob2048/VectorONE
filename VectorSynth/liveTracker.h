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

	uint8_t		frameData[VID_W * VID_H * 3];
	MaskElement	maskData[128 * 88];

	void updateStats();

private:

};