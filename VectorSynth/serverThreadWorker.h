#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include "trackerConnection.h"

QT_USE_NAMESPACE

class ServerThreadWorker : public QThread
{
	Q_OBJECT

signals:

	void OnTrackerConnected(int TrackedId);
	void OnTrackerDisconnected(int TrackedId);
	void OnTrackerFrame(int TrackedId);
	void OnTrackerMarkersFrame(int TrackedId);
	void OnTrackerInfoUpdate(int TrackerId);

public slots:

	void OnStart();
	void OnTcpServerConnectionAvailable();
	void OnDisconnected(TrackerConnection* Tracker);
	void OnNewFrame(TrackerConnection* Tracker);
	void OnNewMarkersFrame(TrackerConnection* Tracker);
	void OnInfoUpdate(TrackerConnection* Tracker);
	void OnSendData(int ClientId, QByteArray Data);
	void OnUpdateTracker(unsigned int SerialId, QByteArray Props);
	void OnMaskChange(int ClientId, QByteArray Data);
	void OnViewFeed(int ClientId, int StreamMode);
	void InternalRecordingStart();
	void OnResetFrameIds();
	void OnStartRecording();
	void OnStartCalibrating(int TrackerId);
	void OnStopCalibrating();

public:

	int64_t takeStartFrameId;

	TrackerConnection* LockConnection(int TrackerId);
	void UnlockConnection(TrackerConnection* Tracker);

private:

	QMutex _connectionMutex;

	std::map<int, TrackerConnection*> _connections;

	bool			_recording = false;
	int				_nextConnectionId = 0;
	QTcpServer*		_tcpServer;

	void _StopAllTrackerCams();
	TrackerConnection* _GetTracker(int ClientId);
};