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
	_medFont = QFont("Arial", 20);
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

	showMask = false;
	markerMode = 0;
	showPixelGrid = false;
	showCamRays = false;
	showDistortedMarkers = false;
	showUndistortedMarkers = true;
	showReprojectedMarkers = true;
	alignHorizontal = true;
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

void CameraView::setMask(bool Visible)
{
	showMask = Visible;
	update();
}

void CameraView::setPixelGrid(bool Visible)
{
	showPixelGrid = Visible;
	update();
}

void CameraView::setMarkerMode(int Mode)
{
	markerMode = Mode;
	update();
}

void CameraView::paintEvent(QPaintEvent* Event)
{
	Q_UNUSED(Event);

	QPainter qp(this);
	qp.fillRect(0, 0, width(), height(), QColor(28, 30, 32));

	//qp.setPen(QPen(QColor(28 + 8, 30 + 8, 32 + 8), 2.0f / viewZoom));
	qp.setPen(QPen(QColor(100, 30 + 8, 32 + 8), 2.0f / viewZoom));
	qp.setFont(_medFont);

	if (take->isLive)
	{
		qp.drawText(5, 30, "LIVE");
	}
	else
	{
		qp.drawText(5, 30, "PRERECORDED");
	}

	qp.setFont(_mainFont);

	qp.translate(width() / 2, height() / 2);
	qp.scale(viewZoom, viewZoom);
	qp.translate(viewTranslate.x(), viewTranslate.y());
	_vt = qp.transform();

	int trackerCount = 0;

	for (std::map<int, LiveTracker*>::iterator it = take->liveTrackers.begin(); it != take->liveTrackers.end(); ++it)
	{
		LiveTracker* tracker = it->second;
		
		float tX = 0;
		float tY = 0;

		if (alignHorizontal)
			tY = trackerCount * (VID_H + 10);
		else
			tX = trackerCount * (VID_W + 10);
		
		qp.setTransform(_vt);

		QImage img = QImage((uchar*)tracker->frameData, VID_W, VID_H, QImage::Format::Format_RGB888);
		qp.drawImage(QPointF(tX + -0.5f, tY + -0.5f), img);

		if (showMask)
		{
			QImage maskImg = QImage((uchar*)tracker->maskVisualData, 64, 44, QImage::Format::Format_RGBA8888);
			qp.drawImage(QRectF(tX + -0.5f, tY + -0.5f, VID_W, VID_H), maskImg);
		}

		qp.setPen(QPen(QColor(50, 50, 50), 1.0f / viewZoom));

		if (showPixelGrid)
		{
			for (int i = 1; i <= 1024; ++i)
			{
				qp.drawLine(QPointF(tX + i - 0.5f, tY - 0.5f), QPointF(tX + i - 0.5f, tY + 704 - 0.5f));
			}

			for (int i = 1; i <= 704; ++i)
			{
				qp.drawLine(QPointF(tX - 0.5f, tY + i - 0.5f), QPointF(tX + 1024 - 0.5f, tY + i - 0.5f));
			}
		}

		if (!tracker->connected && !tracker->loaded)
		{
			qp.setPen(QPen(QColor(200, 50, 50), 2.0f / viewZoom));
			qp.setFont(_largeFont);

			qp.drawText(tX + VID_W / 2 - 200, tY + VID_H / 2, "Not Connected");
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
		qp.drawRect(QRectF(tX - 0.5f, tY + -0.5f, VID_W + 1.0, VID_H + 1.0));

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
				qp.drawEllipse(QPointF(tX + blobs[i].cX, tY + blobs[i].cY), 2.0f, 2.0f);
			}

			qp.setRenderHint(QPainter::Antialiasing, false);

			qp.setBrush(Qt::NoBrush);
			qp.setPen(QPen(QColor(200, 200, 200), 1.0f));

			for (int i = 0; i < blobHeader->foundRegionCount; ++i)
			{
				region* r = &regions[i];
				qp.drawRect(tX + r->minX, tY + r->minY, r->width, r->height);
			}
		}

		qp.resetTransform();

		qp.setPen(QPen(QColor(200, 200, 200), 1.0f));
		qp.setFont(_mainFont);

		char infoText[256];
		sprintf(infoText, "%s (0x%X) V%d FPS: %3d Data %7.2f KB Frame: %7lld Sync: %d", tracker->name.toUtf8().data(), tracker->serial, tracker->version, (int)tracker->fps, tracker->dataRecv, tracker->latestFrameId, (int)tracker->avgMasterOffset);
		qp.drawText(_GetVPoint(tX, tY + -5), infoText);
		
		if (tracker->markerDataSize > 0)
		{
			blobDataHeader* blobHeader = (blobDataHeader*)tracker->markerData;
			qp.drawText(_GetVPoint(tX, tY + 10) + QPoint(0, 0), "Markers: " + QString::number(blobHeader->blobCount));
			qp.drawText(_GetVPoint(tX, tY + 10) + QPoint(0, 15), "Regions: " + QString::number(blobHeader->regionCount));
			qp.drawText(_GetVPoint(tX, tY + 10) + QPoint(0, 30), "Total time: " + QString::number(blobHeader->totalTime));
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
				float tX = 0;
				float tY = 0;

				if (alignHorizontal)
					tY = t * (VID_H + 10);
				else
					tX = t * (VID_W + 10);

				//qp.setBrush(QBrush(QColor::fromRgb(255, 128, 32), Qt::BrushStyle::SolidPattern));
				//qp.setPen(Qt::PenStyle::NoPen);

				for (int fI = tracker->drawMarkerFrameIndex; fI <= tracker->drawMarkerFrameIndex; ++fI)
				{
					if (fI >= 0)
					{
						qp.setBrush(Qt::BrushStyle::NoBrush);

						// Gravity centroids.
						for (int i = 0; i < tracker->vidFrameData[fI].newMarkers.count(); ++i)
						{
							qp.setPen(Qt::PenStyle::NoPen);

							Marker2D* m = &tracker->vidFrameData[fI].newMarkers[i];

							//qDebug() << tracker->drawMarkerFrameIndex;

							// Distorted position
							if (showDistortedMarkers)
							{
								qp.setBrush(QBrush(QColor::fromRgb(255, 0, 255), Qt::BrushStyle::SolidPattern));
								qp.drawEllipse(_GetVPointF(tX + m->distPos.x(), tY + m->distPos.y()), 2.0f * viewZoom, 2.0f * viewZoom);
							}

							// Undistorted position
							if (showUndistortedMarkers)
							{
								qp.setBrush(QBrush(QColor::fromRgb(0, 255, 0), Qt::BrushStyle::SolidPattern));
								qp.drawEllipse(_GetVPointF(tX + m->pos.x(), tY + m->pos.y()), 2.0f * viewZoom, 2.0f * viewZoom);
							}
						}
					}
				}
			}
		}

		qp.setPen(QPen(QColor::fromRgb(255, 0, 255)));
		qp.setBrush(Qt::BrushStyle::NoBrush);

		// Project 3D Markers onto camera image.
		if (showReprojectedMarkers)
		{
			qp.setTransform(_vt);
			qp.setBrush(QBrush(QColor::fromRgb(100, 100, 255), Qt::BrushStyle::SolidPattern));
			qp.setPen(Qt::PenStyle::NoPen);

			for (int t = 0; t < take->trackers.count(); ++t)
			{
				TakeTracker* tracker = take->trackers[t];

				float tX = 0;
				float tY = 0;

				if (alignHorizontal)
					tY = t * (VID_H + 10);
				else
					tX = t * (VID_W + 10);

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

					qp.drawEllipse(QPointF(tX + x, tY + y), 2.0f, 2.0f);
				}				
			}

			qp.resetTransform();
		}

		// Project Rays from camera pair onto camera image.
		if (showCamRays)
		{
			for (int t = 0; t < take->trackers.count(); ++t)
			{
				TakeTracker* tracker = take->trackers[t];
				float tX = 0;
				float tY = 0;

				if (alignHorizontal)
					tY = t * (VID_H + 10);
				else
					tX = t * (VID_W + 10);

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

						oto = _GetVPointF(tX + x, tY + y);

						qp.setPen(QPen(QColor::fromRgb(255, 255, 0)));
						qp.drawEllipse(oto, 3.0f, 3.0f);
					}

					for (int iM = 0; iM < otherTracker->vidFrameData[otherTracker->drawMarkerFrameIndex].newMarkers.size(); ++iM)
					{
						Marker2D* m = &otherTracker->vidFrameData[otherTracker->drawMarkerFrameIndex].newMarkers[iM];

						{
							QVector3D camPos = m->worldRayD * 6.0f + otherTracker->worldPos;

							cv::Mat wp(4, 1, CV_64F);
							wp.at<double>(0) = camPos.x();
							wp.at<double>(1) = camPos.y();
							wp.at<double>(2) = camPos.z();
							wp.at<double>(3) = 1.0;

							cv::Mat imgPt = tracker->projMat * wp;
							float x = imgPt.at<double>(0) / imgPt.at<double>(2);
							float y = imgPt.at<double>(1) / imgPt.at<double>(2);

							QPointF rayE = _GetVPointF(tX + x, tY + y);

							qp.setPen(QPen(QColor::fromRgb(255, 255, 0)));
							qp.drawEllipse(rayE, 3.0f, 3.0f);
							qp.drawLine(oto, rayE);
						}
					}
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
					main->viewFeed(tracker->id, 1);
				}
				else if (result == markers)
				{
					main->viewFeed(tracker->id, 2);
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
	for (std::map<int, LiveTracker*>::iterator it = take->liveTrackers.begin(); it != take->liveTrackers.end(); ++it)
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
	for (std::map<int, LiveTracker*>::iterator it = take->liveTrackers.begin(); it != take->liveTrackers.end(); ++it)
	{
		LiveTracker* tracker = it->second;
		tracker->selected = false;

		trackerCount++;
	}
}