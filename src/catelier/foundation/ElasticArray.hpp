#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct ElasticArray ElasticArray;

    auto ElasticArray_construct(usize initialCapacity) -> ElasticArray*;
    auto ElasticArray_destruct(ElasticArray* self) -> bool;

    auto ElasticArray_copy(const ElasticArray* self, usize inElementSize) -> ElasticArray*;
    auto ElasticArray_move(ElasticArray* self) -> ElasticArray*;

    auto ElasticArray_push(ElasticArray* self, const void* inElement, usize inElementSize) -> bool;
    auto ElasticArray_pushMove(ElasticArray* self, void* inElement) -> bool;
    auto ElasticArray_insertAt(ElasticArray* self, usize index, const void* inElement, usize inElementSize) -> bool;

    auto ElasticArray_pop(ElasticArray* self) -> bool;
    auto ElasticArray_removeAt(ElasticArray* self, usize index) -> bool;
    auto ElasticArray_clear(ElasticArray* self) -> bool;

    auto ElasticArray_reserve(ElasticArray* self, usize newCapacity) -> bool;
    auto ElasticArray_shrinkToFit(ElasticArray* self) -> bool;

    auto ElasticArray_get(const ElasticArray* self, usize index) -> void*;

    auto ElasticArray_set(const ElasticArray* self, usize index, void* inElement) -> bool;

    auto ElasticArray_size(const ElasticArray* self) -> usize;
    auto ElasticArray_capacity(const ElasticArray* self) -> usize;

    auto ElasticArray_isEmpty(const ElasticArray* self) -> bool;
}