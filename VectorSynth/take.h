#pragma once

#include <QString>
#include <QVector3D>
#include <QDebug>
#include "decoder.h"
#include "liveTracker.h"
#include "takeTracker.h"

class MarkerCalib
{
public:

	QVector3D pos;
	int frame;
};

class Marker3D
{
public:

	enum Type
	{
		T_UNLABLED,
		T_LABLED,
		T_GHOST,
	};

	QVector3D pos;
	QList<Marker2D> sources;
	int id;
	QVector3D velocity;
	QVector3D bVelocity;
	Type type;
};

class MarkerGroup
{
public:

	QVector3D pos;
	QVector3D avgPos;
	int count;
	QList<Marker2D> sources;
};

class MarkerLabel
{
public:

	QString name;
};

class Take
{
public:

	QString name;

	QList<TakeTracker*>				trackers;
	std::map<int, LiveTracker*>		liveTrackers;
	int								selectedTracker;
	
	int timeEnd;
	int timeFrames;
	int frameDuration;
	bool isLive;
	bool isRecording;

	std::vector<std::vector<Marker3D>> markers;
	std::map<int, MarkerLabel> labels;
	std::vector<QVector3D> refMarkers;
	
	Take();
	~Take();

	void Destroy();
	void LoadTake(QString Name);	
	void Save();

	void SetFrame(int TimelineFrame);
	void Build2DMarkers(int StartFrame, int EndFrame);
	void Build3DMarkers(int StartFrame, int EndFrame);
	void BuildLabels(int StartFrame, int EndFrame);
	void BuildExtrinsics(int StartFrame, int EndFrame);
	void BundleAdjust(int StartFrame, int EndFrame);
	void SaveSSBAFile();

protected:

	void _AdjustRuntime();
	void _ClosestPointsLines(QVector3D P1, QVector3D D1, QVector3D P2, QVector3D D2, QVector3D* C1, QVector3D* C2);
	void _BuildPose(int StartFrame, int EndFrame, TakeTracker* Root, TakeTracker* Tracker, std::vector<MarkerCalib>& Markers, cv::Mat& Pose);
};

class LiveTake : public Take
{
public:

	LiveTake();
};

class LoadedTake : public Take
{
public:

	LoadedTake();
};