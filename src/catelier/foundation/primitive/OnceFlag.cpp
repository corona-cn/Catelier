#include "OnceFlag.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <cstring>

namespace Catelier::src::foundation::primitive {
    #ifdef _WIN32
    namespace {
        auto asNative(const OnceFlag* self) -> INIT_ONCE* {
            return (INIT_ONCE*) self->storage;
        }

        auto CALLBACK initOnceBridge(PINIT_ONCE, const PVOID parameter, PVOID*) -> BOOL {
            auto* const callback = (void (*)()) parameter;
            callback();
            return TRUE;
        }
    }

    auto OnceFlag_init(OnceFlag* self) -> bool {
        if (!self) {
            return false;
        }

        self->valid = false;

        InitOnceInitialize(asNative(self));

        self->valid = true;

        return true;
    }
    auto OnceFlag_destroy(OnceFlag* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        return true;
    }

    auto OnceFlag_do(const OnceFlag* self, void (*callback)()) -> bool {
        if (!self || !self->valid || !callback) {
            return false;
        }

        return InitOnceExecuteOnce(asNative(self), initOnceBridge, (PVOID) callback, nullptr) != 0;
    }
    #else
    namespace {
        auto asNative(const OnceFlag* self) -> pthread_once_t* {
            return (pthread_once_t*) self->storage;
        }
    }

    auto OnceFlag_init(OnceFlag* self) -> bool {
        if (!self) {
            return false;
        }

        self->valid = false;

        static const pthread_once_t INIT = PTHREAD_ONCE_INIT;
        memcpy(asNative(self), &INIT, sizeof(pthread_once_t));

        self->valid = true;

        return true;
    }
    auto OnceFlag_destroy(OnceFlag* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        return true;
    }

    auto OnceFlag_do(const OnceFlag* self, void (*callback)()) -> bool {
        if (!self || !self->valid || !callback) {
            return false;
        }

        return pthread_once(asNative(self), callback) == 0;
    }
    #endif
}