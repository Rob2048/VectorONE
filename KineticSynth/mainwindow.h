#pragma once

#include <QMainWindow>
#include <QtNetwork>
#include "decoder.h"
#include "tracker.h"
#include "take.h"
#include "timelineWidget.h"
#include "liveTracker.h"
#include "serverThreadWorker.h"

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

	void RefreshTakeDeviceList();

signals:

	void OnServerStart();
	void OnSendData(int ClientId, QByteArray Data);
	void OnStartTimeSync();
	void OnStartLiveFeed(int ClientId);
	void OnStartRecording();
	void OnStartCalibrating(int TrackerId);
	void OnStopCalibrating();

public slots:

	void OnCalibrationStartClick();
	void OnCalibrationStopClick();

	void OnStartTimeSyncClick();
	void OnStartRecordingClick();
	
	void OnTimerTick();
	void OnTimelineTimerTick();
	void OnSceneViewTimerTick();

	void OnTrackerConnected(LiveTracker* Tracker);
	void OnTrackerDisconnected(LiveTracker* Tracker);

	void OnTrackerFrame(LiveTracker* Connection);
	void OnTrackerMarkersFrame(LiveTracker* Connection);

	void OnTakeTrackerTableSelectionChanged();

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

private:

	Ui::MainWindow*		ui;
	TimelineWidget*		_timeline;
	CameraView*			_cameraView;
	QUdpSocket*			_udpSocket;
	QTimer*				_timer;
	QTimer*				_timelineTimer;
	QTimer*				_sceneViewTimer;
	QHostAddress		_localIp;
	SceneView*			_glView;
	ServerThreadWorker*	_serverWorker;
	QThread				_serverThread;
	std::vector<Tracker*> _trackers;
	QSignalMapper*		_deviceListMapper;

	Take*				_take;

	int					_frameCounter;
	int					_newFrames;
	float				_fps;

	int					_timelineRequestedFrame;
	int					_timelineCurrentFrame;

	TakeTracker*		_selectedTakeTracker;

	QTimer*				_playTimer;
	float				_playFrame;

	bool _drawMarkers;

	void _LoadTakeTracker(int Id);
	void _AdvanceTakeFrame(int Camera, int Frames);	
};