#include "SpinMutex.hpp"

#include <assert.h>

#include "Atomic.hpp"
#include "Thread.hpp"

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))

#include <intrin.h>

#endif

namespace Catelier::src::foundation::primitive {
    namespace {
        constexpr usize SPIN_PAUSE_LIMIT = 64;

        auto cpuPause() -> void {
            #if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))

            _mm_pause();

            #elif (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))

            __builtin_ia32_pause();

            #endif
        }
    }

    auto SpinMutex_init(SpinMutex* self) -> bool {
        if (!self) {
            return false;
        }

        self->state = 0;
        self->valid = true;

        return true;
    }
    auto SpinMutex_destroy(SpinMutex* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        #ifndef NDEBUG

        self->state = 0xDEADBEEF;

        #endif

        return true;
    }

    auto SpinMutex_lock(SpinMutex* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        usize spinCount = 0;
        while (true) {
            u32 expected = 0;
            if (Atomic32_compareExchange(&self->state, &expected, 1, ACQUIRE)) {
                return true;
            }

            if (spinCount < SPIN_PAUSE_LIMIT) {
                cpuPause();
                spinCount++;
            } else {
                Thread_yield();
            }
        }
    }
    auto SpinMutex_unlock(SpinMutex* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        Atomic32_store(&self->state, 0, RELEASE);

        return true;
    }
    auto SpinMutex_tryLock(SpinMutex* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        u32 expected = 0;
        return Atomic32_compareExchange(&self->state, &expected, 1, ACQUIRE);
    }
}