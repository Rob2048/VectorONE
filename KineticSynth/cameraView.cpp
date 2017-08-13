#include "cameraView.h"
#include <QPainter>
#include <QMouseEvent>

QT_USE_NAMESPACE

CameraView::CameraView(QWidget* Parent) :
	QWidget(Parent)
{
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	setMouseTracking(true);

	setMinimumSize(200, 200);

	_mainFont = QFont("Arial", 8);
	_detailFont = QFont("Arial", 7);
	mode = 0;
	take = 0;

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	viewZoom = 0.4f;
	viewTranslate = QVector2D(-512, -352);

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

	bool drawMarkers = false;

	if (mode == 1 || mode == 2)
		drawMarkers = true;

	qp.setFont(_mainFont);

	if (mode == 0)
	{
		qp.translate(width() / 2, height() / 2);
		qp.scale(viewZoom, viewZoom);
		qp.translate(viewTranslate.x(), viewTranslate.y());
		qp.drawImage(QPointF(-0.5f, -0.5f), camImage);

		qp.setPen(QPen(QColor(200, 200, 200), 1.0f / viewZoom));
		qp.drawRect(0, 0, 1000, 700);
		qp.drawRect(1010, 0, 1000, 700);

		_vt = qp.transform();
		qp.resetTransform();

		qp.drawText(_GetVPoint(0, -5), "Tracker FPS: " + QString::number(fps) + " / " + QString::number(totalFps) + " - " + QString::number(dataRecvBytes) + "KB");
	}
	else if (mode == 1)
	{
		qp.translate(width() / 2, height() / 2);
		qp.scale(viewZoom, viewZoom);
		qp.translate(viewTranslate.x(), viewTranslate.y());
		qp.drawImage(QPointF(-0.5f, -0.5f), camImageA);
		qp.drawImage(QPointF(1010 - 0.5f, -0.5f), camImageB);

		qp.setPen(QPen(QColor(200, 200, 200), 1.0f / viewZoom));
		qp.drawRect(0, 0, 1000, 700);
		qp.drawRect(1010, 0, 1000, 700);

		_vt = qp.transform();
		qp.resetTransform();

		qp.drawText(_GetVPoint(0, -5), camNameA);
		qp.drawText(_GetVPoint(1010, -5), camNameB);
	}
	else if (mode == 3)
	{
		qp.translate(width() / 2, height() / 2);
		qp.scale(viewZoom, viewZoom);
		qp.translate(viewTranslate.x(), viewTranslate.y());
		//qp.drawImage(QPointF(-0.5f, -0.5f), camImage);

		qp.setPen(QPen(QColor(255, 100, 255), 1.0f / viewZoom));
		qp.drawRect(0, 0, 1000, 700);
		qp.drawRect(1010, 0, 1000, 700);

		_vt = qp.transform();
		qp.resetTransform();

		qp.drawText(_GetVPoint(0, -5), "Tracker FPS: " + QString::number(fps) + " / " + QString::number(totalFps) + " - " + QString::number(dataRecvBytes) + "KB");

		qp.setRenderHint(QPainter::Antialiasing, true);

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

		qp.setRenderHint(QPainter::Antialiasing, false);
	}
	
	if (drawMarkers && take)
	{
		qp.setFont(_detailFont);
		qp.setRenderHint(QPainter::Antialiasing, true);

		for (int t = 0; t < take->trackers.count(); ++t)
		{
			TakeTracker* tracker = take->trackers[t];

			if (tracker->vidFrameData.count() > 0)
			{
				float tX = 1010 * t;

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
	bool moved = (moveDeltaLengthSq > 4 * 4);

	if (_mouseLeft && Event->button() == Qt::MouseButton::LeftButton)
	{
		_mouseLeft = false;

		if (!moved)
		{
			// Not dragging
		}
	}
	else if (_mouseRight && Event->button() == Qt::MouseButton::RightButton)
	{
		_mouseRight = false;
	}
}

void CameraView::mouseMoveEvent(QMouseEvent* Event)
{
	QPointF delta = Event->localPos() - _mouseMovedPos;
	_mouseMovedPos = Event->localPos();

	if (_mouseLeft)
	{
		viewTranslate.setX(viewTranslate.x() + delta.x() / viewZoom);
		viewTranslate.setY(viewTranslate.y() + delta.y() / viewZoom);
		update();
	}
	else if (_mouseRight)
	{
		
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