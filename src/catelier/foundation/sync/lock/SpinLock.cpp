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
    }

    namespace {
        auto cpuPause() -> void {
            #if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))

            _mm_pause();

            #elif (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))

            __builtin_ia32_pause();

            #endif
        }
    }

    auto SpinLock_init(SpinLock* const self, const SpinLock_Policy inPolicy) -> bool {
        if (!self) {
            return false;
        }

        self->state = 0;
        self->policy = inPolicy;
        self->valid = true;

        return true;
    }
    auto SpinLock_destroy(SpinLock* const self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        #ifndef NDEBUG

        self->state = 0xDEADBEEF;

        #endif

        return true;
    }

    auto SpinLock_lock(SpinLock* const self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        switch (self->policy) {
            case SpinLock_Policy::TAS: {
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
            case SpinLock_Policy::TTAS: {
                usize spinCount = 0;
                while (true) {
                    if (primitive::Atomic32_load(&self->state, primitive::RELAXED) == 0) {
                        u32 expected = 0;
                        if (primitive::Atomic32_compareExchange(&self->state, &expected, 1, primitive::ACQUIRE)) {
                            return true;
                        }
                    }

                    if (spinCount < SPIN_PAUSE_LIMIT) {
                        cpuPause();
                        spinCount++;
                    } else {
                        primitive::Thread_yield();
                    }
                }
            }
        }

        return false;
    }
    auto SpinLock_unlock(SpinLock* const self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        primitive::Atomic32_store(&self->state, 0, primitive::RELEASE);

        return true;
    }
    auto SpinLock_tryLock(SpinLock* const self) -> bool {
        if (!self) {
            return false;
        }

        assert(self->valid);

        u32 expected = 0;
        return primitive::Atomic32_compareExchange(&self->state, &expected, 1, primitive::ACQUIRE);
    }
}