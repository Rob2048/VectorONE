#include "cameraView.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include "mainwindow.h"

QT_USE_NAMESPACE

CameraView::CameraView(QWidget* Parent, MainWindow* Main) :
	QWidget(Parent),
	main(Main)
{
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	setMouseTracking(true);

	setMinimumSize(200, 200);

	_mainFont = QFont("Arial", 8);
	_detailFont = QFont("Arial", 7);
	_largeFont = QFont("Arial", 50);
	mode = 0;
	take = 0;
	hoveredId = -1;

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	viewZoom = 0.4f;
	viewTranslate = QVector2D(-VID_W / 2, -VID_H / 2);

	_mouseLeft = false;
	_mouseRight = false;
	_mouseDownTrackerId = 0;
	_editMaskMode = 0;
}

QPointF CameraView::_GetVPointF(float X, float Y)
{
	QVector2D point = (_vt * QVector3D(X, Y, 1)).toVector2D();
	return QPointF(point.x(), point.y());
}

QPoint CameraView::_GetVPoint(float X, float Y)
{
	QPointF point = _GetVPointF(X, Y);
	return QPoint(point.x(), point.y());
}

void CameraView::paintEvent(QPaintEvent* Event)
{
	Q_UNUSED(Event);

	QPainter qp(this);
	qp.fillRect(0, 0, width(), height(), QColor(28, 30, 32));

	qp.setFont(_mainFont);

	qp.translate(width() / 2, height() / 2);
	qp.scale(viewZoom, viewZoom);
	qp.translate(viewTranslate.x(), viewTranslate.y());
	_vt = qp.transform();

	int trackerCount = 0;

	for (std::map<int, LiveTracker*>::iterator it = trackers->begin(); it != trackers->end(); ++it)
	{
		LiveTracker* tracker = it->second;
		float tX = trackerCount * (VID_W + 10);

		qp.setTransform(_vt);

		QImage img = QImage((uchar*)tracker->frameData, VID_W, VID_H, QImage::Format::Format_RGB888);
		qp.drawImage(QPointF(tX + -0.5f, -0.5f), img);

		QImage maskImg = QImage((uchar*)tracker->maskVisualData, 64, 44, QImage::Format::Format_RGBA8888);
		qp.drawImage(QRectF(tX + -0.5f, -0.5f, VID_W, VID_H), maskImg);

		if (!tracker->connected && !tracker->loaded)
		{
			qp.setPen(QPen(QColor(200, 50, 50), 2.0f / viewZoom));
			qp.setFont(_largeFont);

			qp.drawText(tX + VID_W / 2 - 200, VID_H / 2, "Not Connected");
		}
		else if (tracker->id == hoveredId)
		{
			if (tracker->selected)
				qp.setPen(QPen(QColor(100, 200, 255), 2.0f / viewZoom));
			else
				qp.setPen(QPen(QColor(200, 200, 200), 2.0f / viewZoom));
		}
		else
		{
			if (tracker->selected)
				qp.setPen(QPen(QColor(50, 150, 255), 2.0f / viewZoom));
			else
				qp.setPen(QPen(QColor(100, 100, 100), 2.0f / viewZoom));
		}

		qp.setBrush(Qt::BrushStyle::NoBrush);
		qp.drawRect(QRectF(tX - 0.5f, -0.5f, VID_W + 1.0, VID_H + 1.0));

		qp.setPen(QPen(QColor(255, 100, 255), 1.0f / viewZoom));
		
		//_vt = qp.transform();
		
		
		//qp.resetTransform();

		// NOTE: Realtime blobs.
		if (tracker->markerDataSize > 0)
		{
			blobDataHeader* blobHeader = (blobDataHeader*)tracker->markerData;
			blob* blobs = (blob*)(tracker->markerData + sizeof(blobDataHeader));
			region* regions = (region*)(tracker->markerData + sizeof(blobDataHeader) + (sizeof(blob) * blobHeader->blobCount));

			//qDebug() << blobHeader->foundRegionCount;

			qp.setRenderHint(QPainter::Antialiasing, true);
			qp.setBrush(QBrush(QColor::fromRgb(255, 0, 255), Qt::BrushStyle::SolidPattern));
			//qp.setBrush(QBrush(QColor::fromRgb(255, 255, 255), Qt::BrushStyle::SolidPattern));
			qp.setPen(Qt::PenStyle::NoPen);

			for (int i = 0; i < blobHeader->blobCount; ++i)
			{
				//qp.drawEllipse(_GetVPointF(tX + blobs[i].cX, blobs[i].cY), 2.0f, 2.0f);
				qp.drawEllipse(QPointF(tX + blobs[i].cX, blobs[i].cY), 2.0f, 2.0f);
			}

			qp.setRenderHint(QPainter::Antialiasing, false);

			qp.setBrush(Qt::NoBrush);
			qp.setPen(QPen(QColor(200, 200, 200), 1.0f));

			for (int i = 0; i < blobHeader->foundRegionCount; ++i)
			{
				region* r = &regions[i];
				qp.drawRect(tX + r->minX, r->minY, r->width, r->height);
			}
		}

		qp.resetTransform();

		qp.setPen(QPen(QColor(200, 200, 200), 1.0f));
		qp.setFont(_mainFont);

		char infoText[256];
		sprintf(infoText, "%s (0x%X) V%d FPS: %3d Data %7.2f KB Frame: %7lld Sync: %d", tracker->name.toUtf8().data(), tracker->serial, tracker->version, (int)tracker->fps, tracker->dataRecv, tracker->latestFrameId, (int)tracker->avgMasterOffset);
		qp.drawText(_GetVPoint(tX, -5), infoText);
		
		if (tracker->markerDataSize > 0)
		{
			blobDataHeader* blobHeader = (blobDataHeader*)tracker->markerData;
			qp.drawText(_GetVPoint(tX, 10) + QPoint(0, 0), "Markers: " + QString::number(blobHeader->blobCount));
			qp.drawText(_GetVPoint(tX, 10) + QPoint(0, 15), "Regions: " + QString::number(blobHeader->regionCount));
			qp.drawText(_GetVPoint(tX, 10) + QPoint(0, 30), "Total time: " + QString::number(blobHeader->totalTime));
		}

		trackerCount++;
	}

	if (take)
	{
		qp.setFont(_detailFont);
		qp.setRenderHint(QPainter::Antialiasing, true);

		for (int t = 0; t < take->trackers.count(); ++t)
		{
			TakeTracker* tracker = take->trackers[t];

			if (tracker->vidFrameData.count() > 0)
			{
				float tX = (VID_W + 10) * t;

				//qp.setBrush(QBrush(QColor::fromRgb(255, 128, 32), Qt::BrushStyle::SolidPattern));
				//qp.setPen(Qt::PenStyle::NoPen);

				for (int fI = tracker->drawMarkerFrameIndex; fI <= tracker->drawMarkerFrameIndex; ++fI)
				{
					if (fI >= 0)
					{	
						qp.setBrush(Qt::BrushStyle::NoBrush);

						/*
						// OpenCV simpleblobdetector
						for (int i = 0; i < tracker->vidFrameData[fI].markers.count(); ++i)
						{
							float x = tracker->vidFrameData[fI].markers[i].x();
							float y = tracker->vidFrameData[fI].markers[i].y();
							qp.setPen(QPen(QColor::fromRgb(255, 128, 32)));
							qp.drawEllipse(_GetVPointF(tX + x, y), 5.0f, 5.0f);

							if (fI == tracker->drawMarkerFrameIndex)
								qp.drawText(_GetVPointF(tX + x + 5, y + 5), QString::number(x) + ", " + QString::number(y));
						}
						*/
						
						// Gravity centroids.
						for (int i = 0; i < tracker->vidFrameData[fI].newMarkers.count(); ++i)
						{	
							qp.setPen(Qt::PenStyle::NoPen);

							Marker2D* m = &tracker->vidFrameData[fI].newMarkers[i];

							// Distorted position
							qp.setBrush(QBrush(QColor::fromRgb(255, 0, 255), Qt::BrushStyle::SolidPattern));
							qp.drawEllipse(_GetVPointF(tX + m->distPos.x(), m->distPos.y()), 2.0f, 2.0f);

							// Undistorted position
							qp.setBrush(QBrush(QColor::fromRgb(0, 255, 0), Qt::BrushStyle::SolidPattern));
							qp.drawEllipse(_GetVPointF(tX + m->pos.x(), m->pos.y()), 2.0f, 2.0f);

							// Ref undistorted position.
							//qp.setBrush(QBrush(QColor::fromRgb(0, 255, 255), Qt::BrushStyle::SolidPattern));
							//qp.drawEllipse(_GetVPointF(tX + m->refPos.x(), m->refPos.y()), 2.0f, 2.0f);

							/*
							// Distorted bounds.
							if (fI == tracker->drawMarkerFrameIndex)
							{
								qp.setPen(QPen(QColor::fromRgb(0, 255, 0)));
								qp.drawText(_GetVPointF(tX + m->pos.x() + 5, m->pos.y() - 5), QString::number(m->pos.x()) + ", " + QString::number(m->pos.y()));

								QPointF bMin = _GetVPointF(m->bounds.x() + tX - 0.5f, m->bounds.y() - 0.5f);
								QPointF bMax = _GetVPointF(m->bounds.z() + tX + 0.5f, m->bounds.w() + 0.5f);

								qp.setBrush(Qt::BrushStyle::NoBrush);
								qp.drawRect(QRectF(bMin, bMax));
							}
							*/
						}
					}
				}
			}
		}
		
		qp.setPen(QPen(QColor::fromRgb(255, 0, 255)));
		qp.setBrush(Qt::BrushStyle::NoBrush);

		//*
		// NOTE: Draw epipolar line in paired cam view.
		for (int t = 0; t < take->trackers.count(); ++t)
		{
			qp.setPen(QPen(QColor::fromRgb(255, 0, 255)));
			TakeTracker* tracker = take->trackers[t];
			
			//float tX = (VID_W + 10) * (1 - t);
			float tX = 0;
			
			if (t == 0)
				tX = (VID_W + 10);
			
			/*
			// Draw epilines from camera pair onto camera image.
			for (int i = 0; i < tracker->vidFrameData[tracker->drawMarkerFrameIndex].epiLines.count(); ++i)
			{
				EpipolarLine e = tracker->vidFrameData[tracker->drawMarkerFrameIndex].epiLines[i];

				float a = e.a;
				float b = e.b;
				float c = e.c;
				float y1 = -(a * -200 + c) / b;
				float y2 = -(a * 1224 + c) / b;

				qp.drawLine(_GetVPointF(tX - 200, y1), _GetVPointF(tX + 1224, y2));
			}
			*/

			// Project 3D Markers onto camera image.
			for (int m = 0; m < take->markers[timelineFrame].size(); ++m)
			{
				QVector3D markerPos = take->markers[timelineFrame][m].pos;

				cv::Mat wp(4, 1, CV_64F);
				wp.at<double>(0) = markerPos.x();
				wp.at<double>(1) = markerPos.y();
				wp.at<double>(2) = markerPos.z();
				wp.at<double>(3) = 1.0;

				cv::Mat imgPt = tracker->projMat * wp;
				float x = imgPt.at<double>(0) / imgPt.at<double>(2);
				float y = imgPt.at<double>(1) / imgPt.at<double>(2);

				float tX = (VID_W + 10) * t;
				qp.setPen(QPen(QColor::fromRgb(255, 0, 255)));
				qp.drawEllipse(_GetVPointF(tX + x, y), 7.0f, 7.0f);
			}

			// Project 3D Markers onto camera image.
			if (timelineFrame < take->calibMarkers.size())
			{
				QVector3D markerPos = take->calibMarkers[timelineFrame];

				cv::Mat wp(4, 1, CV_64F);
				wp.at<double>(0) = markerPos.x();
				wp.at<double>(1) = markerPos.y();
				wp.at<double>(2) = markerPos.z();
				wp.at<double>(3) = 1.0;

				cv::Mat imgPt = tracker->projMat * wp;
				float x = imgPt.at<double>(0) / imgPt.at<double>(2);
				float y = imgPt.at<double>(1) / imgPt.at<double>(2);

				float tX = (VID_W + 10) * t;
				qp.setPen(QPen(QColor::fromRgb(0, 0, 255)));
				qp.drawEllipse(_GetVPointF(tX + x, y), 7.0f, 7.0f);
			}

			// Ref stuff.
			/*
			{	
				QVector3D markerPos = take->refMarkers[timelineFrame];

				cv::Mat wp(4, 1, CV_64F);
				wp.at<double>(0) = markerPos.x();
				wp.at<double>(1) = markerPos.y();
				wp.at<double>(2) = markerPos.z();
				wp.at<double>(3) = 1.0;

				cv::Mat imgPt = tracker->decoder->refP * wp;
				float x = imgPt.at<double>(0) / imgPt.at<double>(2);
				float y = imgPt.at<double>(1) / imgPt.at<double>(2);

				float tX = (VID_W + 10) * t;
				qp.setPen(QPen(QColor::fromRgb(255, 0, 0)));
				qp.drawEllipse(_GetVPointF(tX + x, y), 3.0f, 3.0f);

				{
					std::vector<cv::Point3f> worldPoints;
					qDebug() << x << y;
					//worldPoints.push_back(cv::Point3f(x, y, 1));
					worldPoints.push_back(cv::Point3f(markerPos.x(), markerPos.y(), markerPos.z()));
					std::vector<cv::Point2f> imgPoints;

					//cv::projectPoints(worldPoints, cv::Mat(cv::Point3f(0, 0, 0)), cv::Mat(cv::Point3f(0, 0, 0)), tracker->decoder->refOptK, tracker->decoder->refD, imgPoints);
					cv::projectPoints(worldPoints, tracker->decoder->refR, tracker->decoder->refT, tracker->decoder->refK, tracker->decoder->refD, imgPoints);

					for (int i = 0; i < imgPoints.size(); ++i)
					{
						qDebug() << "POINT" << imgPoints[i].x << imgPoints[i].y;
						//cv::circle(colMat, imgPoints[i], 4, Scalar(255, 0, 0), -1, CV_AA);
						qp.setPen(QPen(QColor::fromRgb(255, 0, 0)));
						qp.drawEllipse(_GetVPointF(tX + imgPoints[i].x, imgPoints[i].y), 10.0f, 10.0f);
					}
				}
			}
			*/
		}
		//*/

		// Project Rays from camera pair onto camera image.
		for (int t = 0; t < take->trackers.count(); ++t)
		{	
			TakeTracker* tracker = take->trackers[t];
			float tX = (VID_W + 10) * t;

			// Draw rays from all other trackers
			for (int tP = 0; tP < take->trackers.count(); ++tP)
			{
				if (tP == t)
					continue;

				TakeTracker* otherTracker = take->trackers[tP];

				QPointF oto;
				QPointF refOto;

				{
					QVector3D camPos = otherTracker->worldPos;
					
					cv::Mat wp(4, 1, CV_64F);
					wp.at<double>(0) = camPos.x();
					wp.at<double>(1) = camPos.y();
					wp.at<double>(2) = camPos.z();
					wp.at<double>(3) = 1.0;

					cv::Mat imgPt = tracker->projMat * wp;
					float x = imgPt.at<double>(0) / imgPt.at<double>(2);
					float y = imgPt.at<double>(1) / imgPt.at<double>(2);

					float tX = (VID_W + 10) * t;
					oto = _GetVPointF(tX + x, y);

					qp.setPen(QPen(QColor::fromRgb(255, 255, 0)));
					qp.drawEllipse(oto, 3.0f, 3.0f);
				}

				// Ref cam pos
				/*
				{
					QVector3D camPos = otherTracker->decoder->refWorldPos;

					cv::Mat wp(4, 1, CV_64F);
					wp.at<double>(0) = camPos.x();
					wp.at<double>(1) = camPos.y();
					wp.at<double>(2) = camPos.z();
					wp.at<double>(3) = 1.0;

					cv::Mat imgPt = tracker->decoder->refP * wp;
					float x = imgPt.at<double>(0) / imgPt.at<double>(2);
					float y = imgPt.at<double>(1) / imgPt.at<double>(2);

					float tX = (VID_W + 10) * t;
					refOto = _GetVPointF(tX + x, y);

					qp.setPen(QPen(QColor::fromRgb(0, 255, 255)));
					qp.drawEllipse(refOto, 3.0f, 3.0f);
				}
				*/

				for (int iM = 0; iM < otherTracker->vidFrameData[otherTracker->drawMarkerFrameIndex].newMarkers.size(); ++iM)
				{
					Marker2D* m = &otherTracker->vidFrameData[otherTracker->drawMarkerFrameIndex].newMarkers[iM];

					{
						QVector3D camPos = m->worldRayD + otherTracker->worldPos;

						cv::Mat wp(4, 1, CV_64F);
						wp.at<double>(0) = camPos.x();
						wp.at<double>(1) = camPos.y();
						wp.at<double>(2) = camPos.z();
						wp.at<double>(3) = 1.0;

						cv::Mat imgPt = tracker->projMat * wp;
						float x = imgPt.at<double>(0) / imgPt.at<double>(2);
						float y = imgPt.at<double>(1) / imgPt.at<double>(2);

						float tX = (VID_W + 10) * t;
						QPointF rayE = _GetVPointF(tX + x, y);

						qp.setPen(QPen(QColor::fromRgb(255, 255, 0)));
						qp.drawEllipse(rayE, 3.0f, 3.0f);
						qp.drawLine(oto, rayE);
					}

					// Ref rays
					/*
					{
						QVector3D camPos = m->refWorldRayD + otherTracker->decoder->refWorldPos;

						cv::Mat wp(4, 1, CV_64F);
						wp.at<double>(0) = camPos.x();
						wp.at<double>(1) = camPos.y();
						wp.at<double>(2) = camPos.z();
						wp.at<double>(3) = 1.0;

						cv::Mat imgPt = tracker->decoder->refP * wp;
						float x = imgPt.at<double>(0) / imgPt.at<double>(2);
						float y = imgPt.at<double>(1) / imgPt.at<double>(2);

						float tX = (VID_W + 10) * t;
						QPointF rayE = _GetVPointF(tX + x, y);

						qp.setPen(QPen(QColor::fromRgb(0, 255, 255)));
						qp.drawEllipse(rayE, 3.0f, 3.0f);
						qp.drawLine(refOto, rayE);
					}
					*/
				}
			}
		}

		qp.setRenderHint(QPainter::Antialiasing, false);
	}
}

void CameraView::mousePressEvent(QMouseEvent* Event)
{
	if (_mouseLeft || _mouseRight)
		return;

	_mouseDownPos = Event->localPos();
	_mouseMovedPos = _mouseDownPos;

	if (Event->button() == Qt::MouseButton::LeftButton)
	{
		_mouseLeft = true;
		_editMaskMode = 0;

		QVector2D trackerSpace;
		LiveTracker* tracker = _GetTracker(Event->localPos().x(), Event->localPos().y(), &trackerSpace);

		if (tracker)
		{
			_mouseDownTrackerId = tracker->id;
			int mX = trackerSpace.x() * 64;
			int mY = trackerSpace.y() * 44;

			if (mX >= 0 && mX < 64 && mY >= 0 && mY < 44 && tracker->interactMode == 1)
			{
				if (tracker->getMask(mX, mY))
				{
					_editMaskMode = 1;
					_ChangeMask(tracker, mX, mY, false);
				}
				else
				{
					_editMaskMode = 2;
					_ChangeMask(tracker, mX, mY, true);
				}
			}
		}

	}
	else if (Event->button() == Qt::MouseButton::RightButton)
	{
		_mouseRight = true;
	}
}

void CameraView::mouseReleaseEvent(QMouseEvent* Event)
{
	QPointF moveDelta = (_mouseDownPos - _mouseMovedPos);
	float moveDeltaLengthSq = moveDelta.x() * moveDelta.x() + moveDelta.y() * moveDelta.y();
	bool moved = (moveDeltaLengthSq > 2 * 2);

	if (_mouseLeft && Event->button() == Qt::MouseButton::LeftButton)
	{
		_mouseLeft = false;

		QVector2D trackerSpace;
		LiveTracker* tracker = _GetTracker(Event->localPos().x(), Event->localPos().y(), &trackerSpace);
		
		if (!moved)
		{
			_DeselectTrackers();			

			if (tracker)
			{	
				tracker->selected = true;
				main->selectTracker(tracker);
			}
		}
	}
	else if (_mouseRight && Event->button() == Qt::MouseButton::RightButton)
	{
		_mouseRight = false;

		if (!moved)
		{
			LiveTracker* tracker = _GetTracker(Event->localPos().x(), Event->localPos().y());

			if (tracker)
			{
				QMenu contextMenu("Tracker View", this);

				QAction* active = contextMenu.addAction("Active");
				active->setCheckable(true);
				active->setChecked(tracker->active);

				contextMenu.addSeparator();
				QAction* video = contextMenu.addAction("Video");
				video->setCheckable(true);
				video->setChecked(false);

				QAction* markers = contextMenu.addAction("Markers");
				markers->setCheckable(true);
				markers->setChecked(false);

				contextMenu.addSeparator();
				QAction* interactNone = contextMenu.addAction("None");
				interactNone->setCheckable(true);
				interactNone->setChecked(tracker->interactMode == 0);

				QAction* interactMasks = contextMenu.addAction("Edit Mask");
				interactMasks->setCheckable(true);
				interactMasks->setChecked(tracker->interactMode == 1);
				
				QAction* interactMarkers = contextMenu.addAction("Select Markers");
				interactMarkers->setCheckable(true);
				interactMarkers->setChecked(tracker->interactMode == 2);

				QAction* result = contextMenu.exec(Event->globalPos());

				if (result == video)
				{
					main->viewFeed(tracker->id, true);
				}
				else if (result == markers)
				{
					main->viewFeed(tracker->id, false);
				}
				else if (result == interactNone)
				{
					tracker->interactMode = 0;
				}
				else if (result == interactMasks)
				{
					tracker->interactMode = 1;
				}
				else if (result == interactMarkers)
				{
					tracker->interactMode = 2;
				}
			}
		}
	}
}

void CameraView::mouseMoveEvent(QMouseEvent* Event)
{
	QPointF delta = Event->localPos() - _mouseMovedPos;
	_mouseMovedPos = Event->localPos();

	if (_mouseLeft)
	{
		QVector2D trackerSpace;
		LiveTracker* tracker = _GetTracker(Event->localPos().x(), Event->localPos().y(), &trackerSpace);

		if (tracker)
		{
			if (tracker->id == _mouseDownTrackerId)
			{
				int mX = trackerSpace.x() * 64;
				int mY = trackerSpace.y() * 44;

				if (mX >= 0 && mX < 64 && mY >= 0 && mY < 44)
				{
					if (tracker->interactMode == 1 && _editMaskMode == 1)
						_ChangeMask(tracker, mX, mY, false);
					else if (tracker->interactMode == 1 && _editMaskMode == 2)
						_ChangeMask(tracker, mX, mY, true);
				}
			}
		}
	}
	else if (_mouseRight)
	{
		viewTranslate.setX(viewTranslate.x() + delta.x() / viewZoom);
		viewTranslate.setY(viewTranslate.y() + delta.y() / viewZoom);
		update();
	}
	else
	{
		int newHoveredId = -1;
		LiveTracker* tracker = _GetTracker(_mouseMovedPos.x(), _mouseMovedPos.y());

		if (tracker)
		{
			newHoveredId = tracker->id;
		}

		if (hoveredId != newHoveredId)
		{
			hoveredId = newHoveredId;
			update();
		}
	}
}

void CameraView::_ChangeMask(LiveTracker* Tracker, int X, int Y, bool Value)
{
	if (Tracker->getMask(X, Y) != Value)
	{
		Tracker->changeMask(X, Y, Value);
		main->changeMask(Tracker);
		update();
	}
}

void CameraView::wheelEvent(QWheelEvent* Event)
{
	viewZoom += Event->angleDelta().y() * 0.002f * viewZoom;

	if (viewZoom < 0.1)
		viewZoom = 0.1f;

	//viewTranslate.setX(viewTranslate.x() + (viewTranslate.x() - (_mouseMovedPos.x() * viewZoom)));
	//viewTranslate.setY(viewTranslate.y() + (viewTranslate.y() - (_mouseMovedPos.y() * viewZoom)));

	update();
}

LiveTracker* CameraView::_GetTracker(int X, int Y, QVector2D* TrackerSpace)
{
	int trackerCount = 0;
	for (std::map<int, LiveTracker*>::iterator it = trackers->begin(); it != trackers->end(); ++it)
	{
		LiveTracker* tracker = it->second;
		float tX = trackerCount * (VID_W + 10);

		QPoint min = _GetVPoint(tX - 0.5f, -0.5f);
		QPoint max = _GetVPoint(tX - 0.5f + VID_W + 1.0f, -0.5f + VID_H + 1.0f);

		if (X >= min.x() && X <= max.x() && Y >= min.y() && Y <= max.y())
		{
			if (TrackerSpace)
			{
				TrackerSpace->setX(((float)X - min.x()) / (max.x() - min.x()));
				TrackerSpace->setY(((float)Y - min.y()) / (max.y() - min.y()));
			}

			return tracker;
		}

		trackerCount++;
	}

	return 0;
}

void CameraView::_DeselectTrackers()
{
	int trackerCount = 0;
	for (std::map<int, LiveTracker*>::iterator it = trackers->begin(); it != trackers->end(); ++it)
	{
		LiveTracker* tracker = it->second;
		tracker->selected = false;

		trackerCount++;
	}
}