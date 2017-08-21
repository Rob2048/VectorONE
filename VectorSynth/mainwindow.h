#pragma once

#include <QMainWindow>
#include <QtNetwork>
#include "decoder.h"
#include "tracker.h"
#include "take.h"
#include "timelineWidget.h"
#include "trackerConnection.h"
#include "serverThreadWorker.h"
#include "liveTracker.h"

class CameraView;
class SceneView;

namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:

	explicit MainWindow(QWidget* parent = 0);
	~MainWindow();

signals:

	void OnServerStart();
	void OnSendData(int ClientId, QByteArray Data);
	void OnMaskChange(int ClientId, QByteArray Data);
	void OnStartTimeSync();
	void OnStartLiveFeed(int ClientId);
	void OnStartRecording();
	void OnStartCalibrating(int TrackerId);
	void OnStopCalibrating();
	void OnStartViewSteam(int TrackedId, bool Image);

public slots:

	void OnCalibrationStartClick();
	void OnCalibrationStopClick();

	void OnStartTimeSyncClick();
	void OnStartRecordingClick();
	
	void OnTimerTick();
	void OnTimelineTimerTick();
	void OnSceneViewTimerTick();

	void OnTrackerConnected(int TrackerId);
	void OnTrackerDisconnected(int TrackerId);
	void OnTrackerFrame(int TrackerId);
	void OnTrackerMarkersFrame(int TrackerId);
	void OnTrackerInfoUpdate(int TrackerId);

	// UI
	void OnCamSensitivityChange(int Value);

	void OnExposureEditingFinished();
	void OnIsoEditingFinished();
	void OnFpsEditingFinished();

	void OnLoadTakeClick();
	void OnSaveTakeClick();
	void OnNextFrameClick();
	void OnNextFrameJumpClick();
	void OnPrevFrameClick();
	void OnPrevFrameJumpClick();
	void OnPlayClick();

	void OnTimelineChange(int Value);

	void OnTakeOffsetEditingFinished();
	void OnTakeSensitivityChange(int Value);
	void OnTakeThresholdChange(int Value);

	void OnGenerateMaskClicked();
	void OnBuild2DMarkersClicked();
	void OnBuild3DMarkersClicked();
	void OnBuildFundamentalMatClicked();
	void OnAssignWorldBasisClicked();

	void OnDrawMarkersClicked();
	void OnDrawVideoClicked();

	void OnPlayTimerTick();

	void OnBroadcastRead();

private:

	Ui::MainWindow*		ui;
	TimelineWidget*		_timeline;
	CameraView*			_cameraView;
	QUdpSocket*			_udpSocket;
	QUdpSocket*			_recvSocket;
	QTimer*				_timer;
	QTimer*				_timelineTimer;
	QTimer*				_sceneViewTimer;
	QHostAddress		_localIp;
	SceneView*			_glView;
	ServerThreadWorker*	_serverWorker;
	QThread				_serverThread;
	QSignalMapper*		_deviceListMapper;
	QElapsedTimer		_mainTimer;

	Take*				_take;
	std::map<int, LiveTracker*> _liveTrackers;
	TakeTracker*		_selectedTakeTracker;
	int					_selectedTracker;

	int					_timelineRequestedFrame;
	int					_timelineCurrentFrame;

	QTimer*				_playTimer;
	float				_playFrame;

	bool _drawMarkers;

	void _LoadTakeTracker(int Id);
	void _AdvanceTakeFrame(int Camera, int Frames);

public:

	

	void viewFeed(int TrackedId, bool Image);
	void selectTracker(LiveTracker* Tracker);
	LiveTracker* GetTracker(int TrackerId);

	void changeMask(LiveTracker* Tracker);
};