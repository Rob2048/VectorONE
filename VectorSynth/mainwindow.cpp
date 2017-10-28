#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cameraView.h"
#include "sceneView.h"
#include <QLabel>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
	_take(0),
	_drawMarkers(false)
{
	_deviceListMapper = new QSignalMapper(this);

    ui->setupUi(this);
	ui->centralWidget->hide();

	_mainTimer.start();

	//ui->dockLog->hide();
	//ui->dockSceneView->hide();
	
	//this->setTabPosition(Qt::DockWidgetArea::TopDockWidgetArea, QTabWidget::TabPosition::North);

	this->splitDockWidget(ui->dockSceneView, ui->dockTimeline, Qt::Orientation::Vertical);
	this->resizeDocks({ ui->dockSceneView, ui->dockTimeline }, { 100, 0 }, Qt::Orientation::Vertical);

	this->splitDockWidget(ui->dockSceneView, ui->dockTrackers, Qt::Orientation::Horizontal);

	//this->tabifyDockWidget(ui->dockProps, ui->dockRecord);
	this->tabifyDockWidget(ui->dockSynth, ui->dockProps);
	
	this->resizeDocks({ ui->dockProps, ui->dockTrackers }, { 30, 80 }, Qt::Orientation::Horizontal);
	//this->resizeDocks({ ui->dockProps, ui->dockTrackers }, { 30, 80 }, Qt::Orientation::Horizontal);
	//this->resizeDocks({ ui->dockTrackers, ui->dockSceneView }, { 100, 0 }, Qt::Orientation::Vertical);

	_glView = new SceneView(ui->sceneViewDockContents);
	ui->sceneViewDockContents->layout()->addWidget(_glView);
	_sceneViewTimer = new QTimer();
	_sceneViewTimer->start(1000.0f / 60.0f);
	connect(_sceneViewTimer, &QTimer::timeout, this, &MainWindow::OnSceneViewTimerTick);
	
	foreach(const QHostAddress &address, QNetworkInterface::allAddresses())
	{
		if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
		{
			qDebug() << "Address:" << address.toString();
		}
	}

	// NOTE: This doesn't always find the first ethernet adapter address.
	// Need to be sure to select correct network.
	foreach(const QHostAddress &address, QNetworkInterface::allAddresses()) 
	{	
		if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost))
		{
			_localIp = address;
			qDebug() << "Host Address:" << address.toString();
			break;
		}
	}

	_timeline = new TimelineWidget(this);
	ui->timelineLayout->addWidget(_timeline);
	ui->timelineLayout->setAlignment(Qt::AlignTop);

	_cameraView = new CameraView(this, this);
	_cameraView->trackers = &_liveTrackers;
	//ui->cameraViewPanel->layout()->removeWidget(ui->timeline);
	ui->cameraViewPanel->layout()->addWidget(_cameraView);
	//ui->cameraViewPanel->layout()->addWidget(ui->timeline);

	_udpSocket = new QUdpSocket(this);	
	
	_recvSocket = new QUdpSocket(this);
	_recvSocket->bind(45455);
	connect(_recvSocket, &QUdpSocket::readyRead, this, &MainWindow::OnBroadcastRead);

	_timer = new QTimer();
	_timer->start(1000);
	connect(_timer, &QTimer::timeout, this, &MainWindow::OnTimerTick);

	_timelineTimer = new QTimer();
	_timelineTimer->start(100);
	connect(_timelineTimer, &QTimer::timeout, this, &MainWindow::OnTimelineTimerTick);

	qDebug() << "Spawn Server Thread from" << QThread::currentThreadId();

	_serverWorker = new ServerThreadWorker();
	_serverWorker->moveToThread(&_serverThread);

	connect(_serverWorker, &ServerThreadWorker::OnTrackerConnected, this, &MainWindow::OnTrackerConnected);
	connect(_serverWorker, &ServerThreadWorker::OnTrackerDisconnected, this, &MainWindow::OnTrackerDisconnected);
	connect(_serverWorker, &ServerThreadWorker::OnTrackerFrame, this, &MainWindow::OnTrackerFrame);
	connect(_serverWorker, &ServerThreadWorker::OnTrackerMarkersFrame, this, &MainWindow::OnTrackerMarkersFrame);
	connect(_serverWorker, &ServerThreadWorker::OnTrackerInfoUpdate, this, &MainWindow::OnTrackerInfoUpdate);
	
	connect(this, &MainWindow::OnServerStart, _serverWorker, &ServerThreadWorker::OnStart);
	connect(this, &MainWindow::OnSendData, _serverWorker, &ServerThreadWorker::OnSendData);
	connect(this, &MainWindow::OnStartRecording, _serverWorker, &ServerThreadWorker::OnStartRecording);
	connect(this, &MainWindow::OnStartCalibrating, _serverWorker, &ServerThreadWorker::OnStartCalibrating);
	connect(this, &MainWindow::OnStopCalibrating, _serverWorker, &ServerThreadWorker::OnStopCalibrating);
	connect(this, &MainWindow::OnStartViewSteam, _serverWorker, &ServerThreadWorker::OnViewFeed);
	connect(this, &MainWindow::OnMaskChange, _serverWorker, &ServerThreadWorker::OnMaskChange);

	_serverThread.start();
	emit OnServerStart();
	
	// Buttons.
	connect(ui->btnStartCalibration, &QPushButton::clicked, this, &MainWindow::OnCalibrationStartClick);
	connect(ui->btnStopCalibration, &QPushButton::clicked, this, &MainWindow::OnCalibrationStopClick);
	connect(ui->btnStartRecording, &QPushButton::clicked, this, &MainWindow::OnStartRecordingClick);
	connect(ui->btnLoadTake, &QPushButton::clicked, this, &MainWindow::OnLoadTakeClick);
	connect(ui->btnSaveTake, &QPushButton::clicked, this, &MainWindow::OnSaveTakeClick);
	connect(ui->btnGenerateMask, &QPushButton::clicked, this, &MainWindow::OnGenerateMaskClicked);
	connect(ui->btnBuild2D, &QPushButton::clicked, this, &MainWindow::OnBuild2DMarkersClicked);
	connect(ui->btnBuildFundamental, &QPushButton::clicked, this, &MainWindow::OnBuildFundamentalMatClicked);
	connect(ui->btnBuild3D, &QPushButton::clicked, this, &MainWindow::OnBuild3DMarkersClicked);
	connect(ui->btnAssignWorldBasis, &QPushButton::clicked, this, &MainWindow::OnAssignWorldBasisClicked);
	
	// Toolbars.
	connect(ui->btnDrawVideo, &QToolButton::clicked, this, &MainWindow::OnDrawVideoClicked);
	connect(ui->btnDrawMarkers, &QToolButton::clicked, this, &MainWindow::OnDrawMarkersClicked);

	// Exposure //ui->horizontalSlider->setEnabled(false);
	// Frame Sync //ui->horizontalSlider_2->setEnabled(false);
	// Sensitivity //ui->horizontalSlider_3->setEnabled(false);
	ui->horizontalSlider_3->setMaximum(255);
	ui->horizontalSlider_3->setMinimum(0);
	connect(ui->horizontalSlider_3, &QSlider::valueChanged, _serverWorker, &ServerThreadWorker::OnCamSensitivityChange);

	ui->horizontalSlider_2->setMaximum(255);
	ui->horizontalSlider_2->setMinimum(0);
	connect(ui->horizontalSlider_2, &QSlider::valueChanged, _serverWorker, &ServerThreadWorker::OnCamThresholdChange);

	//ui->horizontalSlider->setMaximum(255);
	//ui->horizontalSlider->setMinimum(0);
	//connect(ui->horizontalSlider, &QSlider::valueChanged, _serverWorker, &ServerWorker::OnCamDistortChange);

	ui->txtExposure->setValidator(new QIntValidator(1000, 23000, this));
	connect(ui->txtExposure, &QLineEdit::editingFinished, this, &MainWindow::OnExposureEditingFinished);

	ui->txtIso->setValidator(new QIntValidator(0, 1000, this));
	connect(ui->txtIso, &QLineEdit::editingFinished, this, &MainWindow::OnIsoEditingFinished);

	ui->txtFrameSkip->setValidator(new QIntValidator(0, 100, this));
	connect(ui->txtFrameSkip, &QLineEdit::editingFinished, _serverWorker, &ServerThreadWorker::OnCamFrameSkipChanged);

	ui->txtFps->setValidator(new QIntValidator(40, 120, this));
	connect(ui->txtFps, &QLineEdit::editingFinished, this, &MainWindow::OnFpsEditingFinished);

	//connect(ui->chkDrawGuides, &QCheckBox::stateChanged, _serverWorker, &ServerWorker::OnDrawGuidesChanged);
	//connect(ui->chkMarkers, &QCheckBox::stateChanged, _serverWorker, &ServerWorker::OnDrawMarkersChanged);
	connect(ui->chkFindCalib, &QCheckBox::stateChanged, _serverWorker, &ServerThreadWorker::OnFindCalibChanged);

	// Device Table
	/*
	ui->tblDevices->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	ui->tblDevices->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
	ui->tblDevices->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);
	ui->tblDevices->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeMode::ResizeToContents);
	ui->tblDevices->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
	ui->tblDevices->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);

	connect(_deviceListMapper, SIGNAL(mapped(int)), _serverWorker, SLOT(OnViewFeed(int)));
	*/

	// Take Device Table
	/*
	ui->tblTakeDevices->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	ui->tblTakeDevices->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
	ui->tblTakeDevices->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
	ui->tblTakeDevices->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);
	ui->tblTakeDevices->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeMode::ResizeToContents);
	//ui->tblTakeDevices->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeMode::Fixed);
	//ui->tblDevices->horizontalHeader()->setwi
	ui->tblTakeDevices->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
	ui->tblTakeDevices->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
	connect(ui->tblTakeDevices, &QTableWidget::itemSelectionChanged, this, &MainWindow::OnTakeTrackerTableSelectionChanged);
	*/

	ui->sldTakeSensitivity->setMaximum(255);
	ui->sldTakeSensitivity->setMinimum(0);
	connect(ui->sldTakeSensitivity, &QSlider::valueChanged, this, &MainWindow::OnTakeSensitivityChange);

	ui->sldTakeThreshold->setMaximum(255);
	ui->sldTakeThreshold->setMinimum(0);
	connect(ui->sldTakeThreshold, &QSlider::valueChanged, this, &MainWindow::OnTakeThresholdChange);
	
	ui->txtTakeOffset->setValidator(new QIntValidator(-50, 50, this));
	connect(ui->txtTakeOffset, &QLineEdit::editingFinished, this, &MainWindow::OnTakeOffsetEditingFinished);

	// Timeline
	connect(_timeline, &TimelineWidget::valueChanged, this, &MainWindow::OnTimelineChange);
	connect(ui->btnNextFrame, &QPushButton::clicked, this, &MainWindow::OnNextFrameClick);
	connect(ui->btnNextFrameJump, &QPushButton::clicked, this, &MainWindow::OnNextFrameJumpClick);
	connect(ui->btnPrevFrame, &QPushButton::clicked, this, &MainWindow::OnPrevFrameClick);
	connect(ui->btnPrevFrameJump, &QPushButton::clicked, this, &MainWindow::OnPrevFrameJumpClick);
	connect(ui->btnPlay, &QPushButton::clicked, this, &MainWindow::OnPlayClick);

	_playTimer = new QTimer();
	connect(_playTimer, &QTimer::timeout, this, &MainWindow::OnPlayTimerTick);
	
	ui->txtPlaySpeed->setValidator(new QDoubleValidator(0.01, 9.0, 2, this));

	/*
	ui->scrollArea->setWidgetResizable(true);

	ui->scrollArea->setLayout(new QVBoxLayout());

	QFrame inner = QFrame(ui->scrollArea);
	inner.setLayout(QVBoxLayout())

	scroll.setWidget(inner) # CRITICAL

	for i in range(40) :
		b = QPushButton(inner)
		b.setText(str(i))
		inner.layout().addWidget(b)
		*/
}

MainWindow::~MainWindow()
{
	_serverThread.quit();
	_serverThread.wait();

	delete ui;
	delete _serverWorker;
}

void MainWindow::OnTimelineChange(int Value)
{
	_timelineRequestedFrame = Value;

	int fps = 50;
	int min = Value / fps / 60;
	int sec = (Value / fps) % 60;
	int frame = Value % fps;

	char buff[64];
	sprintf(buff, "%d:%02d:%03d", min, sec, frame);
	ui->lblTimecode->setText(buff);
}

void MainWindow::OnNextFrameClick()
{
	_timeline->adjustSelectedFrame(1);
}

void MainWindow::OnNextFrameJumpClick()
{
	_timeline->adjustSelectedFrame(20);
}

void MainWindow::OnPrevFrameClick()
{
	_timeline->adjustSelectedFrame(-1);
}

void MainWindow::OnPrevFrameJumpClick()
{
	_timeline->adjustSelectedFrame(-20);
}

void MainWindow::OnPlayClick()
{	
	if (_playTimer->isActive())
	{
		_playTimer->stop();
	}
	else
	{
		_playTimer->stop();
		_playFrame = _timeline->selectedFrame;
		_playTimer->start(1000.0f / 60.0f);
	}
}

void MainWindow::OnPlayTimerTick()
{	
	float frameAdvance = (100.0f / 60.0f) * ui->txtPlaySpeed->text().toFloat();
	_playFrame += frameAdvance;

	if ((int)_playFrame >= _timeline->totalFrames)
	{
		_playTimer->stop();
	}
	else
	{
		_timeline->setFrame(_playFrame);
	}
}

void MainWindow::OnTimelineTimerTick()
{
	if (_timelineCurrentFrame != _timelineRequestedFrame)
	{
		if (_take)
		{
			_take->SetFrame(_timelineRequestedFrame, _drawMarkers);
			
			_cameraView->timelineFrame = _timelineRequestedFrame;
			_glView->timelineFrame = _timelineRequestedFrame;
			_cameraView->take = _take;

			/*
			if (_drawMarkers)
			{	
				_cameraView->mode = 2;
			}
			else
			{
				if (_take->trackers.count() == 2)
				{
					//_cameraView->camImageA = QImage((uchar*)_take->trackers[0]->decoder->GetFrameMatData(), VID_W, VID_H, QImage::Format::Format_RGB888).copy();
					//_cameraView->camNameA = _take->trackers[0]->name;
					//_cameraView->camImageB = QImage((uchar*)_take->trackers[1]->decoder->GetFrameMatData(), VID_W, VID_H, QImage::Format::Format_RGB888).copy();
					//_cameraView->camNameB = _take->trackers[1]->name;
					_cameraView->mode = 1;
				}
				else
				{
					//_cameraView->camImage = QImage((uchar*)_take->trackers[0]->decoder->GetFrameMatData(), VID_W, VID_H, QImage::Format::Format_RGB888).copy();
					_cameraView->mode = 0;
				}
			}
			*/
			
			_cameraView->update();

			_timelineCurrentFrame = _timelineRequestedFrame;
		}
	}
}	

void MainWindow::changeMask(LiveTracker* Tracker)
{
	QByteArray data((char*)Tracker->maskData, sizeof(Tracker->maskData));

	emit OnMaskChange(Tracker->id, data);
}

void MainWindow::OnFpsEditingFinished()
{
	int value = ui->txtFps->text().toInt();
	QString cmd = QString("pf,") + QString::number(value) + QString("\n");
	QByteArray test(cmd.toLatin1());
	emit OnSendData(1, test);
}

void MainWindow::OnExposureEditingFinished()
{
	int value = ui->txtExposure->text().toInt();
	QString cmd = QString("pe,") + QString::number(value) + QString("\n");
	QByteArray test(cmd.toLatin1());
	emit OnSendData(1, test);
}

void MainWindow::OnIsoEditingFinished()
{
	int value = ui->txtIso->text().toInt();
	QString cmd = QString("pi,") + QString::number(value) + QString("\n");
	QByteArray test(cmd.toLatin1());
	emit OnSendData(1, test);
}

void MainWindow::OnCamSensitivityChange(int Value)
{
	qDebug() << "Cam Sensitivity:" << Value;
}

void MainWindow::OnTrackerConnected(int TrackerId)
{	
	TrackerConnection* tracker = _serverWorker->LockConnection(TrackerId);

	if (!tracker)
		return;

	LiveTracker* live = new LiveTracker();
	live->id = tracker->id;
	_liveTrackers[live->id] = live;

	_serverWorker->UnlockConnection(tracker);

	_cameraView->update();
}

void MainWindow::OnTrackerDisconnected(int TrackerId)
{
	delete _liveTrackers[TrackerId];
	_liveTrackers.erase(TrackerId);
	_cameraView->update();
}

void MainWindow::OnTrackerFrame(int TrackerId)
{
	TrackerConnection* tracker = _serverWorker->LockConnection(TrackerId);

	if (!tracker)
		return;

	LiveTracker* live = _liveTrackers[tracker->id];

	memcpy(live->frameData, tracker->postFrameData, VID_W * VID_H * 3);
	live->frames += tracker->decoder->newFrames;
	tracker->decoder->newFrames = 0;
	live->data += tracker->decoder->dataRecvBytes;
	tracker->decoder->dataRecvBytes = 0;
	live->avgMasterOffset = tracker->avgMasterOffset;
	live->latestFrameId = tracker->latestFrameId;

	_serverWorker->UnlockConnection(tracker);

	_cameraView->update();
}
void MainWindow::OnTrackerInfoUpdate(int TrackerId)
{
	TrackerConnection* tracker = _serverWorker->LockConnection(TrackerId);

	if (!tracker)
		return;

	LiveTracker* live = _liveTrackers[tracker->id];
	live->connected = tracker->accepted;
	live->serial = tracker->serial;
	live->version = tracker->version;
	live->name = tracker->name;
	live->setMask(tracker->maskData);

	_serverWorker->UnlockConnection(tracker);

	_cameraView->update();
}

void MainWindow::OnTrackerMarkersFrame(int TrackerId)
{
	TrackerConnection* tracker = _serverWorker->LockConnection(TrackerId);

	if (!tracker)
		return;

	LiveTracker* live = _liveTrackers[tracker->id];
	memcpy(live->markerData, tracker->markerData, tracker->markerDataSize);
	live->markerDataSize = tracker->markerDataSize;
	live->frames += tracker->decoder->newFrames;
	tracker->decoder->newFrames = 0;
	live->data += tracker->decoder->dataRecvBytes;
	tracker->decoder->dataRecvBytes = 0;
	live->avgMasterOffset = tracker->avgMasterOffset;
	live->latestFrameId = tracker->latestFrameId;
	
	_serverWorker->UnlockConnection(tracker);

	_cameraView->update();
}

void MainWindow::OnStartRecordingClick()
{
	emit OnStartRecording();
}

void MainWindow::OnCalibrationStartClick()
{
	// Selected Tracker
	emit OnStartCalibrating(1);
}

void MainWindow::OnCalibrationStopClick()
{
	emit OnStopCalibrating();
	
	/*
	TrackerConnection* tracker = _serverWorker->_GetTracker(1);

	qDebug() << "Calibration Image Count:" << tracker->decoder->_calibImageCount;

	// TODO: Wait for calibration frames to finish pouring in.
	vector<vector<cv::Point3f>> objectPoints(1);

	objectPoints[0].clear();

	// Chessboard/Circles Grid
	//for (int i = 0; i < _calibBoardSize.height; ++i)
	//	for (int j = 0; j < _calibBoardSize.width; ++j)
	//		objectPoints[0].push_back(Point3f(float(j*_calibSquareSize), float(i*_calibSquareSize), 0));

	float squareSize = 34.2f * 0.5f;

	// Asymmetric Circles Grid
	for (int i = 0; i < 11; i++)
		for (int j = 0; j < 4; j++)
			objectPoints[0].push_back(cv::Point3f(float((2 * j + i % 2)*squareSize), float(i*squareSize), 0));

	objectPoints.resize(tracker->decoder->_calibImagePoints.size(), objectPoints[0]);

	cv::Size			imageSize(VID_W, VID_H);
	cv::Mat				_calibCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
	cv::Mat				_calibDistCoeffs = cv::Mat::zeros(8, 1, CV_64F);
	vector<cv::Mat>		_calibRvecs;
	vector<cv::Mat>		_calivTvecs;

	double rms = calibrateCamera(objectPoints, tracker->decoder->_calibImagePoints, imageSize, _calibCameraMatrix, _calibDistCoeffs, _calibRvecs, _calivTvecs, CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5); // CV_CALIB_FIX_PRINCIPAL_POINT
	bool ok = checkRange(_calibCameraMatrix) && checkRange(_calibDistCoeffs);

	qDebug() << "Re-projection error reported by calibrateCamera: " << rms << " " << ok;

	//std:vector<float> perViewErrors;
	//double reprojError = computeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, cameraMatrix, distCoeffs, perViewErrors);
	//cout << "Reprojection Error: " << reprojError << "\n";

	double fovX, fovY, focalLength, aspectRatio;
	cv::Point2d principalPoint;

	// 1640 × 1232: 3.674 x 2.760	
	calibrationMatrixValues(_calibCameraMatrix, imageSize, 3.674, 2.066, fovX, fovY, focalLength, principalPoint, aspectRatio);

	qDebug() << "Cam " << fovX << " " << fovY << " " << focalLength << " " << aspectRatio << " " << principalPoint.x << "," << principalPoint.y;

	std::stringstream ss;
	ss << "Dist Coefs: " << _calibDistCoeffs << "\n\nCalib Mat: " << _calibCameraMatrix;
	qDebug() << ss.str().c_str() << _calibDistCoeffs.size().width << "x" << _calibDistCoeffs.size().height;

	tracker->decoder->_calibCameraMatrix = _calibCameraMatrix.clone();
	tracker->decoder->_calibDistCoeffs = _calibDistCoeffs.clone();
	tracker->decoder->CalculateOptMat();

	ui->lblCalibImageCount->setText(QString("Images: ") + QString::number(tracker->decoder->_calibImageCount));
	ui->lblCalibError->setText(QString("Error: ") + QString::number(rms));
	*/
}

void MainWindow::OnTimerTick()
{	
	QByteArray broadcastMsg = (QString("KineticSynth:") + _localIp.toString()).toUtf8();
	_udpSocket->writeDatagram(broadcastMsg.data(), broadcastMsg.size(), QHostAddress::Broadcast, 45454);
	//qDebug() << broadcastMsg;

	for (std::map<int, LiveTracker*>::iterator it = _liveTrackers.begin(); it != _liveTrackers.end(); ++it)
	{
		it->second->updateStats();
	}
}

void MainWindow::OnSceneViewTimerTick()
{
	_glView->tick();
}

void MainWindow::OnLoadTakeClick()
{
	if (_take)
		delete _take;

	_take = new Take();
	_take->LoadTake("take");
	_glView->take = _take;

	_timeline->setParams(_take->timeFrames - 2);
	_timelineCurrentFrame = -1;
	_timelineRequestedFrame = 0;

	_cameraView->trackers = &_take->liveTrackers;
}

void MainWindow::OnSaveTakeClick()
{
	if (!_take)
		return;

	_take->Save();
}

void MainWindow::OnTakeOffsetEditingFinished()
{
	if (_selectedTakeTracker)
	{
		int value = ui->txtTakeOffset->text().toInt();
		_selectedTakeTracker->frameOffset = value;
		_timelineCurrentFrame = -1;
	}
}

void MainWindow::OnTakeSensitivityChange(int Value)
{
	if (_selectedTakeTracker)
	{
		_selectedTakeTracker->sensitivity = (float)Value / 255.0f;
		_selectedTakeTracker->decoder->camSensitivity = (float)Value / 255.0f;
		_timelineCurrentFrame = -1;
	}
}

void MainWindow::OnTakeThresholdChange(int Value)
{
	if (_selectedTakeTracker)
	{
		_selectedTakeTracker->threshold = (float)Value / 255.0f;
		_selectedTakeTracker->decoder->camThreshold = (float)Value / 255.0f;
		_timelineCurrentFrame = -1;
	}
}

void MainWindow::OnGenerateMaskClicked()
{
	LiveTracker* tracker = GetTracker(_selectedTracker);

	if (tracker)
	{
		tracker->generateMask();
		changeMask(tracker);
	}
}

void MainWindow::OnBuild2DMarkersClicked()
{
	if (_take)
	{
		_take->Build2DMarkers(_timeline->rangeStartFrame, _timeline->rangeEndFrame);
	}
}

void MainWindow::OnBuild3DMarkersClicked()
{
	if (_take)
	{
		_glView->restartPattern();
		_take->Build3DMarkers(_timeline->rangeStartFrame, _timeline->rangeEndFrame);

		/*
		for (int i = 0; i < _take->markers.size(); ++i)
		{
			for (int j = 0; j < _take->markers[i].size(); ++j)
			{
				_glView->pushSamplePoint(_take->markers[i][j].pos);
			}
		}
		*/
	}
}

void MainWindow::OnAssignWorldBasisClicked()
{
	if (_take)
	{
		QVector3D n = _glView->_lastSelectedPos[0] - _glView->_lastSelectedPos[1];
		QVector3D t = _glView->_lastSelectedPos[2] - _glView->_lastSelectedPos[1];

		//float scaleFactor = ((1.470f / n.length()) + (0.805f / t.length())) * 0.5f;
		float scaleFactor = ((0.755 / n.length()) + (0.755f / t.length())) * 0.5f;

		qDebug() << "Scale" << scaleFactor;

		n.normalize();
		t.normalize();

		QVector3D proj = n * QVector3D::dotProduct(t, n);
		t = t - proj;
		t.normalize();
		QVector3D bt = QVector3D::crossProduct(n, t);

		_take->wX = n;
		_take->wY = bt;
		_take->wZ = -t;

		QVector3D p = _glView->_lastSelectedPos[1];
		_take->wT = QVector3D(-p.x(), -p.y(), -p.z());
		_take->wScale = scaleFactor;
	}
}

void MainWindow::OnDrawMarkersClicked()
{
	_drawMarkers = true;
	_timelineCurrentFrame = -1;
	_timelineTimer->setInterval(1000 / 60);
}

void MainWindow::OnDrawVideoClicked()
{
	_drawMarkers = false;
	_timelineCurrentFrame = -1;
	_timelineTimer->setInterval(1000 / 10);
}

void MainWindow::OnBuildFundamentalMatClicked()
{
	if (_take)
	{
		_take->BuildExtrinsics(_timeline->rangeStartFrame, _timeline->rangeEndFrame);
	}
}

void MainWindow::viewFeed(int TrackedId, bool Image)
{
	emit OnStartViewSteam(TrackedId, Image);
}

void MainWindow::selectTracker(LiveTracker* Tracker)
{
	_selectedTracker = Tracker->id;

	ui->txtId->setText(Tracker->name);
	ui->txtExposure->setText(QString::number(Tracker->exposure));
	ui->txtIso->setText(QString::number(Tracker->iso));
	ui->txtFps->setText(QString::number(Tracker->targetFps));
}

LiveTracker* MainWindow::GetTracker(int TrackerId)
{
	if (_liveTrackers.find(TrackerId) ==_liveTrackers.end())
	{
		return 0;
	}

	return _liveTrackers[TrackerId];
}

void MainWindow::OnBroadcastRead()
{
	static double prevTime = 0;

	QNetworkDatagram packet = _recvSocket->receiveDatagram();

	QString time = QString(packet.data());
	double ts = (_mainTimer.nsecsElapsed() / 1000) / 1000.0f;

	double td = ts - prevTime;
	prevTime = ts;

	qDebug() << "Bcast " << td << QHostAddress(packet.senderAddress().toIPv4Address()).toString() << time;
}