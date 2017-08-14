#pragma once

#include <QString>
#include <QVector3D>
#include <QDebug>
#include "decoder.h"

class SceneView;

class Marker3D
{
public:
	QVector2D cam1pos;
	QVector2D cam2pos;
	QVector3D pos;
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
	QList<QVector2D> markers;
	QList<NewMarker> newMarkers;
	QList<EpipolarLine> epiLines;
};

class VidKeyFrameData
{
public:
	int time;
	int frameDataIndex;
};

class TakeTracker
{
public:
	static TakeTracker* Create(QString TakeId, QString TakeName, QString FilePath);

	QString				takeName;
	QString				name;
	int					takeId;
	QList<VidFrameData> vidFrameData;
	int					vidPlaybackFrame;
	Decoder*			decoder;
	uint8_t*			takeClipData;
	int					exposure;
	int					fps;
	int					iso;
	float				threshold;
	float				sensitivity;
	
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
	bool ConvertTimelineToFrame(int TimelineFrame, int* KeyFrameIndex, int* FrameIndex);
	void DrawMarkers(int FrameIndex);
	VidFrameData* GetLocalFrame(int TimelineFrame);
};

class Take
{
public:
	QList<TakeTracker*> trackers;
	int timeStart;
	int timeEnd;
	int timeFrames;

	QVector3D wX;
	QVector3D wY;
	QVector3D wZ;
	QVector3D wT;

	float wScale;

	std::vector<std::vector<Marker3D>> markers;

	Take();
	~Take();

	void Destroy();
	void LoadTake(QString Name);	
	void Save();

	//void SetTime();
	void SetFrame(int TimelineFrame, bool DrawMarkers);
	void GenerateMask();
	void Build2DMarkers(int StartFrame, int EndFrame);
	void Build3DMarkers(int StartFrame, int EndFrame);
	void BuildFundamental(int StartFrame, int EndFrame, SceneView* Scene);

private:
	void _ReconfigureTimeline();
	void _LoadTracker(QString TrackerFileName);
	void _AdjustRuntime();
	Marker3D _triangulate(QVector2D P1, QVector2D P2);
};