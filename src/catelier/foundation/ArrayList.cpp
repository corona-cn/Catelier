#include "ArrayList.hpp"

#include <cstdlib>
#include <cstring>

#include "../CommonMacro.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    // 一个要注意的点
    // 这个数组列表维护的是一个指针数组，每个指针都指向堆上数据
    // 所以这个数组列表本质是存储的数据对象本身在内存上不连续
    // 带来的优势是扩容前后数据指针仍然有效
    // 带来的劣势是内存的不连续性，不过这也是要在 C 核心层实现通用数据容器的天然代价
    typedef struct ArrayList {
        void** data;
        usize size;
        usize capacity;
    } ArrayList;

    auto ArrayList_construct(const usize initialCapacity) -> ArrayList* {
        usize capacity = initialCapacity;
        if (capacity < DEFAULT_CAPACITY) {
            capacity = DEFAULT_CAPACITY;
        }

        auto* const self = (ArrayList*) malloc(sizeof(ArrayList));
        if (self == nullptr) {
            return nullptr;
        }

        self->data = (void**) malloc(sizeof(void*) * capacity);
        if (!self->data) {
            free(self);
            return nullptr;
        }

        self->size = 0;
        self->capacity = capacity;

        return self;
    }
    auto ArrayList_destruct(ArrayList* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->data);
        free(self);

        return true;
    }

    auto ArrayList_copy(const ArrayList* self, const usize inElementSize) -> ArrayList* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newSelf = ArrayList_construct(self->capacity);
        if (!newSelf) {
            return nullptr;
        }

        for (usize i = 0; i < self->size; ++i) {
            const void* const element = *(self->data + i);
            if (!ArrayList_push(newSelf, element, inElementSize)) {
                ArrayList_destruct(newSelf);
                return nullptr;
            }
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

        newSelf->data = self->data;
        newSelf->size = self->size;
        newSelf->capacity = self->capacity;

        self->data = nullptr;
        self->size = 0;
        self->capacity = 0;

        return newSelf;
    }

    auto ArrayList_push(ArrayList* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity * 2;
            if (!ArrayList_reserve(self, newCapacity)) {
                return false;
            }
        }

        if (inElement == nullptr) {
            *(self->data + self->size++) = nullptr;
            return true;
        }

        auto* const element = (void*) malloc(inElementSize);
        if (!element) {
            return false;
        }

        memcpy(element, inElement, inElementSize);

        *(self->data + self->size++) = element;

        return true;
    }
    auto ArrayList_pushMove(ArrayList* self, void* inElement) -> bool {
        if (!self) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity * 2;
            if (!ArrayList_reserve(self, newCapacity)) {
                return false;
            }
        }

        *(self->data + self->size++) = inElement;

        return true;
    }
    auto ArrayList_insertAt(ArrayList* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        if (index > self->size) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity * 2;
            if (!ArrayList_reserve(self, newCapacity)) {
                return false;
            }
        }

        if (inElement == nullptr) {
            *(self->data + index) = nullptr;

            self->size++;

            return true;
        }

        // 从尾元素开始，逐元素往后移动一位
        for (usize i = self->size; i > index; --i) {
            *(self->data + i) = *(self->data + i - 1);
        }

        auto* const element = (void*) malloc(inElementSize);
        if (!element) {
            return false;
        }

        memcpy(element, inElement, inElementSize);

        *(self->data + self->size++) = element;

        return true;
    }

    auto ArrayList_pop(ArrayList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        void* const element = *(self->data + self->size - 1);
        free(element);

        self->size--;

        return true;
    }
    auto ArrayList_removeAt(ArrayList* self, const usize index) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        if (index > self->size) {
            return false;
        }

        void* const element = *(self->data + index);
        free(element);

        // 从 index 位元素开始，逐元素往前移动一位
        for (usize i = index; i < self->size - 1; ++i) {
            *(self->data + i) = *(self->data + i + 1);
        }

        self->size--;

        return true;
    }
    auto ArrayList_clear(ArrayList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 遍历释放所有元素的内存
        for (usize i = 0; i < self->size; ++i) {
            void* const element = *(self->data + i);
            free(element);
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

        auto* const newData = (void**) realloc(self->data, sizeof(void*) * newCapacity);
        if (!newData) {
            return false;
        }

        self->data = newData;
        self->capacity = newCapacity;

        return true;
    }
    auto ArrayList_shrinkToFit(ArrayList* self) -> bool {
        if (!self) {
            return false;
        }

        // 当 size 为 0，即没有元素时，直接释放 data 的内存并置空，同时重置 capacity
        // 当前情况被认为 shrinkToFit 生效，返回 true
        if (self->size == 0) {
            PTR_FREE_AND_NULL(self->data);
            self->capacity = 0;
            return true;
        }

        if (self->size == self->capacity) {
            return false;
        }

        auto* const newData = (void**) realloc(self->data, sizeof(void*) * self->size);
        if (newData) {
            self->data = newData;
            self->capacity = self->size;
        }

        return true;
    }

    auto ArrayList_get(const ArrayList* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        void* const element = *(self->data + index);
        return element;
    }

    auto ArrayList_set(const ArrayList* self, const usize index, void* inElement) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        *(self->data + index) = inElement;

        return true;
    }

    auto ArrayList_size(const ArrayList* self) -> usize {
        return self ? self->size : 0;
    }
    auto ArrayList_capacity(const ArrayList* self) -> usize {
        return self ? self->capacity : 0;
    }

    auto ArrayList_isEmpty(const ArrayList* self) -> bool {
        return self ? self->size == 0 : true;
    }
}
