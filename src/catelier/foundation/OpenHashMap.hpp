#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct OpenHashMap OpenHashMap;

    auto OpenHashMap_construct(usize initialCapacity, usize inKeySize, usize inValueSize, u64 (*hash)(const void* key), bool (*keyEquals)(const void* a, const void* b)) -> OpenHashMap*;
    auto OpenHashMap_destruct(OpenHashMap* self) -> bool;

    auto OpenHashMap_copy(const OpenHashMap* self) -> OpenHashMap*;
    auto OpenHashMap_move(OpenHashMap* self) -> OpenHashMap*;

    auto OpenHashMap_insert(OpenHashMap* self, const void* inKey, const void* inValue) -> bool;
    auto OpenHashMap_insertSlot(OpenHashMap* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool;

    auto OpenHashMap_vacate(OpenHashMap* self, const void* inKey) -> bool;
    auto OpenHashMap_vacateSlot(OpenHashMap* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto OpenHashMap_vacateAll(OpenHashMap* self) -> bool;

    auto OpenHashMap_erase(OpenHashMap* self, const void* inKey) -> bool;
    auto OpenHashMap_eraseSlot(OpenHashMap* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto OpenHashMap_eraseAll(OpenHashMap* self) -> bool;

    auto OpenHashMap_find(const OpenHashMap* self, const void* inKey, void** valueOut) -> bool;

    auto OpenHashMap_nextOccupiedSlot(const OpenHashMap* self, usize fromIndex, void** keySlotOut, void** valueSlotOut, usize* nextIndexOut) -> bool;

    auto OpenHashMap_reserve(OpenHashMap* self, usize newCapacity) -> bool;

    auto OpenHashMap_capacity(const OpenHashMap* self) -> usize;
    auto OpenHashMap_size(const OpenHashMap* self) -> usize;
    auto OpenHashMap_keySize(const OpenHashMap* self) -> usize;
    auto OpenHashMap_valueSize(const OpenHashMap* self) -> usize;

    auto OpenHashMap_isEmpty(const OpenHashMap* self) -> bool;
}