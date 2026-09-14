#pragma once
#include "BitArray.hpp"
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BitSet BitSet;

    auto BitSet_construct(usize initialCapacity) -> BitSet*;
    auto BitSet_destruct(BitSet* self) -> bool;

    auto BitSet_copy(const BitSet* self) -> BitSet*;
    auto BitSet_move(BitSet* self) -> BitSet*;

    auto BitSet_insert(const BitSet* self, usize value) -> bool;
    auto BitSet_insertAll(const BitSet* self) -> bool;

    auto BitSet_toggle(BitSet* self, usize value) -> bool;
    auto BitSet_toggleAll(BitSet* self) -> bool;

    auto BitSet_remove(BitSet* self, usize value) -> bool;
    auto BitSet_removeAll(BitSet* self) -> bool;
    auto BitSet_clear(BitSet* self) -> bool;

    auto BitSet_findFirst(const BitSet* self, usize* outValue) -> bool;
    auto BitSet_findNext(const BitSet* self, usize fromValue, usize* outValue) -> bool;

    auto BitSet_unionWith(BitSet* self, const BitSet* other) -> bool;
    auto BitSet_intersectWith(BitSet* self, const BitSet* other) -> bool;
    auto BitSet_differenceWith(BitSet* self, const BitSet* other) -> bool;
    auto BitSet_symmetricDifferenceWith(BitSet* self, const BitSet* other) -> bool;

    auto BitSet_bits(const BitSet* self) -> BitArray*;
    auto BitSet_capacity(const BitSet* self) -> usize;

    auto BitSet_count(const BitSet* self) -> usize;
    auto BitSet_isEmpty(const BitSet* self) -> bool;
    auto BitSet_contains(const BitSet* self, usize value) -> bool;
}