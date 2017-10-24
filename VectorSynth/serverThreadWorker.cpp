#include "serverThreadWorker.h"

void ServerThreadWorker::OnStart()
{
	qDebug() << "Starting Server Thread Worker" << QThread::currentThreadId();

	masterTimer.start();

	_tcpServer = new QTcpServer(this);
	connect(_tcpServer, &QTcpServer::newConnection, this, &ServerThreadWorker::OnTcpServerConnectionAvailable);
	_tcpServer->listen(QHostAddress::Any, 8000);
}

TrackerConnection* ServerThreadWorker::LockConnection(int TrackerId)
{
	_connectionMutex.lock();

	TrackerConnection* tracker = _GetTracker(TrackerId);

	if (!tracker)
	{
		_connectionMutex.unlock();
		return 0;
	}

	tracker->Lock();

	return tracker;
}

void ServerThreadWorker::UnlockConnection(TrackerConnection* Tracker)
{
	if (!Tracker)
		return;

	Tracker->Unlock();
	_connectionMutex.unlock();
}

void ServerThreadWorker::OnTcpServerConnectionAvailable()
{
	QTcpSocket* tcpSocket = _tcpServer->nextPendingConnection();

	if (tcpSocket)
	{
		_connectionMutex.lock();
		qDebug() << "New TCP Client" << QThread::currentThreadId();
		tcpSocket->setReadBufferSize(1024 * 1024);
		TrackerConnection* nc = new TrackerConnection(++_nextConnectionId, tcpSocket, &masterTimer, this);
		_connections[_nextConnectionId] = nc;
		connect(nc, &TrackerConnection::OnNewFrame, this, &ServerThreadWorker::OnNewFrame);
		connect(nc, &TrackerConnection::OnNewMarkersFrame, this, &ServerThreadWorker::OnNewMarkersFrame);
		connect(nc, &TrackerConnection::OnDisconnected, this, &ServerThreadWorker::OnDisconnected);
		connect(nc, &TrackerConnection::OnInfoUpdate, this, &ServerThreadWorker::OnInfoUpdate);
		_connectionMutex.unlock();

		emit OnTrackerConnected(_nextConnectionId);
	}
}

void ServerThreadWorker::OnDisconnected(TrackerConnection* Tracker)
{
	qDebug() << "Tracker Disconnected";

	_connectionMutex.lock();
	int trackerId = Tracker->id;
	_connections.erase(trackerId);
	delete Tracker;
	_connectionMutex.unlock();

	emit OnTrackerDisconnected(trackerId);
}
void ServerThreadWorker::OnInfoUpdate(TrackerConnection* Tracker)
{
	emit OnTrackerInfoUpdate(Tracker->id);
}

void ServerThreadWorker::OnNewFrame(TrackerConnection* Tracker)
{
	emit OnTrackerFrame(Tracker->id);
}

void ServerThreadWorker::OnNewMarkersFrame(TrackerConnection* Tracker)
{
	emit OnTrackerMarkersFrame(Tracker->id);
}

void ServerThreadWorker::OnMaskChange(int ClientId, QByteArray Data)
{
	TrackerConnection* tracker = _GetTracker(ClientId);

	if (tracker)
	{
		memcpy(tracker->maskData, Data.data(), sizeof(tracker->maskData));

		char asciiMask[64 * 44];

		for (int i = 0; i < 64 * 44; ++i)
		{
			asciiMask[i] = tracker->maskData[i] + '0';
		}

		//QByteArray mask((char*)tracker->maskData, sizeof(tracker->maskData));

		QString cmd = QString("sm,") + QString::fromLatin1(asciiMask, sizeof(tracker->maskData)) + QString("\n");
		QByteArray data(cmd.toLatin1());

		tracker->socket->write(data);
	}
}

void ServerThreadWorker::OnSendData(int ClientId, QByteArray Data)
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->socket->write(Data);
}

void ServerThreadWorker::OnCamSensitivityChange(int Value)
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->camSensitivity = (float)Value / 255.0f;
}

void ServerThreadWorker::OnCamFrameSkipChanged()
{
	int value = ((QLineEdit*)QObject::sender())->text().toInt();
	qDebug() << "FS Changed" << value;
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->frameSkip = value;
}

void ServerThreadWorker::OnCamThresholdChange(int Value)
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->camThreshold = (float)Value / 255.0f;
}

void ServerThreadWorker::OnCamDistortChange(int Value)
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->camDistort = (float)Value / 255.0f;
}

void ServerThreadWorker::OnDrawGuidesChanged(int State)
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->drawGuides = (State == 2);
}

void ServerThreadWorker::OnDrawMarkersChanged(int State)
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->drawMarkers = (State == 2);
}

void ServerThreadWorker::OnFindCalibChanged(int State)
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->drawUndistorted = (State == 2);
}

void ServerThreadWorker::InternalSync1()
{
	//_connections[1]->socket->write("ts\n");
	//masterTimer.start();
}

void ServerThreadWorker::OnStartTimeSync()
{
	//_StopAllTrackerCams();
	//QTimer::singleShot(500, this, SLOT(InternalSync1()));
}

void ServerThreadWorker::OnViewFeed(int ClientId, bool Image)
{
	//qDebug() << "View Feed" << ClientId << QThread::currentThreadId();
	//_StopAllTrackerCams();

	TrackerConnection* tracker = _GetTracker(ClientId);
	if (tracker)
	{
		if (Image)
		{
			tracker->socket->write("cm,0\n");
			tracker->socket->write("sc\n");
			tracker->streaming = true;
		}
		else
		{
			tracker->socket->write("cm,1\n");
			tracker->socket->write("sc\n");
			tracker->streaming = true;
			/*
			tracker->socket->write("ec\n");
			tracker->streaming = false;
			tracker->recording = false;
			*/
		}
	}
}

void ServerThreadWorker::InternalRecordingStart()
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->socket->write("sc\n");
	}
}

void ServerThreadWorker::OnStartRecording()
{
	if (_recording)
	{
		qDebug() << "Stop Recording";
		for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		{
			it->second->socket->write("ec\n");
			it->second->StopRecording();
		}
	}
	else
	{
		qDebug() << "Start Recording";
		for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		{
			it->second->socket->write("ec\n");
			it->second->streaming = false;
			it->second->recording = false;
			it->second->StartRecording();
		}

		QTimer::singleShot(2000, this, SLOT(InternalRecordingStart()));
	}

	_recording = !_recording;
}

void ServerThreadWorker::OnStartCalibrating(int TrackerId)
{
	qDebug() << "Start Calibrating";
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->decoder->findCalibrationSheet = false;
	}

	TrackerConnection* tracker = _GetTracker(TrackerId);
	qDebug() << "Star calib" << TrackerId;
	if (tracker)
	{
		qDebug() << "Started";
		tracker->decoder->findCalibrationSheet = true;
	}
}

void ServerThreadWorker::OnStopCalibrating()
{
	qDebug() << "Stop Calibrating";
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->decoder->findCalibrationSheet = false;
	}
}

TrackerConnection* ServerThreadWorker::_GetTracker(int ClientId)
{
	if (_connections.find(ClientId) == _connections.end())
	{
		return 0;
	}

	return _connections[ClientId];
}

void ServerThreadWorker::_StopAllTrackerCams()
{
	for (std::map<int, TrackerConnection*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->socket->write("ec\n");
		it->second->streaming = false;
		it->second->recording = false;
	}
}