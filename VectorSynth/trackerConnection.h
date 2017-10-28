#pragma once

#include <QMainWindow>
#include <QtNetwork>
#include "decoder.h"

QT_USE_NAMESPACE

class TrackerConnection : public QObject
{
	Q_OBJECT

public:

	uint32_t		id;
	uint32_t		serial;
	QString			name;
	int				version;
	bool			accepted;
	bool			streaming;
	bool			recording;	
	QTcpSocket*		socket;
	Decoder*		decoder;
	uint8_t			postFrameData[VID_W * VID_H * 3];
	float			avgMasterOffset;
	int64_t			latestFrameId;
	uint8_t			markerData[1024 * 10];
	int				markerDataSize;
	QElapsedTimer*	masterTimer;
	FILE*			recordFile;
	uint8_t			maskData[64 * 44];

	TrackerConnection(int Id, QTcpSocket* Socket, QElapsedTimer* MasterTimer, QObject* Parent);
	~TrackerConnection();

	void StartRecording();
	void StopRecording();

	void RecordData(uint8_t* Data, int Len);

signals:

	void OnDisconnected(TrackerConnection* Tracker);
	void OnNewFrame(TrackerConnection* Tracker);
	void OnNewMarkersFrame(TrackerConnection* Tracker);
	void OnInfoUpdate(TrackerConnection* Tracker);

public slots:

	void OnTcpSocketDisconnected();
	void OnTcpSocketReadyRead();

	void Lock();
	void Unlock();

private:

	QMutex		_mutex;
	uint8_t		_recvBuffer[1024 * 1024];
	int			_recvLength;
	int			_recvState;
	int			_recvPacketId;
	int			_recvFrameSize;
	bool		_gotDataFrame;
};