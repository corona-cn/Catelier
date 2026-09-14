#include "ArrayList.hpp"

#include <cstdlib>
#include <cstring>

#include "../CommonMacro.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    // 连续字节缓冲区的数组列表
    // 元素在内存中连续排列，elementSize 是实例级属性，构造时固定
    typedef struct ArrayList {
        u8* elements;
        usize elementSize;
        usize capacity;
        usize size;
    } ArrayList;

    auto ArrayList_construct(const usize initialCapacity, const usize inElementSize) -> ArrayList* {
        if (inElementSize == 0) {
            return nullptr;
        }

        usize capacity = initialCapacity;
        if (capacity < DEFAULT_CAPACITY) {
            capacity = DEFAULT_CAPACITY;
        }

        auto* const self = (ArrayList*) malloc(sizeof(ArrayList));
        if (!self) {
            return nullptr;
        }

        self->elements = (u8*) malloc(inElementSize * capacity);
        if (!self->elements) {
            free(self);
            return nullptr;
        }

        self->size = 0;
        self->capacity = capacity;
        self->elementSize = inElementSize;

        return self;
    }
    auto ArrayList_destruct(ArrayList* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->elements);
        free(self);

        return true;
    }

    auto ArrayList_copy(const ArrayList* self) -> ArrayList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = ArrayList_construct(self->capacity, self->elementSize);
        if (!newSelf) {
            return nullptr;
        }

        if (self->size > 0) {
            memcpy(newSelf->elements, self->elements, self->elementSize * self->size);
            newSelf->size = self->size;
        }

        return newSelf;
    }
    auto ArrayList_move(ArrayList* self) -> ArrayList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (ArrayList*) malloc(sizeof(ArrayList));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->elements = self->elements;
        newSelf->size = self->size;
        newSelf->capacity = self->capacity;
        newSelf->elementSize = self->elementSize;

        self->elements = nullptr;
        self->size = 0;
        self->capacity = 0;
        self->elementSize = 0;

        return newSelf;
    }

    auto ArrayList_push(ArrayList* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity * 2;
            if (!ArrayList_reserve(self, newCapacity)) {
                return false;
            }
        }

        if (!inElement) {
            memset(self->elements + self->size * self->elementSize, 0, self->elementSize);
        } else {
            memcpy(self->elements + self->size * self->elementSize, inElement, self->elementSize);
        }

        self->size++;

        return true;
    }
    auto ArrayList_pushSlot(ArrayList* self) -> void* {
        if (!self) {
            return nullptr;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity * 2;
            if (!ArrayList_reserve(self, newCapacity)) {
                return nullptr;
            }
        }

        // 返回末尾新可写槽位，用于更底层的控制
        void* slot = self->elements + self->size * self->elementSize;

        self->size++;

        return slot;
    }
    auto ArrayList_insertAt(ArrayList* self, const usize index, const void* inElement) -> bool {
        if (!self || index > self->size) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity * 2;
            if (!ArrayList_reserve(self, newCapacity)) {
                return false;
            }
        }

        if (index < self->size) {
            u8* dst = self->elements + (index + 1) * self->elementSize;
            const u8* src = self->elements + index * self->elementSize;
            const usize size = (self->size - index) * self->elementSize;
            memmove(dst, src, size);
        }

        if (!inElement) {
            memset(self->elements + index * self->elementSize, 0, self->elementSize);
        } else {
            memcpy(self->elements + index * self->elementSize, inElement, self->elementSize);
        }

        self->size++;

        return true;
    }

    auto ArrayList_pop(ArrayList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        self->size--;

        return true;
    }
    auto ArrayList_popSlot(ArrayList* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        self->size--;

        // 返回末尾旧撤销槽位，用于更底层的控制
        return self->elements + self->size * self->elementSize;
    }
    auto ArrayList_removeAt(ArrayList* self, const usize index) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        if (index < self->size - 1) {
            u8* dst = self->elements + index * self->elementSize;
            const u8* src = self->elements + (index + 1) * self->elementSize;
            const usize size = (self->size - index - 1) * self->elementSize;
            memmove(dst, src, size);
        }

        self->size--;

        return true;
    }
    auto ArrayList_clear(ArrayList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        self->size = 0;

        return true;
    }

    auto ArrayList_reserve(ArrayList* self, const usize newCapacity) -> bool {
        if (!self || newCapacity < DEFAULT_CAPACITY) {
            return false;
        }

        if (newCapacity <= self->capacity) {
            return true;
        }

        auto* const newData = (u8*) realloc(self->elements, self->elementSize * newCapacity);
        if (!newData) {
            return false;
        }

        self->elements = newData;
        self->capacity = newCapacity;

        return true;
    }
    auto ArrayList_shrinkToFit(ArrayList* self) -> bool {
        if (!self) {
            return false;
        }

        if (self->size == 0) {
            PTR_FREE_AND_NULL(self->elements);
            self->capacity = 0;
            return true;
        }

        if (self->size == self->capacity) {
            return false;
        }

        auto* const newData = (u8*) realloc(self->elements, self->elementSize * self->size);
        if (newData) {
            self->elements = newData;
            self->capacity = self->size;
        }

        return true;
    }

    auto ArrayList_get(const ArrayList* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        return self->elements + index * self->elementSize;
    }
    auto ArrayList_set(const ArrayList* self, const usize index, const void* inElement) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        if (!inElement) {
            memset(self->elements + index * self->elementSize, 0, self->elementSize);
        } else {
            memcpy(self->elements + index * self->elementSize, inElement, self->elementSize);
        }

        return true;
    }

    auto ArrayList_elements(const ArrayList* self) -> u8* {
        return self ? self->elements : nullptr;
    }
    auto ArrayList_elementSize(const ArrayList* self) -> usize {
        return self ? self->elementSize : 0;
    }
    auto ArrayList_capacity(const ArrayList* self) -> usize {
        return self ? self->capacity : 0;
    }
    auto ArrayList_size(const ArrayList* self) -> usize {
        return self ? self->size : 0;
    }

    auto ArrayList_isEmpty(const ArrayList* self) -> bool {
        return self ? self->size == 0 : true;
    }
}