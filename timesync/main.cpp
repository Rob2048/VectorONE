#include <iostream>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;
typedef char				i8;
typedef short				i16;
typedef int					i32;
typedef long long			i64;
typedef float				f32;
typedef double				f64;

i64	rttBuffer[100];
i64	rttBufferSorted[100];
i32 rttBufferIdx = 0;
i64 rttMedian = 0;
float rttMedianFiltered = 0.0f;

i64 offsetBuffer[100];
i64 offsetBufferSorted[100];
i32 offsetBufferIdx = 0;
i64 offsetMedian = 0;
float offsetMedianFiltered = 0.0f;

struct timeEntry
{
	i64 localTime;
	i64 masterTime;
};

timeEntry timeEntries[1000];
i32 timeEntryIdx = 0;

//------------------------------------------------------------------------------------------------------------
// Utility functions.
//------------------------------------------------------------------------------------------------------------
inline u64 GetUS()
{
	//struct timespec t1, t2;
	//clock_gettime(CLOCK_MONOTONIC, &t1);

	timespec time;
	clock_gettime(CLOCK_REALTIME, &time);

	return time.tv_sec * 1000000 + time.tv_nsec / 1000;
}

inline u32 GetMS()
{
	return (GetUS() / 1000);
}

//------------------------------------------------------------------------------------------------------------
// Application entry.
//------------------------------------------------------------------------------------------------------------
int cmpfunc(const void* a, const void* b)
{
	return (*(i64*)a - *(i64*)b);
}

int main(int argc, char** argv)
{
	std::cout << "timesync running...\n";

	int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}

	std::cout << s << "\n";

	struct sockaddr_in hostAddr = {};
	memset((char *) &hostAddr, 0, sizeof(hostAddr));
	hostAddr.sin_family = AF_INET;
	hostAddr.sin_port = htons(4894);

	std::cout << inet_aton("192.168.1.107", &hostAddr.sin_addr) << "\n";

	u64 startTimeUs = GetUS();
	i64 startMasterUs = 0;

	FILE* f = fopen("log.txt", "w");

	memset(rttBuffer, 0, sizeof(rttBuffer));
	memset(rttBufferSorted, 0, sizeof(rttBuffer));

	while (1)
	{
		char msg[1500];

		*(i64*)(msg + 8) = (GetUS() - startTimeUs) + offsetMedianFiltered + startMasterUs + (i64)(rttMedianFiltered * 0.5f);
		*(i64*)msg = GetUS() - startTimeUs;
		
		int res = sendto(s, msg, 16, 0, (struct sockaddr*)&hostAddr, sizeof(hostAddr));
		//std::cout << "Send " << res << "\n";
		
		struct sockaddr_in recvAddr;
		unsigned int recvAddrLen = sizeof(recvAddr);
		res = recvfrom(s, msg, sizeof(msg), 0, (struct sockaddr*)&recvAddr, &recvAddrLen);

		i64 recvTimeUs = GetUS() - startTimeUs;

		if (res == -1)
		{
			std::cout << "TIME OUT BITCHES\n";
			continue;
		}

		i64 sendTimeUs = *(i64*)msg;
		i64 rttUs = recvTimeUs - sendTimeUs;
		i64 masterTimeUs = *(i64*)(msg + 8) - startMasterUs - (i64)(rttMedianFiltered * 0.5f);

		if (startMasterUs == 0)
			startMasterUs = masterTimeUs;

		std::cout << "Recv " << res << ": " << (u64)rttUs << " " << masterTimeUs << " " << (masterTimeUs - recvTimeUs) << "\n";
		fprintf(f, "%f %f %f\n", ((float)(recvTimeUs) / 1000.0f), ((float)(masterTimeUs) / 1000.0f), ((float)(rttUs) / 1000.0f));
		fflush(f);

		rttBuffer[rttBufferIdx++] = rttUs;
		if (rttBufferIdx == 100)
			rttBufferIdx = 0;

		memcpy(rttBufferSorted, rttBuffer, sizeof(rttBuffer));

		u64 t = GetUS();
		qsort(rttBufferSorted, 100, sizeof(i64), cmpfunc);
		t = GetUS() - t;
		//std::cout << "Sort time: " << (i64)t << "\n";

		rttMedian = rttBufferSorted[49];
		rttMedianFiltered = rttMedianFiltered * 0.95f + rttMedian * 0.05f;

		std::cout << "Median rtt: " << rttMedian << " " << rttMedianFiltered << "\n";

		offsetBuffer[offsetBufferIdx++] = masterTimeUs - recvTimeUs;
		if (offsetBufferIdx == 100)
			offsetBufferIdx = 0;

		memcpy(offsetBufferSorted, offsetBuffer, sizeof(offsetBuffer));

		qsort(offsetBufferSorted, 100, sizeof(i64), cmpfunc);

		offsetMedian = offsetBufferSorted[49];
		offsetMedianFiltered = offsetMedianFiltered * 0.95f + offsetMedian * 0.05f;

		std::cout << "Median offset: " << offsetMedian << " " << offsetMedianFiltered << "\n";
		

		

		usleep(100000);
	}

	close(s);
	
	return 0;
}