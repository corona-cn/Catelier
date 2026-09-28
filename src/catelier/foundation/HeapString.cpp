#include "HeapString.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    typedef struct HeapString {
        u8* chars;
        usize length;
        usize capacity;
    } HeapString;

    auto HeapString_construct() -> HeapString* {
        auto* const self = (HeapString*) malloc(sizeof(HeapString));
        if (!self) {
            return nullptr;
        }

        self->chars = (u8*) malloc(DEFAULT_CAPACITY);
        if (!self->chars) {
            free(self);
            return nullptr;
        }

        self->length = 0;
        self->capacity = DEFAULT_CAPACITY;

        return self;
    }
    auto HeapString_constructFrom(const u8* inChars, const usize inLength) -> HeapString* {
        if (inLength > 0 && !inChars) {
            return nullptr;
        }

        auto* const self = HeapString_construct();
        if (!self) {
            return nullptr;
        }

        if (inLength > 0) {
            if (!HeapString_appendChars(self, inChars, inLength)) {
                HeapString_destruct(self);
                return nullptr;
            }
        }

        return self;
    }
    auto HeapString_destruct(HeapString* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->chars);

        free(self);

        return true;
    }

    auto HeapString_copy(const HeapString* self) -> HeapString* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (HeapString*) malloc(sizeof(HeapString));
        if (!newSelf) {
            return nullptr;
        }

        // 空串时不需要分配缓冲区
        if (self->capacity == 0) {
            newSelf->chars = nullptr;
            newSelf->length = 0;
            newSelf->capacity = 0;
            return newSelf;
        }

        newSelf->chars = (u8*) malloc(self->capacity);
        if (!newSelf->chars) {
            free(newSelf);
            return nullptr;
        }

        if (self->length > 0) {
            memcpy(newSelf->chars, self->chars, self->length);
        }

        newSelf->length = self->length;
        newSelf->capacity = self->capacity;

        return newSelf;
    }
    auto HeapString_move(HeapString* self) -> HeapString* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (HeapString*) malloc(sizeof(HeapString));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->chars = self->chars;
        newSelf->length = self->length;
        newSelf->capacity = self->capacity;

        self->chars = nullptr;
        self->length = 0;
        self->capacity = 0;

        return newSelf;
    }

    auto HeapString_append(HeapString* self, const HeapString* other) -> bool {
        if (!self || !other) {
            return false;
        }

        return HeapString_appendChars(self, other->chars, other->length);
    }
    auto HeapString_appendChars(HeapString* self, const u8* inChars, const usize inLength) -> bool {
        if (!self || (inLength > 0 && !inChars)) {
            return false;
        }

        if (inLength == 0) {
            return true;
        }

        const usize newLength = self->length + inLength;

        // 容量不足时翻倍扩容
        if (newLength > self->capacity) {
            usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity;
            while (newCapacity < newLength) {
                newCapacity *= 2;
            }

            if (!HeapString_reserve(self, newCapacity)) {
                return false;
            }
        }

        memcpy(self->chars + self->length, inChars, inLength);
        self->length = newLength;

        return true;
    }
    auto HeapString_appendChar(HeapString* self, const u8 inChar) -> bool {
        return HeapString_appendChars(self, &inChar, 1);
    }
    auto HeapString_insert(HeapString* self, const usize index, const HeapString* other) -> bool {
        if (!self || index > self->length || !other) {
            return false;
        }

        if (other->length == 0) {
            return true;
        }

        const usize newLength = self->length + other->length;

        // 容量不足时翻倍扩容
        if (newLength > self->capacity) {
            usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity;
            while (newCapacity < newLength) {
                newCapacity *= 2;
            }

            if (!HeapString_reserve(self, newCapacity)) {
                return false;
            }
        }

        // 移动 [index, length) 的字符到 [index + other->length, ...)
        memmove(self->chars + index + other->length, self->chars + index, self->length - index);

        // 插入新串
        memcpy(self->chars + index, other->chars, other->length);

        self->length = newLength;

        return true;
    }

    auto HeapString_remove(HeapString* self, const usize start, const usize length) -> bool {
        if (!self || start > self->length || length > self->length - start) {
            return false;
        }

        if (length == 0) {
            return true;
        }

        memmove(self->chars + start, self->chars + start + length, self->length - start - length);

        self->length -= length;

        return true;
    }
    auto HeapString_clear(HeapString* self) -> bool {
        if (!self || self->length == 0) {
            return false;
        }

        self->length = 0;

        return true;
    }

    auto HeapString_findBF(const HeapString* self, const u8* inPattern, const usize patternLength) -> ssize {
        if (!self || !inPattern || patternLength == 0) {
            return -1;
        }

        if (self->length < patternLength) {
            return -1;
        }

        const usize end = self->length - patternLength;

        for (usize i = 0; i <= end; ++i) {
            usize j = 0;
            while (j < patternLength && *(self->chars + i + j) == *(inPattern + j)) {
                ++j;
            }

            if (j == patternLength) {
                return (ssize) i;
            }
        }

        return -1;
    }
    auto HeapString_findKMP(const HeapString* self, const u8* inPattern, const usize patternLength) -> ssize {
        if (!self || !inPattern || patternLength == 0) {
            return -1;
        }

        if (self->length < patternLength) {
            return -1;
        }

        // 构造 next 数组
        auto* const next = (ssize*) malloc(sizeof(ssize) * patternLength);
        if (!next) {
            return -1;
        }

        *(next + 0) = -1;

        ssize buildIndex = 0;
        ssize prefixLength = -1;

        while (buildIndex < (ssize) patternLength - 1) {
            if (prefixLength == -1 || *(inPattern + buildIndex) == *(inPattern + prefixLength)) {
                ++buildIndex;
                ++prefixLength;
                *(next + buildIndex) = prefixLength;
            } else {
                prefixLength = *(next + prefixLength);
            }
        }

        // KMP 匹配
        ssize textIndex = 0;
        ssize patternIndex = 0;

        while (textIndex < (ssize) self->length && patternIndex < (ssize) patternLength) {
            if (patternIndex == -1 || *(self->chars + textIndex) == *(inPattern + patternIndex)) {
                ++textIndex;
                ++patternIndex;
            } else {
                patternIndex = *(next + patternIndex);
            }
        }

        free(next);

        if (patternIndex == (ssize) patternLength) {
            return textIndex - patternIndex;
        }

        return -1;
    }
    auto HeapString_find(const HeapString* self, const u8* inPattern, const usize patternLength) -> ssize {
        return HeapString_findKMP(self, inPattern, patternLength);
    }
    auto HeapString_replace(HeapString* self, const u8* inOldPattern, const usize oldLength, const u8* inNewPattern, const usize newLength) -> usize {
        if (!self || !inOldPattern || oldLength == 0 || oldLength > self->length) {
            return 0;
        }

        if (newLength > 0 && !inNewPattern) {
            return 0;
        }

        // 第一遍：统计匹配数量
        usize count = 0;
        usize scanIndex = 0;

        while (scanIndex + oldLength <= self->length) {
            usize j = 0;
            while (j < oldLength && *(self->chars + scanIndex + j) == *(inOldPattern + j)) {
                ++j;
            }

            if (j == oldLength) {
                ++count;
                scanIndex += oldLength;
            } else {
                ++scanIndex;
            }
        }

        if (count == 0) {
            return 0;
        }

        // 计算新长度
        const ssize delta = (ssize) newLength - (ssize) oldLength;
        const ssize newTotalLength = (ssize) self->length + (ssize) count * delta;

        // 新长度为 0 时直接清空，避免后续使用空缓冲区
        if (newTotalLength == 0) {
            free(self->chars);
            self->chars = nullptr;
            self->length = 0;
            self->capacity = 0;

            return count;
        }

        // 分配新缓冲区
        auto* const newChars = (u8*) malloc((usize) newTotalLength);
        if (!newChars) {
            return 0;
        }

        // 第二遍：逐段拷贝
        usize srcIndex = 0;
        usize dstIndex = 0;

        while (srcIndex < self->length) {
            if (srcIndex + oldLength <= self->length) {
                usize j = 0;
                while (j < oldLength && *(self->chars + srcIndex + j) == *(inOldPattern + j)) {
                    ++j;
                }

                if (j == oldLength) {
                    // 匹配，拷贝新串
                    if (newLength > 0) {
                        memcpy(newChars + dstIndex, inNewPattern, newLength);
                        dstIndex += newLength;
                    }

                    srcIndex += oldLength;

                    continue;
                }
            }

            // 不匹配，拷贝一个字符
            *(newChars + dstIndex) = *(self->chars + srcIndex);
            ++srcIndex;
            ++dstIndex;
        }

        // 替换
        free(self->chars);
        self->chars = newChars;
        self->length = (usize) newTotalLength;
        self->capacity = (usize) newTotalLength;

        return count;
    }

    auto HeapString_substring(const HeapString* self, const usize start, const usize length) -> HeapString* {
        if (!self || start > self->length || length > self->length - start) {
            return nullptr;
        }

        auto* const newSelf = HeapString_construct();
        if (!newSelf) {
            return nullptr;
        }

        if (length == 0) {
            return newSelf;
        }

        // 容量不足时扩容
        if (length > newSelf->capacity) {
            if (!HeapString_reserve(newSelf, length)) {
                HeapString_destruct(newSelf);
                return nullptr;
            }
        }

        memcpy(newSelf->chars, self->chars + start, length);
        newSelf->length = length;

        return newSelf;
    }

    auto HeapString_reserve(HeapString* self, const usize newCapacity) -> bool {
        if (!self || newCapacity < DEFAULT_CAPACITY) {
            return false;
        }

        if (newCapacity <= self->capacity) {
            return true;
        }

        auto* const newChars = (u8*) realloc(self->chars, newCapacity);
        if (!newChars) {
            return false;
        }

        self->chars = newChars;
        self->capacity = newCapacity;

        return true;
    }
    auto HeapString_shrinkToFit(HeapString* self) -> bool {
        if (!self) {
            return false;
        }

        if (self->length == 0) {
            free(self->chars);
            self->chars = nullptr;
            self->capacity = 0;
            return true;
        }

        if (self->length == self->capacity) {
            return false;
        }

        auto* const newChars = (u8*) realloc(self->chars, self->length);
        if (newChars) {
            self->chars = newChars;
            self->capacity = self->length;
        }

        return true;
    }

    auto HeapString_chars(const HeapString* self) -> u8* {
        if (!self) {
            return nullptr;
        }

        return self->chars;
    }
    auto HeapString_charAt(const HeapString* self, const usize index) -> u8 {
        if (!self || index >= self->length) {
            return 0;
        }

        return *(self->chars + index);
    }
    auto HeapString_setCharAt(HeapString* self, const usize index, const u8 inChar) -> bool {
        if (!self || index >= self->length) {
            return false;
        }

        *(self->chars + index) = inChar;

        return true;
    }

    auto HeapString_compare(const HeapString* a, const HeapString* b) -> i32 {
        if (!a || !b) {
            return 0;
        }

        const usize minLength = a->length < b->length ? a->length : b->length;

        for (usize i = 0; i < minLength; ++i) {
            const u8 charA = *(a->chars + i);
            const u8 charB = *(b->chars + i);

            if (charA < charB) {
                return -1;
            }

            if (charA > charB) {
                return 1;
            }
        }

        if (a->length < b->length) {
            return -1;
        }

        if (a->length > b->length) {
            return 1;
        }

        return 0;
    }
    auto HeapString_equals(const HeapString* a, const HeapString* b) -> bool {
        return HeapString_compare(a, b) == 0;
    }

    auto HeapString_length(const HeapString* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->length;
    }
    auto HeapString_capacity(const HeapString* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->capacity;
    }

    auto HeapString_isEmpty(const HeapString* self) -> bool {
        if (!self) {
            return true;
        }

        return self->length == 0;
    }
}