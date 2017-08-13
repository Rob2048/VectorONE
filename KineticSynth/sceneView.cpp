#include "sceneView.h"
#include "objLoader.h"

#include <QDebug>
#include <QtMath>
#include <QMouseEvent>
#include <QPainter>

QT_USE_NAMESPACE

Model::Model()
	: indexBuffer(QOpenGLBuffer::IndexBuffer),
	indexCount(0)
{

}

void Model::createFromOBJ(QString FileName)
{
	vsOBJModel obj = CreateOBJ(FileName.toLatin1());
	qDebug() << "Model loaded" << FileName << obj.vertCount << obj.indexCount;

	vertBuffer.create();
	vertBuffer.bind();
	vertBuffer.allocate(obj.verts, sizeof(vsVertex) * obj.vertCount);

	indexBuffer.create();
	indexBuffer.bind();
	indexBuffer.allocate(obj.indices, sizeof(uint16_t) * obj.indexCount);
	indexCount = obj.indexCount;
}

SceneView::SceneView(QWidget *Parent) :
	QOpenGLWidget(Parent),
	_indexBuf(QOpenGLBuffer::IndexBuffer),
	_initialized(false)
{
	_scanCurrentMaxPoints = 0;
	_scanPatternVertCount = 4000;
	_scanData = new VertexData[_scanPatternVertCount];

	QSurfaceFormat format;
	format.setSamples(4);
	setFormat(format);

	_time.start();
	_lastTime = 0;

	_camX = 45;
	_camY = 45;
	_camZoom = 5;
	_camTranslate = QVector3D(0, 0, 0);

	take = 0;

	_mouseLeft = false;
	_mouseRight = false;

	_mainFont = QFont("Arial", 8);
}

void SceneView::gizmoClear()
{
	_gizmoIndex = 0;
}

void SceneView::gizmoPush(VertexData Vert)
{
	_gizmoData[_gizmoIndex++] = Vert;
}

void SceneView::pushSamplePoint(QVector3D Pos)
{
	/*
	// Shift all the points
	if (_scanCurrentMaxPoints == _scanPatternVertCount)
	{
		for (int i = 0; i < _scanPatternVertCount - 1; ++i)
		{
			_scanData[i] = _scanData[i + 1];
		}
	}

	if (_scanCurrentMaxPoints < _scanPatternVertCount)
		++_scanCurrentMaxPoints;

	_scanData[_scanCurrentMaxPoints - 1].position = Pos;
	_scanData[_scanCurrentMaxPoints - 1].color = QVector3D(1, 1, 1);
	*/

	_scanData[_scanCurrentMaxPoints].position = Pos;
	_scanData[_scanCurrentMaxPoints].color = QVector3D(1, 1, 1);
	++_scanCurrentMaxPoints;
}

void SceneView::initializeGL()
{
	QOpenGLFunctions::initializeOpenGLFunctions();

	//---------------------------------------------------------------------------------
	// Minimal Shader.
	//---------------------------------------------------------------------------------
	if (!_minimalShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "assets/shaders/minimal.vert"))
		close();

	if (!_minimalShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "assets/shaders/minimal.frag"))
		close();

	if (!_minimalShader.link())
		close();
	
	//---------------------------------------------------------------------------------
	// Basic Shader.
	//---------------------------------------------------------------------------------
	if (!_basicShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "assets/shaders/basic.vert"))
		close();

	if (!_basicShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "assets/shaders/basic.frag"))
		close();

	if (!_basicShader.link())
		close();

	//---------------------------------------------------------------------------------
	// Shaded Shader.
	//---------------------------------------------------------------------------------
	if (!_shadedShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "assets/shaders/shaded.vert"))
		close();

	if (!_shadedShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "assets/shaders/shaded.frag"))
		close();

	if (!_shadedShader.link())
		close();

	//---------------------------------------------------------------------------------
	// Cube Mesh.
	//---------------------------------------------------------------------------------
	float hw = 0.1f * 0.5f;
	VertexDataShaded verts[] = 
	{
		{ QVector3D(-hw, -hw, -hw),	QVector3D(hw, hw, hw), QVector3D(0, 0, -1) },
		{ QVector3D(-hw, hw, -hw),	QVector3D(hw, hw, hw), QVector3D(0, 0, -1) },
		{ QVector3D(hw, hw, -hw),		QVector3D(hw, hw, hw), QVector3D(0, 0, -1) },
		{ QVector3D(hw, -hw, -hw),	QVector3D(hw, hw, hw), QVector3D(0, 0, -1) },

		{ QVector3D(-hw, -hw, hw),	QVector3D(hw, hw, hw), QVector3D(0, 0, 1) },
		{ QVector3D(-hw, hw, hw),		QVector3D(hw, hw, hw), QVector3D(0, 0, 1) },
		{ QVector3D(hw, hw, hw),		QVector3D(hw, hw, hw), QVector3D(0, 0, 1) },
		{ QVector3D(hw, -hw, hw),		QVector3D(hw, hw, hw), QVector3D(0, 0, 1) },

		{ QVector3D(-hw, hw, -hw),	QVector3D(hw, hw, hw), QVector3D(0, 1, 0) },
		{ QVector3D(-hw, hw, hw),		QVector3D(hw, hw, hw), QVector3D(0, 1, 0) },
		{ QVector3D(hw, hw, hw),		QVector3D(hw, hw, hw), QVector3D(0, 1, 0) },
		{ QVector3D(hw, hw, -hw),		QVector3D(hw, hw, hw), QVector3D(0, 1, 0) },

		{ QVector3D(-hw, -hw, -hw),	QVector3D(hw, hw, hw), QVector3D(0, -1, 0) },
		{ QVector3D(-hw, -hw, hw),	QVector3D(hw, hw, hw), QVector3D(0, -1, 0) },
		{ QVector3D(hw, -hw, hw),		QVector3D(hw, hw, hw), QVector3D(0, -1, 0) },
		{ QVector3D(hw, -hw, -hw),	QVector3D(hw, hw, hw), QVector3D(0, -1, 0) },

		{ QVector3D(hw, -hw, -hw),	QVector3D(hw, hw, hw), QVector3D(1, 0, 0) },
		{ QVector3D(hw, -hw, hw),		QVector3D(hw, hw, hw), QVector3D(1, 0, 0) },
		{ QVector3D(hw, hw, hw),		QVector3D(hw, hw, hw), QVector3D(1, 0, 0) },
		{ QVector3D(hw, hw, -hw),		QVector3D(hw, hw, hw), QVector3D(1, 0, 0) },

		{ QVector3D(-hw, -hw, -hw),	QVector3D(hw, hw, hw), QVector3D(-1, 0, 0) },
		{ QVector3D(-hw, -hw, hw),	QVector3D(hw, hw, hw), QVector3D(-1, 0, 0) },
		{ QVector3D(-hw, hw, hw),		QVector3D(hw, hw, hw), QVector3D(-1, 0, 0) },
		{ QVector3D(-hw, hw, -hw),	QVector3D(hw, hw, hw), QVector3D(-1, 0, 0) },
	};

	GLushort indices[] = {
		0, 1, 2,
		0, 2, 3,

		6, 5, 4,
		7, 6, 4,

		8, 9, 10,
		8, 10, 11,

		14, 13, 12,
		15, 14, 12,

		18, 17, 16,
		19, 18, 16,

		20, 21, 22,
		20, 22, 23
	};

	_arrayBuf.create();	
	_arrayBuf.bind();
	_arrayBuf.allocate(verts, sizeof(verts));

	_indexBuf.create();
	_indexBuf.bind();
	_indexBuf.allocate(indices, sizeof(indices));
	_indexCount = sizeof(indices) / sizeof(GLushort);

	//---------------------------------------------------------------------------------
	// Grid Mesh.
	//---------------------------------------------------------------------------------
	const float gridSpacing = 0.1f;
	const int gridCount = 50;
	const float gridWidth = gridSpacing * gridCount;
	const float gridHalfWidth = gridWidth * 0.5f;

	VertexData gridVerts[(gridCount + 1) * 4];

	for (int i = 0; i < gridCount + 1; ++i)
	{
		QVector3D color(0.25f, 0.25f, 0.25f);

		if (i == gridCount / 2)
			color = QVector3D(0.5f, 0.5f, 0.5f);

		gridVerts[i * 4 + 0].position = QVector3D(-gridHalfWidth, 0, (i * gridSpacing) - gridHalfWidth); gridVerts[i * 4 + 0].color = color;
		gridVerts[i * 4 + 1].position = QVector3D(+gridHalfWidth, 0, (i * gridSpacing) - gridHalfWidth); gridVerts[i * 4 + 1].color = color;

		gridVerts[i * 4 + 2].position = QVector3D((i * gridSpacing) - gridHalfWidth, 0, -gridHalfWidth); gridVerts[i * 4 + 2].color = color;
		gridVerts[i * 4 + 3].position = QVector3D((i * gridSpacing) - gridHalfWidth, 0, +gridHalfWidth); gridVerts[i * 4 + 3].color = color;
	}

	_gridArrayBuffer.create();
	_gridArrayBuffer.bind();
	_gridArrayBuffer.allocate(gridVerts, sizeof(gridVerts));
	_gridPrimCount = sizeof(gridVerts) / sizeof(VertexData);

	//---------------------------------------------------------------------------------
	// Scan Pattern Buffer.
	//---------------------------------------------------------------------------------
	_scanPatternBuffer.create();
	_scanPatternBuffer.bind();
	_scanPatternBuffer.setUsagePattern(QOpenGLBuffer::StreamDraw);
	_scanPatternBuffer.allocate((_scanPatternVertCount) * sizeof(VertexData));
	_scanIndex = 0;

	//---------------------------------------------------------------------------------
	// Gizmo Buffer.
	//---------------------------------------------------------------------------------
	_gizmoBufferSize = 1000;
	_gizmoBuffer.create();
	_gizmoBuffer.bind();
	_gizmoBuffer.setUsagePattern(QOpenGLBuffer::StreamDraw);
	_gizmoBuffer.allocate(_gizmoBufferSize * sizeof(VertexData));
	_gizmoData = new VertexData[_gizmoBufferSize];
	gizmoClear();

	//---------------------------------------------------------------------------------
	// Load Models.
	//---------------------------------------------------------------------------------
	_sphereModel.createFromOBJ("assets/models/sphere.obj");

	//---------------------------------------------------------------------------------
	// General.
	//---------------------------------------------------------------------------------
	backColor = QVector3D(0.2f, 0.2f, 0.2f);
	_initialized = true;
}

void SceneView::resizeGL(int W, int H)
{
	if (H != 0)
	{
		_projMat.setToIdentity();
		_projMat.perspective(60.0f, (float)W / (float)H, 0.1f, 100.0f);
		//glViewport(0, 0, W, H);
	}
}

void SceneView::tick()
{
	// NOTE: Called somewhat at 30Hz.

	if (!_initialized)
		return;

	int64_t nt = _time.elapsed();
	float dt = (nt - _lastTime) / 1000.0f;
	_lastTime = nt;

	update();
}

void SceneView::setPattern(float XFreq, float XMin, float XMax, float YFreq, float YMin, float YMax, float Speed)
{
	_scanXFreq = XFreq;
	_scanXMin = XMin;
	_scanXMax = XMax;
	_scanYFreq = YFreq;
	_scanYMin = YMin;
	_scanYMax = YMax;
	_scanSpeed = Speed;

	restartPattern();
}

void SceneView::restartPattern()
{
	_scanXAccum = (float)M_PI_2;
	_scanYAccum = 0.0f;
	_scanIndex = 0;
	_scanCurrentMaxPoints = 0;
	//memset(_scanData, 0, _scanPatternVertCount * sizeof(VertexData));
}

void _ClosestPointsLines(QVector3D P1, QVector3D D1, QVector3D P2, QVector3D D2, QVector3D* C1, QVector3D* C2)
{
	QVector3D r = P1 - P2;
	float a = QVector3D::dotProduct(D1, D1);
	float b = QVector3D::dotProduct(D1, D2);
	float c = QVector3D::dotProduct(D1, r);
	float e = QVector3D::dotProduct(D2, D2);
	float f = QVector3D::dotProduct(D2, r);
	float denom = a * e - b * b;

	float t = 0.0f;
	float s = 0.0f;

	if (denom != 0.0f)
		s = (b * f - c * e) / denom;

	t = (b * s + f) / e;

	*C1 = P1 + D1 * s;
	*C2 = P2 + D2 * t;
}


void SceneView::paintGL()
{	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(backColor.x(), backColor.y(), backColor.z(), 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//glDisable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	_camViewMat.setToIdentity();
	_camViewMat.rotate(-90, QVector3D(1, 0, 0));
	_camViewMat.translate(0, _camZoom, 0);
	_camViewMat.rotate(_camY, QVector3D(1, 0, 0));
	_camViewMat.rotate(_camX, QVector3D(0, 0, 1));
	_camViewMat.translate(_camTranslate);
	_camViewMat.rotate(-90, QVector3D(1, 0, 0));

	_pointWorldMat.setToIdentity();	
	//_pointWorldMat.rotate(-17.5f, QVector3D(0, 1, 0));
	//_pointWorldMat.rotate(5, QVector3D(1, 0, 0));
	//_pointWorldMat.scale(1.47f / 1.009f);

	if (take)
	{
		_pointWorldMat(0, 0) = take->wX.x();
		_pointWorldMat(0, 1) = take->wX.y();
		_pointWorldMat(0, 2) = take->wX.z();

		_pointWorldMat(1, 0) = take->wY.x();
		_pointWorldMat(1, 1) = take->wY.y();
		_pointWorldMat(1, 2) = take->wY.z();

		_pointWorldMat(2, 0) = take->wZ.x();
		_pointWorldMat(2, 1) = take->wZ.y();
		_pointWorldMat(2, 2) = take->wZ.z();

		_pointWorldMat.translate(take->wT);

		QMatrix4x4 scaleMat;
		scaleMat.scale(take->wScale);
		_pointWorldMat = scaleMat * _pointWorldMat;
	}

	QMatrix4x4 modelMat;
	modelMat.setToIdentity();
	modelMat.scale(0.1f);

	/*
	QVector3D dir = _scanData[_scanPatternVertCount + 1].position - _scanData[_scanPatternVertCount + 0].position;
	dir.normalize();
	QVector3D forward = dir;
	QVector3D up = QVector3D::crossProduct(QVector3D(0, 0, 1), forward).normalized();
	QVector3D right = QVector3D::crossProduct(forward, up);
	modelMat.setColumn(0, QVector4D(right, 0));
	modelMat.setColumn(1, QVector4D(forward, 0));
	modelMat.setColumn(2, QVector4D(up, 0));
	modelMat.setColumn(3, QVector4D(0, 0, 0, 1));
	QMatrix4x4 m2;
	m2.translate(_scanData[_scanPatternVertCount + 0].position);
	modelMat.setToIdentity();
	modelMat.rotate(_scanData[_scanPatternVertCount + 1].position.x() * 22.5f, QVector3D(1, 0, 0));
	modelMat.rotate(_scanData[_scanPatternVertCount + 1].position.y() * 22.5f, QVector3D(0, 1, 0));
	modelMat = m2 * modelMat;
	modelMat.scale(0.1f);
	*/

	int shadedVertexLocation = _shadedShader.attributeLocation("a_position");
	int shadedColorLocation = _shadedShader.attributeLocation("a_color");
	int shadedNormalLocation = _shadedShader.attributeLocation("a_normal");
	_shadedShader.bind();
	_shadedShader.enableAttributeArray(shadedVertexLocation);
	_shadedShader.enableAttributeArray(shadedColorLocation);
	_shadedShader.enableAttributeArray(shadedNormalLocation);
	_shadedShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * modelMat);
	_shadedShader.setUniformValue("u_model_mat", modelMat);
	_shadedShader.setUniformValue("u_color", QVector4D(1.0f, 0.5f, 0.3f, 1.0f));

	_arrayBuf.bind();
	_shadedShader.setAttributeBuffer(shadedVertexLocation, GL_FLOAT, 0 * sizeof(float), 3, sizeof(VertexDataShaded));
	_shadedShader.setAttributeBuffer(shadedColorLocation, GL_FLOAT, 3 * sizeof(float), 3, sizeof(VertexDataShaded));
	_shadedShader.setAttributeBuffer(shadedNormalLocation, GL_FLOAT, 6 * sizeof(float), 3, sizeof(VertexDataShaded));
	_indexBuf.bind();
	/*
	glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_SHORT, 0);

	_shadedShader.setUniformValue("u_mvp_mat", _projMat * viewMat * worldMat * cam2Mat);
	glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_SHORT, 0);
	*/
	_arrayBuf.release();
	_indexBuf.release();

	//----------------------------------------------------------------------------------------
	// Draw Models
	//----------------------------------------------------------------------------------------
	int minimalVertexLocation = _minimalShader.attributeLocation("a_position");
	_minimalShader.bind();
	_minimalShader.enableAttributeArray(minimalVertexLocation);

	_sphereModel.vertBuffer.bind();
	_sphereModel.indexBuffer.bind();
	_minimalShader.setAttributeBuffer(minimalVertexLocation, GL_FLOAT, 0 * sizeof(float), 3, sizeof(vsVertex));

	if (take)
	{
		for (int fI = timelineFrame - 100; fI <= timelineFrame; ++fI)
		{
			if (fI >= 0)
			{
				for (int i = 0; i < take->markers[fI].size(); ++i)
				{
					Marker3D* m = &take->markers[fI][i];
					QVector3D markerPos = (_pointWorldMat * QVector4D(m->pos.x(), m->pos.y(), m->pos.z(), 1.0f)).toVector3DAffine();
					QMatrix4x4 markerMat;
					markerMat.translate(markerPos);

					if (fI == timelineFrame)
					{
						markerMat.scale(0.02f);
						_minimalShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * markerMat);

						if (_selectedIdx != i)
							_minimalShader.setUniformValue("u_color", QVector4D(1, 1, 1, 1));
						else
							_minimalShader.setUniformValue("u_color", QVector4D(0.5f, 1, 0.5f, 1));
					}
					else
					{
						markerMat.scale(0.0075f);
						_minimalShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * markerMat);
						_minimalShader.setUniformValue("u_color", QVector4D(0.5f, 0.5f, 0.5f, 1));
					}

					glDrawElements(GL_TRIANGLES, _sphereModel.indexCount, GL_UNSIGNED_SHORT, 0);
				}
			}
		}
	}

	/*
	{
		// Pick origin.
		QMatrix4x4 markerMat;
		markerMat.translate(_pickPos);

		_minimalShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * _pointWorldMat * markerMat);
		glDrawElements(GL_TRIANGLES, _sphereModel.indexCount, GL_UNSIGNED_SHORT, 0);
	}
	*/

	_sphereModel.vertBuffer.release();
	_sphereModel.indexBuffer.release();

	int vertexLocation = _basicShader.attributeLocation("a_position");
	int colorLocation = _basicShader.attributeLocation("a_color");
	_basicShader.bind();
	_basicShader.enableAttributeArray(vertexLocation);
	_basicShader.enableAttributeArray(colorLocation);
	_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat);
	_basicShader.setUniformValue("u_color", QVector4D(1, 1, 1, 1));

	_gridArrayBuffer.bind();
	_basicShader.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(VertexData));
	_basicShader.setAttributeBuffer(colorLocation, GL_FLOAT, 3 * sizeof(float), 3, sizeof(VertexData));
	_basicShader.setUniformValue("u_color", QVector4D(1, 1, 1, 1.0f));
	glDrawArrays(GL_LINES, 0, _gridPrimCount);
	_gridArrayBuffer.release();
		
	//glDisable(GL_DEPTH_TEST);
	glEnable(GL_PROGRAM_POINT_SIZE);

	_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat * _pointWorldMat);

	_scanPatternBuffer.bind();
	_scanPatternBuffer.write(0, _scanData, _scanCurrentMaxPoints * sizeof(VertexData));
	_basicShader.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(VertexData));
	_basicShader.setAttributeBuffer(colorLocation, GL_FLOAT, 3 * sizeof(float), 3, sizeof(VertexData));
	_basicShader.setUniformValue("u_color", QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
	glDrawArrays(GL_LINE_STRIP, 0, _scanCurrentMaxPoints);

	//_basicShader.setUniformValue("u_color", QVector4D(1.0f, 0.5f, 0.5f, 1.0f));
	//glDrawArrays(GL_LINE_STRIP, 0, _scanCurrentMaxPoints);

	//_basicShader.setUniformValue("u_color", QVector4D(1.0f, 0.5f, 0.3f, 1.0f));
	//glDrawArrays(GL_LINE_STRIP, _scanPatternVertCount, 2);

	glEnable(GL_DEPTH_TEST);

	//---------------------------------------------------------------------------------
	// Draw Gizmos
	//---------------------------------------------------------------------------------

	_gizmoBuffer.bind();	
	_basicShader.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(VertexData));
	_basicShader.setAttributeBuffer(colorLocation, GL_FLOAT, 3 * sizeof(float), 3, sizeof(VertexData));
	_basicShader.setUniformValue("u_color", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
	
	gizmoClear();

	// World Basis.
	QVector3D o = _lastSelectedPos[1];
	/*
	gizmoPush({ o ,{ 1, 0, 0 } }); gizmoPush({ o + QVector3D(1, 0, 0) ,{ 1, 0, 0 } });
	gizmoPush({ o ,{ 0, 1, 0 } }); gizmoPush({ o + QVector3D(0, 1, 0) ,{ 0, 1, 0 } });
	gizmoPush({ o ,{ 0, 0, 1 } }); gizmoPush({ o + QVector3D(0, 0, 1) ,{ 0, 0, 1 } });
	*/

	if (take)
	{
		//gizmoPush({ o ,{ 1, 0, 0 } }); gizmoPush({ o + take->wX ,{ 1, 0, 0 } });
		//gizmoPush({ o ,{ 0, 1, 0 } }); gizmoPush({ o + take->wY ,{ 0, 1, 0 } });
		//gizmoPush({ o ,{ 0, 0, 1 } }); gizmoPush({ o + take->wZ ,{ 0, 0, 1 } });
	}

	// Pick ray.
	//gizmoPush({ _pickPos, { 1, 0, 0 } }); 
	//gizmoPush({ _pickPos + _pickDir * 10, { 1, 0, 0 } });

	for (int i = 0; i < 2; ++i)
	{
		gizmoPush({ _lastSelectedPos[i], { 0.5f, 0.5f, 0.5f } }); 
		gizmoPush({ _lastSelectedPos[i + 1], { 0.5f, 0.5f, 0.5f } });
	}
	
	_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat * _pointWorldMat);
	_gizmoBuffer.write(0, _gizmoData, _gizmoIndex * sizeof(VertexData));
	glDrawArrays(GL_LINES, 0, _gizmoIndex);

	if (take)
	{
		bool markerFound = false;
		QVector3D po[2];
		QVector3D pd[2];

		for (int i = 0; i < take->trackers.count(); ++i)
		{	
			gizmoClear();
			QVector3D gizPos = QVector3D(0, 0, 0);
			float axisS = 0.1f;
			gizmoPush({ gizPos,{ 0, 0, 0 } });
			gizmoPush({ gizPos + QVector3D(axisS, 0, 0),{ 1, 0, 0 } });
			gizmoPush({ gizPos,{ 0, 0, 0 } });
			gizmoPush({ gizPos + QVector3D(0, axisS, 0),{ 0, 1, 0 } });
			gizmoPush({ gizPos,{ 0, 0, 0 } });
			gizmoPush({ gizPos + QVector3D(0, 0, axisS),{ 0, 0, 1 } });

			/*
			gizmoPush({ {0, 0, 0}, { 1, 1, 1 } }); gizmoPush({ {1, 1, 1}, { 1, 1, 1 } });
			gizmoPush({ { 0, 0, 0 },{ 1, 1, 1 } }); gizmoPush({ { -1, 1, 1 },{ 1, 1, 1 } });
			gizmoPush({ { 0, 0, 0 },{ 1, 1, 1 } }); gizmoPush({ { -1, -1, 1 },{ 1, 1, 1 } });
			gizmoPush({ { 0, 0, 0 },{ 1, 1, 1 } }); gizmoPush({ { 1, -1, 1 },{ 1, 1, 1 } });
			*/

			// Cam Frame
			float frameS = 0.02f;
			QVector3D frameC(1, 1, 1);
			QVector3D frameC1(-frameS, -frameS, 0);
			QVector3D frameC2(frameS, -frameS, 0);
			QVector3D frameC3(frameS, frameS, 0);
			QVector3D frameC4(-frameS, frameS, 0);
			QVector3D frameC5(-frameS, -frameS, -frameS);
			QVector3D frameC6(frameS, -frameS, -frameS);
			QVector3D frameC7(frameS, frameS, -frameS);
			QVector3D frameC8(-frameS, frameS, -frameS);

			gizmoPush({ frameC1, frameC }); gizmoPush({ frameC2, frameC });
			gizmoPush({ frameC2, frameC }); gizmoPush({ frameC3, frameC });
			gizmoPush({ frameC3, frameC }); gizmoPush({ frameC4, frameC });
			gizmoPush({ frameC4, frameC }); gizmoPush({ frameC1, frameC });

			gizmoPush({ frameC5, frameC }); gizmoPush({ frameC6, frameC });
			gizmoPush({ frameC6, frameC }); gizmoPush({ frameC7, frameC });
			gizmoPush({ frameC7, frameC }); gizmoPush({ frameC8, frameC });
			gizmoPush({ frameC8, frameC }); gizmoPush({ frameC5, frameC });

			gizmoPush({ frameC1, frameC }); gizmoPush({ frameC5, frameC });
			gizmoPush({ frameC2, frameC }); gizmoPush({ frameC6, frameC });
			gizmoPush({ frameC3, frameC }); gizmoPush({ frameC7, frameC });
			gizmoPush({ frameC4, frameC }); gizmoPush({ frameC8, frameC });

			// Frustum
			QVector3D sc(1, 1, 1);
			QVector3D ec(1, 1, 1);
			float fld = 0.1f;

			cv::Matx33d m33((double*)take->trackers[i]->decoder->optCamMat.ptr());
			//cv::Matx33d m33((double*)take->trackers[0]->decoder->_calibCameraMatrix.ptr());
			cv::Matx33d m33Inv = m33.inv();

			cv::Matx31d imgPt(0, 0, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			imgPt = cv::Matx31d(1000, 0, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			imgPt = cv::Matx31d(1000, 750, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			imgPt = cv::Matx31d(0, 750, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			/*
			// Marker projections.
			for (int mIdx = 0; mIdx < take->markers[timelineFrame].size(); ++mIdx)
			{
				Marker3D* m = &take->markers[timelineFrame][mIdx];
				QMatrix4x4 markerMat;
				markerMat.translate(m->pos);
				markerMat.scale(0.1f);

				if (i == 0)
					imgPt = cv::Matx31d(m->cam1pos.x(), m->cam1pos.y(), 1);
				else
					imgPt = cv::Matx31d(m->cam2pos.x(), m->cam2pos.y(), 1);

				imgPt = m33Inv * imgPt;
				QVector3D d((float)imgPt(0, 0), (float)imgPt(1, 0), (float)imgPt(2, 0));
				d.normalize();

				float mDist = (take->trackers[i]->decoder->worldPos - m->pos).length();

				gizmoPush({ { 0, 0, 0 },{ 0.25f, 0.5f, 0.25f } });
				//gizmoPush({ { d * mDist },{ 0.125f, 0.25f, 0.125f } });
				gizmoPush({ { d * mDist },{ 0.25f, 0.5f, 0.25f } });

				//markerFound = true;
				if (i == 0)
				{
					po[0] = QVector3D(0, 0, 0);
					pd[0] = d;
				}
				else
				{
					po[1] = (take->trackers[i]->decoder->worldMat * QVector4D(0, 0, 0, 1)).toVector3D();
					pd[1] = (take->trackers[i]->decoder->worldMat * QVector4D(d, 0)).toVector3D();
				}
			}
			*/

			/*
			if (frameMarker)
			{
				if (i == 0)
					imgPt = cv::Matx31d(frameMarker->cam1pos.x(), frameMarker->cam1pos.y(), 1);
				else
					imgPt = cv::Matx31d(frameMarker->cam2pos.x(), frameMarker->cam2pos.y(), 1);

				imgPt = m33Inv * imgPt;
				QVector3D d((float)imgPt(0, 0), (float)imgPt(1, 0), (float)imgPt(2, 0));
				d.normalize();

				gizmoPush({ { 0, 0, 0 }, {1, 0, 0} });
				gizmoPush({ { d * 2 },{ 1, 0, 0 } });

				markerFound = true;
				if (i == 0)
				{
					po[0] = QVector3D(0, 0, 0);
					pd[0] = d;
				}
				else
				{
					po[1] = (take->trackers[i]->decoder->worldMat * QVector4D(0, 0, 0, 1)).toVector3D();
					pd[1] = (take->trackers[i]->decoder->worldMat * QVector4D(d, 0)).toVector3D();
				}
			}
			*/

			_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat * _pointWorldMat * take->trackers[i]->decoder->worldMat);

			_gizmoBuffer.write(0, _gizmoData, _gizmoIndex * sizeof(VertexData));
			glDrawArrays(GL_LINES, 0, _gizmoIndex);
		}

		// Draw marker rays closest intersection line.
		if (markerFound)
		{
			gizmoClear();
			_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat * _pointWorldMat);

			gizmoPush({ po[0], { 0, 0, 1 } });
			gizmoPush({ po[0] + pd[0],{ 0, 0, 1 } });

			gizmoPush({ po[1],{ 0, 0, 1 } });
			gizmoPush({ po[1] + pd[1],{ 0, 0, 1 } });

			QVector3D c1;
			QVector3D c2;
			_ClosestPointsLines(po[0], pd[0], po[1], pd[1], &c1, &c2);

			gizmoPush({ c1,{ 1, 0, 1 } });
			gizmoPush({ c2,{ 1, 0, 1 } });

			_gizmoBuffer.write(0, _gizmoData, _gizmoIndex * sizeof(VertexData));
			glDrawArrays(GL_LINES, 0, _gizmoIndex);
		}
	}

	/*
	// Retrieve last OpenGL color to use as a font color
	GLdouble glColor[4];
	//glGetDoublev(GL_CURRENT_COLOR, glColor);
	QColor fontColor = QColor(255, 255, 255, 255);

	// Render text
	QPainter painter(this);
	painter.setPen(fontColor);
	painter.setFont(_mainFont);
	painter.drawText(0, -100, "Hello");
	painter.end();
	*/
}

void SceneView::mousePressEvent(QMouseEvent* Event)
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

void SceneView::mouseReleaseEvent(QMouseEvent* Event)
{
	QPointF moveDelta = (_mouseDownPos - _mouseMovedPos);
	float moveDeltaLengthSq = moveDelta.x() * moveDelta.x() + moveDelta.y() * moveDelta.y();
	bool moved = (moveDeltaLengthSq > 4 * 4);

	if (_mouseLeft && Event->button() == Qt::MouseButton::LeftButton)
	{
		_mouseLeft = false;

		if (!moved)
			_mousePick(QVector2D(_mouseDownPos.x(), _mouseDownPos.y()));
	}
	else if (_mouseRight && Event->button() == Qt::MouseButton::RightButton)
	{
		_mouseRight = false;
	}
}

void SceneView::mouseMoveEvent(QMouseEvent* Event)
{
	QPointF delta = Event->localPos() - _mouseMovedPos;
	_mouseMovedPos = Event->localPos();

	if (_mouseLeft)
	{
		_camX += delta.x();
		_camY += delta.y();
	}
	else if (_mouseRight)
	{
		QVector3D forward = (_camViewMat * QVector4D(1, 0, 0, 0)).toVector3D();
		forward.setY(0);
		forward.normalize();

		QVector3D right = (_camViewMat * QVector4D(0, 0, 1, 0)).toVector3D();
		right.setY(0);
		right.normalize();

		_camTranslate += QVector3D(forward.z() * delta.y(), -forward.x() * delta.y(), 0.0f) * 0.01f;
		_camTranslate += QVector3D(-right.z() * delta.x(), right.x() * delta.x(), 0.0f) * 0.01f;
		//_camTranslate.setX(_camTranslate.x() + delta.x() * 0.01f);
		//_camTranslate.setY(_camTranslate.y() + delta.y() * 0.01f);
	}
}

void SceneView::wheelEvent(QWheelEvent* Event)
{
	_camZoom -= Event->angleDelta().y() * 0.002f;
}

void SceneView::_mousePick(QVector2D ScreenPos)
{
	QVector2D ndc = (ScreenPos / QVector2D(width(), height())) * QVector2D(2, -2) - QVector2D(1, -1);
	QMatrix4x4 invViewProj = (_projMat * _camViewMat * _pointWorldMat).inverted();
	QVector4D p(ndc.x(), ndc.y(), 1.002f, 1.0f);
	QVector4D projP = invViewProj * p;

	_pickDir = QVector3D(projP.x() / projP.w(), projP.y() / projP.w(), projP.z() / projP.w());
	_pickDir.normalize();

	QVector4D hcPos = (_camViewMat * _pointWorldMat).inverted() * QVector4D(0, 0, 0, 1);
	_pickPos = QVector3D(hcPos.x(), hcPos.y(), hcPos.z());

	if (take)
	{
		int closestIdx = -1;
		float closestD = 10000.0f;

		for (int i = 0; i < take->markers[timelineFrame].size(); ++i)
		{
			float d = take->markers[timelineFrame][i].pos.distanceToLine(_pickPos, _pickDir);

			if (d < 0.05f && d < closestD)
			{
				closestD = d;
				closestIdx = i;
			}
		}

		_selectedIdx = closestIdx;

		if (_selectedIdx != -1)
		{
			float scaledDistance = ((_lastSelectedPos[0] - take->markers[timelineFrame][_selectedIdx].pos).length()) * take->wScale;
			qDebug() << "Distance" << scaledDistance;

			for (int i = 1; i >= 0; --i)
			{
				_lastSelectedPos[i + 1] = _lastSelectedPos[i];
			}

			_lastSelectedPos[0] = take->markers[timelineFrame][_selectedIdx].pos;
		}

		qDebug() << "0" << _lastSelectedPos[0];
		qDebug() << "1" << _lastSelectedPos[1];
		qDebug() << "2" << _lastSelectedPos[2];
	}
}