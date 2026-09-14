#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct ArrayList ArrayList;

    auto ArrayList_construct(usize initialCapacity) -> ArrayList*;
    auto ArrayList_destruct(ArrayList* self) -> bool;

    auto ArrayList_copy(const ArrayList* self, usize inElementSize) -> ArrayList*;
    auto ArrayList_move(ArrayList* self) -> ArrayList*;

    auto ArrayList_push(ArrayList* self, const void* inElement, usize inElementSize) -> bool;
    auto ArrayList_pushMove(ArrayList* self, void* inElement) -> bool;
    auto ArrayList_insertAt(ArrayList* self, usize index, const void* inElement, usize inElementSize) -> bool;

    auto ArrayList_pop(ArrayList* self) -> bool;
    auto ArrayList_removeAt(ArrayList* self, usize index) -> bool;
    auto ArrayList_clear(ArrayList* self) -> bool;

    auto ArrayList_reserve(ArrayList* self, usize newCapacity) -> bool;
    auto ArrayList_shrinkToFit(ArrayList* self) -> bool;

    auto ArrayList_get(const ArrayList* self, usize index) -> void*;

    auto ArrayList_set(const ArrayList* self, usize index, void* inElement) -> bool;

    auto ArrayList_size(const ArrayList* self) -> usize;
    auto ArrayList_capacity(const ArrayList* self) -> usize;

    auto ArrayList_isEmpty(const ArrayList* self) -> bool;
}