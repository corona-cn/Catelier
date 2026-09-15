#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct UnboundedQueue UnboundedQueue;

    typedef struct UnboundedQueueNode UnboundedQueueNode;

    auto UnboundedQueue_construct(usize inElementSize) -> UnboundedQueue*;
    auto UnboundedQueue_destruct(UnboundedQueue* self) -> bool;

    auto UnboundedQueue_copy(const UnboundedQueue* self) -> UnboundedQueue*;
    auto UnboundedQueue_move(UnboundedQueue* self) -> UnboundedQueue*;

    auto UnboundedQueue_enqueue(UnboundedQueue* self, const void* inElement) -> bool;
    auto UnboundedQueue_enqueueSlot(UnboundedQueue* self) -> void*;

    auto UnboundedQueue_dequeue(UnboundedQueue* self) -> bool;
    auto UnboundedQueue_dequeueSlot(UnboundedQueue* self) -> void*;
    auto UnboundedQueue_clear(UnboundedQueue* self) -> bool;

    auto UnboundedQueue_head(const UnboundedQueue* self) -> void*;
    auto UnboundedQueue_tail(const UnboundedQueue* self) -> void*;
    auto UnboundedQueue_get(const UnboundedQueue* self, usize index) -> void*;

    auto UnboundedQueue_begin(const UnboundedQueue* self) -> UnboundedQueueNode*;
    auto UnboundedQueue_end() -> UnboundedQueueNode*;

    auto UnboundedQueue_size(const UnboundedQueue* self) -> usize;

    auto UnboundedQueue_isEmpty(const UnboundedQueue* self) -> bool;

    auto UnboundedQueueNode_next(const UnboundedQueueNode* node) -> UnboundedQueueNode*;
    auto UnboundedQueueNode_data(const UnboundedQueueNode* node) -> void*;
}