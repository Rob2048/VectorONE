#pragma once

#include <QMainWindow>
#include <QtNetwork>
#include "decoder.h"

QT_USE_NAMESPACE

class ServerThreadWorker;

class TrackerConnection : public QObject
{
	Q_OBJECT

public:

	// Connection.
	bool			accepted;
	bool			streaming;
	bool			recording;
	int				streamMode;
	FILE*			recordFile;
	QTcpSocket*		socket;
	
	// Tracker params.
	uint32_t		id;
	QString			name;
	uint32_t		serial;
	int				version;
	uint8_t			maskData[64 * 44];

	// Frame data.
	Decoder*		decoder;
	float			avgMasterOffset;
	int64_t			latestFrameId;
	uint8_t			postFrameData[VID_W * VID_H * 3];
	uint8_t			markerData[1024 * 10];
	int				markerDataSize;

	TrackerConnection(int Id, QTcpSocket* Socket, QObject* Parent);
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

	ServerThreadWorker* _serverThreadWorker;
};