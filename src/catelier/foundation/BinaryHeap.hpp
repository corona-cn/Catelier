#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BinaryHeap BinaryHeap;

    auto BinaryHeap_construct(usize initialCapacity, usize inElementSize, bool (*compare)(const void*, const void*)) -> BinaryHeap*;
    auto BinaryHeap_destruct(BinaryHeap* self) -> bool;

    auto BinaryHeap_copy(const BinaryHeap* self) -> BinaryHeap*;
    auto BinaryHeap_move(BinaryHeap* self) -> BinaryHeap*;

    auto BinaryHeap_push(BinaryHeap* self, const void* inElement) -> bool;
    auto BinaryHeap_pushSlot(BinaryHeap* self) -> void*;

    auto BinaryHeap_pop(BinaryHeap* self) -> bool;
    auto BinaryHeap_popSlot(BinaryHeap* self) -> void*;
    auto BinaryHeap_clear(BinaryHeap* self) -> bool;

    auto BinaryHeap_siftUpTail(const BinaryHeap* self) -> bool;
    auto BinaryHeap_siftUp(const BinaryHeap* self, usize index) -> bool;
    auto BinaryHeap_siftDown(const BinaryHeap* self, usize index) -> bool;
    auto BinaryHeap_heapify(const BinaryHeap* self) -> bool;

    auto BinaryHeap_reserve(BinaryHeap* self, usize newCapacity) -> bool;
    auto BinaryHeap_shrinkToFit(BinaryHeap* self) -> bool;

    auto BinaryHeap_peek(const BinaryHeap* self) -> void*;
    auto BinaryHeap_get(const BinaryHeap* self, usize index) -> void*;

    auto BinaryHeap_elements(const BinaryHeap* self) -> u8*;
    auto BinaryHeap_elementSize(const BinaryHeap* self) -> usize;
    auto BinaryHeap_capacity(const BinaryHeap* self) -> usize;
    auto BinaryHeap_size(const BinaryHeap* self) -> usize;

    auto BinaryHeap_isEmpty(const BinaryHeap* self) -> bool;
}