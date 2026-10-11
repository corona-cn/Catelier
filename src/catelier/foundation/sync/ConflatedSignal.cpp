#include "ConflatedSignal.hpp"

#include "../primitive/Thread.hpp"

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
        if (!self || self->valid) {
            return false;
        }

        self->valid = false;
        self->pending = false;
        self->closed = false;

        if (!primitive::Atomic_init(&self->waiterCount, 0)) {
            return false;
        }

        if (!primitive::Mutex_init(&self->mutex)) {
            primitive::Atomic_destroy(&self->waiterCount);
            return false;
        }

        if (!ConditionalVar_init(&self->condition)) {
            primitive::Mutex_destroy(&self->mutex);
            primitive::Atomic_destroy(&self->waiterCount);
            return false;
        }

        self->valid = true;

        return true;
    }
    auto ConflatedSignal_destroy(ConflatedSignal* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        primitive::Mutex_lock(&self->mutex);
        self->closed = true;
        ConditionalVar_broadcast(&self->condition);
        primitive::Mutex_unlock(&self->mutex);

        while (primitive::Atomic_load(&self->waiterCount, primitive::MEMORY_ORDER_ACQUIRE) > 0) {
            primitive::Thread_yield();
        }

        ConditionalVar_destroy(&self->condition);
        primitive::Mutex_destroy(&self->mutex);
        primitive::Atomic_destroy(&self->waiterCount);

        return true;
    }

    auto ConflatedSignal_signal(ConflatedSignal* self) -> void {
        if (!self || !self->valid) {
            return;
        }

        primitive::Mutex_lock(&self->mutex);

        if (!self->closed && !self->pending) {
            self->pending = true;
            ConditionalVar_signal(&self->condition);
        }

        primitive::Mutex_unlock(&self->mutex);
    }
    auto ConflatedSignal_wait(ConflatedSignal* self) -> void {
        if (!self || !self->valid) {
            return;
        }

        primitive::Mutex_lock(&self->mutex);

        primitive::Atomic_fetchAdd(&self->waiterCount, 1, primitive::MEMORY_ORDER_RELAXED);

        while (!self->pending && !self->closed) {
            ConditionalVar_wait(&self->condition, &self->mutex);
        }

        primitive::Mutex_unlock(&self->mutex);

        primitive::Atomic_fetchSub(&self->waiterCount, 1, primitive::MEMORY_ORDER_RELEASE);
    }
    auto ConflatedSignal_timedWait(ConflatedSignal* self, const u64 timeoutMs) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        primitive::Mutex_lock(&self->mutex);

        primitive::Atomic_fetchAdd(&self->waiterCount, 1, primitive::MEMORY_ORDER_RELAXED);

        bool result = true;
        const u64 deadline = currentTimeMs() + timeoutMs;

        while (!self->pending && !self->closed) {
            const u64 now = currentTimeMs();
            if (now >= deadline) {
                result = false;
                break;
            }

            const u64 remaining = deadline - now;
            ConditionalVar_timedWait(&self->condition, &self->mutex, remaining);
        }

        if (self->closed) {
            result = false;
        } else if (result) {
            self->pending = false;
        }

        primitive::Mutex_unlock(&self->mutex);

        primitive::Atomic_fetchSub(&self->waiterCount, 1, primitive::MEMORY_ORDER_RELEASE);

        return result;
    }
}