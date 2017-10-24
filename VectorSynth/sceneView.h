#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QElapsedTimer>

#include "take.h"

struct VertexData
{
	QVector3D position;
	QVector3D color;
};

struct VertexDataShaded
{
	QVector3D position;
	QVector3D color;
	QVector3D normal;
};

class Model
{
public:

	QOpenGLBuffer	vertBuffer;
	QOpenGLBuffer	indexBuffer;
	int				indexCount;

	Model();
	void createFromOBJ(QString Filename);
};

class SceneView : public QOpenGLWidget, protected QOpenGLFunctions
{
public:

	Take*		take;
	int			timelineFrame;
	QVector3D	_lastSelectedPos[3];

	SceneView(QWidget *Parent = 0);
	
	void tick();
	void setPattern(float XFreq, float XMin, float XMax, float YFreq, float YMin, float YMax, float Speed);
	void restartPattern();
	void pushSamplePoint(QVector3D Pos, QVector3D Color);

	QVector3D backColor;

	void gizmoClear();
	void gizmoPush(VertexData Vert);

protected:

	void initializeGL();
	void resizeGL(int W, int H);
	void paintGL();
	void mousePressEvent(QMouseEvent* Event);
	void mouseMoveEvent(QMouseEvent* Event);
	void mouseReleaseEvent(QMouseEvent* Event);
	void wheelEvent(QWheelEvent* Event);

private:

	QFont					_mainFont;

	bool					_initialized;

	QOpenGLShaderProgram	_minimalShader;
	QOpenGLShaderProgram	_basicShader;
	QOpenGLShaderProgram	_shadedShader;

	int						_gizmoBufferSize;
	int						_gizmoIndex;
	QOpenGLBuffer			_gizmoBuffer;
	VertexData				*_gizmoData;

	QOpenGLBuffer			_arrayBuf;
	QOpenGLBuffer			_indexBuf;
	int						_indexCount;

	QOpenGLBuffer			_gridArrayBuffer;
	int						_gridPrimCount;

	QOpenGLBuffer			_scanPatternBuffer;
	int						_scanPatternVertCount;
	int						_scanCurrentMaxPoints;
	VertexData				*_scanData;
	int						_scanIndex;
	float					_scanXAccum;
	float					_scanYAccum;
	float					_scanXFreq;
	float					_scanYFreq;
	float					_scanXMin;
	float					_scanXMax;
	float					_scanYMin;
	float					_scanYMax;
	float					_scanSpeed;

	QMatrix4x4				_projMat;

	QElapsedTimer			_time;
	int64_t					_lastTime;
	
	bool					_mouseLeft;
	bool					_mouseRight;
	QPointF					_mouseDownPos;
	QPointF					_mouseMovedPos;

	QMatrix4x4				_camViewMat;
	float					_camX;
	float					_camY;
	float					_camZoom;
	QVector3D				_camTranslate;
	QMatrix4x4				_pointWorldMat;

	Model					_sphereModel;

	QVector3D				_pickPos;
	QVector3D				_pickDir;

	int						_selectedIdx;	

	void _mousePick(QVector2D ScreenPos);
};