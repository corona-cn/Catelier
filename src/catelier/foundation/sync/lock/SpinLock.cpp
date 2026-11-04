#include "SpinLock.hpp"

#include <assert.h>

#include "../../primitive/Atomic.hpp"
#include "../../primitive/Thread.hpp"

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))

#include <intrin.h>

#endif

namespace Catelier::src::foundation::sync::lock {
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

    auto SpinLock_init(SpinLock* self) -> bool {
        if (!self) {
            return false;
        }

        self->state = 0;
        self->valid = true;

        return true;
    }
    auto SpinLock_destroy(SpinLock* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        #ifndef NDEBUG

        self->state = 0xDEADBEEF;

        #endif

        return true;
    }

    auto SpinLock_lock(SpinLock* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        usize spinCount = 0;
        while (true) {
            u32 expected = 0;
            if (primitive::Atomic32_compareExchange(&self->state, &expected, 1, primitive::ACQUIRE)) {
                return true;
            }

            if (spinCount < SPIN_PAUSE_LIMIT) {
                cpuPause();
                spinCount++;
            } else {
                primitive::Thread_yield();
            }
        }
    }
    auto SpinLock_unlock(SpinLock* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        primitive::Atomic32_store(&self->state, 0, primitive::RELEASE);

        return true;
    }
    auto SpinLock_tryLock(SpinLock* self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        u32 expected = 0;
        return primitive::Atomic32_compareExchange(&self->state, &expected, 1, primitive::ACQUIRE);
    }
}