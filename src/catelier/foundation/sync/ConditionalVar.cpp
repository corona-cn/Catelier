#include "ConditionalVar.hpp"

#include <cassert>

#ifdef _WIN32

#include <windows.h>

#else

#include <pthread.h>
#include <errno.h>
#include <time.h>

#endif

namespace Catelier::src::foundation::sync {
    namespace {
        #ifdef _WIN32

        auto asNative(const ConditionalVar* self) -> CONDITION_VARIABLE* {
            return (CONDITION_VARIABLE*) self->storage;
        }

        auto asCriticalSection(const primitive::Mutex* mutex) -> CRITICAL_SECTION* {
            return (CRITICAL_SECTION*) mutex->storage;
        }

        #else

        auto asNative(const ConditionalVar* self) -> pthread_cond_t* {
            return (pthread_cond_t*) self->storage;
        }

        auto asPthreadMutex(const primitive::Mutex* mutex) -> pthread_mutex_t* {
            return (pthread_mutex_t*) mutex->storage;
        }

        auto computeDeadline(const u64 timeoutMs, struct timespec* deadlineOut) -> void {
            clock_gettime(CLOCK_REALTIME, deadlineOut);

            const time_t addSec = (time_t) (timeoutMs / 1000);
            const long addNsec = (long) ((timeoutMs % 1000) * 1000000L);

            deadlineOut->tv_sec += addSec;
            deadlineOut->tv_nsec += addNsec;

            if (deadlineOut->tv_nsec >= 1000000000L) {
                deadlineOut->tv_sec += 1;
                deadlineOut->tv_nsec -= 1000000000L;
            }
        }

        #endif
    }

    auto ConditionalVar_init(ConditionalVar* self) -> bool {
        if (!self || self->valid) {
            return false;
        }

        memset(self->storage, 0, sizeof(self->storage));

        #ifdef _WIN32

        InitializeConditionVariable(asNative(self));

        #else

        if (pthread_cond_init(asNative(self), nullptr) != 0) {
            return false;
        }

        #endif

        self->valid = true;

        return true;
    }
    auto ConditionalVar_destroy(ConditionalVar* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        #ifndef _WIN32

        pthread_cond_destroy(asNative(self));

        #endif

        self->valid = false;

        #ifndef NDEBUG

        memset(self->storage, 0xDE, sizeof(self->storage));

        #endif

        return true;
    }

    auto ConditionalVar_signal(ConditionalVar* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        #ifdef _WIN32

        WakeConditionVariable(asNative(self));
        return true;

        #else

        return pthread_cond_signal(asNative(self)) == 0;

        #endif
    }
    auto ConditionalVar_broadcast(ConditionalVar* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        #ifdef _WIN32

        WakeAllConditionVariable(asNative(self));
        return true;

        #else

        return pthread_cond_broadcast(asNative(self)) == 0;

        #endif
    }

    auto ConditionalVar_wait(ConditionalVar* self, primitive::Mutex* mutex) -> bool {
        if (!self || !mutex) {
            return false;
        }

        assert(self->valid && mutex->valid);

        #ifdef _WIN32

        const BOOL result = SleepConditionVariableCS(asNative(self), asCriticalSection(mutex), INFINITE);
        return result != FALSE;

        #else

        const int result = pthread_cond_wait(asNative(self), asPthreadMutex(mutex));
        return result == 0;

        #endif
    }
    auto ConditionalVar_timedWait(ConditionalVar* self, primitive::Mutex* mutex, const u64 timeoutMs) -> bool {
        if (!self || !mutex) {
            return false;
        }

        assert(self->valid && mutex->valid);

        #ifdef _WIN32

        const DWORD timeout = (timeoutMs > 0xFFFFFFFFULL) ? 0xFFFFFFFFUL : (DWORD) timeoutMs;
        const BOOL result = SleepConditionVariableCS(asNative(self), asCriticalSection(mutex), timeout);
        return result != FALSE;

        #else

        struct timespec deadline;
        computeDeadline(timeoutMs, &deadline);
        const int result = pthread_cond_timedwait(asNative(self), asPthreadMutex(mutex), &deadline);
        return result == 0;

        #endif
    }
}