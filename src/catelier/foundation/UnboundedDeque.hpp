#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct UnboundedDeque UnboundedDeque;

    typedef struct UnboundedDequeNode UnboundedDequeNode;

    auto UnboundedDeque_construct(usize inElementSize) -> UnboundedDeque*;
    auto UnboundedDeque_destruct(UnboundedDeque* self) -> bool;

    auto UnboundedDeque_copy(const UnboundedDeque* self) -> UnboundedDeque*;
    auto UnboundedDeque_move(UnboundedDeque* self) -> UnboundedDeque*;

    auto UnboundedDeque_pushHead(UnboundedDeque* self, const void* inElement) -> bool;
    auto UnboundedDeque_pushHeadSlot(UnboundedDeque* self) -> void*;
    auto UnboundedDeque_pushTail(UnboundedDeque* self, const void* inElement) -> bool;
    auto UnboundedDeque_pushTailSlot(UnboundedDeque* self) -> void*;

    auto UnboundedDeque_popHead(UnboundedDeque* self) -> bool;
    auto UnboundedDeque_popHeadSlot(UnboundedDeque* self) -> void*;
    auto UnboundedDeque_popTail(UnboundedDeque* self) -> bool;
    auto UnboundedDeque_popTailSlot(UnboundedDeque* self) -> void*;
    auto UnboundedDeque_clear(UnboundedDeque* self) -> bool;

    auto UnboundedDeque_head(const UnboundedDeque* self) -> void*;
    auto UnboundedDeque_tail(const UnboundedDeque* self) -> void*;
    auto UnboundedDeque_get(const UnboundedDeque* self, usize index) -> void*;

    auto UnboundedDeque_begin(const UnboundedDeque* self) -> UnboundedDequeNode*;
    auto UnboundedDeque_end() -> UnboundedDequeNode*;

    auto UnboundedDeque_size(const UnboundedDeque* self) -> usize;

    auto UnboundedDeque_isEmpty(const UnboundedDeque* self) -> bool;

    auto UnboundedDequeNode_prev(const UnboundedDequeNode* node) -> UnboundedDequeNode*;
    auto UnboundedDequeNode_next(const UnboundedDequeNode* node) -> UnboundedDequeNode*;
    auto UnboundedDequeNode_data(const UnboundedDequeNode* node) -> void*;
}