#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BitArray BitArray;

    auto BitArray_construct(usize initialCapacity) -> BitArray*;
    auto BitArray_destruct(BitArray* self) -> bool;

    auto BitArray_copy(const BitArray* self) -> BitArray*;
    auto BitArray_move(BitArray* self) -> BitArray*;

    auto BitArray_setOne(BitArray* self, usize index) -> bool;
    auto BitArray_fillOne(BitArray* self) -> bool;
    auto BitArray_setZero(BitArray* self, usize index) -> bool;
    auto BitArray_fillZero(BitArray* self) -> bool;
    auto BitArray_toggle(BitArray* self, usize index) -> bool;
    auto BitArray_toggleAll(BitArray* self) -> bool;

    auto BitArray_get(const BitArray* self, usize index, bool* outValue) -> bool;

    auto BitArray_findFirstOne(const BitArray* self, usize* outIndex) -> bool;
    auto BitArray_findFirstZero(const BitArray* self, usize* outIndex) -> bool;

    auto BitArray_bytes(const BitArray* self) -> u8*;
    auto BitArray_capacity(const BitArray* self) -> usize;
    auto BitArray_byteCount(const BitArray* self) -> usize;

    auto BitArray_countOnes(const BitArray* self) -> usize;
    auto BitArray_countZeros(const BitArray* self) -> usize;
}