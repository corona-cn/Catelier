#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef enum MemoryOrder {
        MEMORY_ORDER_RELAXED,
        MEMORY_ORDER_ACQUIRE,
        MEMORY_ORDER_RELEASE,
        MEMORY_ORDER_ACQ_REL,
        MEMORY_ORDER_SEQ_CST,
    } MemoryOrder;

    typedef struct Atomic {
        alignas(8) u8 storage[8];
    } Atomic;

    auto Atomic_init(Atomic* self, u64 value) -> bool;
    auto Atomic_destroy(Atomic* self) -> bool;

    auto Atomic_load(const Atomic* self, MemoryOrder order) -> u64;
    auto Atomic_store(Atomic* self, u64 value, MemoryOrder order) -> void;
    auto Atomic_exchange(Atomic* self, u64 value, MemoryOrder order) -> u64;
    auto Atomic_compareExchange(Atomic* self, u64* expected, u64 desired, MemoryOrder order) -> bool;

    auto Atomic_fetchAdd(Atomic* self, u64 delta, MemoryOrder order) -> u64;
    auto Atomic_fetchSub(Atomic* self, u64 delta, MemoryOrder order) -> u64;
}