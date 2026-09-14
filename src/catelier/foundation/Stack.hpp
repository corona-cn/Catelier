#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct Stack Stack;

    auto Stack_construct(usize initialCapacity, usize inElementSize) -> Stack*;
    auto Stack_destruct(Stack* self) -> bool;

    auto Stack_copy(const Stack* self) -> Stack*;
    auto Stack_move(Stack* self) -> Stack*;

    auto Stack_push(Stack* self, const void* inElement) -> bool;
    auto Stack_pushSlot(Stack* self) -> void*;

    auto Stack_pop(Stack* self) -> bool;
    auto Stack_popSlot(Stack* self) -> void*;
    auto Stack_clear(Stack* self) -> bool;

    auto Stack_peek(const Stack* self) -> void*;
    auto Stack_get(const Stack* self, usize index) -> void*;

    auto Stack_reserve(Stack* self, usize newCapacity) -> bool;
    auto Stack_shrinkToFit(Stack* self) -> bool;

    auto Stack_elements(const Stack* self) -> u8*;
    auto Stack_capacity(const Stack* self) -> usize;
    auto Stack_size(const Stack* self) -> usize;

    auto Stack_isEmpty(const Stack* self) -> bool;
}