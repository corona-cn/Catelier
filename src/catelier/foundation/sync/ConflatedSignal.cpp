#include "ConflatedSignal.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

namespace Catelier::src::foundation::sync {
    namespace {
        #ifdef _WIN32
        auto currentTimeMs() -> u64 {
            return (u64) GetTickCount64();
        }
        #else
        auto currentTimeMs() -> u64 {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);

            return (u64) ts.tv_sec * 1000ULL + (u64) ts.tv_nsec / 1000000ULL;
        }
        #endif
    }

    auto ConflatedSignal_init(ConflatedSignal* self) -> bool {
        if (!self || self->valid) {
            return false;
        }

        self->valid = false;
        self->pending = false;

        if (!primitive::Mutex_init(&self->mutex)) {
            return false;
        }

        if (!ConditionalVar_init(&self->condition)) {
            primitive::Mutex_destroy(&self->mutex);
            return false;
        }

        self->valid = true;

        return true;
    }
    auto ConflatedSignal_destroy(ConflatedSignal* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        ConditionalVar_destroy(&self->condition);
        primitive::Mutex_destroy(&self->mutex);

        self->valid = false;

        return true;
    }

    auto ConflatedSignal_signal(ConflatedSignal* self) -> void {
        if (!self || !self->valid) {
            return;
        }

        primitive::Mutex_lock(&self->mutex);

        if (!self->pending) {
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

        while (!self->pending) {
            ConditionalVar_wait(&self->condition, &self->mutex);
        }

        self->pending = false;

        primitive::Mutex_unlock(&self->mutex);
    }
    auto ConflatedSignal_timedWait(ConflatedSignal* self, const u64 timeoutMs) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        primitive::Mutex_lock(&self->mutex);

        bool result = true;
        const u64 deadline = currentTimeMs() + timeoutMs;

        while (!self->pending) {
            const u64 now = currentTimeMs();
            if (now >= deadline) {
                result = false;
                break;
            }

            const u64 remaining = deadline - now;
            ConditionalVar_timedWait(&self->condition, &self->mutex, remaining);
        }

        if (result) {
            self->pending = false;
        }

        primitive::Mutex_unlock(&self->mutex);

        return result;
    }
}