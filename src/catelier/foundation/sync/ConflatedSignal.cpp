#include "ConflatedSignal.hpp"

#include "../primitive/Thread.hpp"
#include "../primitive/Atomic.hpp"

#ifdef _WIN32

#include <windows.h>

#else

#include <time.h>

#endif

namespace Catelier::src::foundation::sync {
    namespace {
        auto currentTimeMs() -> u64 {
            #ifdef _WIN32

            return (u64) GetTickCount64();

            #else

            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            return (u64) ts.tv_sec * 1000ULL + (u64) ts.tv_nsec / 1000000ULL;

            #endif
        }
    }

    auto ConflatedSignal_init(ConflatedSignal* self) -> bool {
        if (!self || primitive::AtomicBool_load(&self->valid, primitive::ACQUIRE)) {
            return false;
        }

        primitive::AtomicBool_store(&self->pending, false, primitive::RELAXED);
        primitive::AtomicBool_store(&self->closed, false, primitive::RELAXED);
        primitive::Atomic64_store(&self->waiterCount, 0, primitive::RELAXED);

        if (!primitive::Mutex_init(&self->mutex)) {
            return false;
        }

        if (!ConditionalVar_init(&self->condition)) {
            primitive::Mutex_destroy(&self->mutex);
            return false;
        }

        primitive::AtomicBool_store(&self->valid, true, primitive::RELEASE);

        return true;
    }
    auto ConflatedSignal_destroy(ConflatedSignal* self) -> bool {
        if (!self) {
            return false;
        }

        bool expected = true;
        if (!primitive::AtomicBool_compareExchange(&self->valid, &expected, false, primitive::SEQUENTIAL_CONSISTENT)) {
            return false;
        }

        primitive::Mutex_lock(&self->mutex);

        primitive::AtomicBool_store(&self->closed, true, primitive::RELEASE);
        ConditionalVar_broadcast(&self->condition);

        primitive::Mutex_unlock(&self->mutex);

        while (primitive::Atomic64_load(&self->waiterCount, primitive::SEQUENTIAL_CONSISTENT) > 0) {
            primitive::Thread_yield();
        }

        ConditionalVar_destroy(&self->condition);

        primitive::Mutex_destroy(&self->mutex);

        return true;
    }

    auto ConflatedSignal_signal(ConflatedSignal* self) -> void {
        if (!self) {
            return;
        }

        primitive::Atomic64_fetchAdd(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
        if (!primitive::AtomicBool_load(&self->valid, primitive::ACQUIRE)) {
            primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
            return;
        }

        primitive::Mutex_lock(&self->mutex);

        if (!primitive::AtomicBool_load(&self->closed, primitive::ACQUIRE) && !primitive::AtomicBool_load(&self->pending, primitive::ACQUIRE)) {
            primitive::AtomicBool_store(&self->pending, true, primitive::RELEASE);
            ConditionalVar_signal(&self->condition);
        }

        primitive::Mutex_unlock(&self->mutex);

        primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
    }

    auto ConflatedSignal_wait(ConflatedSignal* self) -> void {
        if (!self) {
            return;
        }
        primitive::Atomic64_fetchAdd(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
        if (!primitive::AtomicBool_load(&self->valid, primitive::ACQUIRE)) {
            primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
            return;
        }

        primitive::Mutex_lock(&self->mutex);

        if (primitive::AtomicBool_load(&self->closed, primitive::ACQUIRE)) {
            primitive::Mutex_unlock(&self->mutex);
            primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
            return;
        }

        while (!primitive::AtomicBool_load(&self->pending, primitive::ACQUIRE) && !primitive::AtomicBool_load(&self->closed, primitive::ACQUIRE)) {
            ConditionalVar_wait(&self->condition, &self->mutex);
        }

        if (!primitive::AtomicBool_load(&self->closed, primitive::ACQUIRE)) {
            primitive::AtomicBool_store(&self->pending, false, primitive::RELEASE);
        }

        primitive::Mutex_unlock(&self->mutex);

        primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
    }
    auto ConflatedSignal_timedWait(ConflatedSignal* self, const u64 timeoutMs) -> bool {
        if (!self) {
            return false;
        }

        primitive::Atomic64_fetchAdd(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
        if (!primitive::AtomicBool_load(&self->valid, primitive::ACQUIRE)) {
            primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
            return false;
        }

        primitive::Mutex_lock(&self->mutex);

        if (primitive::AtomicBool_load(&self->closed, primitive::ACQUIRE)) {
            primitive::Mutex_unlock(&self->mutex);
            primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);
            return false;
        }

        bool result = true;

        const u64 deadline = currentTimeMs() + timeoutMs;
        while (!primitive::AtomicBool_load(&self->pending, primitive::ACQUIRE) && !primitive::AtomicBool_load(&self->closed, primitive::ACQUIRE)) {
            const u64 now = currentTimeMs();
            if (now >= deadline) {
                result = false;
                break;
            }

            ConditionalVar_timedWait(&self->condition, &self->mutex, deadline - now);
        }

        if (primitive::AtomicBool_load(&self->closed, primitive::ACQUIRE)) {
            result = false;
        } else if (result) {
            primitive::AtomicBool_store(&self->pending, false, primitive::RELEASE);
        }

        primitive::Mutex_unlock(&self->mutex);

        primitive::Atomic64_fetchSub(&self->waiterCount, 1, primitive::SEQUENTIAL_CONSISTENT);

        return result;
    }
}