#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	undef WIN32_LEAN_AND_MEAN
#elif defined(__LINUX__) || defined(__linux__)
extern "C"{
#	define __lint__
#	include <sys/time.h>
}
#endif

int64_t GetMicroSecond(){
#ifdef _WIN32
	static int64_t freq = 0;
	if(freq == 0){
		LARGE_INTEGER f;
		QueryPerformanceFrequency(&f);
		freq = f.QuadPart;
	}
	LARGE_INTEGER perf;
	QueryPerformanceCounter(&perf);
	return perf.QuadPart * 1000000 / freq;
#elif defined(__LINUX__) || defined(__linux__)
	struct timeval tv={0};
    gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
#endif
}

inline int64_t GetMilliSecond(){
	return GetMicroSecond() / 1000;
}

