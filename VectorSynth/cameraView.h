#pragma once

#include <QWidget>

#include "take.h"
#include "liveTracker.h"

QT_USE_NAMESPACE

class MainWindow;

class CameraView : public QWidget
{
	Q_OBJECT

public:

	CameraView(QWidget* Parent, MainWindow* Main);

	MainWindow* main;
	
	int			mode;

	QImage		camImage;
	
	// Take view.
	Take*		take;
	int			timelineFrame;
	
	// Global view controls.
	QVector2D	viewTranslate;
	float		viewZoom;

	int			hoveredId;

	bool		showMask;
	int			markerMode;
	bool		showPixelGrid;
	bool		showCamRays;
	bool		showDistortedMarkers;
	bool		showUndistortedMarkers;
	bool		showReprojectedMarkers;
	bool		alignHorizontal;

	void setMask(bool Visible);
	void setMarkerMode(int Mode);
	void setPixelGrid(bool Visible);

protected:

	void paintEvent(QPaintEvent* Event);
	void mousePressEvent(QMouseEvent* Event);
	void mouseMoveEvent(QMouseEvent* Event);
	void mouseReleaseEvent(QMouseEvent* Event);
	void wheelEvent(QWheelEvent* Event);

private:

	QFont _mainFont;
	QFont _detailFont;
	QFont _largeFont;
	QFont _medFont;

	bool		_mouseLeft;
	bool		_mouseRight;
	QPointF		_mouseDownPos;
	QPointF		_mouseMovedPos;
	int			_mouseDownTrackerId;
	int			_editMaskMode;

	QTransform	_vt;

	QPointF _GetVPointF(float X, float Y);
	QPoint _GetVPoint(float X, float Y);

	LiveTracker* _GetTracker(int X, int Y, QVector2D* TrackerSpace = 0);
	void _DeselectTrackers();

	void _ChangeMask(LiveTracker* Tracker, int X, int Y, bool Value);
};