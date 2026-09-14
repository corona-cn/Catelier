#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct RingBuffer RingBuffer;

    auto RingBuffer_construct(usize initialCapacity, usize inElementSize) -> RingBuffer*;
    auto RingBuffer_destruct(RingBuffer* self) -> bool;

    auto RingBuffer_copy(const RingBuffer* self) -> RingBuffer*;
    auto RingBuffer_move(RingBuffer* self) -> RingBuffer*;

    auto RingBuffer_pushHead(RingBuffer* self, const void* inElement) -> bool;
    auto RingBuffer_overwriteHead(RingBuffer* self, const void* inElement) -> bool;
    auto RingBuffer_pushHeadSlot(RingBuffer* self) -> void*;
    auto RingBuffer_pushTail(RingBuffer* self, const void* inElement) -> bool;
    auto RingBuffer_overwriteTail(RingBuffer* self, const void* inElement) -> bool;
    auto RingBuffer_pushTailSlot(RingBuffer* self) -> void*;

    auto RingBuffer_popHead(RingBuffer* self) -> bool;
    auto RingBuffer_popHeadSlot(RingBuffer* self) -> void*;
    auto RingBuffer_popTail(RingBuffer* self) -> bool;
    auto RingBuffer_popTailSlot(RingBuffer* self) -> void*;
    auto RingBuffer_clear(RingBuffer* self) -> bool;

    auto RingBuffer_get(const RingBuffer* self, usize index) -> void*;
    auto RingBuffer_head(const RingBuffer* self) -> void*;
    auto RingBuffer_tail(const RingBuffer* self) -> void*;

    auto RingBuffer_elements(const RingBuffer* self) -> u8*;
    auto RingBuffer_capacity(const RingBuffer* self) -> usize;
    auto RingBuffer_physicalCapacity(const RingBuffer* self) -> usize;
    auto RingBuffer_size(const RingBuffer* self) -> usize;

    auto RingBuffer_isEmpty(const RingBuffer* self) -> bool;
    auto RingBuffer_isFull(const RingBuffer* self) -> bool;
}