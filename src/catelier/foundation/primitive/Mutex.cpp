#include "Mutex.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace Catelier::src::foundation::primitive {
    #ifdef _WIN32
    namespace {
        auto asNative(const Mutex* self) -> CRITICAL_SECTION* {
            return (CRITICAL_SECTION*) self->storage;
        }
    }

    auto Mutex_init(Mutex* self) -> bool {
        if (!self) {
            return false;
        }

        self->valid = false;

        InitializeCriticalSection(asNative(self));

        self->valid = true;

        return true;
    }
    auto Mutex_destroy(Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        DeleteCriticalSection(asNative(self));

        self->valid = false;

        return true;
    }

    auto Mutex_lock(const Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        EnterCriticalSection(asNative(self));

        return true;
    }
    auto Mutex_unlock(const Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        LeaveCriticalSection(asNative(self));

        return true;
    }
    auto Mutex_tryLock(const Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        return TryEnterCriticalSection(asNative(self)) != 0;
    }
    #else
    namespace {
        auto asNative(const Mutex* self) -> pthread_mutex_t* {
            return (pthread_mutex_t*) self->storage;
        }
    }

    auto Mutex_init(Mutex* self) -> bool {
        if (!self) {
            return false;
        }

        self->valid = false;

        if (pthread_mutex_init(asNative(self), nullptr) != 0) {
            return false;
        }

        self->valid = true;

        return true;
    }
    auto Mutex_destroy(Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        pthread_mutex_destroy(asNative(self));

        self->valid = false;

        return true;
    }

    auto Mutex_lock(const Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        return pthread_mutex_lock(asNative(self)) == 0;
    }
    auto Mutex_unlock(const Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        return pthread_mutex_unlock(asNative(self)) == 0;
    }
    auto Mutex_tryLock(const Mutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        return pthread_mutex_trylock(asNative(self)) == 0;
    }
    #endif
}