
#include "win_patch.h"
#include <windows.h>

#define POW10_7                 10000000
#define POW10_9                 1000000000

/* Number of 100ns-seconds between the beginning of the Windows epoch
 * (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970)
 */
#define DELTA_EPOCH_IN_100NS    INT64_C(116444736000000000)

#define FILETIME_1970 116444736000000000ull /* seconds between 1/1/1601 and 1/1/1970 */
#define HECTONANOSEC_PER_SEC 10000000ull

int getntptimeofday(struct timespec*, struct timezone*);

int getntptimeofday(struct timespec* tp, struct timezone* z)
{
    int res = 0;
    union {
        unsigned long long ns100; /*time since 1 Jan 1601 in 100ns units */
        FILETIME ft;
    }  _now;
    TIME_ZONE_INFORMATION  TimeZoneInformation;
    DWORD tzi;

    if (z != NULL)
    {
        if ((tzi = GetTimeZoneInformation(&TimeZoneInformation)) != TIME_ZONE_ID_INVALID) {
            z->tz_minuteswest = TimeZoneInformation.Bias;
            if (tzi == TIME_ZONE_ID_DAYLIGHT)
                z->tz_dsttime = 1;
            else
                z->tz_dsttime = 0;
        }
        else
        {
            z->tz_minuteswest = 0;
            z->tz_dsttime = 0;
        }
    }

    if (tp != NULL) {
        GetSystemTimeAsFileTime(&_now.ft);	 /* 100-nanoseconds since 1-1-1601 */
        /* The actual accuracy on XP seems to be 125,000 nanoseconds = 125 microseconds = 0.125 milliseconds */
        _now.ns100 -= FILETIME_1970;	/* 100 nano-seconds since 1-1-1970 */
        tp->tv_sec = _now.ns100 / HECTONANOSEC_PER_SEC;	/* seconds since 1-1-1970 */
        tp->tv_nsec = (long)(_now.ns100 % HECTONANOSEC_PER_SEC) * 100; /* nanoseconds */
    }
    return res;
}

int gettimeofday(struct timeval* p, void* z)
{
    struct timespec tp;

    if (getntptimeofday(&tp, (struct timezone*)z))
        return -1;
    p->tv_sec = tp.tv_sec;
    p->tv_usec = (tp.tv_nsec / 1000);
    return 0;
}

int clock_gettime(clockid_t clock_id, struct timespec* tp)
{
    unsigned __int64 t;
    LARGE_INTEGER pf, pc;
    union {
        unsigned __int64 u64;
        FILETIME ft;
    }  ct, et, kt, ut;

    switch (clock_id) {
    case CLOCK_REALTIME:
    {
        GetSystemTimeAsFileTime(&ct.ft);
        t = ct.u64 - DELTA_EPOCH_IN_100NS;
        tp->tv_sec = t / POW10_7;
        tp->tv_nsec = ((int)(t % POW10_7)) * 100;

        return 0;
    }

    case CLOCK_MONOTONIC:
    {
        if (QueryPerformanceFrequency(&pf) == 0)
            return -(EINVAL);

        if (QueryPerformanceCounter(&pc) == 0)
            return -(EINVAL);

        tp->tv_sec = pc.QuadPart / pf.QuadPart;
        tp->tv_nsec = (int)(((pc.QuadPart % pf.QuadPart) * POW10_9 + (pf.QuadPart >> 1)) / pf.QuadPart);
        if (tp->tv_nsec >= POW10_9) {
            tp->tv_sec++;
            tp->tv_nsec -= POW10_9;
        }

        return 0;
    }

    case CLOCK_PROCESS_CPUTIME_ID:
    {
        if (0 == GetProcessTimes(GetCurrentProcess(), &ct.ft, &et.ft, &kt.ft, &ut.ft))
            return -(EINVAL);
        t = kt.u64 + ut.u64;
        tp->tv_sec = t / POW10_7;
        tp->tv_nsec = ((int)(t % POW10_7)) * 100;

        return 0;
    }

    case CLOCK_THREAD_CPUTIME_ID:
    {
        if (0 == GetThreadTimes(GetCurrentThread(), &ct.ft, &et.ft, &kt.ft, &ut.ft))
            return -(EINVAL);
        t = kt.u64 + ut.u64;
        tp->tv_sec = t / POW10_7;
        tp->tv_nsec = ((int)(t % POW10_7)) * 100;

        return 0;
    }

    default:
        break;
    }

    return -(EINVAL);
}

int WStringToUTF8(const WCHAR* wstr, char* buffer, uint32_t buffer_size)
{
    uint32_t len = wcslen(wstr);
    return WideCharToMultiByte(CP_UTF8, 0, wstr, len, buffer,
           buffer_size, NULL, NULL);
}

int UTF8ToWString(const char* bytes, WCHAR* buffer, uint32_t buffer_size)
{
    uint32_t str_size = strlen(bytes);
    return MultiByteToWideChar(CP_UTF8, 0, bytes, str_size, buffer, buffer_size);
}

static int nt_feed_data(const WCHAR* parent, const WCHAR* filename, int (*fn)(const char*, const struct stat* ptr, int flag)) {
    uint32_t parent_len = wcslen(parent);
    uint32_t filename_len = wcslen(filename);
    if (parent_len + filename_len + 1 >= _MAX_PATH) {
        return -1;
    }

    WCHAR wide_buffer[_MAX_PATH];
    memset(wide_buffer, 0, sizeof(WCHAR) * _MAX_PATH);
    wcscpy(wide_buffer, parent);
    uint32_t index = parent_len;
    wide_buffer[index++] = '\\';
    wcscpy(wide_buffer + index, filename);
    
    char buffer[_MAX_PATH];
    memset(buffer, 0, _MAX_PATH);
    WStringToUTF8(wide_buffer, buffer, MAX_PATH);

    int ec = fn(buffer, NULL, 0);
    if (ec < 0) {
        return ec;
    }

    return 0;
}

int nt_ftw(const WCHAR* path, int (*fn)(const char*, const struct stat* ptr, int flag), int depth) {
    WIN32_FIND_DATAW info;
    HANDLE hFind = FindFirstFileW(path, &info);
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }

    int ec = nt_feed_data(path, info.cFileName, fn);
    if (ec < 0) {
        FindClose(hFind);
        return ec;
    }

    while (1) {
        hFind = FindNextFileW(hFind, &info);
        if (hFind == INVALID_HANDLE_VALUE) {
            break;
        }

        ec = nt_feed_data(path, info.cFileName, fn);
        if (ec < 0) {
            FindClose(hFind);
            return ec;
        }
    }

    FindClose(hFind);
    return 0;
}

int
ftw(const char* path, int (*fn)(const char*, const struct stat* ptr, int flag), int depth) {
    WCHAR buffer[_MAX_PATH];
    memset(buffer, 0, sizeof(WCHAR) * _MAX_PATH);
    if (UTF8ToWString(path, buffer, _MAX_PATH) != 0) {
        return -1;
    }

    return nt_ftw(buffer, fn, depth);
}
