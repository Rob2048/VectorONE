#include "trackerConnection.h"

TrackerConnection::TrackerConnection(int Id, QTcpSocket* Socket, QElapsedTimer* MasterTimer, QObject* Parent) :
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
	sprintf(fileName, "project\\take\\%u.mask", serial);
	FILE* maskFile = fopen(fileName, "wb");
	fwrite(maskData, sizeof(maskData), 1, maskFile);
	fclose(maskFile);
	
	sprintf(fileName, "project\\take\\%u.trakvid", serial);
	recordFile = fopen(fileName, "wb");
	recording = true;
	_gotDataFrame = false;
}

void TrackerConnection::StopRecording()
{
	if (recordFile)
		fclose(recordFile);

	recordFile = 0;
	recording = false;
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
		
		//if (socket->bytesAvailable() == 0)
			//break;

		//qDebug() << "Bytes" << socket->bytesAvailable() << _recvPacketId;

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
			else
			{
				break;
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
				
				_recvFrameSize = _recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];
				int frameType = _recvBuffer[7] << 24 | _recvBuffer[6] << 16 | _recvBuffer[5] << 8 | _recvBuffer[4];
				int tempAvgMasterOffset = _recvBuffer[11] << 24 | _recvBuffer[10] << 16 | _recvBuffer[9] << 8 | _recvBuffer[8];
				avgMasterOffset = *(float*)&tempAvgMasterOffset;
				latestFrameId = _recvBuffer[19] << 56 | _recvBuffer[18] << 48 | _recvBuffer[17] << 40 | _recvBuffer[16] << 32 | _recvBuffer[15] << 24 | _recvBuffer[14] << 16 | _recvBuffer[13] << 8 | _recvBuffer[12];

				if (frameType == 2)
					_gotDataFrame = true;

				//qDebug() << "Frame:" << _recvFrameSize << frameType << latestFrameId << avgMasterOffset;

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

			if (socket->bytesAvailable() >= 4)
			{
				socket->read((char*)_recvBuffer, 4);
				_recvFrameSize = _recvFrameSize = _recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];
				//qDebug() << "Frame Size:" << _recvFrameSize;

				if (recording)
				{
					//RecordData(_recvBuffer, 12);
				}

				_recvState = 3;
				_recvPacketId = 0;
				_recvLength = 0;
			}
			else
			{
				break;
			}
		}
		else if (_recvPacketId == 52)
		{
			if (socket->bytesAvailable() >= 8)
			{
				int64_t masterTime = masterTimer->nsecsElapsed() / 1000;
				socket->read((char*)_recvBuffer, 8);				
				int64_t localTime  = _recvBuffer[7] << 56 | _recvBuffer[6] << 48 | _recvBuffer[5] << 40 | _recvBuffer[4] << 32 |
									_recvBuffer[3] << 24 | _recvBuffer[2] << 16 | _recvBuffer[1] << 8 | _recvBuffer[0];
				
				QString cmd = "hb," + QString::number(localTime) + "," + QString::number(masterTime) + "\n";
				QByteArray packet(cmd.toLatin1());
				socket->write(packet);

				//qDebug() << "Got ping " << localTime << masterTime << (localTime - masterTime);
				qDebug() << localTime << masterTime << (localTime - masterTime);

				_recvState = 0;
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
			if (socket->bytesAvailable() > 0 && _recvLength < _recvFrameSize)
			{
				int readBytes = socket->read((char*)_recvBuffer + _recvLength, _recvFrameSize - _recvLength);
				_recvLength += readBytes;

				//qDebug() << "Total" << _recvLength << "/" << _recvFrameSize;

				if (_recvLength == _recvFrameSize)
				{
					++decoder->newFrames;
					decoder->dataRecvBytes += _recvFrameSize;

					if (recording)
					{
						//RecordData(_recvBuffer, readBytes);
					}
					else if (streaming)
					{
						Lock();
						memcpy(markerData, _recvBuffer, _recvFrameSize);
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