#include "ConditionalVar.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <errno.h>
#include <time.h>
#endif

namespace Catelier::src::foundation::sync {
    #ifdef _WIN32
    namespace {
        auto asNative(const ConditionalVar* self) -> CONDITION_VARIABLE* {
            return (CONDITION_VARIABLE*) self->storage;
        }

        auto asCriticalSection(const primitive::Mutex* mutex) -> CRITICAL_SECTION* {
            return (CRITICAL_SECTION*) mutex->storage;
        }
    }

    auto ConditionalVar_init(ConditionalVar* self) -> bool {
        if (!self || self->valid) {
            return false;
        }

        InitializeConditionVariable(asNative(self));

        self->valid = true;

        return true;
    }
    auto ConditionalVar_destroy(ConditionalVar* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        return true;
    }

    auto ConditionalVar_wait(ConditionalVar* self, primitive::Mutex* mutex) -> bool {
        if (!self || !self->valid || !mutex || !mutex->valid) {
            return false;
        }

        const BOOL result = SleepConditionVariableCS(asNative(self), asCriticalSection(mutex), INFINITE);

        return result != 0;
    }
    auto ConditionalVar_timedWait(ConditionalVar* self, primitive::Mutex* mutex, const u64 timeoutMs) -> bool {
        if (!self || !self->valid || !mutex || !mutex->valid) {
            return false;
        }

        const DWORD timeout = (timeoutMs > 0xFFFFFFFFULL) ? 0xFFFFFFFFUL : (DWORD) timeoutMs;
        const BOOL result = SleepConditionVariableCS(asNative(self), asCriticalSection(mutex), timeout);

        return result != 0;
    }
    auto ConditionalVar_signal(ConditionalVar* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        WakeConditionVariable(asNative(self));

        return true;
    }
    auto ConditionalVar_broadcast(ConditionalVar* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        WakeAllConditionVariable(asNative(self));

        return true;
    }
    #else
    namespace {
        auto asNative(const ConditionalVar* self) -> pthread_cond_t* {
            return (pthread_cond_t*) self->storage;
        }

        auto asPthreadMutex(const primitive::Mutex* mutex) -> pthread_mutex_t* {
            return (pthread_mutex_t*) mutex->storage;
        }

        auto computeDeadline(const u64 timeoutMs, struct timespec* deadlineOut) -> void {
            clock_gettime(CLOCK_REALTIME, deadlineOut);

            deadlineOut->tv_sec += (time_t) (timeoutMs / 1000);
            deadlineOut->tv_nsec += (long) ((timeoutMs % 1000) * 1000000L);

            if (deadlineOut->tv_nsec >= 1000000000L) {
                deadlineOut->tv_sec += 1;
                deadlineOut->tv_nsec -= 1000000000L;
            }
        }
    }

    auto ConditionalVar_init(ConditionalVar* self) -> bool {
        if (!self || self->valid) {
            return false;
        }

        if (pthread_cond_init(asNative(self), nullptr) != 0) {
            return false;
        }

        self->valid = true;

        return true;
    }
    auto ConditionalVar_destroy(ConditionalVar* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        pthread_cond_destroy(asNative(self));

        self->valid = false;

        return true;
    }

    auto ConditionalVar_wait(ConditionalVar* self, primitive::Mutex* mutex) -> bool {
        if (!self || !self->valid || !mutex || !mutex->valid) {
            return false;
        }

        return pthread_cond_wait(asNative(self), asPthreadMutex(mutex)) == 0;
    }
    auto ConditionalVar_timedWait(ConditionalVar* self, primitive::Mutex* mutex, const u64 timeoutMs) -> bool {
        if (!self || !self->valid || !mutex || !mutex->valid) {
            return false;
        }

        struct timespec deadline;
        computeDeadline(timeoutMs, &deadline);

        const int result = pthread_cond_timedwait(asNative(self), asPthreadMutex(mutex), &deadline);

        return result == 0;
    }
    auto ConditionalVar_signal(ConditionalVar* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        return pthread_cond_signal(asNative(self)) == 0;
    }
    auto ConditionalVar_broadcast(ConditionalVar* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        return pthread_cond_broadcast(asNative(self)) == 0;
    }
    #endif
}