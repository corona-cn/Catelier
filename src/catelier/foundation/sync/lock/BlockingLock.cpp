#include "BlockingLock.hpp"

#include <assert.h>

#ifdef _WIN32

#include <windows.h>

#else

#include <pthread.h>

#endif

namespace Catelier::src::foundation::sync::lock {
    namespace {
        #ifdef _WIN32

        constexpr DWORD BLOCKING_LOCK_PLAIN_SPIN_COUNT = 0;
        constexpr DWORD BLOCKING_LOCK_ADAPTIVE_SPIN_COUNT = 4000;

        #endif
    }

    namespace {
        #ifdef _WIN32

        auto asNative(const BlockingLock* self) -> CRITICAL_SECTION* {
            return (CRITICAL_SECTION*) (BlockingLock*) self->storage;
        }

        #else

        auto asNative(const BlockingLock* self) -> pthread_mutex_t* {
            return (pthread_mutex_t*) (BlockingLock*) self->storage;
        }

        #endif
    }

    auto BlockingLock_init(BlockingLock* const self, const BlockingLock_Policy inPolicy) -> bool {
        if (!self) {
            return false;
        }

        memset(self->storage, 0, sizeof(self->storage));

        self->policy = inPolicy;
        self->valid = false;

        #ifdef _WIN32

        const DWORD spinCount = inPolicy == BlockingLock_Policy::ADAPTIVE ? BLOCKING_LOCK_ADAPTIVE_SPIN_COUNT : BLOCKING_LOCK_PLAIN_SPIN_COUNT;
        if (!InitializeCriticalSectionAndSpinCount(asNative(self), spinCount)) {
            return false;
        }

        #else

        pthread_mutexattr_t attr;
        if (pthread_mutexattr_init(&attr) != 0) {
            return false;
        }

        #ifdef PTHREAD_MUTEX_ADAPTIVE_NP

        if (inPolicy == BlockingLock_Policy::ADAPTIVE) {
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
        } else {
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
        }

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
    auto BlockingLock_destroy(BlockingLock* const self) -> bool {
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

    auto BlockingLock_lock(BlockingLock* const self) -> bool {
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
    auto BlockingLock_unlock(BlockingLock* const self) -> bool {
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
    auto BlockingLock_tryLock(BlockingLock* const self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        #ifdef _WIN32

        return TryEnterCriticalSection(asNative(self)) != 0;

        #else

        const int ret = pthread_mutex_trylock(asNative(self));
        return ret == 0;

        #endif
    }
}