#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "cameraView.h"
#include "sceneView.h"
#include <QLabel>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
	_take(0)
{	
	_deviceListMapper = new QSignalMapper(this);

    ui->setupUi(this);
	ui->centralWidget->hide();

	_mainTimer.start();

	ui->dockExtra->hide();
	//ui->dockSceneView->hide();

	//ui->dockTimeline->setTitleBarWidget(new QWidget());
	
	//this->setTabPosition(Qt::DockWidgetArea::TopDockWidgetArea, QTabWidget::TabPosition::North);

	this->splitDockWidget(ui->dockSceneView, ui->dockTimeline, Qt::Orientation::Vertical);
	this->resizeDocks({ ui->dockSceneView, ui->dockTimeline }, { 100, 0 }, Qt::Orientation::Vertical);

	this->splitDockWidget(ui->dockSceneView, ui->dockTrackers, Qt::Orientation::Horizontal);

	//this->tabifyDockWidget(ui->dockProps, ui->dockRecord);
	this->tabifyDockWidget(ui->dockSynth, ui->dockProps);
	
	this->resizeDocks({ ui->dockProps, ui->dockTrackers }, { 30, 80 }, Qt::Orientation::Horizontal);
	//this->resizeDocks({ ui->dockProps, ui->dockTrackers }, { 30, 80 }, Qt::Orientation::Horizontal);
	//this->resizeDocks({ ui->dockTrackers, ui->dockSceneView }, { 100, 0 }, Qt::Orientation::Vertical);

	_liveTake = new LiveTake();
	_loadedTake = 0;
	_take = _liveTake;
	_recording = false;

	_glView = new SceneView(ui->sceneViewDockContents);
	ui->sceneViewDockContents->layout()->addWidget(_glView);
	_sceneViewTimer = new QTimer();
	_sceneViewTimer->start(1000.0f / 60.0f);
	connect(_sceneViewTimer, &QTimer::timeout, this, &MainWindow::OnSceneViewTimerTick);

	_glView->take = _take;
	
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
	_timelineTimer->start(1000 / 60);
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
	connect(this, &MainWindow::OnUpdateParams, _serverWorker, &ServerThreadWorker::OnUpdateTracker);
	connect(this, &MainWindow::OnResetFrameIds, _serverWorker, &ServerThreadWorker::OnResetFrameIds);
	connect(this, &MainWindow::OnStartRecording, _serverWorker, &ServerThreadWorker::OnStartRecording);
	connect(this, &MainWindow::OnStopRecording, _serverWorker, &ServerThreadWorker::OnStopRecording);
	connect(this, &MainWindow::OnStartCalibrating, _serverWorker, &ServerThreadWorker::OnStartCalibrating);
	connect(this, &MainWindow::OnStopCalibrating, _serverWorker, &ServerThreadWorker::OnStopCalibrating);
	connect(this, &MainWindow::OnStartViewSteam, _serverWorker, &ServerThreadWorker::OnViewFeed);
	connect(this, &MainWindow::OnMaskChange, _serverWorker, &ServerThreadWorker::OnMaskChange);

	_serverThread.start();
	emit OnServerStart();
	
	// Buttons.
	connect(ui->btnStartCalibration, &QPushButton::clicked, this, &MainWindow::OnCalibrationStartClick);
	connect(ui->btnStopCalibration, &QPushButton::clicked, this, &MainWindow::OnCalibrationStopClick);
	connect(ui->btnResetFrameIds, &QPushButton::clicked, this, &MainWindow::OnResetFrameIds);
	connect(ui->btnStartRecording, &QPushButton::clicked, this, &MainWindow::OnStartRecordingClick);
	connect(ui->btnLoadTake, &QPushButton::clicked, this, &MainWindow::OnLoadTakeClick);
	connect(ui->btnSaveTake, &QPushButton::clicked, this, &MainWindow::OnSaveTakeClick);
	connect(ui->btnBuildFundamental, &QPushButton::clicked, this, &MainWindow::OnBuildFundamentalMatClicked);
	connect(ui->btnBundleAdjust, &QPushButton::clicked, this, &MainWindow::OnBundleAdjustClicked);
	connect(ui->btnBuild3D, &QPushButton::clicked, this, &MainWindow::OnBuild3DMarkersClicked);
	connect(ui->btnAutoLabel, &QPushButton::clicked, this, &MainWindow::OnAutoLabelClicked);
	connect(ui->btnSelectWorldBasis, &QPushButton::clicked, this, &MainWindow::OnSelectWorldBasisClicked);
	connect(ui->btnAssignWorldBasis, &QPushButton::clicked, this, &MainWindow::OnAssignWorldBasisClicked);
	connect(ui->btnPushToLive, &QPushButton::clicked, this, &MainWindow::OnPushToLiveClicked);
	
	// Tracker view buttons.
	connect(ui->btnToggleUpdate, &QToolButton::clicked, this, &MainWindow::OnToggleUpdateClicked);
	connect(ui->btnToggleMask, &QToolButton::clicked, this, &MainWindow::OnToggleMaskClicked);
	connect(ui->btnTogglePixelGrid, &QToolButton::clicked, this, &MainWindow::OnTogglePixelGridClicked);
	connect(ui->btnToggleDistortedMarkers, &QToolButton::clicked, this, &MainWindow::OnToggleDistortedMarkersClicked);
	connect(ui->btnToggleUndistortedMarkers, &QToolButton::clicked, this, &MainWindow::OnToggleUndistortedMarkersClicked);
	connect(ui->btnToggleReprojectedMarkers, &QToolButton::clicked, this, &MainWindow::OnToggleReprojectedMarkersClicked);

	// Scene view buttons.
	connect(ui->btnToggleMarkerSources, &QToolButton::clicked, this, &MainWindow::OnToggleMarkerSourcesClicked);
	connect(ui->btnToggleRays, &QToolButton::clicked, this, &MainWindow::OnToggleRaysClicked);
	connect(ui->btnToggleExpandedMarkers, &QToolButton::clicked, this, &MainWindow::OnToggleExpandedMarkersClicked);
	connect(ui->btnShowLiveToolbar, &QToolButton::clicked, this, &MainWindow::OnShowLiveClicked);

	ui->txtExposure->setValidator(new QIntValidator(1000, 23000, this));
	connect(ui->txtExposure, &QLineEdit::editingFinished, this, &MainWindow::OnExposureEditingFinished);

	ui->txtIso->setValidator(new QIntValidator(0, 1000, this));
	connect(ui->txtIso, &QLineEdit::editingFinished, this, &MainWindow::OnIsoEditingFinished);

	ui->txtFps->setValidator(new QIntValidator(40, 120, this));
	connect(ui->txtFps, &QLineEdit::editingFinished, this, &MainWindow::OnFpsEditingFinished);

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

	updateTakeList();
}

MainWindow::~MainWindow()
{
	_serverThread.quit();
	_serverThread.wait();

	delete ui;
	//delete _serverWorker;
}

void MainWindow::OnTimelineChange(int Value)
{
	_timelineRequestedFrame = Value;

	/*
	int fps = 50;
	int min = Value / fps / 60;
	int sec = (Value / fps) % 60;
	int frame = Value % fps;

	char buff[64];
	sprintf(buff, "%d:%02d:%03d", min, sec, frame);
	ui->lblTimecode->setText(buff);
	*/

	if (_take)
	{
		double totalSeconds = (double)(Value * _take->frameDuration) / 1000000.0;
		float fps = 1000000.0 / _take->frameDuration;

		int min = totalSeconds / 60;
		int sec = totalSeconds - min * 60;
		int msec = (int)(totalSeconds * 1000) % 1000;

		char buff[64];
		sprintf(buff, "%d:%02d:%03d (%d) %.2f FPS", min, sec, msec, Value, fps);
		ui->lblTimecode->setText(buff);
	}
	else
	{
		ui->lblTimecode->setText("(No take)");
	}
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
	if (_take)
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
}

void MainWindow::OnPlayTimerTick()
{	
	float frameAdvance = ((1000000.0 / _take->frameDuration) / 60.0) * ui->txtPlaySpeed->text().toFloat();
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
	if (_take && _timelineCurrentFrame != _timelineRequestedFrame)
	{
		_take->SetFrame(_timelineRequestedFrame);
			
		_cameraView->timelineFrame = _timelineRequestedFrame;
		_glView->timelineFrame = _timelineRequestedFrame;
		_cameraView->take = _take;
		_cameraView->update();

		_timelineCurrentFrame = _timelineRequestedFrame;
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
	emit OnSendData(-1, test);
}

void MainWindow::OnExposureEditingFinished()
{
	int value = ui->txtExposure->text().toInt();
	QString cmd = QString("pe,") + QString::number(value) + QString("\n");
	QByteArray test(cmd.toLatin1());
	emit OnSendData(-1, test);
}

void MainWindow::OnIsoEditingFinished()
{
	int value = ui->txtIso->text().toInt();
	QString cmd = QString("pi,") + QString::number(value) + QString("\n");
	QByteArray test(cmd.toLatin1());
	emit OnSendData(-1, test);
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
	_liveTake->liveTrackers[live->id] = live;

	_serverWorker->UnlockConnection(tracker);

	_cameraView->update();
}

void MainWindow::OnTrackerDisconnected(int TrackerId)
{
	delete _liveTake->liveTrackers[TrackerId];
	_liveTake->liveTrackers.erase(TrackerId);
	_cameraView->update();
}

void MainWindow::OnTrackerFrame(int TrackerId)
{
	TrackerConnection* tracker = _serverWorker->LockConnection(TrackerId);

	if (!tracker)
		return;

	LiveTracker* live = _liveTake->liveTrackers[tracker->id];

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

	LiveTracker* live = _liveTake->liveTrackers[tracker->id];
	live->connected = tracker->accepted;
	live->serial = tracker->serial;
	live->version = tracker->version;
	live->name = tracker->props.name;
	live->setMask(tracker->props.maskData);

	_serverWorker->UnlockConnection(tracker);

	_cameraView->update();
}

void MainWindow::OnTrackerMarkersFrame(int TrackerId)
{
	TrackerConnection* tracker = _serverWorker->LockConnection(TrackerId);

	if (!tracker)
		return;

	LiveTracker* live = _liveTake->liveTrackers[tracker->id];
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
	if (!_recording)
	{
		_take = _liveTake;
		_cameraView->take = _take;
		_glView->take = _take;
		_cameraView->update();

		ui->btnStartRecording->setText("Stop Recording");
		_recording = true;
		_take->isRecording = true;		

		// Find new take name
		QDir dir("project");
		QStringList takes = dir.entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot);
		QString takeName = "";
		int takeNameSuffix = 0;

		while (true)
		{
			bool found = false;
			takeName = "take_" + QString::number(takeNameSuffix++);

			for (int i = 0; i < takes.size(); ++i)
			{
				if (takes[i] == takeName)
				{
					found = true;
					break;
				}
			}

			if (!found)
				break;
		}

		qDebug() << "Saving new take" << takeName;
		QDir().mkdir("project/" + takeName);

		_take->name = takeName;

		// NOTE: This code is duped from Take::Save().
		QJsonObject jsonObj;
		jsonObj["fps"] = 19941;

		QJsonDocument jsonDoc(jsonObj);
		QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Indented);

		QFile file("project/" + takeName + "/" + takeName + ".take");

		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			qDebug() << "Take: Save file failed";
			return;
		}

		file.write(jsonBytes);
		file.close();
		
		emit OnStartRecording(takeName);
	}
	else
	{
		ui->btnStartRecording->setText("Start Recording");
		_recording = false;
		_take->isRecording = false;
		emit OnStopRecording();
		updateTakeList();
	}
}

void MainWindow::OnResetFrameIdsClick()
{
	emit OnResetFrameIds();
}

void MainWindow::OnCalibrationStartClick()
{
	// Selected Tracker
	emit OnStartCalibrating(1);
}

void MainWindow::OnCalibrationStopClick()
{
	emit OnStopCalibrating();
}

void MainWindow::OnTimerTick()
{	
	QByteArray broadcastMsg = (QString("KineticSynth:") + _localIp.toString()).toUtf8();
	// TODO: Sometimes the 255.255.255.255 broadcast address fails.
	//_udpSocket->writeDatagram(broadcastMsg.data(), broadcastMsg.size(), QHostAddress::Broadcast, 45454);
	_udpSocket->writeDatagram(broadcastMsg.data(), broadcastMsg.size(), QHostAddress("192.168.1.255"), 45454);
	//qDebug() << broadcastMsg;

	//QHostAddress t(QHostAddress::Broadcast);
	//qDebug() << t.toString();

	for (std::map<int, LiveTracker*>::iterator it = _liveTake->liveTrackers.begin(); it != _liveTake->liveTrackers.end(); ++it)
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
	if (ui->lstTakes->currentItem() == 0)
	{
		QMessageBox msgBox(QMessageBox::Icon::Warning, "No take selected.", "You need to select a take to load.");
		msgBox.exec();
		return;
	}

	QString takeName = ui->lstTakes->currentItem()->text();

	if (_loadedTake)
		delete _loadedTake;

	_loadedTake = new LoadedTake();
	_loadedTake->LoadTake(takeName);
	_glView->take = _loadedTake;
	_cameraView->take = _loadedTake;
	_take = _loadedTake;
	_timeline->setParams(_take->timeFrames - 2);
	_timelineCurrentFrame = -1;
	_timelineRequestedFrame = 0;
}

void MainWindow::OnSaveTakeClick()
{
	if (!_take)
		return;

	_take->Save();
}

void MainWindow::OnBuild3DMarkersClicked()
{
	if (_take)
	{
		_take->Build3DMarkers(_timeline->rangeStartFrame, _timeline->rangeEndFrame);
	}
}

void MainWindow::OnAutoLabelClicked()
{
	if (_take)
	{
		_take->BuildLabels(_timeline->rangeStartFrame, _timeline->rangeEndFrame);
	}
}

void MainWindow::OnSelectWorldBasisClicked()
{
	_glView->selectWorldBasis = !_glView->selectWorldBasis;
	_glView->_lastSelectedPos[0] = QVector3D(0, 0, 0);
	_glView->_lastSelectedPos[1] = QVector3D(0, 0, 0);
	_glView->_lastSelectedPos[2] = QVector3D(0, 0, 0);
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

		QVector3D wX = n;
		QVector3D wY = bt;
		QVector3D wZ = -t;

		QVector3D p = _glView->_lastSelectedPos[1];
		QVector3D wT = QVector3D(-p.x(), -p.y(), -p.z());
		
		// Apply mat to all poses
		QMatrix4x4 _pointWorldMat;
		_pointWorldMat(0, 0) = wX.x();
		_pointWorldMat(0, 1) = wX.y();
		_pointWorldMat(0, 2) = wX.z();

		_pointWorldMat(1, 0) = wY.x();
		_pointWorldMat(1, 1) = wY.y();
		_pointWorldMat(1, 2) = wY.z();

		_pointWorldMat(2, 0) = wZ.x();
		_pointWorldMat(2, 1) = wZ.y();
		_pointWorldMat(2, 2) = wZ.z();

		QMatrix4x4 transMat;
		transMat.translate(wT);

		for (int i = 0; i < _take->trackers.size(); ++i)
		{
			TakeTracker* t = _take->trackers[i];

			QMatrix4x4 newWorldMat = t->worldMat;
			
			newWorldMat = _pointWorldMat * transMat * newWorldMat;
			newWorldMat(0, 3) *= scaleFactor;
			newWorldMat(1, 3) *= scaleFactor;
			newWorldMat(2, 3) *= scaleFactor;
			
			cv::Mat pose = TakeTracker::PoseFromWorld(newWorldMat);
			t->SetPose(pose);
		}
	}
}

void MainWindow::OnPushToLiveClicked()
{
	if (_take)
	{
		for (int i = 0; i < _take->trackers.size(); ++i)
		{
			TakeTracker* t = _take->trackers[i];
			emit OnUpdateParams(t->serial, t->GetProps().GetJSON(false));
			// TODO: Push to live trackers in live take.
		}
	}
}

void MainWindow::OnToggleUpdateClicked()
{
	if (ui->btnToggleUpdate->isChecked())	
	{
		_timelineTimer->setInterval(1000 / 60);
	}
	else
	{
		_timelineTimer->setInterval(1000 / 10);
	}
}

void MainWindow::OnToggleMaskClicked()
{
	_cameraView->setMask(ui->btnToggleMask->isChecked());
}

void MainWindow::OnToggleDistortedMarkersClicked()
{
	_cameraView->showDistortedMarkers = ui->btnToggleDistortedMarkers->isChecked();
	_cameraView->update();
}

void MainWindow::OnToggleUndistortedMarkersClicked()
{
	_cameraView->showUndistortedMarkers = ui->btnToggleUndistortedMarkers->isChecked();
	_cameraView->update();
}

void MainWindow::OnToggleReprojectedMarkersClicked()
{
	_cameraView->showReprojectedMarkers = ui->btnToggleReprojectedMarkers->isChecked();
	_cameraView->update();
}

void MainWindow::OnTogglePixelGridClicked()
{
	_cameraView->setPixelGrid(ui->btnTogglePixelGrid->isChecked());
}

void MainWindow::OnToggleMarkerSourcesClicked()
{
	_glView->showMarkerSources = ui->btnToggleMarkerSources->isChecked();
}

void MainWindow::OnToggleRaysClicked()
{
	_glView->showRays = ui->btnToggleRays->isChecked();
}

void MainWindow::OnToggleExpandedMarkersClicked()
{
	_glView->showExpandedMarkers = ui->btnToggleExpandedMarkers->isChecked();
}

void MainWindow::OnShowLiveClicked()
{
	_take = _liveTake;
	_cameraView->take = _take;
	_glView->take = _take;
	_cameraView->update();
}

void MainWindow::OnBuildFundamentalMatClicked()
{
	if (_take)
	{
		_take->BuildExtrinsics(_timeline->rangeStartFrame, _timeline->rangeEndFrame);
	}
}

void MainWindow::OnBundleAdjustClicked()
{
	if (_take)
	{
		_take->BundleAdjust(_timeline->rangeStartFrame, _timeline->rangeEndFrame);
	}
}

void MainWindow::viewFeed(int TrackedId, int StreamMode)
{
	emit OnStartViewSteam(TrackedId, StreamMode);
}

void MainWindow::selectTracker(LiveTracker* Tracker)
{
	_take->selectedTracker = Tracker->id;

	ui->txtId->setText(Tracker->name);
	ui->txtExposure->setText(QString::number(Tracker->exposure));
	ui->txtIso->setText(QString::number(Tracker->iso));
	ui->txtFps->setText(QString::number(Tracker->frameDuration));
}

/*
LiveTracker* MainWindow::GetTracker(int TrackerId)
{
	if (_liveTrackers.find(TrackerId) == _liveTrackers.end())
	{
		return 0;
	}

	return _liveTrackers[TrackerId];
}
*/

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

void MainWindow::updateTakeList()
{
	QDir dir("project");

	QStringList takes = dir.entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot);
	ui->lstTakes->clear();
	
	for (int i = 0; i < takes.size(); ++i)
	{
		QString takeFileName = "project/" + takes[i] + "/" + takes[i] + ".take";
		
		qDebug() << "Checking take file:" << takeFileName;

		QFile takeFile(takeFileName);

		if (takeFile.exists())
		{
			ui->lstTakes->addItem(takes[i]);
		}
	}
}