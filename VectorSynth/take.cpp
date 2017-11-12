#include "take.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QElapsedTimer>
#include <QProcess>

Take::Take()
{
	timeFrames = 0;
	timeEnd = 0;
	isLive = false;
	isRecording = false;
	frameDuration = 0;
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

		if (trackers[i]->takeClipData)
			delete[] trackers[i]->takeClipData;
	}

	qDeleteAll(trackers);
	trackers.clear();
}

void Take::LoadTake(QString Name)
{
	Destroy();

	name = Name;

	QDir dir("project/" + Name);

	QStringList trackVidFilter;
	trackVidFilter.push_back("*.tracker");
	QStringList fileList = dir.entryList(trackVidFilter, QDir::Files);

	for (int i = 0; i < fileList.count(); ++i)
	{
		qDebug() << "File: " << fileList[i];

		QString filePath = "project/" + Name + "/" + fileList[i];
		
		int trackerId = trackers.size();
		LiveTracker* liveTracker = new LiveTracker();
		liveTrackers[trackerId] = liveTracker;
		TakeTracker* tracker = TakeTracker::Create(trackerId, Name, fileList[i].split(".", QString::SkipEmptyParts).at(0).toUInt(), filePath, liveTracker);

		if (tracker)
			trackers.push_back(tracker);
		else
			qDebug() << "BAD TRACKER FILES";
	}

	QFile file("project/" + Name + "/" + Name + ".take");

	if (file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QByteArray fileData = file.readAll();
		file.close();

		QJsonObject takeObj = QJsonDocument::fromJson(fileData).object();
		frameDuration = takeObj["fps"].toInt();
	}

	_AdjustRuntime();

	for (int i = 0; i < timeFrames; ++i)
	{
		refMarkers.push_back(QVector3D());
	}
}

void Take::Save()
{
	for (int i = 0; i < trackers.count(); ++i)
	{
		trackers[i]->Save();
	}

	QJsonObject jsonObj;
	jsonObj["fps"] = frameDuration;

	QJsonDocument jsonDoc(jsonObj);
	QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Indented);

	QFile file("project/" + name + "/" + name + ".take");

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
	timeEnd = 0;

	for (int i = 0; i < trackers.count(); ++i)
	{
		TakeTracker* tracker = trackers[i];

		qDebug() << "Local Timeline" << tracker->frameCount;

		if (tracker->frameCount > timeEnd)
			timeEnd = tracker->frameCount;
	}

	timeFrames = timeEnd + 1;

	qDebug() << "Timeline" << timeEnd << timeFrames;

	markers.clear();

	for (int i = 0; i <= timeEnd; ++i)
	{
		markers.push_back(std::vector<Marker3D>());
	}
}

void Take::SetFrame(int TimelineFrame)
{
	for (int i = 0; i < trackers.count(); ++i)
	{
		TakeTracker* tracker = trackers[i];

		int keyFrameIndex;
		int frameIndex;

		if (tracker->mode == 2)
		{
			if (TimelineFrame < tracker->frameCount)
				tracker->drawMarkerFrameIndex = TimelineFrame;
		}
		else if (tracker->mode == 1)
		{
			if (tracker->ConvertTimelineToFrame(TimelineFrame, &keyFrameIndex, &frameIndex))
			{
				//qDebug() << tracker->name << " - Timeline:" << TimelineFrame << "Index:" << frameIndex << "KeyIndex:" << keyFrameIndex;

				tracker->DecodeFrame(frameIndex, keyFrameIndex);
				tracker->decoder->ProcessFrame();
				memcpy(tracker->liveTracker->frameData, tracker->decoder->GetFrameMatData(), VID_W * VID_H * 3);
			}
			else
			{
				tracker->decoder->ShowBlankFrame();
				memcpy(tracker->liveTracker->frameData, tracker->decoder->GetFrameMatData(), VID_W * VID_H * 3);
			}
		}
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

// Returns the squared distance between point c and segment ab
float SqDistPointSegment3D(QVector3D A, QVector3D B, QVector3D C)
{
	QVector3D ab = B - A;
	QVector3D ac = C - A;
	QVector3D bc = C - B;

	float e = QVector3D::dotProduct(ac, ab);

	// Handle cases where c projects outside ab
	if (e <= 0.0f)
		return QVector3D::dotProduct(ac, ac);

	float f = QVector3D::dotProduct(ab, ab);

	if (e >= f)
		return QVector3D::dotProduct(bc, bc);

	// Handle cases where c projects onto ab
	return QVector3D::dotProduct(ac, ac) - e * e / f;
}

void Take::_ClosestPointsLines(QVector3D P1, QVector3D D1, QVector3D P2, QVector3D D2, QVector3D* C1, QVector3D* C2)
{
	QVector3D r = P1 - P2;
	float a = QVector3D::dotProduct(D1, D1);
	float b = QVector3D::dotProduct(D1, D2);
	float c = QVector3D::dotProduct(D1, r);
	float e = QVector3D::dotProduct(D2, D2);
	float f = QVector3D::dotProduct(D2, r);
	float denom = a * e - b * b;

	float t = 0.0f;
	float s = 0.0f;

	if (denom != 0.0f)
		s = (b * f - c * e) / denom;

	t = (b * s + f) / e;

	*C1 = P1 + D1 * s;
	*C2 = P2 + D2 * t;
}

void Take::Build3DMarkers(int StartFrame, int EndFrame)
{	
	// TODO: Don't globally clear so frames can retain through different processing ranges.
	markers.clear();

	for (int i = 0; i < timeFrames; ++i)
	{
		markers.push_back(std::vector<Marker3D>());
	}

	QElapsedTimer t;
	t.start();
	
	for (int i = 0; i < trackers.count(); ++i)
	{
		trackers[i]->BuildRays(StartFrame, EndFrame);
	}

	float pixelTolerance = 3.0f;
	float pixelToleranceSqr = pixelTolerance * pixelTolerance;
	float markerGroupingTolerance = 0.02f;
	float markerGropuingToleranceSqr = markerGroupingTolerance * markerGroupingTolerance;

	// Build markers from contributing cameras;
	for (int i = StartFrame; i <= EndFrame; ++i)
	{
		// Get every ray at this frame and do intersections.
		QList<Marker2D*> markers2d;
		
		for (int iT = 0; iT < trackers.size(); ++iT)
		{
			VidFrameData* trackerFrame = trackers[iT]->GetLocalFrame(i);

			if (!trackerFrame)
				continue;

			for (int iM = 0; iM < trackerFrame->newMarkers.size(); ++iM)
			{
				markers2d.push_back(&trackerFrame->newMarkers[iM]);
			}
		}

		QList<MarkerGroup> markerGroups;

		for (int oM = 0; oM < markers2d.size(); ++oM)
		{
			for (int iM = oM + 1; iM < markers2d.size(); ++iM)
			{
				Marker2D* a = markers2d[oM];
				Marker2D* b = markers2d[iM];

				if (a->trackerId == b->trackerId)
					continue;

				QVector3D c1;
				QVector3D c2;
				_ClosestPointsLines(trackers[a->trackerId]->worldPos, a->worldRayD, trackers[b->trackerId]->worldPos, b->worldRayD, &c1, &c2);

				/*
				// Check that both closest line points are within threshold in pixels.
				// Project c2 onto tracker A
				QVector2D projB = trackers[a->trackerId]->ProjectPoint(c2);
				float distB = (a->pos - projB).lengthSquared();

				// Project c1 onto tracker B
				QVector2D projA = trackers[b->trackerId]->ProjectPoint(c1);
				float distA = (b->pos - projA).lengthSquared();

				if (distA <= pixelToleranceSqr && distB <= pixelToleranceSqr)
				*/

				// Check tolerance in world space
				float distSqr = (c1 - c2).lengthSquared();
				const float rayTol = 0.01f;
				const float rayTolSqr = rayTol * rayTol;
				
				if (distSqr <= rayTolSqr)
				{
					QVector3D mPos = (c1 + c2) / 2;

					/*
					Marker3D m = {};
					m.pos = mPos;
					m.sources.push_back(*a);
					m.sources.push_back(*b);
					markers[i].push_back(m);
					//*/

					//*
					// Combine markers within range of each other.
					bool grouped = false;
					for (int iG = 0; iG < markerGroups.size(); ++iG)
					{
						if ((markerGroups[iG].pos - mPos).lengthSquared() <= markerGropuingToleranceSqr)
						{
							grouped = true;

							markerGroups[iG].count++;
							markerGroups[iG].avgPos += mPos;

							bool foundA = false;
							bool foundB = false;

							for (int iS = 0; iS < markerGroups[iG].sources.size(); ++iS)
							{
								if (markerGroups[iG].sources[iS].markerId == a->markerId &&
									markerGroups[iG].sources[iS].trackerId == a->trackerId)
								{
									foundA = true;
								}

								if (markerGroups[iG].sources[iS].markerId == b->markerId &&
									markerGroups[iG].sources[iS].trackerId == b->trackerId)
								{
									foundB = true;
								}
							}

							if (!foundA)
							{
								markerGroups[iG].sources.push_back(*a);
							}

							if (!foundB)
							{
								markerGroups[iG].sources.push_back(*b);
							}
						}
					}

					if (!grouped)
					{
						MarkerGroup mg = {};
						mg.count = 1;
						mg.pos = mPos;
						mg.avgPos = mPos;
						mg.sources.push_back(*a);
						mg.sources.push_back(*b);
						markerGroups.push_back(mg);
					}
					//*/
				}
			}
		}

		// Distribute groups into 3D markers.
		for (int iG = 0; iG < markerGroups.size(); ++iG)
		{
			if (markerGroups[iG].sources.size() > 2)
			{
				Marker3D m = {};
				m.pos = markerGroups[iG].avgPos /= markerGroups[iG].count;
				m.sources = markerGroups[iG].sources;
				m.id = 0;
				m.type = Marker3D::T_UNLABLED;
				markers[i].push_back(m);
			}
		}
	}

	float t1 = (t.nsecsElapsed() / 1000) / 1000.0;
	qDebug() << "Build 3D markers:" << t1 << "ms";
}

void Take::BuildLabels(int StartFrame, int EndFrame)
{
	qDebug() << "Building labels";

	// Reset labels
	labels.clear();
	for (int i = 0; i < timeFrames; ++i)
	{
		for (int m = 0; m < markers[i].size(); ++m)
		{
			markers[i][m].id = 0;
			markers[i][m].type = Marker3D::T_UNLABLED;
		}
	}

	int labelCount = 0;
	float markerSearchTolerance = 0.10f;
	float markerSearchTolearnceSqr = markerSearchTolerance * markerSearchTolerance;

	for (int f = 0; f < timeFrames; ++f)
	{
		// Uniquely identify each label in the scene.
		for (int iM = 0; iM < markers[f].size(); ++iM)
		{
			Marker3D* m = &markers[f][iM];
			bool found = false;

			if (f > 0)
			{
				// Compare to previous frame.
				Marker3D* c = 0;
				float d = markerSearchTolearnceSqr;

				for (int iP = 0; iP < markers[f - 1].size(); ++iP)
				{
					Marker3D* mP = &markers[f - 1][iP];

					float newDist = (m->pos - mP->pos).lengthSquared();
					if (newDist <= d)
					{
						c = mP;
						d = newDist;
					}
				}

				if (c)
				{
					m->id = c->id;
					m->type = Marker3D::T_LABLED;
					m->velocity = m->pos - c->pos;
					c->bVelocity = c->pos - m->pos;
					found = true;
				}
			}

			if (!found)
			{
				m->id = labelCount++;
				m->type = Marker3D::T_LABLED;				
			}
		}
	}
}

void Take::SaveSSBAFile()
{
	QFile file("unrefined.txt");
	if (file.open(QIODevice::ReadWrite | QIODevice::Text)) 
	{
		QTextStream stream(&file);

		int single3dMarkerCount = 0;
		for (int i = 0; i < markers.size(); ++i)
		{
			if (markers[i].size() == 1)
				++single3dMarkerCount;
		}
		
		stream << single3dMarkerCount << " 2 " << (single3dMarkerCount * 2) << endl;

		for (int t = 0; t < trackers.size(); ++t)
		{
			stream << trackers[t]->camMatOpt.at<double>(0, 0) << " "; // fx
			stream << trackers[t]->camMatOpt.at<double>(0, 1) << " "; // skew
			stream << trackers[t]->camMatOpt.at<double>(0, 2) << " "; // cx
			stream << trackers[t]->camMatOpt.at<double>(1, 1) << " "; // fy
			stream << trackers[t]->camMatOpt.at<double>(1, 2) << " "; // cy

			stream << trackers[t]->distCoefs.at<double>(0) << " "; // k1
			stream << trackers[t]->distCoefs.at<double>(1) << " "; // k2
			stream << trackers[t]->distCoefs.at<double>(2) << " "; // p1
			stream << trackers[t]->distCoefs.at<double>(3) << endl; // p2
		}

		// NOTE: Assume we only have a single marker in each frame
		for (int i = 0; i < markers.size(); ++i)
		{
			if (markers[i].size() == 1)
			{
				stream << i << " " << markers[i][0].pos.x() << " " << markers[i][0].pos.y() << " " << markers[i][0].pos.z() << endl;
			}
		}

		for (int t = 0; t < trackers.size(); ++t)
		{
			stream << t;
			
			for (int iX = 0; iX < 3; ++iX)						
			{				
				for (int iY = 0; iY < 4; ++iY)
				{
					stream << " " << trackers[t]->rtMat.at<double>(iX, iY);
				}
			}

			stream << endl;
		}

		for (int i = 0; i < markers.size(); ++i)
		{
			if (markers[i].size() == 1)
			{
				Marker3D m = markers[i][0];

				for (int iS = 0; iS < m.sources.size(); ++iS)
				{
					Marker2D m2d = m.sources[iS];
					stream << m2d.trackerId << " " << i << " " << m2d.distPos.x() << " " << m2d.distPos.y() << " 1" << endl;
				}

				/*
				for (int t = 0; t < trackers.size(); ++t)
				{
					if (t == 0)
						stream << t << " " << i << " " << markers[i][0].cam1marker.distPos.x() << " " << markers[i][0].cam1marker.distPos.y() << " 1" << endl;
					else
						stream << t << " " << i << " " << markers[i][0].cam2marker.distPos.x() << " " << markers[i][0].cam2marker.distPos.y() << " 1" << endl;
				}
				*/
			}
		}

		file.close();
	}
}

void Take::BuildExtrinsics(int StartFrame, int EndFrame)
{
	for (int t = 0; t < trackers.count(); ++t)
	{
		trackers[t]->UndistortMarkers(StartFrame, EndFrame);
	}

	std::vector<std::vector<MarkerCalib>> markerSets;
	std::vector<cv::Mat> poses;

	// For each camera pair
	for (int i = 1; i < trackers.count(); ++i)
	{
		qDebug() << "Build pose for pair 0 and" << i;

		std::vector<MarkerCalib> markers;
		cv::Mat pose;
		_BuildPose(StartFrame, EndFrame, trackers[0], trackers[i], markers, pose);
		markerSets.push_back(markers);
		poses.push_back(pose);
	}

	// Scale/match pair poses to first pair.
	for (int iS = 1; iS < markerSets.size(); ++iS)
	{
		// TODO: Investigate cv::estimateAffine3D.

		qDebug() << "Marker sets" << markerSets[0].size() << markerSets[iS].size();
		int mcIdx = 0;
		QVector3D rootA;
		QVector3D rootB;
		bool foundRoot = false;
		float scaleAvg = 0.0f;
		int scaleCount = 0;

		for (int i = 0; i < markerSets[0].size(); ++i)
		{
			while (mcIdx < markerSets[iS].size() && markerSets[iS][mcIdx].frame < markerSets[0][i].frame)
			{
				++mcIdx;
			}

			if (mcIdx >= markerSets[iS].size())
				break;

			if (markerSets[0][i].frame == markerSets[iS][mcIdx].frame)
			{
				//qDebug() << "Compare" << i << mcIdx << markerSets[0][i].frame << markerSets[1][mcIdx].frame;

				if (!foundRoot)
				{
					rootA = markerSets[0][i].pos;
					rootB = markerSets[iS][mcIdx].pos;
					foundRoot = true;
				}
				else
				{
					QVector3D pA = markerSets[0][i].pos;
					QVector3D pB = markerSets[iS][mcIdx].pos;

					float dA = (pA - rootA).length();
					float dB = (pB - rootB).length();

					float s = dA / dB;

					scaleAvg += s;
					++scaleCount;

					qDebug() << "Scale " << s;
				}
			}
		}

		scaleAvg /= scaleCount;
		qDebug() << "Avg scale:" << scaleAvg;

		// Manipulate col3 of pose.
		poses[iS].at<double>(0, 3) *= scaleAvg;
		poses[iS].at<double>(1, 3) *= scaleAvg;
		poses[iS].at<double>(2, 3) *= scaleAvg;
	}
	
	// Apply poses back to trackers.
	for (int t = 1; t < trackers.count(); ++t)
	{	
		trackers[t]->SetPose(poses[t - 1]);
	}
}

void Take::BundleAdjust(int StartFrame, int EndFrame)
{
	// Get 3D Markers from first pair.
	std::vector<MarkerCalib> temp3DMarkers;
	std::vector<cv::Point2f> trackerPoints[2];
	std::vector<int> pointFrameIndex;
	int tfIndex[2] = { StartFrame, StartFrame };
	TakeTracker* tracker0 = trackers[0];
	TakeTracker* tracker1 = trackers[1];
	
	//---------------------------------------------------------------------------------------------------
	// Find corresponding points in frames.
	//---------------------------------------------------------------------------------------------------
	while (true)
	{
		VidFrameData* t0 = &tracker0->vidFrameData[tfIndex[0]];
		VidFrameData* t1 = &tracker1->vidFrameData[tfIndex[1]];

		int tfT0 = t0->index;
		int tfT1 = t1->index;

		// Skip dummy frames.
		if (t0->type == 3 || t0->type == 3)
		{
			if (t0->type == 3) tfIndex[0]++;
			if (t1->type == 3) tfIndex[1]++;

			if (tfIndex[0] > EndFrame || tfIndex[1] > EndFrame)
				break;

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
				if (t0->newMarkers.count() == 1 && t1->newMarkers.count() == 1)
				{
					//qDebug() << "Add markers - Frame:" << tfT0 << t0->newMarkers[0].pos << t1->newMarkers[0].pos;

					trackerPoints[0].push_back(cv::Point2f(t0->newMarkers[0].pos.x(), t0->newMarkers[0].pos.y()));
					trackerPoints[1].push_back(cv::Point2f(t1->newMarkers[0].pos.x(), t1->newMarkers[0].pos.y()));
					pointFrameIndex.push_back(tfT0);
				}
			}

			tfIndex[0]++;
			tfIndex[1]++;
		}
	}
	
	cv::Mat Q;
	cv::triangulatePoints(trackers[0]->projMat, trackers[1]->projMat, trackerPoints[0], trackerPoints[1], Q);

	temp3DMarkers.clear();
	for (int i = 0; i < Q.size().width; ++i)
	{
		float w = Q.at<float>(3, i);
		float x = Q.at<float>(0, i) / w;
		float y = Q.at<float>(1, i) / w;
		float z = Q.at<float>(2, i) / w;

		MarkerCalib m = {};
		m.pos = QVector3D(x, y, z);
		m.frame = pointFrameIndex[i];
		temp3DMarkers.push_back(m);
	}

	// Every camera needs to see the markers for bundle adjust?

	// Refine
	// Build SSBA file.
	// Inputs: Cam, dist, pose, 3D Markers, 2D Markers (distorted)
	QFile file("unrefined.txt");
	if (file.open(QIODevice::ReadWrite | QIODevice::Text))
	{
		QTextStream stream(&file);

		int markerCount3d = (int)temp3DMarkers.size();
		int markerCount2d = 0;

		// Count 2D markers.
		for (int t = 0; t < trackers.size(); ++t)
		{
			for (int v = 0; v < trackers[t]->vidFrameData.size(); ++v)
			{
				if (trackers[t]->vidFrameData[v].type == 3)
					continue;

				int globalFrame = trackers[t]->vidFrameData[v].index;

				if (globalFrame < StartFrame || globalFrame > EndFrame)
					continue;

				if (trackers[t]->vidFrameData[v].newMarkers.size() == 1)
					++markerCount2d;
			}
		}

		// Header.
		stream << markerCount3d << " " << trackers.count() << " " << markerCount2d << endl;

		// Camera matrix per view.
		for (int t = 0; t < trackers.size(); ++t)
		{
			stream << trackers[t]->camMat.at<double>(0, 0) << " "; // fx
			stream << trackers[t]->camMat.at<double>(0, 1) << " "; // skew
			stream << trackers[t]->camMat.at<double>(0, 2) << " "; // cx
			stream << trackers[t]->camMat.at<double>(1, 1) << " "; // fy
			stream << trackers[t]->camMat.at<double>(1, 2) << " "; // cy

			stream << trackers[t]->distCoefs.at<double>(0) << " "; // k1
			stream << trackers[t]->distCoefs.at<double>(1) << " "; // k2
			stream << trackers[t]->distCoefs.at<double>(2) << " "; // p1
			stream << trackers[t]->distCoefs.at<double>(3) << endl; // p2
		}

		// 3D Markers.
		for (int i = 0; i < temp3DMarkers.size(); ++i)
		{
			MarkerCalib m = temp3DMarkers[i];
			stream << m.frame << " " << m.pos.x() << " " << m.pos.y() << " " << m.pos.z() << endl;
		}

		// View pose mats.
		for (int t = 0; t < trackers.size(); ++t)
		{
			stream << t;

			for (int iX = 0; iX < 3; ++iX)
			{
				for (int iY = 0; iY < 4; ++iY)
				{
					stream << " " << trackers[t]->rtMat.at<double>(iX, iY);
				}
			}

			stream << endl;
		}

		// Distorted 2D markers.
		for (int t = 0; t < trackers.size(); ++t)
		{
			for (int v = 0; v < trackers[t]->vidFrameData.size(); ++v)
			{
				if (trackers[t]->vidFrameData[v].type == 3)
					continue;

				int globalFrame = trackers[t]->vidFrameData[v].index;

				if (globalFrame < StartFrame || globalFrame > EndFrame)
					continue;

				if (trackers[t]->vidFrameData[v].newMarkers.size() == 1)
				{
					Marker2D m = trackers[t]->vidFrameData[v].newMarkers[0];
					stream << t << " " << globalFrame << " " << m.distPos.x() << " " << m.distPos.y() << " 1" << endl;
				}
			}
		}

		file.close();
	}

	// Run SSBA.
	QFile tempRefFile("refined.txt");
	tempRefFile.remove();

	QProcess ssbaProcess;
	ssbaProcess.start("BundleVarying.exe unrefined.txt tangential");
	if (!ssbaProcess.waitForStarted())
	{
		qDebug() << "Could not start SSBA process";
		return;
	}

	if (!ssbaProcess.waitForFinished())
	{
		qDebug() << "SSBA process failed";
		return;
	}

	QByteArray err = ssbaProcess.readAllStandardError();
	QByteArray stdOut = ssbaProcess.readAllStandardOutput();
	QByteArray result = ssbaProcess.readAll();
	//qDebug() << "SSBA completed:" << result << err << stdOut;

	QStringList lines = QString::fromUtf8(stdOut).split("\r\n");
	for (int i = 0; i < lines.size(); ++i)
	{
		qDebug() << lines[i];
	}

	// Load SSBA results.
	QFile refFile("refined.txt");

	if (refFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QTextStream s(&refFile);

		// Header.
		int markers3dCount = 0;
		int viewsCount = 0;
		int markers2dCount = 0;

		s >> markers3dCount >> viewsCount >> markers2dCount;
		qDebug() << markers3dCount << viewsCount << markers2dCount;

		// Camera & distortion.
		for (int v = 0; v < trackers.count(); ++v)
		{
			cv::Mat camMat = cv::Mat::eye(3, 3, CV_64F);
			s >> camMat.at<double>(0, 0); // fx
			s >> camMat.at<double>(0, 1); // skew
			s >> camMat.at<double>(0, 2); // cx
			s >> camMat.at<double>(1, 1); // fy
			s >> camMat.at<double>(1, 2); // cy

			cv::Mat distCoefs = cv::Mat::zeros(5, 1, CV_64F);
			s >> distCoefs.at<double>(0); // k1
			s >> distCoefs.at<double>(1); // k2
			s >> distCoefs.at<double>(2); // p1
			s >> distCoefs.at<double>(3); // p2

			trackers[v]->SetCamDist(camMat, distCoefs);
		}

		// 3D Markers.
		refMarkers.clear();
		for (int i = 0; i < timeFrames; ++i)
		{
			refMarkers.push_back(QVector3D());
		}

		for (int i = 0; i < markers3dCount; ++i)
		{
			int index;
			float x;
			float y;
			float z;

			s >> index >> x >> y >> z;

			refMarkers[index] = QVector3D(x, y, z);
		}

		// View poses.
		for (int v = 0; v < trackers.count(); ++v)
		{
			cv::Mat pose = cv::Mat::eye(3, 4, CV_64F);
			int index = 0;

			s >> index;

			for (int iX = 0; iX < 3; ++iX)
			{
				for (int iY = 0; iY < 4; ++iY)
				{
					s >> pose.at<double>(iX, iY);
				}
			}

			std::stringstream str;
			str << pose;
			qDebug() << "Pose" << v << str.str().c_str();

			trackers[v]->SetPose(pose);
		}

		refFile.close();
	}

	for (int t = 0; t < trackers.count(); ++t)
	{
		trackers[t]->UndistortMarkers(StartFrame, EndFrame);
	}
}

void Take::_BuildPose(int StartFrame, int EndFrame, TakeTracker* Root, TakeTracker* Tracker, std::vector<MarkerCalib> &Markers, cv::Mat& Pose)
{
	std::vector<cv::Point2f> trackerPoints[2];
	std::vector<cv::Point2f> distTrackerPoints[2];
	std::vector<int> pointFrameIndex;
	int tfIndex[2] = { StartFrame, StartFrame };
	TakeTracker* tracker0 = Root;
	TakeTracker* tracker1 = Tracker;

	//---------------------------------------------------------------------------------------------------
	// Find corresponding points in frames.
	//---------------------------------------------------------------------------------------------------
	while (true)
	{
		VidFrameData* t0 = &tracker0->vidFrameData[tfIndex[0]];
		VidFrameData* t1 = &tracker1->vidFrameData[tfIndex[1]];

		int tfT0 = t0->index;
		int tfT1 = t1->index;

		// Skip dummy frames.
		if (t0->type == 3 || t0->type == 3)
		{
			if (t0->type == 3) tfIndex[0]++;
			if (t1->type == 3) tfIndex[1]++;

			if (tfIndex[0] > EndFrame || tfIndex[1] > EndFrame)
				break;

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
				if (t0->newMarkers.count() == 1 && t1->newMarkers.count() == 1)
				{
					qDebug() << "Add markers - Frame:" << tfT0 << t0->newMarkers[0].pos << t1->newMarkers[0].pos;

					trackerPoints[0].push_back(cv::Point2f(t0->newMarkers[0].pos.x(), t0->newMarkers[0].pos.y()));
					trackerPoints[1].push_back(cv::Point2f(t1->newMarkers[0].pos.x(), t1->newMarkers[0].pos.y()));
					pointFrameIndex.push_back(tfT0);

					distTrackerPoints[0].push_back(cv::Point2f(t0->newMarkers[0].distPos.x(), t0->newMarkers[0].distPos.y()));
					distTrackerPoints[1].push_back(cv::Point2f(t1->newMarkers[0].distPos.x(), t1->newMarkers[0].distPos.y()));
				}
			}

			tfIndex[0]++;
			tfIndex[1]++;
		}
	}

	//---------------------------------------------------------------------------------------------------
	// Build fundamental mat for pair.
	//---------------------------------------------------------------------------------------------------
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

	//---------------------------------------------------------------------------------------------------
	// Recover camera poses.
	//---------------------------------------------------------------------------------------------------
	cv::Mat essentialMat(3, 3, CV_64F);
	// NOTE: Second camera K first.
	essentialMat = tracker1->camMatOpt.t() * fMats * tracker0->camMatOpt;

	cv::Mat r;
	cv::Mat t;
	cv::recoverPose(essentialMat, trackerPoints[0], trackerPoints[1], r, t, 2.2612, cv::Point2d(512, 352));
	
	cv::Mat trueT = -(r).t() * t;
	
	cv::Mat pose0 = cv::Mat::eye(3, 4, r.type());
	cv::Mat pose1(3, 4, r.type());
	pose1(cv::Range::all(), cv::Range(0, 3)) = r * 1.0;
	pose1.col(3) = t * 1.0;
	Pose = pose1.clone();

	cv::Mat proj0 = tracker0->camMatOpt * pose0;
	cv::Mat proj1 = tracker1->camMatOpt * pose1;

	//---------------------------------------------------------------------------------------------------
	// Triangulate points.
	//---------------------------------------------------------------------------------------------------
	cv::Mat Q;
	cv::triangulatePoints(proj0, proj1, trackerPoints[0], trackerPoints[1], Q);

	Markers.clear();
	for (int i = 0; i < Q.size().width; ++i)
	{
		float w = Q.at<float>(3, i);
		float x = Q.at<float>(0, i) / w;
		float y = Q.at<float>(1, i) / w;
		float z = Q.at<float>(2, i) / w;

		MarkerCalib m = {};
		m.pos = QVector3D(x, y, z);
		m.frame = pointFrameIndex[i];
		Markers.push_back(m);
	}
}

LiveTake::LiveTake()
{
	isLive = true;
}

LoadedTake::LoadedTake()
{
	isLive = false;
}