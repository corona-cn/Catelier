#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct ChainHashMap ChainHashMap;

    typedef struct ChainHashMapNode ChainHashMapNode;

    auto ChainHashMap_construct(usize initialCapacity, u64 (*hash)(const void*), bool (*keyEquals)(const void*, const void*)) -> ChainHashMap*;
    auto ChainHashMap_destruct(ChainHashMap* self) -> bool;

    auto ChainHashMap_copy(const ChainHashMap* self) -> ChainHashMap*;
    auto ChainHashMap_move(ChainHashMap* self) -> ChainHashMap*;

    auto ChainHashMap_insert(ChainHashMap* self, const void* inKey, usize inKeySize, const void* inValue, usize inValueSize) -> bool;
    auto ChainHashMap_insertSlot(ChainHashMap* self, const void* inKey, usize inKeySize, usize inValueSize, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool;

    auto ChainHashMap_remove(ChainHashMap* self, const void* inKey) -> bool;
    auto ChainHashMap_removeSlot(ChainHashMap* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto ChainHashMap_clear(ChainHashMap* self) -> bool;

    auto ChainHashMap_find(const ChainHashMap* self, const void* inKey, void** valueOut) -> bool;

    auto ChainHashMap_headAt(const ChainHashMap* self, usize index) -> ChainHashMapNode*;

    auto ChainHashMap_reserve(ChainHashMap* self, usize newCapacity) -> bool;

    auto ChainHashMap_capacity(const ChainHashMap* self) -> usize;
    auto ChainHashMap_size(const ChainHashMap* self) -> usize;
    auto ChainHashMap_keySize(const ChainHashMap* self) -> usize;
    auto ChainHashMap_valueSize(const ChainHashMap* self) -> usize;

    auto ChainHashMap_isEmpty(const ChainHashMap* self) -> bool;

    auto ChainHashMapNode_key(const ChainHashMapNode* node) -> void*;
    auto ChainHashMapNode_value(const ChainHashMapNode* node) -> void*;
    auto ChainHashMapNode_next(const ChainHashMapNode* node) -> ChainHashMapNode*;
}