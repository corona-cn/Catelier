#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct HeapString HeapString;

    auto HeapString_construct() -> HeapString*;
    auto HeapString_constructFrom(const u8* inChars, usize inLength) -> HeapString*;
    auto HeapString_destruct(HeapString* self) -> bool;

    auto HeapString_copy(const HeapString* self) -> HeapString*;
    auto HeapString_move(HeapString* self) -> HeapString*;

    auto HeapString_append(HeapString* self, const HeapString* other) -> bool;
    auto HeapString_appendChars(HeapString* self, const u8* inChars, usize inLength) -> bool;
    auto HeapString_appendChar(HeapString* self, u8 inChar) -> bool;
    auto HeapString_insert(HeapString* self, usize index, const HeapString* other) -> bool;

    auto HeapString_remove(HeapString* self, usize start, usize length) -> bool;
    auto HeapString_clear(HeapString* self) -> bool;

    auto HeapString_findBF(const HeapString* self, const u8* inPattern, usize patternLength) -> ssize;
    auto HeapString_findKMP(const HeapString* self, const u8* inPattern, usize patternLength) -> ssize;
    auto HeapString_find(const HeapString* self, const u8* inPattern, usize patternLength) -> ssize;
    auto HeapString_replace(HeapString* self, const u8* inOldPattern, usize oldLength, const u8* inNewPattern, usize newLength) -> usize;

    auto HeapString_substring(const HeapString* self, usize start, usize length) -> HeapString*;

    auto HeapString_reserve(HeapString* self, usize newCapacity) -> bool;
    auto HeapString_shrinkToFit(HeapString* self) -> bool;

    auto HeapString_chars(const HeapString* self) -> u8*;
    auto HeapString_charAt(const HeapString* self, usize index) -> u8;
    auto HeapString_setCharAt(HeapString* self, usize index, u8 inChar) -> bool;

    auto HeapString_compare(const HeapString* a, const HeapString* b) -> i32;
    auto HeapString_equals(const HeapString* a, const HeapString* b) -> bool;

    auto HeapString_length(const HeapString* self) -> usize;
    auto HeapString_capacity(const HeapString* self) -> usize;

    auto HeapString_isEmpty(const HeapString* self) -> bool;
}