#include "timelineWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

QT_USE_NAMESPACE

TimelineWidget::TimelineWidget(QWidget* Parent) :
	QWidget(Parent)
{
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	setMouseTracking(true);

	setMinimumSize(256, 40);

	_mainFont = QFont("Arial", 8);
	
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	totalFrames = 1000;
	selectedFrame = 0;
	rangeStartFrame = 50;
	rangeEndFrame = 200;
}

void TimelineWidget::paintEvent(QPaintEvent* Event)
{
	Q_UNUSED(Event);

	QPainter qp(this);
	qp.fillRect(0, 0, width(), height(), QColor(28, 30, 32));
	qp.fillRect(0, 25, width(), 40 - 25, QColor(43, 45, 51));
	
	int markCount = width() / 64;

	if (markCount > totalFrames)
		markCount = totalFrames;

	float spacing = (float)width() / markCount;

	int markValueInc = totalFrames / markCount;

	qp.setBrush(Qt::BrushStyle::NoBrush);
	qp.setPen(QPen(QColor::fromRgb(100, 100, 100)));

	for (int i = 0; i < markCount; ++i)
	{
		int x = i * spacing;
		qp.drawLine(x, 15, x, 24);
		qp.drawText(QPointF(x, 14), QString::number(markValueInc * i));
	}

	qp.setRenderHint(QPainter::RenderHint::Antialiasing, true);

	int runnerX = (int)(((float)width() / totalFrames) * selectedFrame);

	int startRunnerX = (int)(((float)width() / totalFrames) * rangeStartFrame);
	int endRunnerX = (int)(((float)width() / totalFrames) * rangeEndFrame);

	qp.setBrush(QBrush(QColor::fromRgb(255, 255, 255, 100)));
	qp.setPen(Qt::PenStyle::NoPen);
	qp.drawRoundedRect(runnerX, 0, 10, 25, 4, 4);

	qp.setBrush(QBrush(QColor::fromRgb(100, 100, 100, 255)));
	qp.setPen(Qt::PenStyle::NoPen);

	qp.drawRect(startRunnerX + 5, 25, endRunnerX - startRunnerX, 5);

	qp.setPen(QPen(QColor::fromRgb(43, 45, 51)));
	qp.drawRoundedRect(startRunnerX, 23, 10, 10, 4, 4);
	qp.drawRoundedRect(endRunnerX, 23, 10, 10, 4, 4);

	qp.setRenderHint(QPainter::RenderHint::Antialiasing, false);
}

void TimelineWidget::mousePressEvent(QMouseEvent* Event)
{
	if (Event->button() == Qt::MouseButton::LeftButton)
	{
		QPointF pos = Event->localPos();
		_grabbedComponent = 0;

		if (pos.y() < 25)
		{
			_grabbedComponent = 1;
		}
		else
		{
			int startRunnerX = (int)(((float)width() / totalFrames) * rangeStartFrame);
			int endRunnerX = (int)(((float)width() / totalFrames) * rangeEndFrame);

			if (pos.x() >= startRunnerX && pos.x() <= startRunnerX + 10)
				_grabbedComponent = 2;
			else if (pos.x() >= endRunnerX && pos.x() <= endRunnerX + 10)
				_grabbedComponent = 3;
		}

		_updateInteract(pos.x());
	}
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* Event)
{
	if (Event->buttons() & Qt::MouseButton::LeftButton)
	{
		QPointF pos = Event->localPos();
		_updateInteract(pos.x());
	}
}

void TimelineWidget::_updateInteract(int LocalXPixel)
{
	int frame = _getFrame(LocalXPixel);
	
	if (_grabbedComponent == 1)
	{
		selectedFrame = frame;
		emit valueChanged(selectedFrame);
	}
	else if (_grabbedComponent == 2)
	{
		if (frame >= rangeEndFrame)
			frame = rangeEndFrame - 1;

		rangeStartFrame = frame;
	}
	else if (_grabbedComponent == 3)
	{
		if (frame <= rangeStartFrame)
			frame = rangeStartFrame + 1;

		rangeEndFrame = frame;
	}

	update();
}

int TimelineWidget::_getFrame(int LocalXPixel)
{
	if (LocalXPixel < 0)
		LocalXPixel = 0;
	else if (LocalXPixel >= width())
		LocalXPixel = width() - 1;

	float t = (float)LocalXPixel / (width() - 1);

	return t * (totalFrames - 1);
}

void TimelineWidget::setParams(int TotalFrames)
{
	totalFrames = TotalFrames;
	selectedFrame = 0;
	rangeStartFrame = 100;
	rangeEndFrame = TotalFrames - 100;
	update();
}

void TimelineWidget::adjustSelectedFrame(int Value)
{
	setFrame(selectedFrame + Value);
}

void TimelineWidget::setFrame(int Value)
{	
	selectedFrame = Value;

	if (selectedFrame < 0)
		selectedFrame = 0;
	else if (selectedFrame >= totalFrames)
		selectedFrame = totalFrames - 1;

	emit valueChanged(selectedFrame);

	update();
}