#pragma once

#include <QString>
#include <QVector3D>
#include <QDebug>
#include "decoder.h"
#include "liveTracker.h"

class VidFrameData
{
public:

	int					type;
	int					time;
	int					index;
	int					size;
	int					bufferPosition;
	QList<Marker2D>		newMarkers;
};

class TakeTracker
{
public:

	static QMatrix4x4 WorldFromPose(cv::Mat Pose);

	static TakeTracker* Create(int Id, QString TakeName, uint32_t Serial, QString FilePath, LiveTracker* LiveTracker);

	// Take.
	QString				takeName;
	LiveTracker*		liveTracker;
	int					drawMarkerFrameIndex;
	int					vidPlaybackFrame;

	// Tracker params.
	int					id;
	QString				name;
	uint32_t			serial;	
	int					exposure;
	int					fps;
	int					iso;
	float				threshold;
	float				sensitivity;
	uint8_t				mask[64 * 44];
	cv::Mat				distCoefs;
	cv::Mat				camMatOpt;
	cv::Mat				camMat;
	cv::Mat				projMat; // camMatOpt * [R|t]
	cv::Mat				rtMat; // [R|t]
	QMatrix4x4			worldMat;
	QVector3D			worldPos;
	
	// Frame data.
	Decoder*			decoder;
	uint8_t*			takeClipData;
	int					frameCount;
	QList<VidFrameData> vidFrameData;	

	TakeTracker();
	~TakeTracker();

	void DecodeFrame(int FrameIndex, int KeyFrameIndex);
	void AdvanceFrame(int FrameCount);
	void Save();
	void UndistortMarkers(int StartFrame, int EndFrame);
	void Build2DMarkers(int StartFrame, int EndFrame);
	void BuildRays(int StartFrame, int EndFrame);
	bool ConvertTimelineToFrame(int TimelineFrame, int* KeyFrameIndex, int* FrameIndex);
	void DrawMarkers(int FrameIndex);
	VidFrameData* GetLocalFrame(int TimelineFrame);
	void SetPose(cv::Mat Pose);
	void SetCamDist(cv::Mat Cam, cv::Mat Dist);

private:

	int					_currentDecodeFrameIndex;
};