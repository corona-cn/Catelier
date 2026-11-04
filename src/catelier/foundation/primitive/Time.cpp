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
        return ((u64) ts.tv_sec * 1000ULL) + ((u64) ts.tv_nsec / 1000000ULL);

        #endif
    }
}