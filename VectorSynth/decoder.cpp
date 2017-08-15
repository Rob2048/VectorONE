#include "decoder.h"
#include <QDebug>
#include <QElapsedTimer>

using namespace cv;

Decoder::Decoder() :
	_calibBoardSize(9, 6)
{
	camThreshold = 0.0f;
	camSensitivity = 0.1f;
	camDistort = 0.54902f;
	drawGuides = false;
	drawMarkers = false;
	frameSkip = 0;
	_frameLimit = 0;
	dataRecvBytes = 0;
	_calibrating = false;
	findCalibrationSheet = false;
	drawUndistorted = false;
	
	_calibUndistortEnable = false;
	_calibCameraMatrix = Mat::eye(3, 3, CV_64F);
	_calibDistCoeffs = Mat::zeros(5, 1, CV_64F);
	projMat = cv::Mat::eye(3, 4, CV_64F);
	unitProjMat = cv::Mat::eye(3, 4, CV_64F);
	fundamentalMat = cv::Mat(3, 3, CV_64F);

	_calibDistCoeffs.at<double>(0) = -0.332945;
	_calibDistCoeffs.at<double>(1) = 0.12465;
	_calibDistCoeffs.at<double>(2) = 0.0020142;
	_calibDistCoeffs.at<double>(3) = 0.000755178;
	_calibDistCoeffs.at<double>(4) = -0.029228;

	_calibCameraMatrix.at<double>(0, 0) = 734.553;
	_calibCameraMatrix.at<double>(1, 0) = 0;
	_calibCameraMatrix.at<double>(2, 0) = 0;
	_calibCameraMatrix.at<double>(0, 1) = 0;
	_calibCameraMatrix.at<double>(1, 1) = 735.274;
	_calibCameraMatrix.at<double>(2, 1) = 0;
	_calibCameraMatrix.at<double>(0, 2) = 476.426;
	_calibCameraMatrix.at<double>(1, 2) = 403.402;
	_calibCameraMatrix.at<double>(2, 2) = 1;
	
	CalculateOptMat();

	_calibSquareSize = 25;
	_calibFramesSince = 0;

	newFrames = 0;
	destWidth = VID_W * 1;
	destHeight = VID_H * 1;

	avcodec_register_all();
	pkt = av_packet_alloc();

	memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	parser = av_parser_init(codec->id);
	if (!parser) {
		qDebug() << "parser not found";
		exit(1);
	}

	c = avcodec_alloc_context3(codec);
	if (!c) {
		qDebug() << "Could not allocate video codec context";
		exit(1);
	}

	/* For some codecs, such as msmpeg4 and mpeg4, width and height
	MUST be initialized there because this information is not
	available in the bitstream. */
	/* open it */

	c->width = VID_W;
	c->height = VID_H;
	c->framerate = { 80, 1 };	
	//c->flags |= CODEC_FLAG_LOW_DELAY;
	qDebug() << "Codec Delay" << c->flags;
	if (avcodec_open2(c, codec, NULL) < 0) {
		qDebug() << "Could not open codec";
		exit(1);
	}

	frame = av_frame_alloc();

	if (!frame) {
		qDebug() << "Could not allocate video frame";
		exit(1);
	}

	frameRGB = av_frame_alloc();

	int numBgrFrameBytes = avpicture_get_size(AV_PIX_FMT_GRAY8, destWidth, destHeight);

	uint8_t* bgrBuffer = (uint8_t *)av_malloc(numBgrFrameBytes);

	avpicture_fill((AVPicture *)frameRGB, bgrBuffer, AV_PIX_FMT_GRAY8, destWidth, destHeight);

	//sws = sws_getContext(VID_W, VID_H, AV_PIX_FMT_YUV420P, destWidth, destHeight, AV_PIX_FMT_RGB24, 0, 0, 0, 0);
	sws = sws_getContext(VID_W, VID_H, AV_PIX_FMT_YUV420P, destWidth, destHeight, AV_PIX_FMT_GRAY8, 0, 0, 0, 0);

	_initOpenCV();

	for (int i = 0; i < VID_W * VID_H; ++i)
		_frameMaskData[i] = 1;

	_frameMask = Mat(VID_H, VID_W, CV_8UC1);
	_frameMask = cv::Scalar(0);

	colMat = Mat(VID_H, VID_W, CV_8UC3);
}

Decoder::~Decoder()
{
	av_parser_close(parser);
	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);
}

void Decoder::_initOpenCV()
{
	SimpleBlobDetector::Params blobDetectorParams;
	blobDetectorParams.filterByCircularity = false;
	blobDetectorParams.filterByConvexity = false;
	blobDetectorParams.filterByInertia = false;

	blobDetectorParams.filterByArea = false;
	blobDetectorParams.minArea = 600;
	blobDetectorParams.maxArea = 60000;

	blobDetectorParams.filterByColor = true;
	blobDetectorParams.blobColor = 255;

	blobDetectorParams.minThreshold = 0;
	blobDetectorParams.maxThreshold = 110;
	blobDetectorParams.thresholdStep = 100;

	qDebug() << "Thresh" << blobDetectorParams.thresholdStep;

	blobDetector = cv::SimpleBlobDetector::create(blobDetectorParams);
}

void Decoder::CalculateOptMat()
{
	//optCamMat = getOptimalNewCameraMatrix(_calibCameraMatrix, _calibDistCoeffs, Size(VID_W, VID_H), 0.0, Size(VID_W, VID_H), NULL, false);
	optCamMat = getOptimalNewCameraMatrix(_calibCameraMatrix, _calibDistCoeffs, Size(VID_W, VID_H), 0.0, Size(VID_W, VID_H), NULL, true);

	double fovX, fovY, focalLength, aspectRatio;
	Point2d principalPoint;

	// Full: 3280x2464 - binned: 1640x1232
	// Mode 6 1280x720
	// Final Res: 1024x704
	// Sensor Space: 2000x1400
	// 3296 x 2512 = 5.095mm x 4.930mm
	// 3.092mm x 2.748mm
	// 2.240, 1680
	calibrationMatrixValues(_calibCameraMatrix, cv::Size(VID_W, VID_H), 3.092, 2.748, fovX, fovY, focalLength, principalPoint, aspectRatio);
	//calibrationMatrixValues(_calibCameraMatrix, cv::Size(VID_W, VID_H), 2.240, 1.680, fovX, fovY, focalLength, principalPoint, aspectRatio);

	qDebug() << "Cam - Fov: " << fovX << "x" << fovY << " Fl:" << focalLength << " Aspect:" << aspectRatio << " PP:" << principalPoint.x << "," << principalPoint.y;

	QVector4D hwPos = worldMat * QVector4D(0, 0, 0, 1);

	worldPos.setX(hwPos.x() / hwPos.w());
	worldPos.setY(hwPos.y() / hwPos.w());
	worldPos.setZ(hwPos.z() / hwPos.w());
}

void Decoder::_detectMarkers()
{
	//cvFrame = Mat(Height, Width, CV_8UC1, Scalar::all(128));

	// Blob detection.
	std::vector<KeyPoint> keypoints;
	blobDetector->detect(cvFrame, keypoints);

	//rectangle(colMat, Rect(10, 10, 100, 200), Scalar(0, 0, 255), 3);	
	//rectangle(cvFrame, Rect(10, 10, 100, 200), Scalar(0, 0, 255), 3);

	//colMat = Mat(VID_H, VID_W, CV_8UC3);

	//cv::Mat pointMat(keypoints.size(), 1, CV_32FC2);

	if (keypoints.size() == 0)
		return;

	/*
	for (int i = 0; i < keypoints.size(); ++i)
	{
		cv::Point2f fp = keypoints[i].pt;
		cv::Point2i ip(fp.x, fp.y);

		ip.x = ip.x << 8 | (int)((fp.x - ip.x) * 255.0f);
		ip.y = ip.y << 8 | (int)((fp.y - ip.y) * 255.0f);

		circle(colMat, ip, 3 << 8, Scalar(255, 128, 50), -1, CV_AA, 8);

		char strBuf[256];
		sprintf(strBuf, "%0.2f %0.2f %0.2f", keypoints[i].pt.x, keypoints[i].pt.y, keypoints[i].size);
		putText(colMat, strBuf, Point(keypoints[i].pt.x + 5, keypoints[i].pt.y - 5), 0, 0.3, Scalar(255, 255, 255));
	}
	//*/

	//*
	cv::Mat_<cv::Point2f> matPoint(1, keypoints.size());

	for (int i = 0; i < keypoints.size(); ++i)
	{
		matPoint(i) = keypoints[i].pt;
	}

	cv::Mat matOutPoints;

	//cv::undistortPoints(matPoint, matOutPoints, _calibCameraMatrix, _calibDistCoeffs, cv::noArray(), optCamMat);
	cv::undistortPoints(matPoint, matOutPoints, _calibCameraMatrix, _calibDistCoeffs, cv::noArray());

	qDebug() << CV_32FC3 << matOutPoints.type() << matOutPoints.size().width << matOutPoints.size().height;

	for (int i = 0; i < matOutPoints.size().width; ++i)
	{
		//circle(colMat, matOutPoints.at<cv::Point2f>(i), 4.0f, Scalar(255, 128, 50), -1, 8);

		cv::Point2f fp = matOutPoints.at<cv::Point2f>(i);
		cv::Point2i ip(fp.x, fp.y);

		ip.x = ip.x << 8 | (int)((fp.x - ip.x) * 255.0f);
		ip.y = ip.y << 8 | (int)((fp.y - ip.y) * 255.0f);

		circle(colMat, ip, 3 << 8, Scalar(255, 128, 50), -1, CV_AA, 8);
	}
	//*/
	
	//for (int k = 0; k < keypoints.size(); ++k)
	//{
		//circle(colMat, keypoints[k].pt, 4.0f, Scalar(255, 255, 255), -1, CV_AA);
		/*
		circle(colMat, keypoints[k].pt, keypoints[k].size * 0.5f + 4.0f, Scalar(255, 0, 0), 1, 8);
		drawMarker(colMat, keypoints[k].pt, Scalar(255, 0, 255), cv::MarkerTypes::MARKER_CROSS, 40, 1);

		char strBuf[256];
		sprintf(strBuf, "%0.2f %0.2f %0.2f", keypoints[k].pt.x, keypoints[k].pt.y, keypoints[k].size);
		putText(colMat, strBuf, Point(keypoints[k].pt.x + 5, keypoints[k].pt.y - 5), 0, 0.3, Scalar(255, 255, 255));
		*/

		//float s = keypoints[k].size;
		//rectangle(colMat, Rect(keypoints[k].pt.x - s * 0.5f, keypoints[k].pt.y - s * 0.5f, s, s), Scalar(0, 0, 255), 1, 8);
	//}
	
	//qDebug() << keypoints.size();
}

bool Decoder::DoDecode(uint8_t* Data, int Len)
{
	uint8_t *data;
	size_t   data_size;
	int ret;

	dataRecvBytes += Len;

	data = Data;
	data_size = Len;

	bool decodedFrame = false;
	while (data_size > 0)
	{
		ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

		if (ret < 0)
		{
			qDebug() << "Error while parsing";
			exit(1);
		}

		data += ret;
		data_size -= ret;

		if (pkt->size)
		{
			if (_Decode())
			{
				ProcessFrameLive();
				decodedFrame = true;
			}
		}
	}

	return decodedFrame;
}

bool Decoder::DoDecodeSingleFrame(uint8_t* Data, int Len, int* Consumed)
{
	uint8_t *data;
	size_t   data_size;
	int ret;

	data = Data;
	data_size = Len;
	
	bool decodedFrame = false;
	while (true)
	{
		//qDebug() << "Decode" << data_size;
		ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		//qDebug() << "Ret" << ret << pkt->size;

		if (ret < 0)
		{
			qDebug() << "Error while parsing";
			exit(1);
		}

		data += ret;
		data_size -= ret;
		*Consumed = *Consumed + ret;

		if (pkt->size)
		{
			//qDebug() << "Got Packet";
			if (_Decode())
			{
				decodedFrame = true;				
				return true;
			}
		}
		else
		{
			continue;
		}

		if (data_size <= 0)
			break;
	}

	return false;
}

void Decoder::_CreateFrameGray()
{
	//sws_scale(sws, frame->data, frame->linesize, 0, VID_H, frameRGB->data, frameRGB->linesize);
	
	QElapsedTimer t;
	t.start();

	int lineSize = frame->linesize[0];
	uint8_t* lumBuffer = frame->data[0];

	// 1024 x 704

	for (int iY = 0; iY < VID_H; ++iY)
	{
		for (int iX = 0; iX < VID_W; ++iX)
		{
			//uint8_t lum = frameRGB->data[0][iY * VID_W + iX];
			
			// NOTE: H264 YUV from PI after decode seems to be in the range of 16 - 254.
			// NOTE: Ringing undershoots, and overshoots?
			uint8_t lum = lumBuffer[iY * lineSize + iX];
			float l = ((float)lum - 16.0f) / (255.0f - 16.0f);

			if (l < camThreshold)
				l = 0.0f;

			l = l * camSensitivity * 10;
			
			if (l < 0.0f) l = 0.0f;
			else if (l > 1.0f) l = 1.0f;

			int dataIndex = iY * VID_W + iX;
			_postFrameData[dataIndex] = (uint8_t)(l * 255) * _frameMaskData[dataIndex];
		}
	}
	
#if 0
	float invByteRange = 1.0f / (255.0f - 16.0f);

	__m128i v16 = _mm_set1_epi8(16);
	//__m256i v16 = _mm256_set1_epi8(16);

	//qDebug() << "Aligned Data:" << ((unsigned long)(_alignedData) & 15);

#if 0
	for (int iY = 0; iY < VID_H; ++iY)
	{
		for (int iX = 0; iX < 992; iX += 32)
		{
			//__m128i lum = _mm_loadu_si128((__m128i const*)&lumBuffer[iY * lineSize + iX]);
			//__m128i lumSub16 = _mm_subs_epu8(lum, v16);
			//_mm_store_si128((__m128i*)(&_alignedData[iY * 1024 + iX]), lumSub16);

			__m256i lum = _mm256_loadu_si256((__m256i const*)&lumBuffer[iY * lineSize + iX]);
			__m256i lumSub16 = _mm256_subs_epu8(lum, v16);
			_mm256_store_si256((__m256i*)(&_alignedData[iY * 1024 + iX]), lumSub16);

			/*
			uint8_t lum0 = lumBuffer[iY * lineSize + iX + 0];
			uint8_t lum1 = lumBuffer[iY * lineSize + iX + 1];
			uint8_t lum2 = lumBuffer[iY * lineSize + iX + 2];
			uint8_t lum3 = lumBuffer[iY * lineSize + iX + 3];

			float l0 = ((float)lum0 - 16.0f) * invByteRange;
			float l1 = ((float)lum1 - 16.0f) * invByteRange;
			float l2 = ((float)lum2 - 16.0f) * invByteRange;
			float l3 = ((float)lum3 - 16.0f) * invByteRange;

			if (l0 < 0.0f) l0 = 0.0f;	else if (l0 > 1.0f) l0 = 1.0f;
			if (l1 < 0.0f) l1 = 0.0f;	else if (l1 > 1.0f) l1 = 1.0f;
			if (l2 < 0.0f) l2 = 0.0f;	else if (l2 > 1.0f) l2 = 1.0f;
			if (l3 < 0.0f) l3 = 0.0f;	else if (l3 > 1.0f) l3 = 1.0f;

			int dataIndex = iY * VID_W + iX + 0;
			_postFrameData[dataIndex] = (uint8_t)(l0 * 255) * _frameMaskData[dataIndex];
			dataIndex = iY * VID_W + iX + 1;
			_postFrameData[dataIndex] = (uint8_t)(l1 * 255) * _frameMaskData[dataIndex];
			dataIndex = iY * VID_W + iX + 2;
			_postFrameData[dataIndex] = (uint8_t)(l2 * 255) * _frameMaskData[dataIndex];
			dataIndex = iY * VID_W + iX + 3;
			_postFrameData[dataIndex] = (uint8_t)(l3 * 255) * _frameMaskData[dataIndex];
			*/
		}
	}
#endif

	for (int iX = 0; iX < 992 * VID_H; iX += 16)
	{
		__m128i lum = _mm_loadu_si128((__m128i const*)&lumBuffer[iX]);
		__m128i lumSub16 = _mm_subs_epu8(lum, v16);
		_mm_store_si128((__m128i*)(&_alignedData[iX]), lumSub16);
	}

	float t1 = (t.nsecsElapsed() / 1000) / 1000.0;
	//qDebug() << "Gray Pass1" << t1;

	for (int iY = 0; iY < VID_H; ++iY)
	{
		for (int iX = 0; iX < VID_W; ++iX)
		{
			_postFrameData[iY * VID_W + iX] = _alignedData[iY * 1024 + iX];
		}
	}
#endif

	cvFrame = Mat(VID_H, VID_W, CV_8UC1, _postFrameData);
}

void Decoder::ProcessFrameLive()
{
	if (_frameLimit++ < frameSkip)
		return;

	_frameLimit = 0;
	
	_CreateFrameGray();

	if (drawUndistorted)
	{
		_undistort();
		cv::cvtColor(undistortMat, colMat, cv::COLOR_GRAY2RGB);
	}
	else
	{
		cv::cvtColor(cvFrame, colMat, cv::COLOR_GRAY2RGB);
	}

	if (findCalibrationSheet)
	{
		if (!_calibrating)
		{
			qDebug() << "Calibrating!";
			calibMutex.lock();
			_calibrating = true;
			_calibImageCount = 0;
			_calibImagePoints.clear();
			calibMutex.unlock();
		}

		_findCalibrationSheet();
	}
	else
	{
		if (_calibrating)
		{
			calibMutex.lock();
			_calibrating = false;
			calibMutex.unlock();
		}
	}
}

void Decoder::ProcessFrame()
{
	_CreateFrameGray();

	/*
	if (drawUndistorted)
	{
		_undistort();
	}
	*/

	//_undistort();
	//cv::cvtColor(undistortMat, colMat, cv::COLOR_GRAY2RGB);
	cv::cvtColor(cvFrame, colMat, cv::COLOR_GRAY2RGB);

	/*
	std::vector<Point3f> worldPoints;
	worldPoints.push_back(Point3f(0, 0, 0));

	std::vector<Point2f> imgPoints;

	cv::projectPoints(worldPoints, Mat(Point3f(0, 0, 0)), Mat(Point3f(0, 0, 0)), optCamMat, _calibDistCoeffs, imgPoints);

	for (int i = 0; i < imgPoints.size(); ++i)
	{
		cv::circle(colMat, imgPoints[i], 4, Scalar(255, 0, 0), -1, CV_AA);
	}
	*/

	//cv::circle(colMat, Point2f(500, 500), 4, Scalar(255, 0, 0), 1, 8);

	return;
}

QList<QVector2D> Decoder::ProcessFrameMarkers()
{	
	QList<QVector2D> markers;

	if (blankFrame)
		return markers;

	_CreateFrameGray();

	QElapsedTimer t;
	t.start();
	// Blob detection.
	std::vector<KeyPoint> keypoints;
	blobDetector->detect(cvFrame, keypoints);

	qDebug() << "CV:" << ((t.nsecsElapsed() / 1000) / 1000.0) << "ms";
	
	if (keypoints.size() == 0)
		return markers;

	markers.reserve(keypoints.size());

	for (int i = 0; i < keypoints.size(); ++i)
	{
		markers.push_back(QVector2D(keypoints[i].pt.x, keypoints[i].pt.y));
	}

	/*
	cv::Mat_<cv::Point2f> matPoint(1, keypoints.size());

	for (int i = 0; i < keypoints.size(); ++i)
	{
		matPoint(i) = keypoints[i].pt;
	}

	cv::Mat matOutPoints;
	cv::undistortPoints(matPoint, matOutPoints, _calibCameraMatrix, _calibDistCoeffs, cv::noArray(), optCamMat);

	for (int i = 0; i < matOutPoints.size().width; ++i)
	{
		Point2f p = matOutPoints.at<cv::Point2f>(i);

		if (p.x >= 0 & p.x < VID_W && p.y >= 0 && p.y < VID_H)
		markers.push_back(QVector2D(p.x, p.y));
	}
	*/

	return markers;
}

struct blob
{
	float minX;
	float minY;
	float maxX;
	float maxY;
};

float distSq(float X1, float Y1, float X2, float Y2)
{
	return (X2 - X1) * (X2 - X1) + (Y2 - Y1) * (Y2 - Y1);
}

QList<NewMarker> Decoder::ProcessFrameNewMarkers()
{
	QList<NewMarker> markers;

	if (blankFrame)
		return markers;

	//_CreateFrameGray();

	QElapsedTimer t;
	t.start();

	blob 	blobs[1024];
	int 	blobCount = 0;

	const int width = VID_W;
	const int height = VID_H;
	const int pixelCount = width * height;

	//uint8_t buffer[pixelCount];

	int brightPixels = 0;

	blobCount = 0;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			//if (buffer[y * width + x] > 95 && buffer[y * width + x] < 189)
			if (_postFrameData[y * width + x] > 20)
			{
				++brightPixels;

				bool blobFound = false;

				// TODO: Acceleration structure for closest blob search?

				for (int b = 0; b < blobCount; ++b)
				{
					blob* cb = blobs + b;

					float cx = (cb->minX + cb->maxX) / 2;
					float cy = (cb->minY + cb->maxY) / 2;
					float d = distSq(x, y, cx, cy);

					if (d < 8 * 8)
					{
						if (x < cb->minX) cb->minX = x;
						if (y < cb->minY) cb->minY = y;
						if (x > cb->maxX) cb->maxX = x;
						if (y > cb->maxY) cb->maxY = y;

						blobFound = true;
						break;
					}
				}

				if (!blobFound)
				{
					blobs[blobCount].minX = x;
					blobs[blobCount].minY = y;
					blobs[blobCount].maxX = x;
					blobs[blobCount].maxY = y;

					++blobCount;
				}
			}
		}
	}

	qDebug() << "Custom:" << ((t.nsecsElapsed() / 1000) / 1000.0) << "ms";

	for (int i = 0; i < blobCount; ++i)
	{
		NewMarker m = {};
		m.pos = QVector2D((blobs[i].maxX - blobs[i].minX) * 0.5f + blobs[i].minX, (blobs[i].maxY - blobs[i].minY) * 0.5f + blobs[i].minY);
		m.bounds = QVector4D(blobs[i].minX, blobs[i].minY, blobs[i].maxX, blobs[i].maxY);


		m.pos = QVector2D(0, 0);
		float totalWeight = 0.0f;
		// Count total weight
		for (int y = blobs[i].minY; y < blobs[i].maxY + 1; ++y)
		{
			for (int x = blobs[i].minX; x < blobs[i].maxX + 1; ++x)
			{
				//if (buffer[y * width + x] > 95 && buffer[y * width + x] < 189)
				if (_postFrameData[y * width + x] > 20)
				{
					totalWeight += _postFrameData[y * width + x];
				}
			}
		}

		for (int y = blobs[i].minY; y < blobs[i].maxY + 1; ++y)
		{
			for (int x = blobs[i].minX; x < blobs[i].maxX + 1; ++x)
			{
				//if (buffer[y * width + x] > 95 && buffer[y * width + x] < 189)
				if (_postFrameData[y * width + x] > 20)
				{
					float pixelV = _postFrameData[y * width + x];

					m.pos += QVector2D(x, y) * (pixelV / totalWeight);
				}
			}
		}

		markers.push_back(m);
	}

	// Blob detection.
	/*
	std::vector<KeyPoint> keypoints;
	blobDetector->detect(cvFrame, keypoints);
	
	if (keypoints.size() == 0)
		return markers;

	markers.reserve(keypoints.size());

	for (int i = 0; i < keypoints.size(); ++i)
	{
		NewMarker m = {};
		m.pos = QVector2D(keypoints[i].pt.x, keypoints[i].pt.y);
		markers.push_back(m);
	}
	*/

	//*
	// Undistort markers.
	cv::Mat_<cv::Point2f> matPoint(1, markers.size());
	for (int i = 0; i < markers.size(); ++i)
		matPoint(i) = Point2f(markers[i].pos.x(), markers[i].pos.y());
	
	cv::Mat matOutPoints;
	cv::undistortPoints(matPoint, matOutPoints, _calibCameraMatrix, _calibDistCoeffs, cv::noArray(), optCamMat);
	
	markers.clear();

	// Clip markers.
	for (int i = 0; i < matOutPoints.size().width; ++i)
	{
		Point2f p = matOutPoints.at<cv::Point2f>(i);

		if (p.x >= 0 && p.x < VID_W && p.y >= 0 && p.y < VID_H)
		{
			NewMarker m = {};
			m.pos = QVector2D(p.x, p.y);
			markers.push_back(m);
		}
	}
	//*/

	return markers;
}

bool Decoder::_Decode()
{
	int ret;
	ret = avcodec_send_packet(c, pkt);
	if (ret < 0) 
	{
		qDebug() << "Error sending a packet for decoding";
		exit(1);
	}

	bool decodedFrame = false;
	
	// TODO: Check if we really only ever process a single frame here.
	//while (ret >= 0)
	{	
		ret = avcodec_receive_frame(c, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			return decodedFrame;
		}
		else if (ret < 0)
		{
			qDebug() << "Error during decoding";
			exit(1);
		}
		
		++_parsedFrames;
		++newFrames;

		decodedFrame = true;
	}

	return decodedFrame;
}

void Decoder::TransferFrameToBuffer(uint8_t* Data)
{
	//memcpy(Data, _postFrameData, sizeof(_postFrameData));
	memcpy(Data, colMat.data, VID_W * VID_H * 3);
}

void Decoder::_detectValibSheet()
{
	std::vector<Point2f> pointBuf;

	if (findChessboardCorners(cvFrame, _calibBoardSize, pointBuf, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE))
	{
		cornerSubPix(cvFrame, pointBuf, Size(11, 11), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
		_calibImagePoints.push_back(pointBuf);
		++_calibImageCount;

		drawChessboardCorners(colMat, _calibBoardSize, Mat(pointBuf), true);
	}
	else
	{
		//std::cout << "No Board Found!\n";
	}
}

void Decoder::_undistort()
{
	//cvFrame = umat.clone(); Mat umat;
	undistort(cvFrame, undistortMat, _calibCameraMatrix, _calibDistCoeffs, optCamMat);
	//undistort(cvFrame, undistortMat, _calibCameraMatrix, _calibDistCoeffs);
}

void Decoder::_findCalibrationSheet()
{
	SimpleBlobDetector::Params blobDetectorParams;

	blobDetectorParams.filterByCircularity = false;
	blobDetectorParams.filterByConvexity = false;
	blobDetectorParams.filterByInertia = false;

	blobDetectorParams.filterByArea = true;
	blobDetectorParams.minArea = 50;
	blobDetectorParams.maxArea = 2000;

	blobDetectorParams.filterByColor = true;
	blobDetectorParams.blobColor = 0;

	blobDetectorParams.minThreshold = 80;
	blobDetectorParams.maxThreshold = 255;

	Ptr<FeatureDetector> blobDetector = SimpleBlobDetector::create(blobDetectorParams);

	std::vector<Point2f> pointBuf;
	bool found = findCirclesGrid(cvFrame, Size(4, 11), pointBuf, CALIB_CB_ASYMMETRIC_GRID, blobDetector);

	if (found)
	{
		_calibImagePoints.push_back(pointBuf);
		++_calibImageCount;
		drawChessboardCorners(colMat, Size(4, 11), Mat(pointBuf), found);
	}
	else
	{
		std::vector<KeyPoint> keypoints;
		blobDetector->detect(cvFrame, keypoints);

		for (int k = 0; k < keypoints.size(); ++k)
		{
			circle(colMat, keypoints[k].pt, keypoints[k].size * 0.5f + 4.0f, Scalar(255, 0, 0), 1, 8);
			drawMarker(colMat, keypoints[k].pt, Scalar(255, 0, 255), cv::MarkerTypes::MARKER_CROSS, 40, 1);
		}
	}
}

void Decoder::GenerateMask()
{	
	int kSize = 5;

	for (int iY = 0; iY < VID_H; ++iY)
	{
		for (int iX = 0; iX < VID_W; ++iX)
		{
			int mask = 1;

			int kXStart = iX - kSize;
			int kXEnd = iX + kSize;
			int kYStart = iY - kSize;
			int kYEnd = iY + kSize;

			if (kXStart < 0) kXStart = 0;
			if (kXEnd > VID_W) kXEnd = VID_W;
			if (kYStart < 0) kYStart = 0;
			if (kYEnd > VID_H) kYEnd = VID_H;

			for (int kY = kYStart; kY < kYEnd; ++kY)
			{
				for (int kX = kXStart; kX < kXEnd; ++kX)
				{
					if (_postFrameData[kY * VID_W + kX] > 0)
					{
						mask = 0;
						break;
					}
				}

				if (mask == 0)
					break;
			}

			_frameMaskData[iY * VID_W + iX] = mask;
			_frameMask.at<uint8_t>(iY, iX) = 255 * (1 - mask);
		}
	}
}

void Decoder::ShowBlankFrame()
{
	colMat = cv::Scalar(128, 0, 0);
}

/*
void MainWindow::OnCalibrationStopClick()
{
	_calibEnable = false;

	vector<vector<Point3f>> objectPoints(1);

	objectPoints[0].clear();
	for (int i = 0; i < _calibBoardSize.height; ++i)
		for (int j = 0; j < _calibBoardSize.width; ++j)
			objectPoints[0].push_back(Point3f(float(j*_calibSquareSize), float(i*_calibSquareSize), 0));

	objectPoints.resize(_calibImagePoints.size(), objectPoints[0]);

	Size imageSize(VID_W, VID_H);

	double rms = calibrateCamera(objectPoints, _calibImagePoints, imageSize, _calibCameraMatrix, _calibDistCoeffs, _calibRvecs, _calivTvecs, CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5); // CV_CALIB_FIX_PRINCIPAL_POINT
	//double rms = fisheye::calibrate(objectPoints, _calibImagePoints, imageSize, _calibCameraMatrix, _calibDistCoeffs, _calibRvecs, _calivTvecs,  CV_CALIB_FIX_K4 | CV_CALIB_FIX_K5); // CV_CALIB_FIX_PRINCIPAL_POINT
	bool ok = checkRange(_calibCameraMatrix) && checkRange(_calibDistCoeffs);

	qDebug() << "Re-projection error reported by calibrateCamera: " << rms << " " << ok;

	//std:vector<float> perViewErrors;
	//double reprojError = computeReprojectionErrors(objectPoints, imagePoints, rvecs, tvecs, cameraMatrix, distCoeffs, perViewErrors);
	//cout << "Reprojection Error: " << reprojError << "\n";

	double fovX, fovY, focalLength, aspectRatio;
	Point2d principalPoint;

	// 1640 × 1232: 3.674 x 2.760	
	calibrationMatrixValues(_calibCameraMatrix, imageSize, 3.674, 2.066, fovX, fovY, focalLength, principalPoint, aspectRatio);

	qDebug() << "Cam " << fovX << " " << fovY << " " << focalLength << " " << aspectRatio << " " << principalPoint.x << "," << principalPoint.y;
	
	ui->lblCalibImageCount->setText(QString("Images: ") + QString::number(_calibImageCount));
	ui->lblCalibError->setText(QString("Error: ") + QString::number(rms));
}
*/