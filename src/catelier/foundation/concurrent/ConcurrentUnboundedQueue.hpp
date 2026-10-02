#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::concurrent {
    typedef struct ConcurrentUnboundedQueue ConcurrentUnboundedQueue;
    typedef struct ConcurrentUnboundedQueueNode ConcurrentUnboundedQueueNode;

    auto ConcurrentUnboundedQueue_construct(usize inElementSize) -> ConcurrentUnboundedQueue*;
    auto ConcurrentUnboundedQueue_destruct(ConcurrentUnboundedQueue* self) -> bool;

    auto ConcurrentUnboundedQueue_copy(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueue*;
    auto ConcurrentUnboundedQueue_move(ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueue*;

    auto ConcurrentUnboundedQueue_enqueue(ConcurrentUnboundedQueue* self, const void* inElement) -> bool;
    auto ConcurrentUnboundedQueue_enqueueSlot(ConcurrentUnboundedQueue* self) -> void*;

    auto ConcurrentUnboundedQueue_dequeue(ConcurrentUnboundedQueue* self) -> bool;
    auto ConcurrentUnboundedQueue_dequeueSlot(ConcurrentUnboundedQueue* self) -> void*;
    auto ConcurrentUnboundedQueue_clear(ConcurrentUnboundedQueue* self) -> bool;

    auto ConcurrentUnboundedQueue_head(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueueNode*;
    auto ConcurrentUnboundedQueue_tail(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueueNode*;
    auto ConcurrentUnboundedQueue_get(const ConcurrentUnboundedQueue* self, usize index) -> ConcurrentUnboundedQueueNode*;

    auto ConcurrentUnboundedQueue_begin(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueueNode*;
    auto ConcurrentUnboundedQueue_end() -> ConcurrentUnboundedQueueNode*;

    auto ConcurrentUnboundedQueue_size(const ConcurrentUnboundedQueue* self) -> usize;

    auto ConcurrentUnboundedQueue_isEmpty(const ConcurrentUnboundedQueue* self) -> bool;

    auto ConcurrentUnboundedQueueNode_next(const ConcurrentUnboundedQueueNode* node) -> ConcurrentUnboundedQueueNode*;
    auto ConcurrentUnboundedQueueNode_data(const ConcurrentUnboundedQueueNode* node) -> void*;
}