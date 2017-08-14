#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include "liveTracker.h"

QT_USE_NAMESPACE

class ServerThreadWorker : public QThread
{
	Q_OBJECT

signals:

	void OnTrackerConnected(LiveTracker* Tracker);
	void OnTrackerDisconnected(LiveTracker* Tracker);
	void OnTrackerFrame(LiveTracker* Tracker);
	void OnTrackerMarkersFrame(LiveTracker* Tracker);

public slots:

	void OnStart();
	void OnTcpServerConnectionAvailable();
	void OnDisconnected(LiveTracker* Tracker);
	void OnNewFrame(LiveTracker* Tracker);
	void OnNewMarkersFrame(LiveTracker* Tracker);
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
	void OnViewFeed(int ClientId);
	void InternalRecordingStart();
	void OnStartRecording();
	void OnStartCalibrating(int TrackerId);
	void OnStopCalibrating();

	LiveTracker* _GetTracker(int ClientId);

private:

	bool _recording = false;
	int _nextConnectionId = 0;
	QTcpServer* _tcpServer;
	std::map<int, LiveTracker*> _connections;

	QElapsedTimer masterTimer;

	void _StopAllTrackerCams();
};