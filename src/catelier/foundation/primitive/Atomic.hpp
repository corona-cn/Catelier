#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef enum MemoryOrder {
        RELAXED,
        ACQUIRE,
        RELEASE,
        ACQUIRE_RELEASE,
        SEQUENTIAL_CONSISTENT,
    } MemoryOrder;

    auto Atomic8_load(const u8* self, MemoryOrder order) -> u8;
    auto Atomic8_store(u8* self, u8 value, MemoryOrder order) -> void;
    auto Atomic8_exchange(u8* self, u8 value, MemoryOrder order) -> u8;
    auto Atomic8_compareExchange(u8* self, u8* expected, u8 desired, MemoryOrder order) -> bool;
    auto Atomic8_fetchAdd(u8* self, u8 delta, MemoryOrder order) -> u8;
    auto Atomic8_fetchSub(u8* self, u8 delta, MemoryOrder order) -> u8;

    auto Atomic16_load(const u16* self, MemoryOrder order) -> u16;
    auto Atomic16_store(u16* self, u16 value, MemoryOrder order) -> void;
    auto Atomic16_exchange(u16* self, u16 value, MemoryOrder order) -> u16;
    auto Atomic16_compareExchange(u16* self, u16* expected, u16 desired, MemoryOrder order) -> bool;
    auto Atomic16_fetchAdd(u16* self, u16 delta, MemoryOrder order) -> u16;
    auto Atomic16_fetchSub(u16* self, u16 delta, MemoryOrder order) -> u16;

    auto Atomic32_load(const u32* self, MemoryOrder order) -> u32;
    auto Atomic32_store(u32* self, u32 value, MemoryOrder order) -> void;
    auto Atomic32_exchange(u32* self, u32 value, MemoryOrder order) -> u32;
    auto Atomic32_compareExchange(u32* self, u32* expected, u32 desired, MemoryOrder order) -> bool;
    auto Atomic32_fetchAdd(u32* self, u32 delta, MemoryOrder order) -> u32;
    auto Atomic32_fetchSub(u32* self, u32 delta, MemoryOrder order) -> u32;

    auto Atomic64_load(const u64* self, MemoryOrder order) -> u64;
    auto Atomic64_store(u64* self, u64 value, MemoryOrder order) -> void;
    auto Atomic64_exchange(u64* self, u64 value, MemoryOrder order) -> u64;
    auto Atomic64_compareExchange(u64* self, u64* expected, u64 desired, MemoryOrder order) -> bool;
    auto Atomic64_fetchAdd(u64* self, u64 delta, MemoryOrder order) -> u64;
    auto Atomic64_fetchSub(u64* self, u64 delta, MemoryOrder order) -> u64;

    auto AtomicPtr_load(void* const* self, MemoryOrder order) -> void*;
    auto AtomicPtr_store(void** self, void* value, MemoryOrder order) -> void;
    auto AtomicPtr_exchange(void** self, void* value, MemoryOrder order) -> void*;
    auto AtomicPtr_compareExchange(void** self, void** expected, void* desired, MemoryOrder order) -> bool;

    auto AtomicBool_load(const bool* self, MemoryOrder order) -> bool;
    auto AtomicBool_store(bool* self, bool value, MemoryOrder order) -> void;
    auto AtomicBool_exchange(bool* self, bool value, MemoryOrder order) -> bool;
    auto AtomicBool_compareExchange(bool* self, bool* expected, bool desired, MemoryOrder order) -> bool;
}