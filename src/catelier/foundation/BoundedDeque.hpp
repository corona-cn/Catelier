#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BoundedDeque BoundedDeque;

    auto BoundedDeque_construct(usize initialCapacity, usize inElementSize) -> BoundedDeque*;
    auto BoundedDeque_destruct(BoundedDeque* self) -> bool;

    auto BoundedDeque_copy(const BoundedDeque* self) -> BoundedDeque*;
    auto BoundedDeque_move(BoundedDeque* self) -> BoundedDeque*;

    auto BoundedDeque_pushHead(BoundedDeque* self, const void* inElement) -> bool;
    auto BoundedDeque_pushHeadSlot(BoundedDeque* self) -> void*;
    auto BoundedDeque_pushTail(BoundedDeque* self, const void* inElement) -> bool;
    auto BoundedDeque_pushTailSlot(BoundedDeque* self) -> void*;

    auto BoundedDeque_popHead(BoundedDeque* self) -> bool;
    auto BoundedDeque_popHeadSlot(BoundedDeque* self) -> void*;
    auto BoundedDeque_popTail(BoundedDeque* self) -> bool;
    auto BoundedDeque_popTailSlot(BoundedDeque* self) -> void*;
    auto BoundedDeque_clear(BoundedDeque* self) -> bool;

    auto BoundedDeque_head(const BoundedDeque* self) -> void*;
    auto BoundedDeque_tail(const BoundedDeque* self) -> void*;
    auto BoundedDeque_get(const BoundedDeque* self, usize index) -> void*;

    auto BoundedDeque_capacity(const BoundedDeque* self) -> usize;
    auto BoundedDeque_size(const BoundedDeque* self) -> usize;

    auto BoundedDeque_isEmpty(const BoundedDeque* self) -> bool;
    auto BoundedDeque_isFull(const BoundedDeque* self) -> bool;
}