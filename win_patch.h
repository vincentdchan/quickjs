
#ifndef WIN_PATCH
#define WIN_PATCH

#include <time.h>
#include <Windows.h>
#include "inttypes.h"

struct timezone {
	int     tz_minuteswest; /* minutes west of Greenwich */
	int     tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval* p, void* z);

typedef enum {
	_CLOCK_REALTIME = 0,
#define CLOCK_REALTIME _CLOCK_REALTIME
	_CLOCK_MONOTONIC = 6,
#define CLOCK_MONOTONIC _CLOCK_MONOTONIC
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
	_CLOCK_MONOTONIC_RAW = 4,
#define CLOCK_MONOTONIC_RAW _CLOCK_MONOTONIC_RAW
	_CLOCK_MONOTONIC_RAW_APPROX = 5,
#define CLOCK_MONOTONIC_RAW_APPROX _CLOCK_MONOTONIC_RAW_APPROX
	_CLOCK_UPTIME_RAW = 8,
#define CLOCK_UPTIME_RAW _CLOCK_UPTIME_RAW
	_CLOCK_UPTIME_RAW_APPROX = 9,
#define CLOCK_UPTIME_RAW_APPROX _CLOCK_UPTIME_RAW_APPROX
#endif
	_CLOCK_PROCESS_CPUTIME_ID = 12,
#define CLOCK_PROCESS_CPUTIME_ID _CLOCK_PROCESS_CPUTIME_ID
	_CLOCK_THREAD_CPUTIME_ID = 16
#define CLOCK_THREAD_CPUTIME_ID _CLOCK_THREAD_CPUTIME_ID
} clockid_t;

int
ftw(const char* path, int (*fn)(const char*, const struct stat* ptr, int flag), int depth);

#endif
