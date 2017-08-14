#pragma once

#include <QMainWindow>
#include <QtNetwork>
#include "decoder.h"

QT_USE_NAMESPACE

class LiveTracker : public QObject
{
	Q_OBJECT

public:

	int				id;
	bool			accepted;
	bool			streaming;
	bool			recording;
	QTcpSocket*		socket;
	Decoder*		decoder;
	QMutex			bufferMutex;
	uint8_t			postFrameData[VID_W * VID_H * 3];
	uint8_t			markerData[1024 * 10];
	int				markerDataSize;
	int				newFrames;
	QElapsedTimer*	masterTimer;
	FILE*			recordFile;

	LiveTracker(int Id, QTcpSocket* Socket, QElapsedTimer* MasterTimer, QObject* Parent);
	~LiveTracker();

	void StartRecording();
	void StopRecording();

	void RecordData(uint8_t* Data, int Len);

signals:

	void OnDisconnected(LiveTracker* Tracker);
	void OnNewFrame(LiveTracker* Tracker);
	void OnNewMarkersFrame(LiveTracker* Tracker);

public slots:

	void OnTcpSocketDisconnected();
	void OnTcpSocketReadyRead();

private:

	uint8_t _recvBuffer[1024 * 1024];
	int _recvLength;
	int _recvState;
	int _recvPacketId;
	int _recvFrameSize;
};