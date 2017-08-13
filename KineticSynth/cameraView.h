#pragma once

#include <QWidget>

#include "take.h"

QT_USE_NAMESPACE

struct blob
{
	float minX;
	float minY;
	float maxX;
	float maxY;
	float cX;
	float cY;
};

class CameraView : public QWidget
{
	Q_OBJECT

public:

	CameraView(QWidget* Parent = 0);

	int			mode;

	// Live view.
	QImage		camImage;
	float		fps;
	float		totalFps;
	float		dataRecvBytes;

	uint8_t		markerData[1024 * 10];
	int			markerDataSize;

	// Take view.
	Take*		take;
	int			timelineFrame;

	int			timestampA;
	QImage		camImageA;
	QString		camNameA;

	int			timestampB;
	QImage		camImageB;
	QString		camNameB;

	// Global view controls.
	QVector2D	viewTranslate;
	float		viewZoom;

protected:

	void paintEvent(QPaintEvent* Event);
	void mousePressEvent(QMouseEvent* Event);
	void mouseMoveEvent(QMouseEvent* Event);
	void mouseReleaseEvent(QMouseEvent* Event);
	void wheelEvent(QWheelEvent* Event);

private:

	QFont _mainFont;
	QFont _detailFont;

	bool		_mouseLeft;
	bool		_mouseRight;
	QPointF		_mouseDownPos;
	QPointF		_mouseMovedPos;

	QTransform	_vt;

	QPointF _GetVPointF(float X, float Y);
	QPoint _GetVPoint(float X, float Y);
};