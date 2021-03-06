#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "mainwindow.h"
#include <QApplication>
#include <iostream>
#include <WinSock2.h>

HANDLE		timeSyncThreadHandle;
int64_t		timeFrequency;
int64_t		timeCounterStart;

double		startTime;

struct pingCamEntry
{
	unsigned long ip;
	float avgTime;
};

pingCamEntry	pingEntries[64];
int				pingEntryCount = 0;

inline pingCamEntry* getPingEntry(unsigned long Ip)
{
	for (int i = 0; i < pingEntryCount; ++i)
	{
		if (pingEntries[i].ip == Ip)
			return &pingEntries[i];
	}

	pingCamEntry* result = &pingEntries[pingEntryCount++];
	result->ip = Ip;
	result->avgTime = 0.0f;

	return result;
}

double GetTime()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	int64_t time = counter.QuadPart - timeCounterStart;
	double result = (double)time / ((double)timeFrequency);

	return result;
}

DWORD WINAPI timeSyncThreadProc(LPVOID Parameter)
{
	LARGE_INTEGER freq;
	LARGE_INTEGER counter;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&counter);
	timeCounterStart = counter.QuadPart;
	timeFrequency = freq.QuadPart;

	startTime = GetTime();

	OutputDebugStringA("Time sync thread started\n");

	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (s == INVALID_SOCKET)
	{
		OutputDebugStringA("Invalid socket\n");
	}

	SOCKADDR_IN server;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(4894);

	
	if (bind(s, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		OutputDebugStringA("Bind failed\n");
		return 1;
	}

	OutputDebugStringA("Now we bound boyz\n");
	
	char msg[1500];

	while (true)
	{
		SOCKADDR_IN clientAddr;
		int clientLen = sizeof(clientAddr);
		
		int bytesRead = recvfrom(s, msg, sizeof(msg), 0, (SOCKADDR*)&clientAddr, &clientLen);
		int64_t recvTimeUs = (int64_t)((GetTime() - startTime) * 1000000.0);
		
		int64_t guessedTime = *(int64_t*)(msg + 8);
		int64_t diff = recvTimeUs - guessedTime;

		pingCamEntry* pce = getPingEntry(clientAddr.sin_addr.s_addr);
		
		if (diff < 10000)
			pce->avgTime = pce->avgTime * 0.8f + (float)diff * 0.2f;
		else
			pce->avgTime = 10000.0f;

		*(int64_t*)(msg + 8) = recvTimeUs;
		*(float*)(msg + 16) = pce->avgTime;
		sendto(s, msg, 20, 0, (SOCKADDR*)&clientAddr, clientLen);

		//char outMsg[256];
		//sprintf(outMsg, "Got UDP packet (%u) %s:%d - %d %d %d %d %f\n", clientAddr.sin_addr, inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), bytesRead, recvTimeUs, guessedTime, diff, pce->avgTime);
		//OutputDebugStringA(outMsg);
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

int main(int argc, char* argv[])
{
	timeSyncThreadHandle = CreateThread(0, 0, timeSyncThreadProc, NULL, 0, NULL);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
