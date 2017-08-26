#pragma once

#include <QString>
#include <QVector3D>
#include <QDebug>
#include "decoder.h"
#include "liveTracker.h"

class SceneView;

class Marker3D
{
public:

	QVector3D pos;

	// TODO: All cams that cams that can see marker.
	NewMarker cam1marker;
	NewMarker cam2marker;
};

class EpipolarLine
{
public:

	int timelineFrame;
	float a;
	float b;
	float c;

	float a2;
	float b2;
	float c2;
};

class VidFrameData
{
public:

	int type;
	int time;
	int index;
	int size;
	int bufferPosition;
	//QList<QVector2D> markers;
	QList<NewMarker> newMarkers;
	
	QList<EpipolarLine> epiLines;
};

class TakeTracker
{
public:

	static TakeTracker* Create(int Id, QString TakeName, uint32_t Serial, QString FilePath, LiveTracker* LiveTracker);

	LiveTracker*		liveTracker;
	QString				takeName;
	QString				name;
	uint32_t			serial;
	int					id;
	QList<VidFrameData> vidFrameData;
	int					vidPlaybackFrame;
	Decoder*			decoder;
	uint8_t*			takeClipData;
	int					exposure;
	int					fps;
	int					iso;
	float				threshold;
	float				sensitivity;
	uint8_t				mask[128 * 88];
	
	int					frameCount;
	int					frameOffset;
	//int					frameEndIndex;
	int					currentFrameIndex;
	int					drawMarkerFrameIndex;

	TakeTracker();
	~TakeTracker();

	void DecodeFrame(int FrameIndex, int KeyFrameIndex);
	void AdvanceFrame(int FrameCount);
	void Save();
	void Build2DMarkers(int StartFrame, int EndFrame);
	void BuildEpilines(int StartFrame, int EndFrame);
	void BuildRays(int StartFrame, int EndFrame);
	bool ConvertTimelineToFrame(int TimelineFrame, int* KeyFrameIndex, int* FrameIndex);
	void DrawMarkers(int FrameIndex);
	VidFrameData* GetLocalFrame(int TimelineFrame);
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
	void SetFrame(int TimelineFrame, bool DrawMarkers);
	void Build2DMarkers(int StartFrame, int EndFrame);
	void Build3DMarkers(int StartFrame, int EndFrame);
	void BuildFundamental(int StartFrame, int EndFrame, SceneView* Scene);
	void SaveSSBAFile();

private:

	void _ReconfigureTimeline();
	void _LoadTracker(QString TrackerFileName);
	void _AdjustRuntime();
	Marker3D _triangulate(NewMarker M1, NewMarker M2);
};