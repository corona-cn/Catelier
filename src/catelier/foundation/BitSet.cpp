#include "BitSet.hpp"

#include <cstdlib>

#include "BitArray.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 8;
    }

    typedef struct BitSet {
        BitArray* bits;
    } BitSet;

    auto BitSet_construct(const usize initialCapacity) -> BitSet* {
        usize capacity = initialCapacity;
        if (capacity < DEFAULT_CAPACITY) {
            capacity = DEFAULT_CAPACITY;
        }

        auto* const self = (BitSet*) malloc(sizeof(BitSet));
        if (!self) {
            return nullptr;
        }

        self->bits = BitArray_construct(capacity);
        if (!self->bits) {
            free(self);
            return nullptr;
        }

        return self;
    }
    auto BitSet_destruct(BitSet* self) -> bool {
        if (!self) {
            return false;
        }

        BitArray_destruct(self->bits);
        free(self);

        return true;
    }

    auto BitSet_copy(const BitSet* self) -> BitSet* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BitSet*) malloc(sizeof(BitSet));
        if (!newSelf) {
            return nullptr;
        }

        // 深拷贝底层位数组
        newSelf->bits = BitArray_copy(self->bits);
        if (!newSelf->bits) {
            free(newSelf);
            return nullptr;
        }

        return newSelf;
    }
    auto BitSet_move(BitSet* self) -> BitSet* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BitSet*) malloc(sizeof(BitSet));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->bits = self->bits;

        self->bits = nullptr;

        return newSelf;
    }

    auto BitSet_insert(const BitSet* self, const usize value) -> bool {
        if (!self) {
            return false;
        }

        return BitArray_setOne(self->bits, value);
    }
    auto BitSet_insertAll(const BitSet* self) -> bool {
        if (!self) {
            return false;
        }

        return BitArray_fillOne(self->bits);
    }

    auto BitSet_toggle(const BitSet* self, const usize value) -> bool {
        if (!self) {
            return false;
        }

        return BitArray_toggle(self->bits, value);
    }
    auto BitSet_toggleAll(const BitSet* self) -> bool {
        if (!self) {
            return false;
        }

        return BitArray_toggleAll(self->bits);
    }

    auto BitSet_remove(const BitSet* self, const usize value) -> bool {
        if (!self) {
            return false;
        }

        return BitArray_setZero(self->bits, value);
    }
    auto BitSet_removeAll(const BitSet* self) -> bool {
        if (!self) {
            return false;
        }

        return BitArray_fillZero(self->bits);
    }
    auto BitSet_clear(const BitSet* self) -> bool {
        if (!self) {
            return false;
        }

        return BitArray_fillZero(self->bits);
    }

    auto BitSet_findFirst(const BitSet* self, usize* outValue) -> bool {
        if (!self || !outValue) {
            return false;
        }

        return BitArray_findFirstOne(self->bits, outValue);
    }
    auto BitSet_findNext(const BitSet* self, const usize fromValue, usize* outValue) -> bool {
        if (!self || !outValue) {
            return false;
        }

        const usize capacity = BitArray_capacity(self->bits);
        if (fromValue >= capacity) {
            return false;
        }

        for (usize value = fromValue; value < capacity; ++value) {
            bool contains = false;
            if (!BitArray_get(self->bits, value, &contains)) {
                return false;
            }

            if (contains) {
                *outValue = value;
                return true;
            }
        }

        return false;
    }

    auto BitSet_unionWith(const BitSet* self, const BitSet* other) -> bool {
        if (!self || !other) {
            return false;
        }

        // 集合语义：self = self ∪ other，等价于逐字节按位或
        // 只在两个集合共同覆盖的字节范围内操作，超出部分保持原状
        const usize selfByteCount = BitArray_byteCount(self->bits);
        const usize otherByteCount = BitArray_byteCount(other->bits);
        const usize commonByteCount = selfByteCount < otherByteCount ? selfByteCount : otherByteCount;

        u8* const selfBytes = BitArray_bytes(self->bits);
        const u8* const otherBytes = BitArray_bytes(other->bits);
        for (usize i = 0; i < commonByteCount; ++i) {
            *(selfBytes + i) |= *(otherBytes + i);
        }

        return true;
    }
    auto BitSet_intersectWith(const BitSet* self, const BitSet* other) -> bool {
        if (!self || !other) {
            return false;
        }

        // 集合语义：self = self ∩ other，等价于逐字节按位与
        // 未与 other 覆盖的字节要全部清零，因为交集不会包含它们
        const usize selfByteCount = BitArray_byteCount(self->bits);
        const usize otherByteCount = BitArray_byteCount(other->bits);
        const usize commonByteCount = selfByteCount < otherByteCount ? selfByteCount : otherByteCount;

        u8* const selfBytes = BitArray_bytes(self->bits);
        const u8* const otherBytes = BitArray_bytes(other->bits);
        for (usize i = 0; i < commonByteCount; ++i) {
            *(selfBytes + i) &= *(otherBytes + i);
        }

        // self 超出 other 覆盖范围的字节不属于交集，清零
        for (usize i = commonByteCount; i < selfByteCount; ++i) {
            *(selfBytes + i) = 0;
        }

        return true;
    }
    auto BitSet_differenceWith(const BitSet* self, const BitSet* other) -> bool {
        if (!self || !other) {
            return false;
        }

        // 集合语义：self = self \ other，等价于逐字节 a & ~b
        // 只在两个集合共同覆盖的字节范围内操作，超出部分保持原状
        const usize selfByteCount = BitArray_byteCount(self->bits);
        const usize otherByteCount = BitArray_byteCount(other->bits);
        const usize commonByteCount = selfByteCount < otherByteCount ? selfByteCount : otherByteCount;

        u8* const selfBytes = BitArray_bytes(self->bits);
        const u8* const otherBytes = BitArray_bytes(other->bits);
        for (usize i = 0; i < commonByteCount; ++i) {
            *(selfBytes + i) &= (u8) ~*(otherBytes + i);
        }

        return true;
    }
    auto BitSet_symmetricDifferenceWith(const BitSet* self, const BitSet* other) -> bool {
        if (!self || !other) {
            return false;
        }

        // 集合语义：self = self △ other，等价于逐字节按位异或
        // 只在两个集合共同覆盖的字节范围内操作，超出部分保持原状
        const usize selfByteCount = BitArray_byteCount(self->bits);
        const usize otherByteCount = BitArray_byteCount(other->bits);
        const usize commonByteCount = selfByteCount < otherByteCount ? selfByteCount : otherByteCount;

        u8* const selfBytes = BitArray_bytes(self->bits);
        const u8* const otherBytes = BitArray_bytes(other->bits);
        for (usize i = 0; i < commonByteCount; ++i) {
            *(selfBytes + i) ^= *(otherBytes + i);
        }

        return true;
    }

    auto BitSet_bits(const BitSet* self) -> BitArray* {
        if (!self) {
            return nullptr;
        }

        return self->bits;
    }
    auto BitSet_capacity(const BitSet* self) -> usize {
        if (!self) {
            return 0;
        }

        return BitArray_capacity(self->bits);
    }

    auto BitSet_contains(const BitSet* self, const usize value) -> bool {
        if (!self) {
            return false;
        }

        bool result = false;
        BitArray_get(self->bits, value, &result);

        return result;
    }
    auto BitSet_count(const BitSet* self) -> usize {
        if (!self) {
            return 0;
        }

        return BitArray_countOnes(self->bits);
    }
    auto BitSet_isEmpty(const BitSet* self) -> bool {
        if (!self) {
            return true;
        }

        return BitArray_countOnes(self->bits) == 0;
    }
}