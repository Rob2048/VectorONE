#include "serverThreadWorker.h"

void ServerThreadWorker::OnStart()
{
	qDebug() << "Starting Server Thread Worker" << QThread::currentThreadId();

	_tcpServer = new QTcpServer(this);
	connect(_tcpServer, &QTcpServer::newConnection, this, &ServerThreadWorker::OnTcpServerConnectionAvailable);
	_tcpServer->listen(QHostAddress::Any, 8000);
}

void ServerThreadWorker::OnTcpServerConnectionAvailable()
{
	QTcpSocket* tcpSocket = _tcpServer->nextPendingConnection();

	if (tcpSocket)
	{
		qDebug() << "New TCP Client" << QThread::currentThreadId();
		LiveTracker* nc = new LiveTracker(++_nextConnectionId, tcpSocket, &masterTimer, this);
		_connections[_nextConnectionId] = nc;
		connect(nc, &LiveTracker::OnNewFrame, this, &ServerThreadWorker::OnNewFrame);
		connect(nc, &LiveTracker::OnNewMarkersFrame, this, &ServerThreadWorker::OnNewMarkersFrame);
		connect(nc, &LiveTracker::OnDisconnected, this, &ServerThreadWorker::OnDisconnected);
		emit OnTrackerConnected(nc);
	}
}

void ServerThreadWorker::OnDisconnected(LiveTracker* Tracker)
{
	qDebug() << "Tracker Disconnected";
	emit OnTrackerDisconnected(Tracker);
}

void ServerThreadWorker::OnNewFrame(LiveTracker* Tracker)
{
	emit OnTrackerFrame(Tracker);
}

void ServerThreadWorker::OnNewMarkersFrame(LiveTracker* Tracker)
{
	emit OnTrackerMarkersFrame(Tracker);
}

void ServerThreadWorker::OnSendData(int ClientId, QByteArray Data)
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->socket->write(Data);
}

void ServerThreadWorker::OnCamSensitivityChange(int Value)
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->camSensitivity = (float)Value / 255.0f;
}

void ServerThreadWorker::OnCamFrameSkipChanged()
{
	int value = ((QLineEdit*)QObject::sender())->text().toInt();
	qDebug() << "FS Changed" << value;
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->frameSkip = value;
}

void ServerThreadWorker::OnCamThresholdChange(int Value)
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->camThreshold = (float)Value / 255.0f;
}

void ServerThreadWorker::OnCamDistortChange(int Value)
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->camDistort = (float)Value / 255.0f;
}

void ServerThreadWorker::OnDrawGuidesChanged(int State)
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->drawGuides = (State == 2);
}

void ServerThreadWorker::OnDrawMarkersChanged(int State)
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->drawMarkers = (State == 2);
}

void ServerThreadWorker::OnFindCalibChanged(int State)
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		it->second->decoder->drawUndistorted = (State == 2);
}

void ServerThreadWorker::InternalSync1()
{
	_connections[1]->socket->write("ts\n");
	masterTimer.start();
}

void ServerThreadWorker::OnStartTimeSync()
{
	_StopAllTrackerCams();
	QTimer::singleShot(500, this, SLOT(InternalSync1()));
}

void ServerThreadWorker::OnViewFeed(int ClientId)
{
	qDebug() << "View Feed" << ClientId;
	_StopAllTrackerCams();

	LiveTracker* tracker = _GetTracker(ClientId);
	if (tracker)
	{
		//tracker->socket->write("cm,1\n");
		tracker->socket->write("sc\n");
		tracker->streaming = true;
	}
}

void ServerThreadWorker::InternalRecordingStart()
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->socket->write("sc\n");
	}
}

void ServerThreadWorker::OnStartRecording()
{
	qDebug() << "Button";

	if (_recording)
	{
		qDebug() << "Stop Recording";
		for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		{
			it->second->socket->write("ec\n");
			it->second->StopRecording();
		}
	}
	else
	{
		qDebug() << "Start Recording";
		for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
		{
			it->second->socket->write("ec\n");
			it->second->streaming = false;
			it->second->recording = false;
			it->second->StartRecording();
		}

		QTimer::singleShot(500, this, SLOT(InternalRecordingStart()));
	}

	_recording = !_recording;
	qDebug() << "Done";
}

void ServerThreadWorker::OnStartCalibrating(int TrackerId)
{
	qDebug() << "Start Calibrating";
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->decoder->findCalibrationSheet = false;
	}

	LiveTracker* tracker = _GetTracker(TrackerId);
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
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->decoder->findCalibrationSheet = false;
	}
}

LiveTracker* ServerThreadWorker::_GetTracker(int ClientId)
{
	if (_connections.find(ClientId) == _connections.end())
	{
		return 0;
	}

	return _connections[ClientId];
}

void ServerThreadWorker::_StopAllTrackerCams()
{
	for (std::map<int, LiveTracker*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
	{
		it->second->socket->write("ec\n");
		it->second->streaming = false;
		it->second->recording = false;
	}
}