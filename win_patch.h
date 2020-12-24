
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

#endif
