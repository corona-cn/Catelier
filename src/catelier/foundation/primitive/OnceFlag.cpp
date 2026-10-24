#include "OnceFlag.hpp"

#include <cassert>

#ifdef _WIN32

#include <windows.h>

#else

#include <pthread.h>

#endif

namespace Catelier::src::foundation::primitive {
    namespace {
        #ifdef _WIN32

        auto asNative(const OnceFlag* self) -> INIT_ONCE* {
            return (INIT_ONCE*) (u8*) self->storage;
        }

        #else

        auto asNative(const OnceFlag* self) -> pthread_once_t* {
            return (pthread_once_t*) (u8*) self->storage;
        }

        #endif
    }

    namespace {
        #ifdef _WIN32

        auto CALLBACK initOnceBridge(PINIT_ONCE, const PVOID parameter, PVOID*) -> BOOL {
            auto* const callback = (void (*)()) parameter;
            callback();
            return TRUE;
        }

        #endif
    }

    auto OnceFlag_init(OnceFlag* self) -> bool {
        if (!self) {
            return false;
        }

        memset(self->storage, 0, sizeof(self->storage));

        #ifdef _WIN32

        InitOnceInitialize(asNative(self));

        #endif

        self->valid = true;

        return true;
    }
    auto OnceFlag_destroy(OnceFlag* self) -> bool {
        if (!self) {
            return false;
        }

        self->valid = false;

        #ifndef NDEBUG

        memset(self->storage, 0xDE, sizeof(self->storage));

        #endif

        return true;
    }

    auto OnceFlag_do(const OnceFlag* self, void (*callback)()) -> bool {
        if (!self || !callback) {
            return false;
        }

        assert(self->valid);

        #ifdef _WIN32

        const BOOL result = InitOnceExecuteOnce(asNative(self), initOnceBridge, (PVOID) callback, nullptr);
        return result != FALSE;

        #else

        const int result = pthread_once(asNative(self), callback);
        return result == 0;

        #endif
    }
}