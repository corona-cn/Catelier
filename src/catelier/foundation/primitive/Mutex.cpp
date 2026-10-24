#include "Mutex.hpp"

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

#else

#include <pthread.h>

#endif

namespace Catelier::src::foundation::primitive {
    namespace {
        #ifdef _WIN32

        constexpr DWORD MUTEX_SPIN_COUNT = 4000;

        #endif
    }

    namespace {
        #ifdef _WIN32

        auto asNative(const Mutex* self) -> CRITICAL_SECTION* {
            return (CRITICAL_SECTION*) (Mutex*) self->storage;
        }

        #else

        auto asNative(const Mutex* self) -> pthread_mutex_t* {
            return (pthread_mutex_t*) (Mutex*) self->storage;
        }

        #endif
    }

    auto Mutex_init(Mutex* self) -> bool {
        if (!self) {
            return false;
        }

        memset(self->storage, 0, sizeof(self->storage));

        self->valid = false;

        #ifdef _WIN32

        if (!InitializeCriticalSectionAndSpinCount(asNative(self), MUTEX_SPIN_COUNT)) {
            return false;
        }

        #else

        pthread_mutexattr_t attr;
        if (pthread_mutexattr_init(&attr) != 0) {
            return false;
        }

        #ifdef PTHREAD_MUTEX_ADAPTIVE_NP

        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);

        #else

        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

        #endif

        const int ret = pthread_mutex_init(asNative(self), &attr);
        pthread_mutexattr_destroy(&attr);

        if (ret != 0) {
            return false;
        }

        #endif

        self->valid = true;

        return true;
    }
    auto Mutex_destroy(Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        #ifdef _WIN32

        DeleteCriticalSection(asNative(self));

        #else

        const int ret = pthread_mutex_destroy(asNative(self));
        if (ret != 0) {
            return false;
        }

        #endif

        self->valid = false;

        #ifndef NDEBUG

        memset(self->storage, 0xDE, sizeof(self->storage));

        #endif

        return true;
    }

    auto Mutex_lock(Mutex* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        #ifdef _WIN32

        EnterCriticalSection(asNative(self));

        return true;

        #else

        const int ret = pthread_mutex_lock(asNative(self));
        return ret == 0;

        #endif
    }
    auto Mutex_unlock(Mutex* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        #ifdef _WIN32

        LeaveCriticalSection(asNative(self));

        return true;

        #else

        const int ret = pthread_mutex_unlock(asNative(self));
        return ret == 0;

        #endif
    }
    auto Mutex_tryLock(Mutex* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        #ifdef _WIN32

        return TryEnterCriticalSection(asNative(self)) != 0;

        #else

        const int ret = pthread_mutex_trylock(asNative(self));
        if (ret == 0) {
            return true;
        }

        return false;

        #endif
    }
}