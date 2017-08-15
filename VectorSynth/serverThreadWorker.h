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
	void OnCamSensitivityChange(int Value);
	void OnCamFrameSkipChanged();
	void OnCamThresholdChange(int Value);
	void OnCamDistortChange(int Value);
	void OnDrawGuidesChanged(int State);
	void OnDrawMarkersChanged(int State);
	void OnFindCalibChanged(int State);
	void InternalSync1();
	void OnStartTimeSync();
	void OnViewFeed(int ClientId, bool Image);
	void InternalRecordingStart();
	void OnStartRecording();
	void OnStartCalibrating(int TrackerId);
	void OnStopCalibrating();

	TrackerConnection* LockConnection(int TrackerId);
	void UnlockConnection(TrackerConnection* Tracker);

private:

	QMutex _connectionMutex;

	std::map<int, TrackerConnection*> _connections;

	QElapsedTimer	masterTimer;
	bool			_recording = false;
	int				_nextConnectionId = 0;
	QTcpServer*		_tcpServer;

	void _StopAllTrackerCams();
	TrackerConnection* _GetTracker(int ClientId);
};