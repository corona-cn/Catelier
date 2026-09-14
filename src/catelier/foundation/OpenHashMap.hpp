#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct OpenHashMap OpenHashMap;

    auto OpenHashMap_construct(usize initialCapacity, u64 (*hash)(const void* key), bool (*keyEquals)(const void* a, const void* b)) -> OpenHashMap*;
    auto OpenHashMap_destruct(OpenHashMap* self) -> bool;

    auto OpenHashMap_copy(const OpenHashMap* self) -> OpenHashMap*;
    auto OpenHashMap_move(OpenHashMap* self) -> OpenHashMap*;

    auto OpenHashMap_insert(OpenHashMap* self, const void* inKey, usize inKeySize, const void* inValue, usize inValueSize) -> bool;

    auto OpenHashMap_vacate(OpenHashMap* self, const void* inKey) -> bool;
    auto OpenHashMap_vacateAll(OpenHashMap* self) -> bool;
    auto OpenHashMap_erase(OpenHashMap* self, const void* inKey) -> bool;
    auto OpenHashMap_eraseAll(OpenHashMap* self) -> bool;

    auto OpenHashMap_find(const OpenHashMap* self, const void* inKey, void** valueOut) -> bool;

    auto OpenHashMap_reserve(OpenHashMap* self, usize newCapacity) -> bool;

    auto OpenHashMap_size(const OpenHashMap* self) -> usize;
    auto OpenHashMap_keySize(const OpenHashMap* self) -> usize;
    auto OpenHashMap_valueSize(const OpenHashMap* self) -> usize;
    auto OpenHashMap_capacity(const OpenHashMap* self) -> usize;

    auto OpenHashMap_isEmpty(const OpenHashMap* self) -> bool;
}