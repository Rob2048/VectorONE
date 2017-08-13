#pragma once

#include <QWidget>

QT_USE_NAMESPACE

class TimelineWidget : public QWidget
{
	Q_OBJECT
signals:
	void valueChanged(int Value);

public:
	int totalFrames;
	int selectedFrame;
	int rangeStartFrame;
	int rangeEndFrame;

	TimelineWidget(QWidget* Parent = 0);
	void setParams(int TotalFrames);
	void adjustSelectedFrame(int Value);
	void setFrame(int Value);

protected:
	void paintEvent(QPaintEvent* Event);
	void mousePressEvent(QMouseEvent* Event);
	void mouseMoveEvent(QMouseEvent* Event);
	//void mouseReleaseEvent(QMouseEvent* Event);
	
private:
	QFont _mainFont;
	int _grabbedComponent;

	void _updateInteract(int LocalXPixel);
	int _getFrame(int LocalXPixel);
};