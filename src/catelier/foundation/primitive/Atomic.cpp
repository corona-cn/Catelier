#include "Atomic.hpp"

#ifdef _WIN32
#include <windows.h>
#include <intrin.h>
#endif

namespace Catelier::src::foundation::primitive {
    #ifdef _WIN32
    namespace {
        auto asNative(const Atomic* self) -> volatile LONG64* {
            return (volatile LONG64*) self->storage;
        }
    }

    auto Atomic_init(Atomic* self, const u64 value) -> bool {
        if (!self) {
            return false;
        }

        *asNative(self) = (LONG64) value;

        return true;
    }
    auto Atomic_destroy(Atomic* self) -> bool {
        if (!self) {
            return false;
        }

        return true;
    }

    auto Atomic_load(const Atomic* self, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        const u64 value = (u64) *asNative(self);

        if (order != MEMORY_ORDER_RELAXED) {
            _ReadBarrier();
        }

        return value;
    }
    auto Atomic_store(Atomic* self, const u64 value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        if (order != MEMORY_ORDER_RELAXED) {
            _WriteBarrier();
        }

        *asNative(self) = (LONG64) value;
    }
    auto Atomic_exchange(Atomic* self, const u64 value, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        (void) order;

        return (u64) _InterlockedExchange64(asNative(self), (LONG64) value);
    }
    auto Atomic_compareExchange(Atomic* self, u64* expected, const u64 desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        (void) order;

        const LONG64 original = _InterlockedCompareExchange64(asNative(self), (LONG64) desired, (LONG64) *expected);
        if ((u64) original == *expected) {
            return true;
        }

        *expected = (u64) original;

        return false;
    }

    auto Atomic_fetchAdd(Atomic* self, const u64 delta, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        (void) order;

        return (u64) _InterlockedExchangeAdd64(asNative(self), (LONG64) delta);
    }
    auto Atomic_fetchSub(Atomic* self, const u64 delta, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        (void) order;

        return (u64) _InterlockedExchangeAdd64(asNative(self), -(LONG64) delta);
    }
    #else
    namespace {
        auto asNative(const Atomic* self) -> u64* {
            return (u64*) self->storage;
        }

        auto toNativeOrder(MemoryOrder order) -> int {
            switch (order) {
                case MEMORY_ORDER_RELAXED: return __ATOMIC_RELAXED;
                case MEMORY_ORDER_ACQUIRE: return __ATOMIC_ACQUIRE;
                case MEMORY_ORDER_RELEASE: return __ATOMIC_RELEASE;
                case MEMORY_ORDER_ACQ_REL: return __ATOMIC_ACQ_REL;
                case MEMORY_ORDER_SEQ_CST: return __ATOMIC_SEQ_CST;
            }

            return __ATOMIC_SEQ_CST;
        }
    }

    auto Atomic_init(Atomic* self, const u64 value) -> bool {
        if (!self) {
            return false;
        }

        __atomic_store_n(asNative(self), value, __ATOMIC_RELAXED);

        return true;
    }
    auto Atomic_destroy(Atomic* self) -> bool {
        if (!self) {
            return false;
        }

        return true;
    }

    auto Atomic_load(const Atomic* self, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        return __atomic_load_n(asNative(self), toNativeOrder(order));
    }
    auto Atomic_store(Atomic* self, const u64 value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        __atomic_store_n(asNative(self), value, toNativeOrder(order));
    }
    auto Atomic_exchange(Atomic* self, const u64 value, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        return __atomic_exchange_n(asNative(self), value, toNativeOrder(order));
    }
    auto Atomic_compareExchange(Atomic* self, u64* expected, const u64 desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        return __atomic_compare_exchange_n(asNative(self), expected, desired, false, toNativeOrder(order), __ATOMIC_RELAXED);
    }

    auto Atomic_fetchAdd(Atomic* self, const u64 delta, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        return __atomic_fetch_add(asNative(self), delta, toNativeOrder(order));
    }
    auto Atomic_fetchSub(Atomic* self, const u64 delta, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        return __atomic_fetch_sub(asNative(self), delta, toNativeOrder(order));
    }
    #endif
}