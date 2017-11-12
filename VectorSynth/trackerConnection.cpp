#include "trackerConnection.h"
#include "serverThreadWorker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

void TrackerProperties::Populate(QByteArray JSON)
{
	QJsonObject trackerObj = QJsonDocument::fromJson(JSON).object();

	name = trackerObj["name"].toString();
	exposure = trackerObj["exposure"].toInt();
	iso = trackerObj["iso"].toInt();

	QJsonArray jsonIntrinMat = trackerObj["intrinsic"].toObject()["camera"].toArray();
	QJsonArray jsonIntrinDist = trackerObj["intrinsic"].toObject()["distortion"].toArray();
	QJsonArray jsonExtrinProj = trackerObj["extrinsic"].toObject()["pose"].toArray();

	distCoefs = cv::Mat::zeros(5, 1, CV_64F);
	distCoefs.at<double>(0) = jsonIntrinDist[0].toDouble();
	distCoefs.at<double>(1) = jsonIntrinDist[1].toDouble();
	distCoefs.at<double>(2) = jsonIntrinDist[2].toDouble();
	distCoefs.at<double>(3) = jsonIntrinDist[3].toDouble();
	distCoefs.at<double>(4) = jsonIntrinDist[4].toDouble();

	camMat = cv::Mat::eye(3, 3, CV_64F);
	camMat.at<double>(0, 0) = jsonIntrinMat[0].toDouble();
	camMat.at<double>(1, 0) = jsonIntrinMat[1].toDouble();
	camMat.at<double>(2, 0) = jsonIntrinMat[2].toDouble();
	camMat.at<double>(0, 1) = jsonIntrinMat[3].toDouble();
	camMat.at<double>(1, 1) = jsonIntrinMat[4].toDouble();
	camMat.at<double>(2, 1) = jsonIntrinMat[5].toDouble();
	camMat.at<double>(0, 2) = jsonIntrinMat[6].toDouble();
	camMat.at<double>(1, 2) = jsonIntrinMat[7].toDouble();
	camMat.at<double>(2, 2) = jsonIntrinMat[8].toDouble();

	rtMat = cv::Mat::eye(3, 4, CV_64F);
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			rtMat.at<double>(iX, iY) = jsonExtrinProj[iY * 3 + iX].toDouble();
		}
	}

	QString maskStr = trackerObj["mask"].toString();
	for (int i = 0; i < maskStr.size(); ++i)
	{
		maskData[i] = maskStr[i].toLatin1() - 48;
	}
}

QByteArray TrackerProperties::GetJSON(bool Pretty)
{
	QJsonObject jsonObj;
	jsonObj["name"] = name;
	
	QString maskString;
	for (int i = 0; i < sizeof(maskData); ++i)
	{
		maskString += maskData[i] + 48;
	}
	jsonObj["mask"] = maskString;

	QJsonArray jsonIntrinMat;
	jsonIntrinMat.append(camMat.at<double>(0, 0));
	jsonIntrinMat.append(camMat.at<double>(1, 0));
	jsonIntrinMat.append(camMat.at<double>(2, 0));
	jsonIntrinMat.append(camMat.at<double>(0, 1));
	jsonIntrinMat.append(camMat.at<double>(1, 1));
	jsonIntrinMat.append(camMat.at<double>(2, 1));
	jsonIntrinMat.append(camMat.at<double>(0, 2));
	jsonIntrinMat.append(camMat.at<double>(1, 2));
	jsonIntrinMat.append(camMat.at<double>(2, 2));

	QJsonArray jsonIntrinDist;
	jsonIntrinDist.append(distCoefs.at<double>(0));
	jsonIntrinDist.append(distCoefs.at<double>(1));
	jsonIntrinDist.append(distCoefs.at<double>(2));
	jsonIntrinDist.append(distCoefs.at<double>(3));
	jsonIntrinDist.append(distCoefs.at<double>(4));

	QJsonObject jsonIntrin;
	jsonIntrin["camera"] = jsonIntrinMat;
	jsonIntrin["distortion"] = jsonIntrinDist;

	jsonObj["intrinsic"] = jsonIntrin;

	QJsonArray pose;
	for (int iY = 0; iY < 4; ++iY)
	{
		for (int iX = 0; iX < 3; ++iX)
		{
			pose.append(rtMat.at<double>(iX, iY));
		}
	}

	QJsonObject jsonExtrin;
	jsonExtrin["pose"] = pose;

	jsonObj["extrinsic"] = jsonExtrin;

	QJsonDocument jsonDoc(jsonObj);
	QByteArray jsonBytes;
	
	if (Pretty)
		jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Indented);
	else
		jsonBytes = jsonDoc.toJson(QJsonDocument::JsonFormat::Compact);

	return jsonBytes;
}

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
	memset(props.maskData, 1, sizeof(props.maskData));
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

void TrackerConnection::StartRecording(QString TakeName)
{
	StopRecording();

	if (streamMode == 1 || streamMode == 2)
	{
		QByteArray jsonBytes = props.GetJSON(true);
		QFile file("project/" + TakeName + "/" + QString::number(serial) + ".tracker");

		if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			qDebug() << "Tracker: Save file failed";
			return;
		}

		file.write(jsonBytes);
		file.close();
	}

	char fileName[256];

	if (streamMode == 1)
	{
		sprintf(fileName, "project\\%s\\%u.trakvid", TakeName.toLatin1().data(), serial);
		recordFile = fopen(fileName, "wb");
		recording = true;
		_gotDataFrame = false;
	}
	else if (streamMode == 2)
	{
		sprintf(fileName, "project\\%s\\%u.trakblobs", TakeName.toLatin1().data(), serial);
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

void TrackerConnection::UpdateProperties(QByteArray Props)
{
	props.Populate(Props);
	QString cmd = "sp," + Props + "\n";

	qDebug() << "Update:" << cmd;
	socket->write(cmd.toUtf8());
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
			if (socket->bytesAvailable() >= 12)
			{
				qDebug() << "Got GI";

				socket->read((char*)_recvBuffer, 12);
				version = *(int*)&_recvBuffer[0];
				serial = *(int*)&_recvBuffer[4];
				_recvFrameSize = *(int*)&_recvBuffer[8];
				_recvState = 4;
				_recvPacketId = 0;
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
			
			//*
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
			//*/
			
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
		else if (_recvState == 4)
		{
			if (socket->bytesAvailable() >= _recvFrameSize)
			{
				socket->read((char*)_recvBuffer, _recvFrameSize);

				QByteArray jsonData((char*)_recvBuffer, _recvFrameSize);
				//qDebug() << _recvFrameSize << QString::fromUtf8(jsonData);

				props.Populate(jsonData);

				/*
				props.name = "Unnamed";
				memcpy(props.maskData, _recvBuffer + 8, sizeof(props.maskData));

				for (int i = 0; i < sizeof(props.maskData); ++i)
				{
					props.maskData[i] -= 48;
				}
				*/

				accepted = true;
				_recvState = 0;

				emit OnInfoUpdate(this);
			}
			else
			{
				break;
			}
		}
	}
}