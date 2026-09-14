#include "Stack.hpp"

#include <cstdlib>
#include <cstring>

#include "../CommonMacro.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    // 后进先出的栈结构
    // 元素在内存中连续排列，容量可自动扩容，语义上不存在满的状态
    typedef struct Stack {
        u8* elements;
        usize elementSize;
        usize capacity;
        usize size;
    } Stack;

    auto Stack_construct(const usize initialCapacity, const usize inElementSize) -> Stack* {
        if (inElementSize == 0) {
            return nullptr;
        }

        usize capacity = initialCapacity;
        if (capacity < DEFAULT_CAPACITY) {
            capacity = DEFAULT_CAPACITY;
        }

        auto* const self = (Stack*) malloc(sizeof(Stack));
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
    auto Stack_destruct(Stack* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->elements);
        free(self);

        return true;
    }

    auto Stack_copy(const Stack* self) -> Stack* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = Stack_construct(self->capacity, self->elementSize);
        if (!newSelf) {
            return nullptr;
        }

        if (self->size > 0) {
            memcpy(newSelf->elements, self->elements, self->elementSize * self->size);
            newSelf->size = self->size;
        }

        return newSelf;
    }
    auto Stack_move(Stack* self) -> Stack* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (Stack*) malloc(sizeof(Stack));
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

    auto Stack_push(Stack* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity * 2;
            if (!Stack_reserve(self, newCapacity)) {
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
    auto Stack_pushSlot(Stack* self) -> void* {
        if (!self) {
            return nullptr;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity * 2;
            if (!Stack_reserve(self, newCapacity)) {
                return nullptr;
            }
        }

        // 返回末尾新压入槽位，用于更底层的控制
        void* const slot = self->elements + self->size * self->elementSize;

        self->size++;

        return slot;
    }

    auto Stack_pop(Stack* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        self->size--;

        return true;
    }
    auto Stack_popSlot(Stack* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        self->size--;

        // 返回末尾旧弹出槽位，用于更底层的控制
        return self->elements + self->size * self->elementSize;
    }
    auto Stack_clear(Stack* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        self->size = 0;

        return true;
    }

    auto Stack_peek(const Stack* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 栈顶元素在末尾，物理位置是 size - 1
        return self->elements + (self->size - 1) * self->elementSize;
    }
    auto Stack_get(const Stack* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        // 逻辑索引 0 表示栈底，逻辑索引 size - 1 表示栈顶
        return self->elements + index * self->elementSize;
    }

    auto Stack_reserve(Stack* self, const usize newCapacity) -> bool {
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
    auto Stack_shrinkToFit(Stack* self) -> bool {
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

    auto Stack_elements(const Stack* self) -> u8* {
        return self ? self->elements : nullptr;
    }
    auto Stack_capacity(const Stack* self) -> usize {
        return self ? self->capacity : 0;
    }
    auto Stack_size(const Stack* self) -> usize {
        return self ? self->size : 0;
    }

    auto Stack_isEmpty(const Stack* self) -> bool {
        return self ? self->size == 0 : true;
    }
}