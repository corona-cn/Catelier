#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::concurrent {
    typedef struct ConcurrentChainHashMap ConcurrentChainHashMap;

    typedef struct ConcurrentChainHashMapNode ConcurrentChainHashMapNode;

    auto ConcurrentChainHashMap_construct(usize segmentCount, usize initialCapacity, usize expectedElementCount, usize inKeySize, usize inValueSize, u64 (*hash)(const void*), bool (*keyEquals)(const void*, const void*)) -> ConcurrentChainHashMap*;
    auto ConcurrentChainHashMap_destruct(ConcurrentChainHashMap* self) -> bool;

    auto ConcurrentChainHashMap_copy(const ConcurrentChainHashMap* self) -> ConcurrentChainHashMap*;
    auto ConcurrentChainHashMap_move(ConcurrentChainHashMap* self) -> ConcurrentChainHashMap*;

    auto ConcurrentChainHashMap_insert(ConcurrentChainHashMap* self, const void* inKey, const void* inValue) -> bool;
    auto ConcurrentChainHashMap_insertSlot(ConcurrentChainHashMap* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool;

    auto ConcurrentChainHashMap_remove(ConcurrentChainHashMap* self, const void* inKey) -> bool;
    auto ConcurrentChainHashMap_removeSlot(ConcurrentChainHashMap* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto ConcurrentChainHashMap_clear(ConcurrentChainHashMap* self) -> bool;

    auto ConcurrentChainHashMap_find(const ConcurrentChainHashMap* self, const void* inKey, void** valueOut) -> bool;

    auto ConcurrentChainHashMap_forEach(const ConcurrentChainHashMap* self, void (*action)(const void* key, void* value, void* userData), void* userData) -> void;

    auto ConcurrentChainHashMap_headAt(const ConcurrentChainHashMap* self, usize segmentIndex, usize bucketIndex) -> ConcurrentChainHashMapNode*;

    auto ConcurrentChainHashMap_reserve(ConcurrentChainHashMap* self, usize newCapacity) -> bool;

    auto ConcurrentChainHashMap_segmentCount(const ConcurrentChainHashMap* self) -> usize;
    auto ConcurrentChainHashMap_capacity(const ConcurrentChainHashMap* self) -> usize;
    auto ConcurrentChainHashMap_size(const ConcurrentChainHashMap* self) -> usize;
    auto ConcurrentChainHashMap_keySize(const ConcurrentChainHashMap* self) -> usize;
    auto ConcurrentChainHashMap_valueSize(const ConcurrentChainHashMap* self) -> usize;

    auto ConcurrentChainHashMap_isEmpty(const ConcurrentChainHashMap* self) -> bool;

    auto ConcurrentChainHashMapNode_key(const ConcurrentChainHashMapNode* node) -> void*;
    auto ConcurrentChainHashMapNode_value(const ConcurrentChainHashMapNode* node) -> void*;
    auto ConcurrentChainHashMapNode_next(const ConcurrentChainHashMapNode* node) -> ConcurrentChainHashMapNode*;
}