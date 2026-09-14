#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BoundedQueue BoundedQueue;

    auto BoundedQueue_construct(usize initialCapacity, usize inElementSize) -> BoundedQueue*;
    auto BoundedQueue_destruct(BoundedQueue* self) -> bool;

    auto BoundedQueue_copy(const BoundedQueue* self) -> BoundedQueue*;
    auto BoundedQueue_move(BoundedQueue* self) -> BoundedQueue*;

    auto BoundedQueue_enqueue(BoundedQueue* self, const void* inElement) -> bool;
    auto BoundedQueue_enqueueSlot(BoundedQueue* self) -> void*;

    auto BoundedQueue_dequeue(BoundedQueue* self) -> bool;
    auto BoundedQueue_dequeueSlot(BoundedQueue* self) -> void*;
    auto BoundedQueue_clear(BoundedQueue* self) -> bool;

    auto BoundedQueue_head(const BoundedQueue* self) -> void*;
    auto BoundedQueue_tail(const BoundedQueue* self) -> void*;
    auto BoundedQueue_get(const BoundedQueue* self, usize index) -> void*;

    auto BoundedQueue_capacity(const BoundedQueue* self) -> usize;
    auto BoundedQueue_size(const BoundedQueue* self) -> usize;

    auto BoundedQueue_isEmpty(const BoundedQueue* self) -> bool;
    auto BoundedQueue_isFull(const BoundedQueue* self) -> bool;
}