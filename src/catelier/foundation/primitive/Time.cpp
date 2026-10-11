#include "Time.hpp"

#ifdef _WIN32

#include <windows.h>

#else

#include <time.h>

#endif

namespace Catelier::src::foundation::primitive {
    auto Time_currentMs() -> u64 {
        #ifdef _WIN32

        return (u64) GetTickCount64();

        #else

        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (u64) ts.tv_sec * 1000ULL + (u64) ts.tv_nsec / 1000000ULL;

        #endif
    }

    auto Time_sleepMs(const u64 inMs) -> void {
        #ifdef _WIN32

        Sleep((DWORD) inMs);

        #else

        struct timespec ts;
        ts.tv_sec = (time_t) (inMs / 1000);
        ts.tv_nsec = (long) ((inMs % 1000) * 1000000L);
        nanosleep(&ts, nullptr);

        #endif
    }
}