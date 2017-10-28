#include "takeTracker.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QElapsedTimer>
#include "sceneView.h"

QMatrix4x4 TakeTracker::WorldFromPose(cv::Mat Pose)
{
	QMatrix4x4 result;

	cv::Mat camR = cv::Mat(3, 3, CV_64F);
	cv::Mat camT = cv::Mat(3, 1, CV_64F);

	for (int iY = 0; iY < 3; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			camR.at<double>(iX, iY) = Pose.at<double>(iX, iY);
		}
	}

	for (int iX = 0; iX < 3; ++iX)
	{
		camT.at<double>(iX, 0) = Pose.at<double>(iX, 3);
	}

	cv::Mat trueT = -(camR).t() * camT;

	result(0, 0) = camR.at<double>(0, 0);
	result(1, 0) = camR.at<double>(0, 1);
	result(2, 0) = camR.at<double>(0, 2);
	result(3, 0) = 0;

	result(0, 1) = camR.at<double>(1, 0);
	result(1, 1) = camR.at<double>(1, 1);
	result(2, 1) = camR.at<double>(1, 2);
	result(3, 1) = 0;

	result(0, 2) = camR.at<double>(2, 0);
	result(1, 2) = camR.at<double>(2, 1);
	result(2, 2) = camR.at<double>(2, 2);
	result(3, 2) = 0;

	result(0, 3) = trueT.at<double>(0);
	result(1, 3) = trueT.at<double>(1);
	result(2, 3) = trueT.at<double>(2);
	result(3, 3) = 1;

	return result;
}

TakeTracker* TakeTracker::Create(int Id, QString TakeName, uint32_t Serial, QString FilePath, LiveTracker* LiveTracker)
{
	QFile file(FilePath);

	qDebug() << "Tracker: Load" << Id << TakeName << Serial << FilePath;

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Tracker: Load file failed";
		return 0;
	}
	
	QByteArray fileData = file.readAll();
	file.close();
	QJsonObject trackerObj = QJsonDocument::fromJson(fileData).object();
	
	TakeTracker* tracker = new TakeTracker();
	tracker->id = Id;
	tracker->takeName = TakeName;
	tracker->serial = Serial;
	tracker->liveTracker = LiveTracker;
	tracker->name = trackerObj["name"].toString();
	tracker->exposure = trackerObj["exposure"].toInt();
	tracker->fps = trackerObj["fps"].toInt();
	tracker->iso = trackerObj["iso"].toInt();
	tracker->threshold = trackerObj["threshold"].toDouble();
	tracker->sensitivity = trackerObj["sensitivity"].toDouble();
	tracker->frameOffset = trackerObj["offset"].toInt();
	tracker->frameCount = 0;

	LiveTracker->loaded = true;
	LiveTracker->id = tracker->id;
	LiveTracker->name = tracker->name;
	LiveTracker->serial = tracker->serial;
	
	QJsonArray jsonIntrinMat = trackerObj["intrinsic"].toObject()["camera"].toArray();
	QJsonArray jsonIntrinDist = trackerObj["intrinsic"].toObject()["distortion"].toArray();
	QJsonArray jsonExtrinProj = trackerObj["extrinsic"].toObject()["pose"].toArray();
	
	tracker->decoder = new Decoder();
	tracker->decoder->camSensitivity = tracker->sensitivity;
	tracker->decoder->drawMarkers = false;
	tracker->decoder->drawUndistorted = false;
	tracker->decoder->camThreshold = tracker->threshold;

	// Default cam calibration.
	/*
	calibDistCoeffs.at<double>(0) = -0.332945;
	calibDistCoeffs.at<double>(1) = 0.12465;
	calibDistCoeffs.at<double>(2) = 0.0020142;
	calibDistCoeffs.at<double>(3) = 0.000755178;
	calibDistCoeffs.at<double>(4) = -0.029228;

	calibCameraMatrix.at<double>(0, 0) = 740;
	calibCameraMatrix.at<double>(1, 0) = 0;
	calibCameraMatrix.at<double>(2, 0) = 0;
	calibCameraMatrix.at<double>(0, 1) = 0;
	calibCameraMatrix.at<double>(1, 1) = 740;
	calibCameraMatrix.at<double>(2, 1) = 0;
	calibCameraMatrix.at<double>(0, 2) = 512;
	calibCameraMatrix.at<double>(1, 2) = 352;
	calibCameraMatrix.at<double>(2, 2) = 1;
	*/

	tracker->distCoefs = cv::Mat::zeros(5, 1, CV_64F);
	tracker->distCoefs.at<double>(0) = jsonIntrinDist[0].toDouble();
	tracker->distCoefs.at<double>(1) = jsonIntrinDist[1].toDouble();
	tracker->distCoefs.at<double>(2) = jsonIntrinDist[2].toDouble();
	tracker->distCoefs.at<double>(3) = jsonIntrinDist[3].toDouble();
	tracker->distCoefs.at<double>(4) = jsonIntrinDist[4].toDouble();

	tracker->camMat = cv::Mat::eye(3, 3, CV_64F);
	tracker->camMat.at<double>(0, 0) = jsonIntrinMat[0].toDouble();
	tracker->camMat.at<double>(1, 0) = jsonIntrinMat[1].toDouble();
	tracker->camMat.at<double>(2, 0) = jsonIntrinMat[2].toDouble();
	tracker->camMat.at<double>(0, 1) = jsonIntrinMat[3].toDouble();
	tracker->camMat.at<double>(1, 1) = jsonIntrinMat[4].toDouble();
	tracker->camMat.at<double>(2, 1) = jsonIntrinMat[5].toDouble();
	tracker->camMat.at<double>(0, 2) = jsonIntrinMat[6].toDouble();
	tracker->camMat.at<double>(1, 2) = jsonIntrinMat[7].toDouble();
	tracker->camMat.at<double>(2, 2) = jsonIntrinMat[8].toDouble();

	//optCamMat = getOptimalNewCameraMatrix(calibCameraMatrix, calibDistCoeffs, Size(VID_W, VID_H), 0.0, Size(VID_W, VID_H), NULL, false);
	tracker->camMatOpt = cv::getOptimalNewCameraMatrix(tracker->camMat, tracker->distCoefs, cv::Size(VID_W, VID_H), 0.0, cv::Size(VID_W, VID_H), NULL, true);

	cv::Mat pose = cv::Mat::eye(3, 4, CV_64F);
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			pose.at<double>(iX, iY) = jsonExtrinProj[iY * 3 + iX].toDouble();
		}
	}

	tracker->SetPose(pose);

#if 0
	//---------------------------------------------------------------------------------------------------------------
	// Load and process refined data.
	//---------------------------------------------------------------------------------------------------------------
	Decoder* d = tracker->decoder;

	d->refR = cv::Mat(3, 3, CV_64F);
	d->refT = cv::Mat(3, 1, CV_64F);
	d->refD = cv::Mat::zeros(4, 1, CV_64F);
	d->refK = cv::Mat::eye(3, 3, CV_64F);
	d->refRt = cv::Mat::eye(3, 4, CV_64F);

	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			d->refRt.at<double>(iX, iY) = jsonExtrinRt[iX * 4 + iY].toDouble();
		}
	}

	for (int iY = 0; iY < 3; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			d->refR.at<double>(iX, iY) = d->refRt.at<double>(iX, iY);
		}
	}

	for (int iX = 0; iX < 3; ++iX)
	{
		d->refT.at<double>(iX, 0) = d->refRt.at<double>(iX, 3);
	}

	for (int iX = 0; iX < 4; ++iX)
	{
		d->refD.at<double>(iX) = jsonExtrinD[iX].toDouble();
	}

	for (int iY = 0; iY < 3; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			d->refK.at<double>(iX, iY) = jsonExtrinK[iY * 3 + iX].toDouble();
		}
	}

	d->refOptK = cv::getOptimalNewCameraMatrix(d->refK, d->refD, cv::Size(VID_W, VID_H), 0.0, cv::Size(VID_W, VID_H), NULL, true);

	cv::Mat trueT = -(d->refR).t() * d->refT;
	d->refPu = cv::Mat(3, 4, d->refR.type());
	d->refPu(cv::Range::all(), cv::Range(0, 3)) = d->refR * 1.0;
	d->refPu.col(3) = d->refT * 1.0;

	d->refP = d->refOptK * d->refPu.clone();

	d->refWorldMat(0, 0) = d->refR.at<double>(0, 0);
	d->refWorldMat(1, 0) = d->refR.at<double>(0, 1);
	d->refWorldMat(2, 0) = d->refR.at<double>(0, 2);
	d->refWorldMat(3, 0) = 0;

	d->refWorldMat(0, 1) = d->refR.at<double>(1, 0);
	d->refWorldMat(1, 1) = d->refR.at<double>(1, 1);
	d->refWorldMat(2, 1) = d->refR.at<double>(1, 2);
	d->refWorldMat(3, 1) = 0;

	d->refWorldMat(0, 2) = d->refR.at<double>(2, 0);
	d->refWorldMat(1, 2) = d->refR.at<double>(2, 1);
	d->refWorldMat(2, 2) = d->refR.at<double>(2, 2);
	d->refWorldMat(3, 2) = 0;

	d->refWorldMat(0, 3) = trueT.at<double>(0);
	d->refWorldMat(1, 3) = trueT.at<double>(1);
	d->refWorldMat(2, 3) = trueT.at<double>(2);
	d->refWorldMat(3, 3) = 1;

	QVector4D hwPos = d->refWorldMat * QVector4D(0, 0, 0, 1);

	d->refWorldPos.setX(hwPos.x() / hwPos.w());
	d->refWorldPos.setY(hwPos.y() / hwPos.w());
	d->refWorldPos.setZ(hwPos.z() / hwPos.w());

	/*
	stringstream ss;
	ss << tracker->rtMat << endl << endl << tracker->decoder->refinedMat << endl << endl
		<< tracker->decoder->refR << endl << endl << tracker->decoder->refT << endl << endl;
	qDebug() << "Check Mats:" << ss.str().c_str();
	*/

	//---------------------------------------------------------------------------------------------------------------
#endif

	char fileName[256];
	QString maskFilePath = "project/" + TakeName + "/" + QString::number(Serial) + ".mask";
	FILE* maskFile = fopen(maskFilePath.toLatin1(), "rb");

	if (maskFile)
	{
		fread(tracker->mask, sizeof(mask), 1, maskFile);
		LiveTracker->setMask(tracker->mask);
		memcpy(tracker->decoder->frameMaskData, tracker->mask, sizeof(mask));
		fclose(maskFile);
	}

	QString infoFilePath = "project/" + TakeName + "/" + QString::number(Serial) + ".trakvid";
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
		if (rp >= dataSize - 20)
			break;

		uint8_t* md = tracker->takeClipData + rp;
		/*
		int size = md[3] << 24 | md[2] << 16 | md[1] << 8 | md[0];
		int type = md[7] << 24 | md[6] << 16 | md[5] << 8 | md[4];
		int time = md[11] << 24 | md[10] << 16 | md[9] << 8 | md[8];
		rp += 12;
		//*/
		//*
		int size = md[3] << 24 | md[2] << 16 | md[1] << 8 | md[0];
		int type = md[7] << 24 | md[6] << 16 | md[5] << 8 | md[4];
		int tempAvgMasterOffset = md[11] << 24 | md[10] << 16 | md[9] << 8 | md[8];
		int64_t frameId = md[19] << 56 | md[18] << 48 | md[17] << 40 | md[16] << 32 | md[15] << 24 | md[14] << 16 | md[13] << 8 | md[12];
		frameId -= 6000;
		rp += 20;
		//int time = frameId * 19941;
		int time = frameId * 9970;
		//*/

		if (rp + size >= dataSize)
			break;

		memcpy(tracker->takeClipData + wp, tracker->takeClipData + rp, size);
		rp += size;

		//qDebug() << frameId << type << size;

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
			//int frameProgress = dtMs / (1000 / tracker->fps);
			//int frameProgress = dtMs / (19941 / 1000);
			int tempDummies = frameId - tracker->frameCount;
			prevTime = time;

			//while (frameProgress-- > 1)
			while (tempDummies-- > 0)
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

	QFile m2dFile("project/" + TakeName + "/" + QString::number(Serial) + ".m2d");

	if (m2dFile.open(QIODevice::ReadOnly))
	{
		QDataStream stream(&m2dFile);

		for (int i = 0; i < tracker->vidFrameData.count(); ++i)
		{
			//int markerCount = 0;
			//stream >> markerCount;

			//for (int m = 0; m < markerCount; ++m)
			{
				//QVector2D pos;
				//stream >> pos;
				//tracker->vidFrameData[i].markers.push_back(pos);
			}

			int newMarkerCount = 0;
			stream >> newMarkerCount;

			for (int m = 0; m < newMarkerCount; ++m)
			{
				QVector2D pos;
				QVector2D distPos;
				QVector4D bounds;
				stream >> bounds;
				stream >> pos;
				stream >> distPos;
				Marker2D marker = {};
				marker.pos = pos;
				marker.bounds = bounds;
				marker.distPos = distPos;
				marker.trackerId = Id;
				tracker->vidFrameData[i].newMarkers.push_back(marker);
			}
		}
		
		m2dFile.close();
	}
	else
	{
		qDebug() << "No 2D Markers file";
	}


	qDebug() << "Tracker: Loaded" << Serial;

	//*
	for (int i = 0; i < tracker->vidFrameData.count(); ++i)
	{
		VidFrameData* vfdp = &tracker->vidFrameData[i];
		qDebug() << "Post - Index:" << i << "Frame:" << vfdp->index << "Type:" << vfdp->type << "Time:" << vfdp->time;
	}
	//*/
	
	return tracker;
}

TakeTracker::TakeTracker()
{
	drawMarkerFrameIndex = 0;
}

TakeTracker::~TakeTracker()
{
}

void TakeTracker::SetCamDist(cv::Mat Cam, cv::Mat Dist)
{
	camMat = Cam.clone();
	distCoefs = Dist.clone();

	camMatOpt = cv::getOptimalNewCameraMatrix(camMat, distCoefs, cv::Size(VID_W, VID_H), 0.0, cv::Size(VID_W, VID_H), NULL, true);

	std::stringstream s;
	s << camMat << "\n\n" << camMatOpt;
	qDebug() << "OPT" << s.str().c_str();

	projMat = camMatOpt * rtMat;
}

void TakeTracker::SetPose(cv::Mat Pose)
{
	rtMat = Pose.clone();
	worldMat = WorldFromPose(rtMat);

	QVector4D hwPos = worldMat * QVector4D(0, 0, 0, 1);
	worldPos.setX(hwPos.x() / hwPos.w());
	worldPos.setY(hwPos.y() / hwPos.w());
	worldPos.setZ(hwPos.z() / hwPos.w());

	projMat = camMatOpt * rtMat;

	// Rebuild rays for 2D markers.
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

	_currentDecodeFrameIndex = procFrame;
	int t = pt.elapsed();

	//qDebug() << "Perf" << t;
}

void TakeTracker::AdvanceFrame(int FrameCount)
{
	DecodeFrame(_currentDecodeFrameIndex + FrameCount - 1, _currentDecodeFrameIndex);
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
	qDebug() << "Tracker: Save" << id << takeName << name;

	QJsonObject jsonObj;
	jsonObj["name"] = name;
	jsonObj["fps"] = fps;
	jsonObj["exposure"] = exposure;
	jsonObj["iso"] = iso;
	jsonObj["threshold"] = threshold;
	jsonObj["sensitivity"] = sensitivity;
	jsonObj["offset"] = frameOffset;

	QJsonArray jsonIntrinMat;
	jsonIntrinMat.append(camMat.at<double>(0, 0));
	jsonIntrinMat.append(camMat.at<double>(1, 0));
	jsonIntrinMat.append(camMat.at<double>(2, 0));
	jsonIntrinMat.append(camMat.at<double>(0, 1));
	jsonIntrinMat.append(camMat.at<double>(1, 1));
	jsonIntrinMat.append(camMat.at<double>(2, 1));
	jsonIntrinMat.append(camMat.at<double>(0, 2));
	jsonIntrinMat.append(camMat.at<double>(1, 2));
	jsonIntrinMat.append(camMat.at<double>(2, 2));

	QJsonArray jsonIntrinDist;
	jsonIntrinDist.append(distCoefs.at<double>(0));
	jsonIntrinDist.append(distCoefs.at<double>(1));
	jsonIntrinDist.append(distCoefs.at<double>(2));
	jsonIntrinDist.append(distCoefs.at<double>(3));
	jsonIntrinDist.append(distCoefs.at<double>(4));

	QJsonObject jsonIntrin;
	jsonIntrin["camera"] = jsonIntrinMat;
	jsonIntrin["distortion"] = jsonIntrinDist;

	jsonObj["intrinsic"] = jsonIntrin;

	QJsonArray jsonExtrinProj;
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			jsonExtrinProj.append(rtMat.at<double>(iX, iY));
		}
	}
	
	/*
	QJsonArray jsonExtrinWorld;
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 4; ++iX)
		{
			jsonExtrinWorld.append(worldMat(iX, iY));
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
	*/

	QJsonObject jsonExtrin;
	jsonExtrin["pose"] = jsonExtrinProj;
	//jsonExtrin["world"] = jsonExtrinWorld;
	//jsonExtrin["fundamental"] = jsonExtrinFundamental;

	jsonObj["extrinsic"] = jsonExtrin;

	QJsonDocument jsonDoc(jsonObj);
	QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Indented);

	QFile file("project/" + takeName + "/" + QString::number(serial) + ".tracker");

	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qDebug() << "Tracker: Save file failed";
		return;
	}

	file.write(jsonBytes);
	file.close();

	QFile m2dFile("project/" + takeName + "/" + QString::number(serial) + ".m2d");

	if (!m2dFile.open(QIODevice::WriteOnly))
	{
		qDebug() << "Tracker: Save file failed";
		return;
	}

	QDataStream m2dStream(&m2dFile);

	for (int i = 0; i < vidFrameData.count(); ++i)
	{
		/*
		m2dStream << vidFrameData[i].markers.count();

		for (int m = 0; m < vidFrameData[i].markers.count(); ++m)
		{
			m2dStream << vidFrameData[i].markers[m];
		}
		*/

		m2dStream << vidFrameData[i].newMarkers.count();

		for (int iM = 0; iM < vidFrameData[i].newMarkers.count(); ++iM)
		{
			Marker2D* m = &vidFrameData[i].newMarkers[iM];
			m2dStream << m->bounds << m->pos << m->distPos;
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
		vidFrameData[i].newMarkers = decoder->ProcessFrameNewMarkers();
		qint64 t2 = tmr.nsecsElapsed();

		float tTotal = (t2 / 1000) / 1000.0;
		float tDecode = (t1 / 1000) / 1000.0;
		float tCentroids = ((t2 - t1) / 1000) / 1000.0;

		qDebug() << i << "Markers:" << vidFrameData[i].newMarkers.count() << "Time:" << tTotal << tDecode << tCentroids;
	}

	UndistortMarkers(StartFrame, EndFrame);
}

void TakeTracker::UndistortMarkers(int StartFrame, int EndFrame)
{
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

	for (int i = localStartFrame; i <= localEndFrame; ++i)
	{
		// Undistort
		if (vidFrameData[i].newMarkers.size() > 0)
		{
			cv::Mat_<cv::Point2f> matPoint(1, vidFrameData[i].newMarkers.size());
			for (int j = 0; j < vidFrameData[i].newMarkers.size(); ++j)
				matPoint(j) = cv::Point2f(vidFrameData[i].newMarkers[j].distPos.x(), vidFrameData[i].newMarkers[j].distPos.y());

			cv::Mat matOutPoints;
			// NOTE: Just use the opt calib matrix.
			cv::undistortPoints(matPoint, matOutPoints, camMat, distCoefs, cv::noArray(), camMatOpt);
			//cv::undistortPoints(matPoint, matOutPoints, camMatOpt, decoder->_calibDistCoeffs, cv::noArray(), camMatOpt);

			// Clip markers.
			for (int j = 0; j < matOutPoints.size().width; ++j)
			{
				cv::Point2f p = matOutPoints.at<cv::Point2f>(j);

				if (p.x >= 0 && p.x < VID_W && p.y >= 0 && p.y < VID_H)
				{
					vidFrameData[i].newMarkers[j].pos = QVector2D(p.x, p.y);
				}
			}
		}
	}
}

void TakeTracker::BuildRays(int StartFrame, int EndFrame)
{
	UndistortMarkers(StartFrame, EndFrame);

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

	cv::Matx33d m33((double*)camMatOpt.ptr());
	cv::Matx33d m33Inv = m33.inv();

	for (int i = localStartFrame; i <= localEndFrame; ++i)
	{	
		// Project rays.
		for (int j = 0; j < vidFrameData[i].newMarkers.count(); ++j)
		{
			Marker2D* m = &vidFrameData[i].newMarkers[j];

			cv::Matx31d imgPt(m->pos.x(), m->pos.y(), 1);
			imgPt = m33Inv * imgPt;
			QVector3D d((float)imgPt(0, 0), (float)imgPt(1, 0), (float)imgPt(2, 0));
			d.normalize();

			m->worldRayD = (worldMat * QVector4D(d, 0)).toVector3D();
		}
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