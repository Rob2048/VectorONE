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

	for (std::map<int, LiveTracker*>::iterator it = main->liveTrackers.begin(); it != main->liveTrackers.end(); ++it)
	{
		LiveTracker* tracker = it->second;
		float tX = trackerCount * (VID_W + 10);

		qp.setTransform(_vt);

		QImage img = QImage((uchar*)tracker->frameData, VID_W, VID_H, QImage::Format::Format_RGB888);
		qp.drawImage(QPointF(tX + -0.5f, -0.5f), img);

		QImage maskImg = QImage((uchar*)tracker->maskData, 128, 88, QImage::Format::Format_RGBA8888);
		qp.drawImage(QRectF(tX + -0.5f, -0.5f, VID_W, VID_H), maskImg);

		if (!tracker->connected)
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

		qp.drawRect(QRectF(tX - 0.5f, -0.5f, VID_W + 1.0, VID_H + 1.0));
		
		qp.resetTransform();

		qp.setPen(QPen(QColor(200, 200, 200), 1.0f));
		qp.setFont(_mainFont);
		qp.drawText(_GetVPoint(tX, -5), tracker->name + " (0x" + QString::number(tracker->serial, 16) + ") " + QString::number(tracker->version) +  " - " + QString::number(tracker->fps) + "fps " + QString::number(tracker->dataRecv) + "kb");

		trackerCount++;
	}

	if (mode == 3)
	{
		//qp.drawImage(QPointF(-0.5f, -0.5f), camImage);

		qp.setPen(QPen(QColor(255, 100, 255), 1.0f / viewZoom));
		qp.drawRect(0, 0, VID_W, VID_H);
		qp.drawRect(VID_W + 10, 0, VID_W, VID_H);

		_vt = qp.transform();
		qp.resetTransform();

		qp.setRenderHint(QPainter::Antialiasing, true);

		/*
		if (markerDataSize > 0)
		{
			int blobCount = *((int*)markerData);
			blob* blobs = (blob*)(markerData + 4);

			qDebug() << blobCount << markerDataSize;

			qp.setBrush(QBrush(QColor::fromRgb(255, 0, 255), Qt::BrushStyle::SolidPattern));
			qp.setPen(Qt::PenStyle::NoPen);

			for (int i = 0; i < blobCount; ++i)
			{
				qp.drawEllipse(_GetVPointF(blobs[i].cX, blobs[i].cY), 2.0f, 2.0f);
			}
		}
		*/

		qp.setRenderHint(QPainter::Antialiasing, false);
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
						qp.setPen(QPen(QColor::fromRgb(255, 128, 32)));
						qp.setBrush(Qt::BrushStyle::NoBrush);

						for (int i = 0; i < tracker->vidFrameData[fI].markers.count(); ++i)
						{
							float x = tracker->vidFrameData[fI].markers[i].x();
							float y = tracker->vidFrameData[fI].markers[i].y();
							qp.drawEllipse(_GetVPointF(tX + x, y), 5.0f, 5.0f);

							if (fI == tracker->drawMarkerFrameIndex)
								qp.drawText(_GetVPointF(tX + x + 5, y + 5), QString::number(x) + ", " + QString::number(y));
						}
						
						for (int i = 0; i < tracker->vidFrameData[fI].newMarkers.count(); ++i)
						{
							qp.setBrush(QBrush(QColor::fromRgb(0, 255, 0), Qt::BrushStyle::SolidPattern));
							qp.setPen(Qt::PenStyle::NoPen);

							NewMarker* m = &tracker->vidFrameData[fI].newMarkers[i];

							qp.drawEllipse(_GetVPointF(tX + m->pos.x(), m->pos.y()), 2.0f, 2.0f);

							if (fI == tracker->drawMarkerFrameIndex)
							{
								qp.setPen(QPen(QColor::fromRgb(0, 255, 0)));
								qp.drawText(_GetVPointF(tX + m->pos.x() + 5, m->pos.y() - 5), QString::number(m->pos.x()) + ", " + QString::number(m->pos.y()));

								QPointF bMin = _GetVPointF(m->bounds.x() + tX - 0.5f, m->bounds.y() - 0.5f);
								QPointF bMax = _GetVPointF(m->bounds.z() + tX + 0.5f, m->bounds.w() + 0.5f);

								qp.setBrush(Qt::BrushStyle::NoBrush);
								qp.drawRect(QRectF(bMin, bMax));
							}
						}
					}
				}
			}
		}
		
		qp.setPen(QPen(QColor::fromRgb(255, 255, 255)));
		qp.setBrush(Qt::BrushStyle::NoBrush);

		/*
		// NOTE: Draw epipolar line in paired cam view.
		for (int t = 0; t < take->trackers.count(); ++t)
		{
			TakeTracker* tracker = take->trackers[t];

			float tX = 10;
			float tY = oY;

			if (t == 0)
				tX += 1010;

			for (int i = 0; i < tracker->vidFrameData[tracker->drawMarkerFrameIndex].epiLines.count(); ++i)
			{
				EpipolarLine e = tracker->vidFrameData[tracker->drawMarkerFrameIndex].epiLines[i];

				float a = e.a;
				float b = e.b;
				float c = e.c;
				float y1 = -(a * 0 + c) / b;
				float y2 = -(a * 1000 + c) / b;

				qp.drawLine(QPointF(tX, y1 + oY), QPointF(tX + 1000, y2 + oY));
			}

			for (int m = 0; m < take->markers[timelineFrame].size(); ++m)
			{
				QVector3D markerPos = take->markers[timelineFrame][m].pos;

				cv::Mat wp(4, 1, CV_64F);
				wp.at<double>(0) = markerPos.x();
				wp.at<double>(1) = markerPos.y();
				wp.at<double>(2) = markerPos.z();
				wp.at<double>(3) = 1.0;

				cv::Mat imgPt = tracker->decoder->projMat * wp;
				float tX = 10 + 1010 * t;
				float tY = oY;
				float x = imgPt.at<double>(0) / imgPt.at<double>(2);
				float y = imgPt.at<double>(1) / imgPt.at<double>(2);
				qp.drawEllipse(QPointF(tX + x, tY + y), 7.0f, 7.0f);
			}
		}
		*/

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
		_mouseLeft = true;
	else if (Event->button() == Qt::MouseButton::RightButton)
		_mouseRight = true;
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

		if (tracker)
		{
			int mX = trackerSpace.x() * 128;
			int mY = trackerSpace.y() * 88;

			tracker->maskData[mY * 128 + mX] = { 255, 0, 0, 128 };
		}
		
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
				interactNone->setChecked(true);

				QAction* interactMasks = contextMenu.addAction("Edit Mask");
				
				QAction* interactMarkers = contextMenu.addAction("Select Markers");

				QAction* result = contextMenu.exec(Event->globalPos());

				if (result == video)
				{
					main->viewFeed(tracker->id, true);
				}
				else if (result == markers)
				{
					main->viewFeed(tracker->id, false);
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
	for (std::map<int, LiveTracker*>::iterator it = main->liveTrackers.begin(); it != main->liveTrackers.end(); ++it)
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
	for (std::map<int, LiveTracker*>::iterator it = main->liveTrackers.begin(); it != main->liveTrackers.end(); ++it)
	{
		LiveTracker* tracker = it->second;
		tracker->selected = false;

		trackerCount++;
	}
}