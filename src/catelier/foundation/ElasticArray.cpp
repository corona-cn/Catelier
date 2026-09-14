#include "ElasticArray.hpp"

#include <cstdlib>
#include <cstring>

#include "../CommonMacro.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    typedef struct ElasticArray {
        void** data;
        usize size;
        usize capacity;
    } ElasticArray;

    auto ElasticArray_construct(const usize initialCapacity) -> ElasticArray* {
        usize capacity = initialCapacity;
        if (capacity < DEFAULT_CAPACITY) {
            capacity = DEFAULT_CAPACITY;
        }

        auto* const self = (ElasticArray*) malloc(sizeof(ElasticArray));
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
    auto ElasticArray_destruct(ElasticArray* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->data);
        free(self);

        return true;
    }

    auto ElasticArray_copy(const ElasticArray* self, const usize inElementSize) -> ElasticArray* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newSelf = ElasticArray_construct(self->capacity);
        if (!newSelf) {
            return nullptr;
        }

        for (usize i = 0; i < self->size; ++i) {
            const void* const element = *(self->data + i);
            if (!ElasticArray_push(newSelf, element, inElementSize)) {
                ElasticArray_destruct(newSelf);
                return nullptr;
            }
        }

        return newSelf;
    }
    auto ElasticArray_move(ElasticArray* self) -> ElasticArray* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (ElasticArray*) malloc(sizeof(ElasticArray));
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

    auto ElasticArray_push(ElasticArray* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity * 2;
            if (!ElasticArray_reserve(self, newCapacity)) {
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
    auto ElasticArray_pushMove(ElasticArray* self, void* inElement) -> bool {
        if (!self) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity * 2;
            if (!ElasticArray_reserve(self, newCapacity)) {
                return false;
            }
        }

        *(self->data + self->size++) = inElement;

        return true;
    }
    auto ElasticArray_insertAt(ElasticArray* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        if (index > self->size) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity * 2;
            if (!ElasticArray_reserve(self, newCapacity)) {
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

    auto ElasticArray_pop(ElasticArray* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        void* const element = *(self->data + self->size - 1);
        free(element);

        self->size--;

        return true;
    }
    auto ElasticArray_removeAt(ElasticArray* self, const usize index) -> bool {
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
    auto ElasticArray_clear(ElasticArray* self) -> bool {
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

    auto ElasticArray_reserve(ElasticArray* self, const usize newCapacity) -> bool {
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
    auto ElasticArray_shrinkToFit(ElasticArray* self) -> bool {
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

    auto ElasticArray_get(const ElasticArray* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        void* const element = *(self->data + index);
        return element;
    }

    auto ElasticArray_set(const ElasticArray* self, const usize index, void* inElement) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        *(self->data + index) = inElement;

        return true;
    }

    auto ElasticArray_size(const ElasticArray* self) -> usize {
        return self ? self->size : 0;
    }
    auto ElasticArray_capacity(const ElasticArray* self) -> usize {
        return self ? self->capacity : 0;
    }

    auto ElasticArray_isEmpty(const ElasticArray* self) -> bool {
        return self ? self->size == 0 : true;
    }
}
