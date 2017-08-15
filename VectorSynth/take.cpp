#include "take.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QElapsedTimer>
#include "sceneView.h"

TakeTracker* TakeTracker::Create(QString TakeName, QString TakeId, QString Path)
{
	QFile file(Path);

	qDebug() << "Tracker: Load" << TakeName << TakeId << Path;

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Tracker: Load file failed";
		return 0;
	}
	
	QByteArray fileData = file.readAll();
	file.close();
	QJsonObject trackerObj = QJsonDocument::fromJson(fileData).object();
	
	TakeTracker* tracker = new TakeTracker();
	tracker->takeId = TakeId.toInt();
	tracker->takeName = TakeName;
	tracker->name = trackerObj["name"].toString();
	tracker->exposure = trackerObj["exposure"].toInt();
	tracker->fps = trackerObj["fps"].toInt();
	tracker->iso = trackerObj["iso"].toInt();
	tracker->threshold = trackerObj["threshold"].toDouble();
	tracker->sensitivity = trackerObj["sensitivity"].toDouble();
	tracker->frameOffset = trackerObj["offset"].toInt();
	tracker->frameCount = 0;
	
	QJsonArray jsonIntrinMat = trackerObj["intrinsic"].toObject()["matrix"].toArray();
	QJsonArray jsonIntrinDist = trackerObj["intrinsic"].toObject()["distortion"].toArray();
	QJsonArray jsonExtrinProj = trackerObj["extrinsic"].toObject()["proj"].toArray();
	QJsonArray jsonExtrinWorld = trackerObj["extrinsic"].toObject()["world"].toArray();
	QJsonArray jsonExtrinFundamental = trackerObj["extrinsic"].toObject()["fundamental"].toArray();

	tracker->decoder = new Decoder();
	tracker->decoder->camSensitivity = tracker->sensitivity;
	tracker->decoder->drawMarkers = false;
	tracker->decoder->drawUndistorted = false;
	tracker->decoder->camThreshold = tracker->threshold;

	tracker->decoder->_calibDistCoeffs.at<double>(0) = jsonIntrinDist[0].toDouble();
	tracker->decoder->_calibDistCoeffs.at<double>(1) = jsonIntrinDist[1].toDouble();
	tracker->decoder->_calibDistCoeffs.at<double>(2) = jsonIntrinDist[2].toDouble();
	tracker->decoder->_calibDistCoeffs.at<double>(3) = jsonIntrinDist[3].toDouble();
	tracker->decoder->_calibDistCoeffs.at<double>(4) = jsonIntrinDist[4].toDouble();

	tracker->decoder->_calibCameraMatrix.at<double>(0, 0) = jsonIntrinMat[0].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(1, 0) = jsonIntrinMat[1].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(2, 0) = jsonIntrinMat[2].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(0, 1) = jsonIntrinMat[3].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(1, 1) = jsonIntrinMat[4].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(2, 1) = jsonIntrinMat[5].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(0, 2) = jsonIntrinMat[6].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(1, 2) = jsonIntrinMat[7].toDouble();
	tracker->decoder->_calibCameraMatrix.at<double>(2, 2) = jsonIntrinMat[8].toDouble();
	
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			tracker->decoder->unitProjMat.at<double>(iX, iY) = jsonExtrinProj[iY * 3 + iX].toDouble();
		}
	}

	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 4; ++iX)
		{
			tracker->decoder->worldMat(iX, iY) = jsonExtrinWorld[iY * 4 + iX].toDouble();
		}
	}

	for (int iY = 0; iY < 3; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			tracker->decoder->fundamentalMat.at<double>(iX, iY) = jsonExtrinFundamental[iY * 3 + iX].toDouble();
		}
	}

	tracker->decoder->CalculateOptMat();

	tracker->decoder->projMat = tracker->decoder->optCamMat * tracker->decoder->unitProjMat;
	
	char fileName[256];
	QString infoFilePath = "project/" + TakeName + "/" + TakeId + ".trakvid";
	FILE* vidFile = fopen(infoFilePath.toLatin1(), "rb");

	fseek(vidFile, 0, SEEK_END);
	int dataSize = ftell(vidFile);
	fseek(vidFile, 0, SEEK_SET);
	tracker->takeClipData = new uint8_t[dataSize];
	fread(tracker->takeClipData, dataSize, 1, vidFile);
	fclose(vidFile);

	int wp = 0;
	int rp = 0;
	tracker->vidFrameData.clear();
	bool nextFrameKey = false;

	int prevTime = 0;
	bool prevFrameWasData = false;
	int prevWritePtr = 0;
	int prevSize = 0;

	while (rp < dataSize)
	{
		if (rp >= dataSize - 12)
			break;

		uint8_t* md = tracker->takeClipData + rp;
		int size = md[3] << 24 | md[2] << 16 | md[1] << 8 | md[0];
		int type = md[7] << 24 | md[6] << 16 | md[5] << 8 | md[4];
		int time = md[11] << 24 | md[10] << 16 | md[9] << 8 | md[8];
		rp += 12;

		if (rp + size >= dataSize)
			break;

		memcpy(tracker->takeClipData + wp, tracker->takeClipData + rp, size);
		rp += size;

		if (type == 2)
		{
			// Skip data frame, but remember wp for next frame start.
			prevFrameWasData = true;
			prevWritePtr = wp;
			prevSize = size;
		}
		else
		{
			int dt = time - prevTime;
			int dtMs = (dt + 500) / 1000;
			int frameProgress = dtMs / (1000 / tracker->fps);
			prevTime = time;

			while (frameProgress-- > 1)
			{
				// Dummy Frame.
				VidFrameData vfdDummy = {};
				vfdDummy.type = 3;
				vfdDummy.time = time;
				vfdDummy.index = tracker->frameCount++;
				vfdDummy.size = 0;
				vfdDummy.bufferPosition = 0;
				tracker->vidFrameData.push_back(vfdDummy);
			}

			VidFrameData vfd = {};
			vfd.type = type;
			vfd.time = time;
			vfd.index = tracker->frameCount++;
			vfd.size = size;
			vfd.bufferPosition = wp;
			
			if (prevFrameWasData)
			{
				prevFrameWasData = false;
				vfd.bufferPosition = prevWritePtr;
				vfd.size += prevSize;
			}

			tracker->vidFrameData.push_back(vfd);

			//qDebug() << "Frame - Index:" << vfd.index << "DT:" << dt << "(" << dtMs << ") Type:" << vfd.type << "Size:" << vfd.size;
		}

		wp += size;

		//qDebug() << "Tracker:" << TakeId << "Frame Data:" << tracker->vidFrameData.count() << size << "bytes" << type << time << "us";
	}

	tracker->vidPlaybackFrame = 0;

	QFile m2dFile("project/" + TakeName + "/" + TakeId + ".m2d");

	if (m2dFile.open(QIODevice::ReadOnly))
	{
		QDataStream stream(&m2dFile);

		for (int i = 0; i < tracker->vidFrameData.count(); ++i)
		{
			int markerCount = 0;
			stream >> markerCount;

			for (int m = 0; m < markerCount; ++m)
			{
				QVector2D pos;
				stream >> pos;
				tracker->vidFrameData[i].markers.push_back(pos);
			}
		}
		
		m2dFile.close();
	}
	else
	{
		qDebug() << "No 2D Markers file";
	}


	qDebug() << "Tracker: Loaded" << TakeId;

	/*
	for (int i = 0; i < tracker->vidFrameData.count(); ++i)
	{
		VidFrameData* vfdp = &tracker->vidFrameData[i];
		qDebug() << "Post - Index:" << i << "Frame:" << vfdp->index << "Type:" << vfdp->type << "Time:" << vfdp->time;
	}
	*/
	
	return tracker;
}

TakeTracker::TakeTracker()
{
	drawMarkerFrameIndex = 0;
}

TakeTracker::~TakeTracker()
{
}

void TakeTracker::DecodeFrame(int FrameIndex, int KeyFrameIndex)
{
	int frameCount = FrameIndex - KeyFrameIndex + 1;
	int procFrame = KeyFrameIndex;

	QElapsedTimer pt;
	pt.start();

	while (frameCount-- > 0)
	{
		if (procFrame < 0 || procFrame >= vidFrameData.count())
		{
			qDebug() << "Out of Range Local Frame Decode";
			return;
		}

		VidFrameData* vfd = &vidFrameData[procFrame++];

		if (vfd->type == 3)
		{
			decoder->ShowBlankFrame();
			decoder->blankFrame = true;
			continue;
		}

		int consumed = 0;
		bool frame = decoder->DoDecodeSingleFrame(takeClipData + vfd->bufferPosition, vfd->size, &consumed);
		decoder->blankFrame = false;

		if (!frame)
			qDebug() << "Failed to Decode Frame";
	}

	currentFrameIndex = procFrame;
	int t = pt.elapsed();

	//qDebug() << "Perf" << t;
}

void TakeTracker::AdvanceFrame(int FrameCount)
{
	DecodeFrame(currentFrameIndex + FrameCount - 1, currentFrameIndex);
}

/*
void TakeTracker::AdvanceFrame(int FrameCount)
{
	int frameCount = FrameCount;
	int procFrame = currentFrameIndex;

	QElapsedTimer pt;
	pt.start();

	while (frameCount > 0)
	{
		int consumed = 0;

		VidFrameData* vfd = &vidFrameData[procFrame++];
		bool frame = decoder->DoDecodeSingleFrame(takeClipData + vfd->bufferPosition, vfd->size, &consumed);

		if (procFrame >= vidFrameData.count())
			return;

		if (frame)
			qDebug() << "SHOULD NEVER BE TRUE";

		if (!frame && vfd->type != 2)
		{
			VidFrameData* vfdn = &vidFrameData[procFrame];
			frame = decoder->DoDecodeSingleFrame(takeClipData + vfdn->bufferPosition, vfdn->size, &consumed);
			frameCount--;			
		}

		//qDebug() << "Decode Frame" << frame << consumed;
	}

	currentFrameIndex = procFrame;
	int t = pt.elapsed();

	//qDebug() << "Perf" << t;
}
*/

void TakeTracker::Save()
{
	qDebug() << "Tracker: Save" << takeId << takeName << name;

	QJsonObject jsonObj;
	jsonObj["name"] = name;
	jsonObj["fps"] = fps;
	jsonObj["exposure"] = exposure;
	jsonObj["iso"] = iso;
	jsonObj["threshold"] = threshold;
	jsonObj["sensitivity"] = sensitivity;
	jsonObj["offset"] = frameOffset;

	QJsonArray jsonIntrinMat;
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(0, 0));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(1, 0));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(2, 0));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(0, 1));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(1, 1));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(2, 1));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(0, 2));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(1, 2));
	jsonIntrinMat.append(decoder->_calibCameraMatrix.at<double>(2, 2));

	QJsonArray jsonIntrinDist;
	jsonIntrinDist.append(decoder->_calibDistCoeffs.at<double>(0));
	jsonIntrinDist.append(decoder->_calibDistCoeffs.at<double>(1));
	jsonIntrinDist.append(decoder->_calibDistCoeffs.at<double>(2));
	jsonIntrinDist.append(decoder->_calibDistCoeffs.at<double>(3));
	jsonIntrinDist.append(decoder->_calibDistCoeffs.at<double>(4));

	QJsonObject jsonIntrin;
	jsonIntrin["matrix"] = jsonIntrinMat;
	jsonIntrin["distortion"] = jsonIntrinDist;

	jsonObj["intrinsic"] = jsonIntrin;

	QJsonArray jsonExtrinProj;
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			jsonExtrinProj.append(decoder->unitProjMat.at<double>(iX, iY));
		}
	}
	
	QJsonArray jsonExtrinWorld;
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 4; ++iX)
		{
			jsonExtrinWorld.append(decoder->worldMat(iX, iY));
		}
	}

	QJsonArray jsonExtrinFundamental;
	for (int iY = 0; iY < 3; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			jsonExtrinFundamental.append(decoder->fundamentalMat.at<double>(iX, iY));
		}
	}

	QJsonObject jsonExtrin;
	jsonExtrin["proj"] = jsonExtrinProj;
	jsonExtrin["world"] = jsonExtrinWorld;
	jsonExtrin["fundamental"] = jsonExtrinFundamental;

	jsonObj["extrinsic"] = jsonExtrin;

	QJsonDocument jsonDoc(jsonObj);
	QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Indented);

	QFile file("project/" + takeName + "/" + QString::number(takeId) + ".tracker");

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qDebug() << "Tracker: Save file failed";
		return;
	}

	file.write(jsonBytes);
	file.close();

	QFile m2dFile("project/" + takeName + "/" + QString::number(takeId) + ".m2d");

	if (!m2dFile.open(QIODevice::WriteOnly))
	{
		qDebug() << "Tracker: Save file failed";
		return;
	}

	QDataStream m2dStream(&m2dFile);

	for (int i = 0; i < vidFrameData.count(); ++i)
	{
		m2dStream << vidFrameData[i].markers.count();

		for (int m = 0; m < vidFrameData[i].markers.count(); ++m)
		{
			m2dStream << vidFrameData[i].markers[m];
		}
	}
	
	m2dFile.close();
}

void TakeTracker::Build2DMarkers(int StartFrame, int EndFrame)
{
	qDebug() << "Build 2D Markers" << name;
	int startKeyFrameIndex;
	int startFrameIndex;

	int localStartFrame = StartFrame - frameOffset;
	int localEndFrame = EndFrame - frameOffset;

	if (localStartFrame < 0) 
		localStartFrame = 0;
	else if (localStartFrame >= vidFrameData.count())
		localStartFrame = vidFrameData.count();

	if (localEndFrame < 0)
		localEndFrame = 0;
	else if (localEndFrame >= vidFrameData.count())
		localEndFrame = vidFrameData.count();

	// Find actual start frame.
	for (int i = localStartFrame; i <= localEndFrame; ++i)
	{
		if (vidFrameData[i].type != 3)
			break;
		else
			localStartFrame++;
	}

	// Find keyframe for start frame.
	int keyFrameIndex = localStartFrame;
	while (keyFrameIndex >= 0)
	{
		if (vidFrameData[keyFrameIndex].type == 1)
		{
			break;
		}
		--keyFrameIndex;
	}

	if (keyFrameIndex == -1)
	{
		qDebug() << "Can't find frames to process";
		return;
	}

	int processFrameCount = localEndFrame - localStartFrame + 1;

	DecodeFrame(localStartFrame, keyFrameIndex);

	for (int i = localStartFrame + 1; i <= localEndFrame; ++i)
	{
		QElapsedTimer tmr;
		tmr.start();
		AdvanceFrame(1);
		qint64 t1 = tmr.nsecsElapsed();
		vidFrameData[i].markers = decoder->ProcessFrameMarkers();
		vidFrameData[i].newMarkers = decoder->ProcessFrameNewMarkers();
		qint64 t2 = tmr.nsecsElapsed();

		float tT = (t2 / 1000) / 1000.0;
		float tf1 = (t1 / 1000) / 1000.0;
		float tf2 = ((t2 - t1) / 1000) / 1000.0;

		//qDebug() << i << "Markers:" << vidFrameData[i].markers.count() << "Time:" << tf1 << tf2;
		qDebug() << i << "Markers:" << vidFrameData[i].newMarkers.count() << "Time:" << tT;
	}
}

void TakeTracker::BuildEpilines(int StartFrame, int EndFrame)
{
	qDebug() << "Build 3D Markers" << name;
	int startKeyFrameIndex;
	int startFrameIndex;

	int localStartFrame = StartFrame - frameOffset;
	int localEndFrame = EndFrame - frameOffset;

	if (localStartFrame < 0)
		localStartFrame = 0;
	else if (localStartFrame >= vidFrameData.count())
		localStartFrame = vidFrameData.count();

	if (localEndFrame < 0)
		localEndFrame = 0;
	else if (localEndFrame >= vidFrameData.count())
		localEndFrame = vidFrameData.count();

	std::vector<cv::Point2f> points;
	std::vector<cv::Point3f> elines;

	for (int i = localStartFrame; i <= localEndFrame; ++i)
	{
		for (int j = 0; j < vidFrameData[i].newMarkers.count(); ++j)
		{
			points.push_back(cv::Point2f(vidFrameData[i].newMarkers[j].pos.x(), vidFrameData[i].newMarkers[j].pos.y()));
		}
	}
	
	// Camera pair epiline generation.
	if (takeId != 1 && takeId != 2)
	{
		qDebug() << "Fatal epipolar generation";
		return;
	}

	cv::computeCorrespondEpilines(points, takeId, decoder->fundamentalMat, elines);
	
	int currentE = 0;
	for (int i = localStartFrame; i <= localEndFrame; ++i)
	{
		vidFrameData[i].epiLines.clear();

		for (int j = 0; j < vidFrameData[i].newMarkers.count(); ++j)
		{
			EpipolarLine e = {};
			e.a = elines[currentE].x;
			e.b = elines[currentE].y;
			e.c = elines[currentE].z;
			
			vidFrameData[i].epiLines.push_back(e);

			++currentE;
		}
	}

	for (int i = localStartFrame; i <= localEndFrame; ++i)
	{
		qDebug() << "Tracker" << name << "frame:" << i << "elines:" << vidFrameData[i].epiLines.count();
	}
}

bool TakeTracker::ConvertTimelineToFrame(int TimelineFrame, int* KeyFrameIndex, int* FrameIndex)
{
	int keyFrameIndex = -1;
	int frameIndex = -1;
	int l = 0;
	int r = vidFrameData.count() - 1;

	*KeyFrameIndex = keyFrameIndex;
	*FrameIndex = frameIndex;

	while (l <= r)
	{
		int m = l + (r - l) / 2;

		int mIdx = vidFrameData[m].index + frameOffset;

		if (mIdx == TimelineFrame)
		{
			frameIndex = m;
			break;
		}

		if (mIdx < TimelineFrame)
		{
			l = m + 1;
		}
		else
		{
			r = m - 1;
		}
	}

	keyFrameIndex = frameIndex;

	//qDebug() << "Search:" << TimelineFrame << "FrameIndex:" << frameIndex;

	while (keyFrameIndex >= 0)
	{
		if (vidFrameData[keyFrameIndex].type == 1)
		{
			break;
		}
		--keyFrameIndex;
	}

	if (frameIndex != -1 && keyFrameIndex != -1)
	{
		//if (vidFrameData[frameIndex].type == 3)
			//return false;

		*KeyFrameIndex = keyFrameIndex;
		*FrameIndex = frameIndex;
		return true;
	}
	
	return false;
}

void TakeTracker::DrawMarkers(int FrameIndex)
{

}

VidFrameData* TakeTracker::GetLocalFrame(int TimelineFrame)
{
	int localFrame = TimelineFrame - frameOffset;
	
	if (localFrame < 0 || localFrame >= vidFrameData.count())
		return 0;

	return &vidFrameData[localFrame];
}

Take::Take() :
	wScale(1.0f),
	wX(1, 0, 0),
	wY(0, 1, 0),
	wZ(0, 0, 1)
{

}

Take::~Take()
{
	Destroy();
}

void Take::Destroy()
{
	for (int i = 0; i < trackers.count(); ++i)
	{
		delete trackers[i]->decoder;
		delete[] trackers[i]->takeClipData;
		delete trackers[i];
	}

	trackers.clear();
}

void Take::LoadTake(QString Name)
{
	Destroy();

	QDir dir("project/" + Name);

	QStringList trackVidFilter;
	trackVidFilter.push_back("*.tracker");
	QStringList fileList = dir.entryList(trackVidFilter, QDir::Files);

	for (int i = 0; i < fileList.count(); ++i)
	{
		qDebug() << "File: " << fileList[i];

		QString filePath = "project/" + Name + "/" + fileList[i];
		
		TakeTracker* tracker = TakeTracker::Create(Name, fileList[i].split(".", QString::SkipEmptyParts).at(0), filePath);

		if (tracker)
			trackers.push_back(tracker);
		else
			qDebug() << "BAD TRACKER FILES";
	}

	QFile file("project/take/info.take");

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QByteArray fileData = file.readAll();
		file.close();

		QJsonObject takeObj = QJsonDocument::fromJson(fileData).object();
		wScale = takeObj["world"].toObject()["s"].toDouble();
		QJsonArray jsonWorldX = takeObj["world"].toObject()["x"].toArray();
		QJsonArray jsonWorldY = takeObj["world"].toObject()["y"].toArray();
		QJsonArray jsonWorldZ = takeObj["world"].toObject()["z"].toArray();
		QJsonArray jsonWorldT = takeObj["world"].toObject()["t"].toArray();

		wX = QVector3D(jsonWorldX[0].toDouble(), jsonWorldX[1].toDouble(), jsonWorldX[2].toDouble());
		wY = QVector3D(jsonWorldY[0].toDouble(), jsonWorldY[1].toDouble(), jsonWorldY[2].toDouble());
		wZ = QVector3D(jsonWorldZ[0].toDouble(), jsonWorldZ[1].toDouble(), jsonWorldZ[2].toDouble());
		wT = QVector3D(jsonWorldT[0].toDouble(), jsonWorldT[1].toDouble(), jsonWorldT[2].toDouble());
	}

	_AdjustRuntime();
}

void Take::Save()
{
	for (int i = 0; i < trackers.count(); ++i)
	{
		trackers[i]->Save();
	}

	QJsonObject jsonObj;
	jsonObj["name"] = "Default Name";

	QJsonArray jsonWorldX;
	jsonWorldX.append(wX.x());
	jsonWorldX.append(wX.y());
	jsonWorldX.append(wX.z());

	QJsonArray jsonWorldY;
	jsonWorldY.append(wY.x());
	jsonWorldY.append(wY.y());
	jsonWorldY.append(wY.z());

	QJsonArray jsonWorldZ;
	jsonWorldZ.append(wZ.x());
	jsonWorldZ.append(wZ.y());
	jsonWorldZ.append(wZ.z());

	QJsonArray jsonWorldTranslate;
	jsonWorldTranslate.append(wT.x());
	jsonWorldTranslate.append(wT.y());
	jsonWorldTranslate.append(wT.z());

	QJsonObject jsonWorld;
	jsonWorld["x"] = jsonWorldX;
	jsonWorld["y"] = jsonWorldY;
	jsonWorld["z"] = jsonWorldZ;
	jsonWorld["t"] = jsonWorldTranslate;
	jsonWorld["s"] = wScale;

	jsonObj["world"] = jsonWorld;

	QJsonDocument jsonDoc(jsonObj);
	QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Indented);

	QFile file("project/take/info.take");

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qDebug() << "Take: Save file failed";
		return;
	}

	file.write(jsonBytes);
	file.close();
}

void Take::_AdjustRuntime()
{
	timeStart = INT32_MAX;
	timeEnd = 0;

	for (int i = 0; i < trackers.count(); ++i)
	{
		TakeTracker* tracker = trackers[i];

		if (tracker->frameCount + tracker->frameOffset > timeEnd)
			timeEnd = tracker->frameCount + tracker->frameOffset;

		if (tracker->frameOffset < timeStart)
			timeStart = tracker->frameOffset;
	}

	timeFrames = timeEnd - timeStart + 1;

	qDebug() << "Timeline" << timeStart << timeEnd << timeFrames;

	markers.clear();

	for (int i = 0; i <= timeEnd; ++i)
		markers.push_back(std::vector<Marker3D>());
}

void Take::SetFrame(int TimelineFrame, bool DrawMarkers)
{
	for (int i = 0; i < trackers.count(); ++i)
	{
		TakeTracker* tracker = trackers[i];

		int keyFrameIndex;
		int frameIndex;
		
		if (tracker->ConvertTimelineToFrame(TimelineFrame, &keyFrameIndex, &frameIndex))
		{
			//qDebug() << tracker->name << " - Timeline:" << TimelineFrame << "Index:" << frameIndex << "KeyIndex:" << keyFrameIndex;

			tracker->drawMarkerFrameIndex = frameIndex;

			if (DrawMarkers)
			{	
				//tracker->DrawMarkers(frameIndex);
			}
			else
			{	
				tracker->DecodeFrame(frameIndex, keyFrameIndex);
				tracker->decoder->ProcessFrame();
			}
		}
		else
		{
			tracker->decoder->ShowBlankFrame();
		}
	}
}

void Take::GenerateMask()
{
	for (int i = 0; i < trackers.count(); ++i)
	{
		trackers[i]->decoder->GenerateMask();
	}
}

void Take::Build2DMarkers(int StartFrame, int EndFrame)
{
	qDebug() << "Start Building 2D Markers";

	for (int i = 0; i < trackers.count(); ++i)
	{
		trackers[i]->Build2DMarkers(StartFrame, EndFrame);
	}

	qDebug() << "Done Building Markers";
}

Marker3D Take::_triangulate(QVector2D P1, QVector2D P2)
{
	std::vector<cv::Point2f> trackerPoints[2];
	trackerPoints[0].push_back(cv::Point2f(P1.x(), P1.y()));
	trackerPoints[1].push_back(cv::Point2f(P2.x(), P2.y()));

	// Match epilines to markers
	cv::Mat Q;
	triangulatePoints(trackers[0]->decoder->projMat, trackers[1]->decoder->projMat, trackerPoints[0], trackerPoints[1], Q);

	// Convert points from homogeneous to world.
	float w = Q.at<float>(3, 0);
	float x = Q.at<float>(0, 0) / w;
	float y = Q.at<float>(1, 0) / w;
	float z = Q.at<float>(2, 0) / w;

	Marker3D m = {};
	m.cam1pos = P1;
	m.cam2pos = P2;
	m.pos = QVector3D(x, y, z);

	return m;
}

// Returns the squared distance between point c and segment ab
float SqDistPointSegment(QVector2D A, QVector2D B, QVector2D C)
{
	QVector2D ab = B - A;
	QVector2D ac = C - A;
	QVector2D bc = C - B;

	float e = QVector2D::dotProduct(ac, ab);

	// Handle cases where c projects outside ab
	if (e <= 0.0f)
		return QVector2D::dotProduct(ac, ac);

	float f = QVector2D::dotProduct(ab, ab);

	if (e >= f)
		return QVector2D::dotProduct(bc, bc);
	
	// Handle cases where c projects onto ab
	return QVector2D::dotProduct(ac, ac) - e * e / f;
}

void Take::Build3DMarkers(int StartFrame, int EndFrame)
{
	qDebug() << "Start Building 3D Markers";

	// TODO: Don't globally clear so frames can retain through different processing ranges.
	markers.clear();

	for (int i = 0; i < timeFrames; ++i)
	{
		markers.push_back(std::vector<Marker3D>());
	}

	for (int i = 0; i < trackers.count(); ++i)
	{
		trackers[i]->BuildEpilines(StartFrame, EndFrame);
	}

#if 1
	// Build markers from the camera pair.
	for (int i = StartFrame; i <= EndFrame; ++i)
	{
		VidFrameData* a = trackers[0]->GetLocalFrame(i);
		VidFrameData* b = trackers[1]->GetLocalFrame(i);

		if (a && b)
		{
			if (a->newMarkers.count() > 0 && b->newMarkers.count() > 0)
			{
				// Go through all epilines in cam A and look for points in cam B.
				for (int e = 0; e < a->epiLines.count(); ++e)
				{
					int closestIndex = -1;
					float closestD = 10000.0f;

					EpipolarLine line = a->epiLines[e];
					float y1 = -(line.a * 0 + line.c) / line.b;
					float y2 = -(line.a * 1000 + line.c) / line.b;

					for (int p = 0; p < b->newMarkers.count(); ++p)
					{
						float dist = SqDistPointSegment(QVector2D(0, y1), QVector2D(1000, y2), b->newMarkers[p].pos);

						if (dist <= 10.0f && dist < closestD)
						{
							closestD = dist;
							closestIndex = p;
						}
					}

					if (closestIndex != -1)
					{
						qDebug() << "closest" << closestD;
						markers[i].push_back(_triangulate(a->newMarkers[e].pos, b->newMarkers[closestIndex].pos));
					}
				}
			}
		}
	}
#else
	// Build markers from the camera pair.
	for (int i = StartFrame; i <= EndFrame; ++i)
	{
		VidFrameData* a = trackers[0]->GetLocalFrame(i);
		VidFrameData* b = trackers[1]->GetLocalFrame(i);

		if (a && b)
		{
			if (a->markers.count() > 0 && b->markers.count() > 0)
			{
				// Go through all epilines in cam A and look for points in cam B.
				for (int e = 0; e < a->epiLines.count(); ++e)
				{
					int closestIndex = -1;
					float closestD = 10000.0f;

					EpipolarLine line = a->epiLines[e];
					float y1 = -(line.a * 0 + line.c) / line.b;
					float y2 = -(line.a * 1000 + line.c) / line.b;

					for (int p = 0; p < b->markers.count(); ++p)
					{
						float dist = SqDistPointSegment(QVector2D(0, y1), QVector2D(1000, y2), b->markers[p]);

						if (dist <= 10.0f && dist < closestD)
						{
							closestD = dist;
							closestIndex = p;
						}
					}

					if (closestIndex != -1)
					{
						qDebug() << "closest" << closestD;
						markers[i].push_back(_triangulate(a->markers[e], b->markers[closestIndex]));
					}
				}
			}
		}
	}
#endif

	for (int i = 0; i < markers.size(); ++i)
	{
		qDebug() << "3D Marker" << i << markers[i].size();
	}

	qDebug() << "Done Building Markers";
}

void Take::BuildFundamental(int StartFrame, int EndFrame, SceneView* Scene)
{
	qDebug() << "Start Building Fundamental";

	std::vector<cv::Point2f> trackerPoints[2];
	//epiLines.clear();
	
	int tfIndex[2] = {};

	while (true)
	{
		VidFrameData* t0 = &trackers[0]->vidFrameData[tfIndex[0]];
		VidFrameData* t1 = &trackers[1]->vidFrameData[tfIndex[1]];

		int tfT0 = trackers[0]->frameOffset + t0->index;
		int tfT1 = trackers[1]->frameOffset + t1->index;

		if (t0->type == 2 || t0->type == 2)
		{
			if (t0->type == 2) tfIndex[0]++;
			if (t1->type == 2) tfIndex[1]++;
			continue;
		}
		
		if (tfT0 < tfT1)
		{
			tfIndex[0]++;
		}
		else if (tfT0 > tfT1)
		{
			tfIndex[1]++;
		}
		else
		{
			if (tfT0 > EndFrame)
			{
				break;
			}
			else if (tfT0 >= StartFrame)
			{
				if (t0->markers.count() == 1 && t1->markers.count() == 1)
				{
					qDebug() << "Add markers - Frame:" << tfT0;

					trackerPoints[0].push_back(cv::Point2f(t0->markers[0].x(), t0->markers[0].y()));
					trackerPoints[1].push_back(cv::Point2f(t1->markers[0].x(), t1->markers[0].y()));
					
					//EpipolarLine line = {};
					//line.timelineFrame = tfT0;
					//epiLines.push_back(line);

					//Marker3D marker = {};
					//marker.timelineFrame = tfT0;
					//marker.cam1pos = QVector2D(t0->markers[0].x(), t0->markers[0].y());
					//marker.cam2pos = QVector2D(t1->markers[0].x(), t1->markers[0].y());
					//markers.push_back(marker);
				}
			}

			tfIndex[0]++;
			tfIndex[1]++;
		}
	}

	cv::Mat fMats;
	try
	{
		fMats = cv::findFundamentalMat(trackerPoints[0], trackerPoints[1], CV_FM_LMEDS, 3.0, 0.99);
		//fMats = cv::findFundamentalMat(trackerPoints[0], trackerPoints[1], CV_FM_RANSAC, 3.0, 0.99);
	}
	catch (cv::Exception& e)
	{
		const char* err_msg = e.what();
		qDebug() << "Fundamental Mat Failed: " << err_msg;
		return;
	}

	qDebug() << "Done Building Fundamental" << fMats.size().width << "x" << fMats.size().height;

	std::vector<cv::Point3f> elines;
	std::vector<cv::Point3f> elines2;

	trackers[0]->decoder->fundamentalMat = fMats;
	trackers[1]->decoder->fundamentalMat = fMats;

	cv::computeCorrespondEpilines(trackerPoints[0], 1, fMats, elines);
	cv::computeCorrespondEpilines(trackerPoints[1], 2, fMats, elines2);
	
	for (int i = 0; i < elines.size(); ++i)
	{
		/*
		epiLines[i].a = elines[i].x;
		epiLines[i].b = elines[i].y;
		epiLines[i].c = elines[i].z;

		epiLines[i].a2 = elines2[i].x;
		epiLines[i].b2 = elines2[i].y;
		epiLines[i].c2 = elines2[i].z;
		*/
	}

	cv::Mat essentialMat(3, 3, CV_64F);
	// NOTE: Second camera K first.
	essentialMat = trackers[1]->decoder->optCamMat.t() * fMats * trackers[0]->decoder->optCamMat;

	/*
	// Manual	
	cv::SVD svd(essentialMat);
	cv::Mat u = svd.u;
	cv::Mat vt = svd.vt;
	cv::Mat w = svd.w;
	cv::Mat W(3, 3, CV_64F);
	W.at<double>(0, 0) = 0;
	W.at<double>(0, 1) = -1;
	W.at<double>(0, 2) = 0;
	W.at<double>(1, 0) = 1;
	W.at<double>(1, 1) = 0;
	W.at<double>(1, 2) = 0;
	W.at<double>(2, 0) = 0;
	W.at<double>(2, 1) = 0;
	W.at<double>(2, 2) = 1;
	cv::Mat_<double> R1 = u * cv::Mat(W) * vt;
	cv::Mat_<double> R2 = u * cv::Mat(W).t() * vt;
	cv::Mat_<double> T = u.col(2);

	s.str("");
	s << T << "\n\n" << R1 << "\n\n" << R2;
	qDebug() << "SVD: " << s.str().c_str();

	cv::Mat decompR;
	cv::Rodrigues(r, decompR);

	s.str("");
	s << decompR;
	qDebug() << "Decomp: " << s.str().c_str();
	*/

	cv::Mat r;
	cv::Mat t;
	// TODO: Different FL and CP for each camera.
	cv::recoverPose(essentialMat, trackerPoints[0], trackerPoints[1], r, t, 2.2612, cv::Point2d(500, 350));
	cv::Mat trueT = -(r).t() * t;

	// Triangulate Test
	cv::Mat P0 = cv::Mat::eye(3, 4, r.type());
	cv::Mat P1(3, 4, r.type());
	P1(cv::Range::all(), cv::Range(0, 3)) = r * 1.0;
	P1.col(3) = t * 1.0;

	trackers[0]->decoder->unitProjMat = P0.clone();
	trackers[1]->decoder->unitProjMat = P1.clone();

	P0 = trackers[0]->decoder->optCamMat * P0;
	P1 = trackers[1]->decoder->optCamMat * P1;
	
	trackers[0]->decoder->projMat = P0;
	trackers[1]->decoder->projMat = P1;

	cv::Mat Q;
	triangulatePoints(P0, P1, trackerPoints[0], trackerPoints[1], Q);
	
	for (int i = 0; i < Q.size().width; ++i)
	{
		float w = Q.at<float>(3, i);
		float x = Q.at<float>(0, i) / w;
		float y = Q.at<float>(1, i) / w;
		float z = Q.at<float>(2, i) / w;

		Scene->pushSamplePoint(QVector3D(x, y, z));
		//markers[i].pos = QVector3D(x, y, z);
	}

	QMatrix4x4 worldMat;
	worldMat(0, 0) = r.at<double>(0, 0);
	worldMat(1, 0) = r.at<double>(0, 1);
	worldMat(2, 0) = r.at<double>(0, 2);
	worldMat(3, 0) = 0;

	worldMat(0, 1) = r.at<double>(1, 0);
	worldMat(1, 1) = r.at<double>(1, 1);
	worldMat(2, 1) = r.at<double>(1, 2);
	worldMat(3, 1) = 0;

	worldMat(0, 2) = r.at<double>(2, 0);
	worldMat(1, 2) = r.at<double>(2, 1);
	worldMat(2, 2) = r.at<double>(2, 2);
	worldMat(3, 2) = 0;

	worldMat(0, 3) = trueT.at<double>(0);
	worldMat(1, 3) = trueT.at<double>(1);
	worldMat(2, 3) = trueT.at<double>(2);
	worldMat(3, 3) = 1;

	trackers[0]->decoder->worldMat.setToIdentity();
	trackers[1]->decoder->worldMat = worldMat;

	/*
	cv::Mat dpmC;
	cv::Mat dpmR;
	cv::Mat dpmT;
	cv::Mat eulers;
	cv::decomposeProjectionMatrix(P1, dpmC, dpmR, dpmT, cv::noArray(), cv::noArray(), cv::noArray(), eulers);
	Scene->cam2Rot = { (float)eulers.at<double>(0), (float)eulers.at<double>(1), (float)eulers.at<double>(2) };

	// MAGICAL MATRIX CONSTRUCTION ACHEIVED
	QMatrix4x4 cam2Mat;
	cam2Mat.setToIdentity();
	cam2Mat.translate(Scene->cam2Translate.x(), Scene->cam2Translate.y(), Scene->cam2Translate.z());	
	cam2Mat.rotate(-Scene->cam2Rot.x(), 1.0f, 0.0f, 0.0f);	
	cam2Mat.rotate(-Scene->cam2Rot.y(), 0.0f, 1.0f, 0.0f);
	cam2Mat.rotate(-Scene->cam2Rot.z(), 0.0f, 0.0f, 1.0f);
	qDebug() << "Built Mat:" << cam2Mat;
	*/
}