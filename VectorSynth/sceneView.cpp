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
	//_indexBuf(QOpenGLBuffer::IndexBuffer),
	_initialized(false)
{
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

	showMarkerSources = false;
	showRays = false;
	showExpandedMarkers = false;
	selectWorldBasis = false;

	// Font stuff.
	int fontSheetWidth = 1024;
	int fontSheetHeight = 512;
	int fontGlyphPadding = 10;
	_fontSheet = new QImage(fontSheetWidth, fontSheetHeight, QImage::Format::Format_ARGB32);
	QPainter p(_fontSheet);
	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);
	p.setRenderHint(QPainter::HighQualityAntialiasing);
	p.fillRect(0, 0, fontSheetWidth, fontSheetHeight, QColor(0, 0, 0, 255));

	QFont font("Arial", 24);
	QFontMetrics fm(font);

	p.setFont(font);
	p.setPen(QPen(QColor(255, 255, 255, 255)));

	int x = fontGlyphPadding;
	int y = fontGlyphPadding;
	int maxY = 0;

	for (int i = 33; i < 127; ++i)
	{
		char c = i;

		FontGlyph glyph;
		glyph.bounds = fm.boundingRect(c);
		glyph.advance = fm.width(c);

		if (x + glyph.bounds.width() + fontGlyphPadding >= fontSheetWidth)
		{
			x = fontGlyphPadding;
			y += maxY + fontGlyphPadding;
			maxY = 0;
		}

		if (glyph.bounds.height() > maxY)
			maxY = glyph.bounds.height();

		int left = fm.leftBearing(c);

		QRect sheetR(x, y, glyph.bounds.width(), glyph.bounds.height());

		//p.fillRect(x, y, glyph.bounds.width(), glyph.bounds.height(), QColor(128, 0, 0, 255));
		//p.fillRect(x - 1, y - 1, 1, 1, QColor(0, 128, 0, 255));
		p.drawText(x - glyph.bounds.x(), y - glyph.bounds.y(), QChar(c));

		int bx = glyph.bounds.x();

		glyph.uvX1 = (x - 1) / (float)fontSheetWidth;
		glyph.uvY1 = (y - 1) / (float)fontSheetHeight;
		glyph.uvX2 = (x + glyph.bounds.width() + 1) / (float)fontSheetWidth;
		glyph.uvY2 = (y + glyph.bounds.height() + 1) / (float)fontSheetHeight;

		x += glyph.bounds.width() + fontGlyphPadding;

		_fontGlyphs[c] = glyph;
	}

	_fontSheet->save("font_sheet.png");
}

void SceneView::gizmoClear()
{
	_gizmoIndex = 0;
}

void SceneView::gizmoPush(VertexData Vert)
{
	_gizmoData[_gizmoIndex++] = Vert;
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
	// Font Shader
	//---------------------------------------------------------------------------------
	if (!_fontShader.addShaderFromSourceFile(QOpenGLShader::Vertex, "assets/shaders/font.vert"))
		close();

	if (!_fontShader.addShaderFromSourceFile(QOpenGLShader::Fragment, "assets/shaders/font.frag"))
		close();

	if (!_fontShader.link())
		close();

	//---------------------------------------------------------------------------------
	// Cube Mesh.
	//---------------------------------------------------------------------------------
	/*
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
	*/

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
	// Fonts.
	//---------------------------------------------------------------------------------
	_fontTexture = new QOpenGLTexture(*_fontSheet);
	//_fontTexture->setMinificationFilter(QOpenGLTexture::Nearest);
	//_fontTexture->setMagnificationFilter(QOpenGLTexture::Nearest);

	_fontTexture->generateMipMaps();
	_fontTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	_fontTexture->setMagnificationFilter(QOpenGLTexture::Linear);

	_fontSpriteMax = 2048;
	_fontSpriteCount = 0;

	_fontVertBuf.create();
	_fontVertBuf.bind();
	_fontVertBuf.allocate(_fontSpriteMax * 6 * sizeof(FontVertex));
	_fontVerts = new FontVertex[_fontSpriteMax * 4];

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
		_projMat.perspective(60.0f, (float)W / (float)H, 0.01f, 100.0f);
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

	_fontSpriteCount = 0;

	_camViewMat.setToIdentity();
	_camViewMat.rotate(-90, QVector3D(1, 0, 0));
	_camViewMat.translate(0, _camZoom, 0);
	_camViewMat.rotate(_camY, QVector3D(1, 0, 0));
	_camViewMat.rotate(_camX, QVector3D(0, 0, 1));
	_camViewMat.translate(_camTranslate);
	_camViewMat.rotate(-90, QVector3D(1, 0, 0));

	QMatrix4x4 modelMat;
	modelMat.setToIdentity();
	modelMat.scale(0.1f);

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

	/*
	_arrayBuf.bind();
	_shadedShader.setAttributeBuffer(shadedVertexLocation, GL_FLOAT, 0 * sizeof(float), 3, sizeof(VertexDataShaded));
	_shadedShader.setAttributeBuffer(shadedColorLocation, GL_FLOAT, 3 * sizeof(float), 3, sizeof(VertexDataShaded));
	_shadedShader.setAttributeBuffer(shadedNormalLocation, GL_FLOAT, 6 * sizeof(float), 3, sizeof(VertexDataShaded));
	_indexBuf.bind();
	*/
	/*
	glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_SHORT, 0);

	_shadedShader.setUniformValue("u_mvp_mat", _projMat * viewMat * worldMat * cam2Mat);
	glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_SHORT, 0);
	*/
	/*
	_arrayBuf.release();
	_indexBuf.release();
	*/

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
		int ghostBalls = 0;
		for (int fI = timelineFrame - ghostBalls; fI <= timelineFrame; ++fI)
		{
			if (fI >= 0 && fI < take->timeFrames)
			{
				for (int i = 0; i < take->markers[fI].size(); ++i)
				{
					Marker3D* m = &take->markers[fI][i];
					QVector3D markerPos = (QVector4D(m->pos.x(), m->pos.y(), m->pos.z(), 1.0f)).toVector3DAffine();
					QMatrix4x4 markerMat;
					markerMat.translate(markerPos);

					if (fI == timelineFrame)
					{
						if (showExpandedMarkers)
							markerMat.scale(0.03f);
						else
							markerMat.scale(0.01f);

						_minimalShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * markerMat);

						QVector4D markerColor;

						if (_selectedIdx != i)
						{
							if (m->type == Marker3D::T_UNLABLED)
							{
								markerColor = QVector4D(1.0f, 0, 0.0f, 1);
							}
							else if (m->type == Marker3D::T_GHOST)
							{
								markerColor = QVector4D(0.0f, 0, 0.0f, 0.5f);
							}
							else if (m->type == Marker3D::T_LABLED)
							{
								//markerColor = QVector4D(0.0f, 1.0f, 0.0f, 1);

								markerColor.setX((((m->id + 1) * 120) % 255) / 255.0f);
								markerColor.setY((((m->id + 5) * 460) % 255) / 255.0f);
								markerColor.setZ((((m->id + 6) * 1380) % 255) / 255.0f);
								markerColor.setW(1.0f);

								QVector2D screenPos;
								if (_projectToScreen(m->pos + QVector3D(0, -0.02f, 0), screenPos))
								{
									_drawText(screenPos.x(), screenPos.y(), QString::number(m->id), markerColor, 0.4f, SVTA_MIDDLE);
								}
							}
						}
						else
						{
							markerColor = QVector4D(0.1f, 1, 0.1f, 1);
						}

						_minimalShader.setUniformValue("u_color", markerColor);
					}
					else
					{
						markerMat.scale(0.006f);//0.001
						_minimalShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * markerMat);
						_minimalShader.setUniformValue("u_color", QVector4D(0.5f, 0.5f, 0.5f, 1));
					}

					glDrawElements(GL_TRIANGLES, _sphereModel.indexCount, GL_UNSIGNED_SHORT, 0);
				}

				/*
				if (fI == timelineFrame)
				{
					QVector3D rm = take->refMarkers[fI];
					QVector3D markerPos = (_pointWorldMat * QVector4D(rm.x(), rm.y(), rm.z(), 1.0f)).toVector3DAffine();
					QMatrix4x4 markerMat;
					markerMat.translate(markerPos);
					//markerMat.scale(0.015f);
					markerMat.scale(0.004f);
					_minimalShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * markerMat);
					_minimalShader.setUniformValue("u_color", QVector4D(1, 0, 1, 1));
					glDrawElements(GL_TRIANGLES, _sphereModel.indexCount, GL_UNSIGNED_SHORT, 0);
				}
				*/

				/*
				// Calib marker
				if (fI == timelineFrame && take->calibMarkers.size() > fI + 1)
				{
					QVector3D rm = take->calibMarkers[fI];
					QVector3D markerPos = (_pointWorldMat * QVector4D(rm.x(), rm.y(), rm.z(), 1.0f)).toVector3DAffine();
					QMatrix4x4 markerMat;
					markerMat.translate(markerPos);
					//markerMat.scale(0.015f);
					markerMat.scale(0.004f);
					_minimalShader.setUniformValue("u_mvp_mat", _projMat * _camViewMat * markerMat);
					_minimalShader.setUniformValue("u_color", QVector4D(0, 0, 1, 1));
					glDrawElements(GL_TRIANGLES, _sphereModel.indexCount, GL_UNSIGNED_SHORT, 0);
				}
				*/
			}
		}
	}

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

	_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat);

	glEnable(GL_DEPTH_TEST);

	//---------------------------------------------------------------------------------
	// Draw Gizmos
	//---------------------------------------------------------------------------------

	_gizmoBuffer.bind();	
	_basicShader.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(VertexData));
	_basicShader.setAttributeBuffer(colorLocation, GL_FLOAT, 3 * sizeof(float), 3, sizeof(VertexData));
	_basicShader.setUniformValue("u_color", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
	
	gizmoClear();

	if (take && timelineFrame >= 0 && timelineFrame < take->timeFrames)
	{
		if (showRays)
		{
			for (int i = 0; i < take->trackers.count(); ++i)
			{
				TakeTracker* t = take->trackers[i];

				if (timelineFrame < t->frameCount)
				{
					int localTrackerFrame = timelineFrame;

					for (int mIdx = 0; mIdx < t->vidFrameData[localTrackerFrame].newMarkers.size(); ++mIdx)
					{
						Marker2D* m = &t->vidFrameData[localTrackerFrame].newMarkers[mIdx];

						gizmoPush({ t->worldPos ,{ 1, 0, 1 } });
						gizmoPush({ t->worldPos + m->worldRayD * 6, { 1, 0, 1 } });

						//gizmoPush({ t->decoder->refWorldPos ,{ 0, 1, 1 } });
						//gizmoPush({ t->decoder->refWorldPos + m->refWorldRayD,{ 0, 1, 1 } });
					}
				}
			}
		}

		{
			for (int i = 0; i < take->markers[timelineFrame].size(); ++i)
			{
				Marker3D* m = &take->markers[timelineFrame][i];
				gizmoPush({ m->pos ,{ 1, 0, 0 } });
				gizmoPush({ m->pos + m->velocity,{ 1, 0, 0 } });

				gizmoPush({ m->pos ,{ 0, 1, 0 } });
				gizmoPush({ m->pos + m->bVelocity,{ 0, 1, 0 } });
			}
		}

		if (showMarkerSources)
		{
			for (int i = 0; i < take->markers[timelineFrame].size(); ++i)
			{
				Marker3D* m = &take->markers[timelineFrame][i];

				for (int t = 0; t < m->sources.size(); ++t)
				{
					QVector3D tWP = take->trackers[m->sources[t].trackerId]->worldPos;

					float length = (tWP - m->pos).length();
					
					gizmoPush({ tWP, { 0.3f, 0.3f, 0.3f } });
					gizmoPush({ tWP + m->sources[t].worldRayD * length, { 0.3f, 0.3f, 0.3f } });
				}

				/*
				QVector3D markerPos = (_pointWorldMat * QVector4D(m->pos.x(), m->pos.y(), m->pos.z(), 1.0f)).toVector3DAffine();
				QMatrix4x4 markerMat;
				markerMat.translate(markerPos);
				*/
			}
		}
	}

	// Pick ray.
	//gizmoPush({ _pickPos, { 1, 0, 0 } }); 
	//gizmoPush({ _pickPos + _pickDir * 10, { 1, 0, 0 } });

	if (selectWorldBasis)
	{
		for (int i = 0; i < 2; ++i)
		{
			gizmoPush({ _lastSelectedPos[i], { 0.5f, 0.5f, 0.5f } });
			gizmoPush({ _lastSelectedPos[i + 1], { 0.5f, 0.5f, 0.5f } });
		}
	}
	
	_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat);
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

			cv::Matx33d m33((double*)take->trackers[i]->camMatOpt.ptr());
			cv::Matx33d m33Inv = m33.inv();

			cv::Matx31d imgPt(0, 0, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			imgPt = cv::Matx31d(VID_W, 0, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			imgPt = cv::Matx31d(VID_W, VID_H, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			imgPt = cv::Matx31d(0, VID_H, 1);
			imgPt = m33Inv * imgPt;
			gizmoPush({ { 0, 0, 0 },sc });
			gizmoPush({ { (float)imgPt(0, 0) * fld, (float)imgPt(1, 0) * fld, (float)imgPt(2, 0) * fld }, ec });

			_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat * take->trackers[i]->worldMat);
			_gizmoBuffer.write(0, _gizmoData, _gizmoIndex * sizeof(VertexData));
			glDrawArrays(GL_LINES, 0, _gizmoIndex);
		}

		// Draw marker rays closest intersection line.
		if (markerFound)
		{
			gizmoClear();
			_basicShader.setUniformValue("mvp_matrix", _projMat * _camViewMat);

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

	//_drawText(0, 50, "Large font is large 1234567890 {}", QVector4D(1, 1, 1, 1), 1.0f, SVTA_LEFT);
	//_drawText(0, 60, "Marker trajectory test!", QVector4D(1, 0, 0, 1), 1.0f, SVTA_LEFT);

	if (take)
	{
		for (int i = 0; i < take->trackers.count(); ++i)
		{
			TakeTracker* t = take->trackers[i];

			QVector2D screenPos;
			if (_projectToScreen(t->worldPos + QVector3D(0, -0.1f, 0), screenPos))
			{
				_drawText(screenPos.x(), screenPos.y(), t->name, QVector4D(1, 1, 1, 1), 0.4f, SVTA_MIDDLE);
			}
		}
	}

	if (take)
	{
		if (take->isRecording)
		{
			_drawText(10, 40, "RECORDING " + take->name, QVector4D(0.5, 0.2, 0.2, 1), 1.0f, SVTA_LEFT);
		}
	}

	//--------------------------------------------------------------------------------------------------
	// Render Font Sprites.
	//--------------------------------------------------------------------------------------------------
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	
	if (_fontSpriteCount > 0)
	{
		int fontVertexLocation = _fontShader.attributeLocation("a_position");
		int fontUVsLocation = _fontShader.attributeLocation("a_uvs");
		int fontColorLocation = _fontShader.attributeLocation("a_color");
		_fontShader.bind();
		_fontShader.enableAttributeArray(fontVertexLocation);
		_fontShader.enableAttributeArray(fontUVsLocation);
		_fontShader.enableAttributeArray(fontColorLocation);
		_fontShader.setUniformValue("u_color", QVector4D(1, 1, 1, 1));
		_fontTexture->bind();

		_fontVertBuf.bind();
		_fontVertBuf.write(0, _fontVerts, _fontSpriteCount * 6 * sizeof(FontVertex));
		_fontShader.setAttributeBuffer(fontVertexLocation, GL_FLOAT, 0 * sizeof(float), 3, sizeof(FontVertex));
		_fontShader.setAttributeBuffer(fontUVsLocation, GL_FLOAT, 3 * sizeof(float), 2, sizeof(FontVertex));
		_fontShader.setAttributeBuffer(fontColorLocation, GL_FLOAT, 5 * sizeof(float), 4, sizeof(FontVertex));

		QMatrix4x4 pm;
		glDrawArrays(GL_TRIANGLES, 0, _fontSpriteCount * 6);

		_fontVertBuf.release();
	}

	glDisable(GL_BLEND);
}

bool SceneView::_projectToScreen(QVector3D Pos, QVector2D& ScreenPoint)
{
	QVector4D pos = _projMat * _camViewMat * QVector4D(Pos.x(), Pos.y(), Pos.z(), 1);
	ScreenPoint = (pos.toVector3DAffine().toVector2D() * 0.5f + QVector2D(0.5f, 0.5f)) * QVector2D(width(), height());
	ScreenPoint.setY(height() - ScreenPoint.y());

	return (pos.z() > 0);
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
	QVector4D rayClip(ndc.x(), ndc.y(), -1.0f, 1.0f);
	QVector4D rayEye = _projMat.inverted() * rayClip;
	rayEye.setZ(-1.0f);
	rayEye.setW(0.0);
	QVector3D rayWorld = (_camViewMat.inverted() * rayEye).toVector3D();
	rayWorld.normalize();
	
	QVector4D hcPos = (_camViewMat).inverted() * QVector4D(0, 0, 0, 1);
	_pickPos = QVector3D(hcPos.x(), hcPos.y(), hcPos.z());
	_pickDir = rayWorld;

	if (take)
	{
		int closestIdx = -1;
		float closestD = 10000.0f;

		if (timelineFrame >= 0 && timelineFrame <= take->timeEnd)
		{
			for (int i = 0; i < take->markers[timelineFrame].size(); ++i)
			{
				float d = take->markers[timelineFrame][i].pos.distanceToLine(_pickPos, _pickDir);

				if (d < 0.02f && d < closestD)
				{
					closestD = d;
					closestIdx = i;
				}
			}

			_selectedIdx = closestIdx;

			if (selectWorldBasis)
			{
				if (_selectedIdx != -1)
				{
					float scaledDistance = ((_lastSelectedPos[0] - take->markers[timelineFrame][_selectedIdx].pos).length());
					qDebug() << "Distance" << scaledDistance;

					for (int i = 1; i >= 0; --i)
					{
						_lastSelectedPos[i + 1] = _lastSelectedPos[i];
					}

					_lastSelectedPos[0] = take->markers[timelineFrame][_selectedIdx].pos;
				}

				//qDebug() << "0" << _lastSelectedPos[0];
				//qDebug() << "1" << _lastSelectedPos[1];
				//qDebug() << "2" << _lastSelectedPos[2];
			}
		}
	}
}

void SceneView::_drawText(int X, int Y, QString Text, QVector4D Color, float Scale, TextAlignment Alignment)
{
	float scale = Scale;
	float x = X;
	float y = -Y;
	float xOffset = 0.0f;

	if (Alignment == SVTA_RIGHT || Alignment == SVTA_MIDDLE)
	{
		for (int i = 0; i < Text.length(); ++i)
		{
			char c = Text[i].toLatin1();
			FontGlyph* g = &_fontGlyphs[c];
			xOffset -= g->advance * scale;
		}
	}

	if (Alignment == SVTA_MIDDLE)
	{
		xOffset *= 0.5f;
	}

	x += xOffset;
	
	for (int i = 0; i < Text.length(); ++i)
	{
		char c = Text[i].toLatin1();

		if (c == ' ')
		{
			x += 24 * scale;
			continue;
		}

		FontGlyph* g = &_fontGlyphs[c];

		QVector3D v0(x + (g->bounds.left() - 1) * scale, y - (g->bounds.top() + 1) * scale, 1);
		QVector3D v1(x + (g->bounds.left() - 1) * scale, y - (g->bounds.bottom() + 4) * scale, 1);
		QVector3D v2(x + (g->bounds.right() + 2) * scale, y - (g->bounds.bottom() + 4) * scale, 1);
		QVector3D v3(x + (g->bounds.right() + 2) * scale, y - (g->bounds.top() + 1) * scale, 1);

		v0 /= QVector2D(width(), height());
		v1 /= QVector2D(width(), height());
		v2 /= QVector2D(width(), height());
		v3 /= QVector2D(width(), height());

		FontVertex fv0 = { v0,{ g->uvX1, g->uvY1 }, Color };
		FontVertex fv1 = { v1,{ g->uvX1, g->uvY2 }, Color };
		FontVertex fv2 = { v2,{ g->uvX2, g->uvY2 }, Color };
		FontVertex fv3 = { v3,{ g->uvX2, g->uvY1 }, Color };

		_fontVerts[_fontSpriteCount * 6 + 0] = fv0;
		_fontVerts[_fontSpriteCount * 6 + 1] = fv1;
		_fontVerts[_fontSpriteCount * 6 + 2] = fv2;

		_fontVerts[_fontSpriteCount * 6 + 3] = fv0;
		_fontVerts[_fontSpriteCount * 6 + 4] = fv2;
		_fontVerts[_fontSpriteCount * 6 + 5] = fv3;

		x += g->advance * scale;
		++_fontSpriteCount;
	}
}