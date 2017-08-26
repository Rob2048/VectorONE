#pragma once

#include <Vector>

#include <QMutex>
#include <QVector2D>
#include <QMatrix4x4>

#include <opencv2\core.hpp>
#include <opencv2\calib3d\calib3d.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\imgcodecs.hpp>
#include <opencv2\imgproc.hpp>
#include <opencv2\videoio.hpp>
#include <opencv2\video.hpp>

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavdevice/avdevice.h>
	#include <libavfilter/avfilter.h>
	#include <libavformat/avformat.h>
	#include <libavformat/avio.h>
	#include <libavutil/avutil.h>
	#include <libpostproc/postprocess.h>
	#include <libswresample/swresample.h>
	#include <libswscale/swscale.h>
}

using namespace std;

#define VID_W		1024
#define VID_H		704
#define INBUF_SIZE	8192

class NewMarker
{
public:

	QVector2D openCVPos;
	QVector2D openCVPosUndistorted;
	
	QVector2D pos;	
	QVector2D distPos;
	QVector4D bounds;
	QVector3D worldRayD;

	QVector2D refPos;
	QVector3D refWorldRayD;
};

class Decoder
{
public:
	Decoder();
	~Decoder();

	float camSensitivity;
	float camThreshold;
	float camDistort;
	int frameSkip;
	bool drawGuides;
	bool drawMarkers;
	bool drawUndistorted;
	volatile bool findCalibrationSheet;
	

	bool blankFrame;

	int dataRecvBytes;

	int newFrames;

	QMutex calibMutex;

	int							_calibImageCount;
	vector<vector<cv::Point2f>>	_calibImagePoints;
	cv::Mat						_calibCameraMatrix;
	cv::Mat						_calibDistCoeffs;
	cv::Mat						optCamMat;
	cv::Mat						projMat;
	cv::Mat						unitProjMat;	
	QMatrix4x4					worldMat;
	cv::Mat						fundamentalMat;
	QVector3D					worldPos;

	cv::Mat						refRt; // Same as Pu
	cv::Mat						refR;
	cv::Mat						refT;
	QMatrix4x4					refWorldMat;
	cv::Mat						refK;
	cv::Mat						refOptK;
	cv::Mat						refPu;
	cv::Mat						refP;
	cv::Mat						refD;
	QVector3D					refWorldPos;

	uint8_t						frameMaskData[128 * 88];
	
	bool DoDecode(uint8_t* Data, int Len);
	bool DoDecodeSingleFrame(uint8_t* Data, int Len, int* Consumed);

	void TransferFrameToBuffer(uint8_t* Data);

	bool isCalibrating() { return _calibrating;	};

	uint8_t* GetFrameMatData() { return colMat.data; }

	void CalculateOptMat();
	void ShowBlankFrame();

	void ProcessFrameLive();
	void ProcessFrame();
	QList<QVector2D> ProcessFrameMarkers();
	QList<NewMarker> ProcessFrameNewMarkers();

private:

	// LibAV
	const AVCodec *codec;
	AVCodecParserContext *parser;
	AVCodecContext *c = NULL;
	AVFrame *frame;
	AVFrame *frameRGB;
	uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
	AVPacket *pkt;
	SwsContext *sws;

	// OpenCV
	cv::Ptr<cv::SimpleBlobDetector> blobDetector;
	cv::Mat cvFrame;
	cv::Mat colMat;
	cv::Mat undistortMat;
	cv::Mat maskMat;

	// Calibration
	bool					_calibrating;
	bool					_calibUndistortEnable;

	vector<cv::Mat>				_calibRvecs;
	vector<cv::Mat>				_calivTvecs;
	cv::Size					_calibBoardSize;
	float					_calibSquareSize;
	int						_calibFramesSince;

	int destWidth;
	int destHeight;
	int lastFrame = 0;
	int _parsedFrames = 0;

	// NOTE: Align to 16byte boundry
	uint8_t __pad[8];
	uint8_t _postFrameData[VID_W * VID_H];

	int _frameLimit;

	bool _Decode();
	
	void _initOpenCV();
	void _detectMarkers();
	void _detectValibSheet();
	void _undistort();
	void _findCalibrationSheet();

	void _CreateFrameGray();
};