#include <Python.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>

uint8_t _frameBuffer[1024 * 704];

pthread_t 			_workerThread;
pthread_mutex_t		_queueMutex;
pthread_cond_t		_queueSignal;
volatile uint32_t	_frameAvailable = 0;

typedef struct 
{
	float minX;
	float minY;
	float maxX;
	float maxY;
	float cX;
	float cY;

} blob;

int 	blobCount = 0;
blob 	blobs[256];

typedef struct
{
	int		blobCount;
	blob 	blobs[256];
	
} sd;

sd sendData;

inline int64_t PlatformGetMicrosecond(void)
{
	struct timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

inline int32_t PlatformGetMS(void)
{
	return (PlatformGetMicrosecond() / 1000);
}

float distSq(float X1, float Y1, float X2, float Y2)
{
	return (X2 - X1) * (X2 - X1) + (Y2 - Y1) * (Y2 - Y1);
}

void ProcessPixels(void)
{
	int width = 1024;
	int height = 704;
	//int pixelCount = width * height;

	int brightPixels = 0;

	int64_t startTime = PlatformGetMicrosecond();

	blobCount = 0;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			//if (buffer[y * width + x] > 95 && buffer[y * width + x] < 189)
			if (_frameBuffer[y * width + x] > 60)
			{
				++brightPixels;

				bool blobFound = false;
				
				// Could check all blobs and find closest.

				for (int b = 0; b < blobCount; ++b)
				{
					blob* cb = blobs + b;

					float cx = (cb->minX + cb->maxX) / 2;
					float cy = (cb->minY + cb->maxY) / 2;
					float d = distSq(x, y, cx, cy);

					if (d < 16 * 16)
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

	int64_t t1 = PlatformGetMicrosecond() - startTime;
	startTime = PlatformGetMicrosecond();

	for (int i = 0; i < blobCount; ++i)
	{
		blob* b = blobs + i;

		b->cX = 0.0f;
		b->cY = 0.0f;

		float totalWeight = 0.0f;
		// Count total weight
		for (int y = b->minY; y < b->maxY + 1; ++y)
		{
			for (int x = b->minX; x < b->maxX + 1; ++x)
			{
				uint8_t p = _frameBuffer[y * width + x];
				if (p > 60)
				{
					totalWeight += p;
				}
			}
		}

		for (int y = blobs[i].minY; y < blobs[i].maxY + 1; ++y)
		{
			for (int x = blobs[i].minX; x < blobs[i].maxX + 1; ++x)
			{
				uint8_t p = _frameBuffer[y * width + x];
				if (p > 60)
				{
					float pixelV = p;

					b->cX += x * (pixelV / totalWeight);
					b->cY += y * (pixelV / totalWeight);
				}
			}
		}
	}

	startTime = PlatformGetMicrosecond() - startTime;

	//printf("%d %d %d %d\n", brightPixels, blobCount, (int)t1, (int)startTime);
}

void *runWorkerThread(void *Args)
{
	printf("IO thread running.\n");

	while (1)
	{
		pthread_mutex_lock(&_queueMutex);

		while (_frameAvailable == 0)
			pthread_cond_wait(&_queueSignal, &_queueMutex);

		_frameAvailable = 0;

		ProcessPixels();
		pthread_mutex_unlock(&_queueMutex);
	}

	printf("IO thread done.\n");
	pthread_exit(0);
}

void enqueueWork()
{
	pthread_mutex_lock(&_queueMutex);

	_frameAvailable = 1;

	pthread_cond_signal(&_queueSignal);
	pthread_mutex_unlock(&_queueMutex);
}

static PyObject* blobdetect_getblobcount(PyObject* Self, PyObject* Args)
{
	pthread_mutex_lock(&_queueMutex);
	int b = blobCount;
	pthread_mutex_unlock(&_queueMutex);

	return Py_BuildValue("i", b);
}

static PyObject* blobdetect_getblobdata(PyObject* Self, PyObject* Args)
{
	pthread_mutex_lock(&_queueMutex);
	sendData.blobCount = blobCount;
	memcpy(sendData.blobs, blobs, sizeof(blob) * blobCount);
	pthread_mutex_unlock(&_queueMutex);

	return PyBuffer_FromMemory(&sendData, 4 + sendData.blobCount * sizeof(blob));
}

static PyObject* blobdetect_startworkers(PyObject* Self, PyObject* Args)
{
	pthread_mutex_init(&_queueMutex, 0);
	pthread_cond_init(&_queueSignal, 0);

	pthread_create(&_workerThread, 0, runWorkerThread, 0);

	//int intVal = data.len;
	//return Py_BuildValue("i", intVal);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* blobdetect_pushframe(PyObject* Self, PyObject* Args)
{
	int frameTime = 0;
	Py_buffer data;

	if (!PyArg_ParseTuple(Args, "ls*", &frameTime, &data))
		return NULL;

	memcpy(_frameBuffer, data.buf, 1024 * 704);
	enqueueWork();

	PyBuffer_Release(&data);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef BlobdetectMethods[] = 
{
	{"pushframe", blobdetect_pushframe, METH_VARARGS, "Push a YUV frame for processing."},
	{"startworkers", blobdetect_startworkers, METH_VARARGS, "Start threads to process frame data."},
	{"getblobcount", blobdetect_getblobcount, METH_VARARGS, "Get processed blob count."},
	{"getblobdata", blobdetect_getblobdata, METH_VARARGS, "Get processed blob data."},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initblobdetect(void)
{
	(void)Py_InitModule("blobdetect", BlobdetectMethods);
}