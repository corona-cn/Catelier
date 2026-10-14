#include "Atomic.hpp"

#ifdef _WIN32

#include <windows.h>
#include <intrin.h>

#endif

namespace Catelier::src::foundation::primitive {
    #ifndef _WIN32

    namespace {
        auto toNativeOrder(const MemoryOrder order) -> int {
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

    #endif

    auto Atomic8_load(const u8* const self, const MemoryOrder order) -> u8 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        const u8 value = *((const volatile u8*) self);
        if (order != RELAXED) {
            _ReadBarrier();
        }
        return value;

        #else

        return __atomic_load_n(self, toNativeOrder(order));

        #endif
    }
    auto Atomic8_store(u8* const self, const u8 value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        #ifdef _WIN32

        if (order != RELAXED) {
            _WriteBarrier();
        }
        *((volatile u8*) self) = value;

        #else

        __atomic_store_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic8_exchange(u8* const self, const u8 value, const MemoryOrder order) -> u8 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u8) _InterlockedExchange8((volatile char*) self, (char) value);

        #else

        return __atomic_exchange_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic8_compareExchange(u8* const self, u8* const expected, const u8 desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        #ifdef _WIN32

        const u8 original = (u8) _InterlockedCompareExchange8((volatile char*) self, (char) desired, (char) *expected);
        if (original == *expected) {
            return true;
        }
        *expected = original;
        return false;

        #else

        return __atomic_compare_exchange_n(self, expected, desired, false, toNativeOrder(order), __ATOMIC_RELAXED);

        #endif
    }
    auto Atomic8_fetchAdd(u8* const self, const u8 delta, const MemoryOrder order) -> u8 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u8) _InterlockedExchangeAdd8((volatile char*) self, (char) delta);

        #else

        return __atomic_fetch_add(self, delta, toNativeOrder(order));

        #endif
    }
    auto Atomic8_fetchSub(u8* const self, const u8 delta, const MemoryOrder order) -> u8 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u8) _InterlockedExchangeAdd8((volatile char*) self, (char) -delta);

        #else

        return __atomic_fetch_sub(self, delta, toNativeOrder(order));

        #endif
    }

    auto Atomic16_load(const u16* const self, const MemoryOrder order) -> u16 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        const u16 value = *((const volatile u16*) self);
        if (order != RELAXED) {
            _ReadBarrier();
        }
        return value;

        #else

        return __atomic_load_n(self, toNativeOrder(order));

        #endif
    }
    auto Atomic16_store(u16* const self, const u16 value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        #ifdef _WIN32

        if (order != RELAXED) {
            _WriteBarrier();
        }
        *((volatile u16*) self) = value;

        #else

        __atomic_store_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic16_exchange(u16* const self, const u16 value, const MemoryOrder order) -> u16 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u16) _InterlockedExchange16((volatile short*) self, (short) value);

        #else

        return __atomic_exchange_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic16_compareExchange(u16* const self, u16* const expected, const u16 desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        #ifdef _WIN32

        const u16 original = (u16) _InterlockedCompareExchange16((volatile short*) self, (short) desired, (short) *expected);
        if (original == *expected) {
            return true;
        }
        *expected = original;
        return false;

        #else

        return __atomic_compare_exchange_n(self, expected, desired, false, toNativeOrder(order), __ATOMIC_RELAXED);

        #endif
    }
    auto Atomic16_fetchAdd(u16* const self, const u16 delta, const MemoryOrder order) -> u16 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u16) _InterlockedExchangeAdd16((volatile short*) self, (short) delta);

        #else

        return __atomic_fetch_add(self, delta, toNativeOrder(order));

        #endif
    }
    auto Atomic16_fetchSub(u16* const self, const u16 delta, const MemoryOrder order) -> u16 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u16) _InterlockedExchangeAdd16((volatile short*) self, (short) -delta);

        #else

        return __atomic_fetch_sub(self, delta, toNativeOrder(order));

        #endif
    }

    auto Atomic32_load(const u32* const self, const MemoryOrder order) -> u32 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        const u32 value = *((const volatile u32*) self);
        if (order != RELAXED) {
            _ReadBarrier();
        }
        return value;

        #else

        return __atomic_load_n(self, toNativeOrder(order));

        #endif
    }
    auto Atomic32_store(u32* const self, const u32 value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        #ifdef _WIN32

        if (order != RELAXED) {
            _WriteBarrier();
        }
        *((volatile u32*) self) = value;

        #else

        __atomic_store_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic32_exchange(u32* const self, const u32 value, const MemoryOrder order) -> u32 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u32) _InterlockedExchange((volatile long*) self, (long) value);

        #else

        return __atomic_exchange_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic32_compareExchange(u32* const self, u32* const expected, const u32 desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        #ifdef _WIN32

        const u32 original = (u32) _InterlockedCompareExchange((volatile long*) self, (long) desired, (long) *expected);
        if (original == *expected) {
            return true;
        }
        *expected = original;
        return false;

        #else

        return __atomic_compare_exchange_n(self, expected, desired, false, toNativeOrder(order), __ATOMIC_RELAXED);

        #endif
    }
    auto Atomic32_fetchAdd(u32* const self, const u32 delta, const MemoryOrder order) -> u32 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u32) _InterlockedExchangeAdd((volatile long*) self, (long) delta);

        #else

        return __atomic_fetch_add(self, delta, toNativeOrder(order));

        #endif
    }
    auto Atomic32_fetchSub(u32* const self, const u32 delta, const MemoryOrder order) -> u32 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u32) _InterlockedExchangeAdd((volatile long*) self, -(long) delta);

        #else

        return __atomic_fetch_sub(self, delta, toNativeOrder(order));

        #endif
    }

    auto Atomic64_load(const u64* const self, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        const u64 value = (u64) *((const volatile LONG64*) self);
        if (order != RELAXED) {
            _ReadBarrier();
        }
        return value;

        #else

        return __atomic_load_n(self, toNativeOrder(order));

        #endif
    }
    auto Atomic64_store(u64* const self, const u64 value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        #ifdef _WIN32

        if (order != RELAXED) {
            _WriteBarrier();
        }
        *((volatile LONG64*) self) = (LONG64) value;

        #else

        __atomic_store_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic64_exchange(u64* const self, const u64 value, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u64) _InterlockedExchange64((volatile LONG64*) self, (LONG64) value);

        #else

        return __atomic_exchange_n(self, value, toNativeOrder(order));

        #endif
    }
    auto Atomic64_compareExchange(u64* const self, u64* const expected, const u64 desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        #ifdef _WIN32

        const u64 original = (u64) _InterlockedCompareExchange64((volatile LONG64*) self, (LONG64) desired, (LONG64) *expected);
        if (original == *expected) {
            return true;
        }
        *expected = original;
        return false;

        #else

        return __atomic_compare_exchange_n(self, expected, desired, false, toNativeOrder(order), __ATOMIC_RELAXED);

        #endif
    }
    auto Atomic64_fetchAdd(u64* const self, const u64 delta, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u64) _InterlockedExchangeAdd64((volatile LONG64*) self, (LONG64) delta);

        #else

        return __atomic_fetch_add(self, delta, toNativeOrder(order));

        #endif
    }
    auto Atomic64_fetchSub(u64* const self, const u64 delta, const MemoryOrder order) -> u64 {
        if (!self) {
            return 0;
        }

        #ifdef _WIN32

        return (u64) _InterlockedExchangeAdd64((volatile LONG64*) self, -(LONG64) delta);

        #else

        return __atomic_fetch_sub(self, delta, toNativeOrder(order));

        #endif
    }

    auto AtomicPtr_load(void* const* const self, const MemoryOrder order) -> void* {
        if (!self) {
            return nullptr;
        }

        #ifdef _WIN32

        void* const value = *((void* const volatile*) self);
        if (order != RELAXED) {
            _ReadBarrier();
        }
        return value;

        #else

        return __atomic_load_n((void**) self, toNativeOrder(order));

        #endif
    }
    auto AtomicPtr_store(void** const self, void* const value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        #ifdef _WIN32

        if (order != RELAXED) {
            _WriteBarrier();
        }
        *((void* volatile*) self) = value;

        #else

        __atomic_store_n(self, value, toNativeOrder(order));

        #endif
    }
    auto AtomicPtr_exchange(void** const self, void* const value, const MemoryOrder order) -> void* {
        if (!self) {
            return nullptr;
        }

        #ifdef _WIN32

        return _InterlockedExchangePointer((void* volatile*) self, value);

        #else

        return __atomic_exchange_n(self, value, toNativeOrder(order));

        #endif
    }
    auto AtomicPtr_compareExchange(void** const self, void** const expected, void* const desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        #ifdef _WIN32

        void* const original = _InterlockedCompareExchangePointer((void* volatile*) self, desired, *expected);
        if (original == *expected) {
            return true;
        }
        *expected = original;
        return false;

        #else

        return __atomic_compare_exchange_n(self, expected, desired, false, toNativeOrder(order), __ATOMIC_RELAXED);

        #endif
    }

    auto AtomicBool_load(const bool* const self, const MemoryOrder order) -> bool {
        if (!self) {
            return false;
        }

        #ifdef _WIN32

        const u8 value = *((const volatile u8*) self);
        if (order != RELAXED) {
            _ReadBarrier();
        }
        return value != 0;

        #else

        return __atomic_load_n((const u8*) self, toNativeOrder(order)) != 0;

        #endif
    }
    auto AtomicBool_store(bool* const self, const bool value, const MemoryOrder order) -> void {
        if (!self) {
            return;
        }

        #ifdef _WIN32

        if (order != RELAXED) {
            _WriteBarrier();
        }
        *((volatile u8*) self) = value ? 1 : 0;

        #else

        __atomic_store_n((u8*) self, value ? 1 : 0, toNativeOrder(order));

        #endif
    }
    auto AtomicBool_exchange(bool* const self, const bool value, const MemoryOrder order) -> bool {
        if (!self) {
            return false;
        }

        #ifdef _WIN32

        return _InterlockedExchange8((volatile char*) self, (char) (value ? 1 : 0)) != 0;

        #else

        return __atomic_exchange_n((u8*) self, value ? 1 : 0, toNativeOrder(order)) != 0;

        #endif
    }
    auto AtomicBool_compareExchange(bool* const self, bool* const expected, const bool desired, const MemoryOrder order) -> bool {
        if (!self || !expected) {
            return false;
        }

        #ifdef _WIN32

        const u8 original = (u8) _InterlockedCompareExchange8((volatile char*) self, (char) (desired ? 1 : 0), (char) (*expected ? 1 : 0));
        if ((original != 0) == *expected) {
            return true;
        }
        *expected = original != 0;
        return false;

        #else

        u8 expectedByte = *expected ? 1 : 0;
        const bool result = __atomic_compare_exchange_n((u8*) self, &expectedByte, desired ? 1 : 0, false, toNativeOrder(order), __ATOMIC_RELAXED);
        *expected = expectedByte != 0;
        return result;

        #endif
    }
}