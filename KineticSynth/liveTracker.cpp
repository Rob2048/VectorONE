#include "liveTracker.h"

LiveTracker::LiveTracker(int Id, QTcpSocket* Socket, QElapsedTimer* MasterTimer, QObject* Parent) :
	QObject(Parent),
	id(Id),
	accepted(false),
	streaming(false),
	recording(false),
	recordFile(0),
	socket(Socket),
	masterTimer(MasterTimer),
	_recvLength(0),
	_recvState(0),
	_recvPacketId(0)
{
	decoder = new Decoder();

	connect(Socket, &QTcpSocket::readyRead, this, &LiveTracker::OnTcpSocketReadyRead);
	connect(Socket, &QTcpSocket::disconnected, this, &LiveTracker::OnTcpSocketDisconnected);

	//Socket->write("gi\n");
}

LiveTracker::~LiveTracker()
{
	StopRecording();
}

void LiveTracker::StartRecording()
{
	StopRecording();
	char fileName[256];
	sprintf(fileName, "project\\take\\%d.trakvid", id);
	recordFile = fopen(fileName, "wb");
	recording = true;
}

void LiveTracker::StopRecording()
{
	if (recordFile)
		fclose(recordFile);

	recordFile = 0;
	recording = false;
}

void LiveTracker::RecordData(uint8_t* Data, int Len)
{
	fwrite(Data, Len, 1, recordFile);
	//fflush(recordFile);
}

void LiveTracker::OnTcpSocketDisconnected()
{
	qDebug() << "Client" << id << "Disconnected";
	emit OnDisconnected(this);
}

void LiveTracker::OnTcpSocketReadyRead()
{
	while (true)
	{
		if (socket->bytesAvailable() == 0)
			break;

		if (_recvState == 0)
		{
			_recvLength = 0;
			_recvPacketId = 0;

			if (socket->bytesAvailable() >= 4)
			{
				socket->read((char*)_recvBuffer, 4);
				_recvPacketId = _recvBuffer[0];
				_recvState = 1;
				//qDebug() << "Got PacketId" << _recvPacketId;

				if (_recvPacketId == 1)
				{
					int masterTime = masterTimer->nsecsElapsed() / 1000;

					QByteArray sendBytes(4, Qt::Initialization::Uninitialized);
					QDataStream stream(&sendBytes, QIODevice::OpenModeFlag::WriteOnly);
					stream << masterTime;
					socket->write(sendBytes);

					qDebug() << "Time Sync" << time << masterTime << sendBytes.length();

					_recvState = 0;
				}
			}
		}
		else if (_recvPacketId == 2)
		{
			if (socket->bytesAvailable() >= 4)
			{
				socket->read((char*)_recvBuffer, 4);
				int time = _recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];
				int masterTime = masterTimer->nsecsElapsed() / 1000;

				qDebug() << "Time Check" << (masterTime - time);

				_recvState = 0;
			}
		}
		else if (_recvPacketId == 3)
		{
			if (socket->bytesAvailable() >= 12)
			{
				socket->read((char*)_recvBuffer, 12);
				_recvFrameSize = _recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];

				int frameType = _recvBuffer[7] << 24 | _recvBuffer[6] << 16 | _recvBuffer[5] << 8 | _recvBuffer[4];
				int frameTime = _recvBuffer[11] << 24 | _recvBuffer[10] << 16 | _recvBuffer[9] << 8 | _recvBuffer[8];

				if (recording)
				{
					RecordData(_recvBuffer, 12);
				}

				_recvState = 2;
				_recvPacketId = 0;
			}
		}
		else if (_recvPacketId == 4)
		{
			if (socket->bytesAvailable() >= 4)
			{
				socket->read((char*)_recvBuffer, 4);
				int version = _recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];
				_recvState = 0;
			}
		}
		else if (_recvPacketId == 5)
		{
			if (socket->bytesAvailable() >= 4)
			{
				socket->read((char*)_recvBuffer, 4);
				_recvFrameSize = _recvFrameSize = _recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];
				qDebug() << "Got" << _recvFrameSize;

				if (recording)
				{
					//RecordData(_recvBuffer, 12);
				}

				_recvState = 3;
				_recvPacketId = 0;
			}
		}
		else if (_recvState == 2)
		{
			int readBytes = socket->read((char*)_recvBuffer, _recvFrameSize);
			_recvFrameSize -= readBytes;

			if (recording)
			{
				RecordData(_recvBuffer, readBytes);
			}
			else if (streaming)
			{
				if (decoder->DoDecode(_recvBuffer, readBytes))
				{
					if (bufferMutex.tryLock())
					{
						decoder->TransferFrameToBuffer(postFrameData);
						newFrames = decoder->newFrames;
						decoder->newFrames = 0;
						bufferMutex.unlock();
						emit OnNewFrame(this);
					}
				}
			}

			if (_recvFrameSize == 0)
				_recvState = 0;
		}
		else if (_recvState == 3)
		{
			qDebug() << "Test Avail" << socket->bytesAvailable();
			if (socket->bytesAvailable() >= _recvFrameSize)
			{
				qDebug() << "Avail" << _recvFrameSize;
				int readBytes = socket->read((char*)_recvBuffer, _recvFrameSize);

				if (recording)
				{
					//RecordData(_recvBuffer, readBytes);
				}
				else if (streaming)
				{
					if (bufferMutex.tryLock())
					{
						memcpy(markerData, _recvBuffer, _recvFrameSize);
						qDebug() << "R" << _recvBuffer[0] << _recvBuffer[1] << _recvBuffer[2] << _recvBuffer[3];
						markerDataSize = _recvFrameSize;

						bufferMutex.unlock();
						emit OnNewMarkersFrame(this);
					}
				}

				_recvState = 0;
			}
		}
	}
}