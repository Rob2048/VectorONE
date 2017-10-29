#include "trackerConnection.h"
#include "serverThreadWorker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

TrackerConnection::TrackerConnection(int Id, QTcpSocket* Socket, QObject* Parent) :
	QObject(Parent),
	id(Id),
	accepted(false),
	streamMode(0),
	streaming(false),
	recording(false),
	recordFile(0),
	socket(Socket),
	_recvLength(0),
	_recvState(0),
	_recvPacketId(0),
	_serverThreadWorker((ServerThreadWorker*)Parent)
{
	memset(maskData, 1, sizeof(maskData));
	decoder = new Decoder();

	connect(Socket, &QTcpSocket::readyRead, this, &TrackerConnection::OnTcpSocketReadyRead);
	connect(Socket, &QTcpSocket::disconnected, this, &TrackerConnection::OnTcpSocketDisconnected);

	Socket->write("gi\n");
}

TrackerConnection::~TrackerConnection()
{
	StopRecording();
}

void TrackerConnection::Lock()
{
	_mutex.lock();
}

void TrackerConnection::Unlock()
{
	_mutex.unlock();
}

void TrackerConnection::StartRecording()
{
	StopRecording();

	char fileName[256];

	if (streamMode == 1 || streamMode == 2)
	{
		/*
		QJsonObject jsonObj;
		jsonObj["name"] = name;
		jsonObj["fps"] = fps;
		jsonObj["exposure"] = exposure;
		jsonObj["iso"] = iso;
		jsonObj["threshold"] = threshold;
		jsonObj["sensitivity"] = sensitivity;
		jsonObj["offset"] = frameOffset;
		QJsonDocument jsonDoc(jsonObj);
		QByteArray jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Indented);
		QFile file("project\\take\\" + QString::number(serial) + ".tracker");

		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			qDebug() << "Tracker: Save file failed for recording.";
			return;
		}

		file.write(jsonBytes);
		file.close();
		*/

		sprintf(fileName, "project\\take\\%u.mask", serial);
		FILE* maskFile = fopen(fileName, "wb");
		fwrite(maskData, sizeof(maskData), 1, maskFile);
		fclose(maskFile);
	}

	if (streamMode == 1)
	{
		sprintf(fileName, "project\\take\\%u.trakvid", serial);
		recordFile = fopen(fileName, "wb");
		recording = true;
		_gotDataFrame = false;
	}
	else if (streamMode == 2)
	{
		sprintf(fileName, "project\\take\\%u.trakblobs", serial);
		recordFile = fopen(fileName, "wb");
		recording = true;
	}
}

void TrackerConnection::StopRecording()
{
	if (recordFile)
		fclose(recordFile);

	recordFile = 0;
	recording = false;
	_gotDataFrame = false;
}

void TrackerConnection::RecordData(uint8_t* Data, int Len)
{
	fwrite(Data, Len, 1, recordFile);
	//fflush(recordFile);
}

void TrackerConnection::OnTcpSocketDisconnected()
{
	qDebug() << "Client" << id << "Disconnected";
	emit OnDisconnected(this);
}

void TrackerConnection::OnTcpSocketReadyRead()
{
	while (socket->bytesAvailable())
	{
		// NOTE: Socket buffer (socket->bytesAvailable()) only updates on next Qt gui loop.
		
		if (_recvState == 0)
		{
			_recvLength = 0;
			_recvPacketId = 0;

			if (socket->bytesAvailable() >= 4)
			{
				socket->read((char*)_recvBuffer, 4);
				_recvPacketId = _recvBuffer[0];
				_recvState = 1;
			}
			else
			{
				break;
			}
		}
		else if (_recvPacketId == 3)
		{
			if (socket->bytesAvailable() >= 20)
			{
				socket->read((char*)_recvBuffer, 20);
				
				_recvFrameSize = *(int*)&_recvBuffer[0];
				int frameType = *(int*)&_recvBuffer[4];
				avgMasterOffset = *(float*)&_recvBuffer[8];
				int64_t tempLatestFrameId = *(int64_t*)&_recvBuffer[12];

				if (tempLatestFrameId > 0)
					latestFrameId = tempLatestFrameId;

				if (frameType == 2)
				{
					_gotDataFrame = true;
					if (_serverThreadWorker->takeStartFrameId == 0)
					{
						_serverThreadWorker->takeStartFrameId = latestFrameId + 1;
					}
				}

				if (tempLatestFrameId > 0)
				{
					latestFrameId -= _serverThreadWorker->takeStartFrameId;
					*(int64_t*)&_recvBuffer[12] = latestFrameId;
				}

				if (recording && _gotDataFrame)
				{
					RecordData(_recvBuffer, 20);
				}

				_recvState = 2;
				_recvPacketId = 0;
			}
			else
			{
				break;
			}
		}
		else if (_recvPacketId == 4)
		{
			if (socket->bytesAvailable() >= 8 + sizeof(maskData))
			{
				qDebug() << "Got GI";

				socket->read((char*)_recvBuffer, 8 + sizeof(maskData));
				version = _recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];
				serial = _recvBuffer[7] << 24 | _recvBuffer[6] << 16 | _recvBuffer[5] << 8 | _recvBuffer[4];
				name = "Unnamed";
				memcpy(maskData, _recvBuffer + 8, sizeof(maskData));

				for (int i = 0; i < sizeof(maskData); ++i)
				{
					maskData[i] -= 48;
				}

				accepted = true;
				_recvState = 0;

				emit OnInfoUpdate(this);
			}
			else
			{
				break;
			}
		}
		else if (_recvPacketId == 5)
		{
			// NOTE: Realtime blob packet header.

			if (socket->bytesAvailable() >= 16)
			{
				socket->read((char*)_recvBuffer, 16);
				_recvFrameSize = *(int*)&_recvBuffer[0];
				latestFrameId = *(int64_t*)&_recvBuffer[4];
				avgMasterOffset = *(float*)&_recvBuffer[12];

				if (_serverThreadWorker->takeStartFrameId == 0)
				{
					_serverThreadWorker->takeStartFrameId = latestFrameId;
				}

				latestFrameId -= _serverThreadWorker->takeStartFrameId;
				*(int64_t*)&_recvBuffer[4] = latestFrameId;

				_recvState = 3;
				_recvPacketId = 0;
				_recvLength = 16;
			}
			else
			{
				break;
			}
		}
		else if (_recvState == 2)
		{
			// NOTE: Recv realtime image stream.

			int readBytes = socket->read((char*)_recvBuffer, _recvFrameSize);
			_recvFrameSize -= readBytes;

			if (recording && _gotDataFrame)
			{
				RecordData(_recvBuffer, readBytes);
			}
			
			if (streaming)
			{
				if (decoder->DoDecode(_recvBuffer, readBytes))
				{
					Lock();
					decoder->TransferFrameToBuffer(postFrameData);
					Unlock();

					emit OnNewFrame(this);
				}
			}
			
			if (_recvFrameSize == 0)
				_recvState = 0;
		}
		else if (_recvState == 3)
		{
			// NOTE: Recv realtime blob stream.

			//qDebug() << "Test Avail" << socket->bytesAvailable();
			if (socket->bytesAvailable() > 0 && _recvLength < _recvFrameSize + 4)
			{
				int readBytes = socket->read((char*)_recvBuffer + _recvLength, (_recvFrameSize + 4) - _recvLength);
				_recvLength += readBytes;

				//qDebug() << "Total" << _recvLength << "/" << _recvFrameSize;

				if (_recvLength == _recvFrameSize + 4)
				{
					++decoder->newFrames;
					decoder->dataRecvBytes += _recvFrameSize + 4;

					if (recording)
					{
						RecordData(_recvBuffer, _recvFrameSize + 4);
					}
					else if (streaming)
					{
						Lock();
						memcpy(markerData, _recvBuffer + 4, _recvFrameSize);
						markerDataSize = _recvFrameSize;
						Unlock();

						emit OnNewMarkersFrame(this);
					}

					_recvState = 0;
				}
			}
			else
			{
				break;
			}
		}
	}
}