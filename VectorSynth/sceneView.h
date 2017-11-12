#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

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

struct FontGlyph
{
	QRect bounds;
	float advance;
	float uvX1;
	float uvY1;
	float uvX2;
	float uvY2;
};

struct FontVertex
{
	QVector3D pos;
	QVector2D uv;
	QVector4D col;
};

class SceneView : public QOpenGLWidget, protected QOpenGLFunctions
{
public:

	Take*		take;
	int			timelineFrame;
	bool		selectWorldBasis;
	QVector3D	_lastSelectedPos[3];
	bool		showMarkerSources;
	bool		showRays;
	bool		showExpandedMarkers;

	SceneView(QWidget *Parent = 0);
	
	void tick();
	
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

	enum TextAlignment
	{
		SVTA_LEFT,
		SVTA_MIDDLE,
		SVTA_RIGHT,
	};

	QImage*					_fontSheet;
	FontGlyph				_fontGlyphs[256];
	QOpenGLTexture*			_fontTexture;
	QOpenGLShaderProgram	_fontShader;
	int						_fontSpriteMax;
	int						_fontSpriteCount;
	QOpenGLBuffer			_fontVertBuf;
	FontVertex*				_fontVerts;

	bool					_initialized;

	QOpenGLShaderProgram	_minimalShader;
	QOpenGLShaderProgram	_basicShader;
	QOpenGLShaderProgram	_shadedShader;

	int						_gizmoBufferSize;
	int						_gizmoIndex;
	QOpenGLBuffer			_gizmoBuffer;
	VertexData				*_gizmoData;

	/*
	QOpenGLBuffer			_arrayBuf;
	QOpenGLBuffer			_indexBuf;
	int						_indexCount;
	*/

	QOpenGLBuffer			_gridArrayBuffer;
	int						_gridPrimCount;

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
	
	Model					_sphereModel;

	QVector3D				_pickPos;
	QVector3D				_pickDir;

	int						_selectedIdx;

	void _mousePick(QVector2D ScreenPos);
	void _drawText(int X, int Y, QString Text, QVector4D Color, float Scale, TextAlignment Alignment);

	bool _projectToScreen(QVector3D Pos, QVector2D& ScreenPoint);
};