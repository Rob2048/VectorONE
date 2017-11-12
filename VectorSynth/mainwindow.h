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
	void OnUpdateParams(unsigned int SerialId, QByteArray Props);
	void OnMaskChange(int ClientId, QByteArray Data);
	void OnStartLiveFeed(int ClientId);
	void OnResetFrameIds();
	void OnStartRecording(QString TakeName);
	void OnStopRecording();
	void OnStartCalibrating(int TrackerId);
	void OnStopCalibrating();
	void OnStartViewSteam(int TrackedId, int StreamMode);

public slots:

	void OnCalibrationStartClick();
	void OnCalibrationStopClick();

	void OnResetFrameIdsClick();
	void OnStartRecordingClick();
	
	void OnTimerTick();
	void OnTimelineTimerTick();
	void OnSceneViewTimerTick();

	void OnTrackerConnected(int TrackerId);
	void OnTrackerDisconnected(int TrackerId);
	void OnTrackerFrame(int TrackerId);
	void OnTrackerMarkersFrame(int TrackerId);
	void OnTrackerInfoUpdate(int TrackerId);

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

	void OnBuild3DMarkersClicked();
	void OnAutoLabelClicked();
	void OnBuildFundamentalMatClicked();
	void OnBundleAdjustClicked();
	void OnSelectWorldBasisClicked();
	void OnAssignWorldBasisClicked();
	void OnPushToLiveClicked();

	// Tracker view buttons.
	void OnToggleUpdateClicked();
	void OnToggleMaskClicked();
	void OnToggleDistortedMarkersClicked();
	void OnToggleUndistortedMarkersClicked();
	void OnToggleReprojectedMarkersClicked();
	void OnTogglePixelGridClicked();
	
	// Scene view buttons.
	void OnToggleMarkerSourcesClicked();
	void OnToggleRaysClicked();
	void OnToggleExpandedMarkersClicked();
	void OnShowLiveClicked();

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

	LiveTake*			_liveTake;
	LoadedTake*			_loadedTake;
	Take*				_take;
	
	int					_timelineRequestedFrame;
	int					_timelineCurrentFrame;

	QTimer*				_playTimer;
	float				_playFrame;

	bool				_recording;

	void _LoadTakeTracker(int Id);
	void _AdvanceTakeFrame(int Camera, int Frames);

public:

	void viewFeed(int TrackedId, int StreamMode);
	void selectTracker(LiveTracker* Tracker);
	//LiveTracker* GetTracker(int TrackerId);
	void changeMask(LiveTracker* Tracker);
	void updateTakeList();
};