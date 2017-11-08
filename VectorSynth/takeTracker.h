#pragma once

#include <QString>
#include <QVector3D>
#include <QDebug>
#include "decoder.h"
#include "liveTracker.h"
#include "trackerConnection.h"

class VidFrameData
{
public:

	int					type;
	int					index;
	int					size;
	int					bufferPosition;
	QList<Marker2D>		newMarkers;
};

class TakeTracker
{
public:

	static QMatrix4x4 WorldFromPose(cv::Mat Pose);
	static cv::Mat PoseFromWorld(QMatrix4x4 World);

	static TakeTracker* Create(int Id, QString TakeName, uint32_t Serial, QString FilePath, LiveTracker* LiveTracker);

	// Take.
	QString				takeName;
	LiveTracker*		liveTracker;
	int					drawMarkerFrameIndex;
	int					vidPlaybackFrame;

	// Tracker params.
	int					id;	
	uint32_t			serial;

	QString				name;
	int					exposure;
	int					iso;
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
	int					mode;

	TakeTracker();
	~TakeTracker();

	void DecodeFrame(int FrameIndex, int KeyFrameIndex);
	void AdvanceFrame(int FrameCount);
	void Save();
	void UndistortMarkers(int StartFrame, int EndFrame);
	void Build2DMarkers(int StartFrame, int EndFrame);
	void BuildRays(int StartFrame, int EndFrame);
	bool ConvertTimelineToFrame(int TimelineFrame, int* KeyFrameIndex, int* FrameIndex);
	VidFrameData* GetLocalFrame(int TimelineFrame);
	void SetPose(cv::Mat Pose);
	void SetCamDist(cv::Mat Cam, cv::Mat Dist);
	QVector2D ProjectPoint(QVector3D P);
	TrackerProperties GetProps();

private:

	int					_currentDecodeFrameIndex;
};