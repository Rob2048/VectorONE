#pragma once

#include <QString>
#include <QVector3D>
#include <QDebug>
#include "decoder.h"
#include "liveTracker.h"
#include "takeTracker.h"

class SceneView;

class MarkerCalib
{
public:

	QVector3D pos;
	int frame;
};

class Marker3D
{
public:

	QVector3D pos;
	QList<Marker2D> sources;
};

class MarkerGroup
{
public:

	QVector3D pos;
	QVector3D avgPos;
	int count;
	QList<Marker2D> sources;
};

class Take
{
public:

	// TODO: Combine TakeTracker and LiveTracker.
	QList<TakeTracker*>				trackers;
	std::map<int, LiveTracker*>		liveTrackers;
	
	int timeStart;
	int timeEnd;
	int timeFrames;
	int frameDuration;

	QVector3D wX;
	QVector3D wY;
	QVector3D wZ;
	QVector3D wT;
	float wScale;

	std::vector<std::vector<Marker3D>> markers;
	std::vector<QVector3D> refMarkers;
	std::vector<QVector3D> calibMarkers;

	Take();
	~Take();

	void Destroy();
	void LoadTake(QString Name);	
	void Save();

	//void SetTime();
	void SetFrame(int TimelineFrame);
	void Build2DMarkers(int StartFrame, int EndFrame);
	void Build3DMarkers(int StartFrame, int EndFrame);
	void BuildExtrinsics(int StartFrame, int EndFrame);
	void BundleAdjust(int StartFrame, int EndFrame);
	void SaveSSBAFile();

private:

	void _ReconfigureTimeline();
	void _LoadTracker(QString TrackerFileName);
	void _AdjustRuntime();	
	//Marker3D _triangulate(Marker2D M1, Marker2D M2);
	void _ClosestPointsLines(QVector3D P1, QVector3D D1, QVector3D P2, QVector3D D2, QVector3D* C1, QVector3D* C2);
	void _BuildPose(int StartFrame, int EndFrame, TakeTracker* Root, TakeTracker* Tracker, std::vector<MarkerCalib>& Markers, cv::Mat& Pose);
};