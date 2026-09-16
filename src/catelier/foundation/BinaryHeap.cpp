#include "BinaryHeap.hpp"

#include <cstdlib>
#include <cstring>

#include "../CommonMacro.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    typedef struct BinaryHeap {
        u8* elements;
        u8* tempBuffer;
        usize elementSize;
        usize capacity;
        usize size;
        bool (*compare)(const void*, const void*);
    } BinaryHeap;

    auto BinaryHeap_construct(const usize initialCapacity, const usize inElementSize, bool (*compare)(const void*, const void*)) -> BinaryHeap* {
        if (inElementSize == 0 || !compare) {
            return nullptr;
        }

        usize capacity = initialCapacity;
        if (capacity < DEFAULT_CAPACITY) {
            capacity = DEFAULT_CAPACITY;
        }

        auto* const self = (BinaryHeap*) malloc(sizeof(BinaryHeap));
        if (!self) {
            return nullptr;
        }

        self->elements = (u8*) malloc(inElementSize * capacity);
        if (!self->elements) {
            free(self);
            return nullptr;
        }

        // 分配 siftUp / siftDown 复用的临时缓冲区
        self->tempBuffer = (u8*) malloc(inElementSize);
        if (!self->tempBuffer) {
            free(self->elements);
            free(self);
            return nullptr;
        }

        self->elementSize = inElementSize;
        self->capacity = capacity;
        self->size = 0;
        self->compare = compare;

        return self;
    }
    auto BinaryHeap_destruct(BinaryHeap* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->tempBuffer);
        free(self->elements);
        free(self);

        return true;
    }

    auto BinaryHeap_copy(const BinaryHeap* self) -> BinaryHeap* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = BinaryHeap_construct(self->capacity, self->elementSize, self->compare);
        if (!newSelf) {
            return nullptr;
        }

        if (self->size > 0) {
            memcpy(newSelf->elements, self->elements, self->elementSize * self->size);
            newSelf->size = self->size;
        }

        return newSelf;
    }
    auto BinaryHeap_move(BinaryHeap* self) -> BinaryHeap* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BinaryHeap*) malloc(sizeof(BinaryHeap));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->elements = self->elements;
        newSelf->tempBuffer = self->tempBuffer;
        newSelf->elementSize = self->elementSize;
        newSelf->capacity = self->capacity;
        newSelf->size = self->size;
        newSelf->compare = self->compare;

        self->elements = nullptr;
        self->tempBuffer = nullptr;
        self->elementSize = 0;
        self->capacity = 0;
        self->size = 0;
        self->compare = nullptr;

        return newSelf;
    }

    auto BinaryHeap_push(BinaryHeap* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity * 2;
            if (!BinaryHeap_reserve(self, newCapacity)) {
                return false;
            }
        }

        if (!inElement) {
            memset(self->elements + self->size * self->elementSize, 0, self->elementSize);
        } else {
            memcpy(self->elements + self->size * self->elementSize, inElement, self->elementSize);
        }

        self->size++;

        // 新元素在数组末尾，逐级上浮到正确位置
        BinaryHeap_siftUp(self, self->size - 1);

        return true;
    }
    auto BinaryHeap_pushSlot(BinaryHeap* self) -> void* {
        if (!self) {
            return nullptr;
        }

        if (self->size >= self->capacity) {
            const usize newCapacity = self->capacity == 0 ? DEFAULT_CAPACITY : self->capacity * 2;
            if (!BinaryHeap_reserve(self, newCapacity)) {
                return nullptr;
            }
        }

        // 返回末尾新槽位，让上层构造元素
        // 此时元素尚未构造，无法比较，所以不执行 siftUp
        // 上层构造完成后必须调用 BinaryHeap_siftUpTail 触发上浮
        void* const slot = self->elements + self->size * self->elementSize;

        self->size++;

        return slot;
    }

    auto BinaryHeap_pop(BinaryHeap* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 交换堆顶与堆尾元素
        u8* const head = self->elements;
        u8* const tail = self->elements + (self->size - 1) * self->elementSize;

        memcpy(self->tempBuffer, head, self->elementSize);
        memcpy(head, tail, self->elementSize);
        memcpy(tail, self->tempBuffer, self->elementSize);

        // 逻辑上移除堆尾（原堆顶元素）
        self->size--;

        // 如果堆还有元素，对新堆顶执行下沉
        if (self->size > 0) {
            BinaryHeap_siftDown(self, 0);
        }

        return true;
    }
    auto BinaryHeap_popSlot(BinaryHeap* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 交换堆顶与堆尾元素
        u8* const head = self->elements;
        u8* const tail = self->elements + (self->size - 1) * self->elementSize;

        memcpy(self->tempBuffer, head, self->elementSize);
        memcpy(head, tail, self->elementSize);
        memcpy(tail, self->tempBuffer, self->elementSize);

        // 逻辑上移除堆尾（原堆顶元素）
        self->size--;

        // 如果堆还有元素，对新堆顶执行下沉
        if (self->size > 0) {
            BinaryHeap_siftDown(self, 0);
        }

        // 返回被移除的原堆顶元素（现在位于堆尾之后）
        // 该指针指向堆内部数组，上层析构元素后不需要 free
        // 下一次 push 会覆盖这块内存
        return self->elements + self->size * self->elementSize;
    }
    auto BinaryHeap_clear(BinaryHeap* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        self->size = 0;

        return true;
    }

    auto BinaryHeap_peek(const BinaryHeap* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 堆顶是优先级最高的元素，位于 elements[0]
        return self->elements;
    }

    auto BinaryHeap_siftUp(const BinaryHeap* self, const usize index) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        // 把待上浮元素取出到临时缓冲区
        memcpy(self->tempBuffer, self->elements + index * self->elementSize, self->elementSize);

        usize currentIndex = index;
        while (currentIndex > 0) {
            const usize parentIndex = (currentIndex - 1) / 2;
            const u8* const parent = self->elements + parentIndex * self->elementSize;

            // compare(parent, tempBuffer) 为 true 表示父节点优先级更高，位置正确
            if (self->compare(parent, self->tempBuffer)) {
                break;
            }

            // 父节点下移到当前空位，洞上移到父节点位置
            memcpy(self->elements + currentIndex * self->elementSize, parent, self->elementSize);
            currentIndex = parentIndex;
        }

        // 把待上浮元素放到最终位置
        memcpy(self->elements + currentIndex * self->elementSize, self->tempBuffer, self->elementSize);

        return true;
    }
    auto BinaryHeap_siftUpTail(const BinaryHeap* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        return BinaryHeap_siftUp(self, self->size - 1);
    }
    auto BinaryHeap_siftDown(const BinaryHeap* self, const usize index) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        // 把待下沉元素取出到临时缓冲区
        memcpy(self->tempBuffer, self->elements + index * self->elementSize, self->elementSize);

        usize currentIndex = index;
        while (true) {
            const usize leftIndex = 2 * currentIndex + 1;
            const usize rightIndex = 2 * currentIndex + 2;

            // 找左右子节点中优先级最高的
            usize bestChildIndex = self->size;
            if (leftIndex < self->size) {
                bestChildIndex = leftIndex;
            }

            if (rightIndex < self->size) {
                const u8* const right = self->elements + rightIndex * self->elementSize;
                const u8* const bestChild = self->elements + bestChildIndex * self->elementSize;
                if (self->compare(right, bestChild)) {
                    bestChildIndex = rightIndex;
                }
            }

            // 无子节点，停止下沉
            if (bestChildIndex == self->size) {
                break;
            }

            // 比较最优子节点与待下沉元素
            // compare(tempBuffer, bestChild) 为 true 表示待下沉元素优先级更高，位置正确
            const u8* const bestChild = self->elements + bestChildIndex * self->elementSize;
            if (self->compare(self->tempBuffer, bestChild)) {
                break;
            }

            // 最优子节点上移到当前空位，洞下移到子节点位置
            memcpy(self->elements + currentIndex * self->elementSize, bestChild, self->elementSize);
            currentIndex = bestChildIndex;
        }

        // 把待下沉元素放到最终位置
        memcpy(self->elements + currentIndex * self->elementSize, self->tempBuffer, self->elementSize);

        return true;
    }

    auto BinaryHeap_heapify(const BinaryHeap* self) -> bool {
        if (!self || self->size <= 1) {
            return false;
        }

        // 从最后一个非叶子节点开始，依次向下 siftDown
        // 最后一个非叶子节点下标是 size / 2 - 1
        // 叶子节点（下标 >= size / 2）没有子节点，无需下沉
        for (usize i = self->size / 2; i > 0; --i) {
            BinaryHeap_siftDown(self, i - 1);
        }

        return true;
    }

    auto BinaryHeap_reserve(BinaryHeap* self, const usize newCapacity) -> bool {
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
    auto BinaryHeap_shrinkToFit(BinaryHeap* self) -> bool {
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

    auto BinaryHeap_get(const BinaryHeap* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        // 返回层序第 index 个元素，与大小排名无关
        return self->elements + index * self->elementSize;
    }

    auto BinaryHeap_elements(const BinaryHeap* self) -> u8* {
        return self ? self->elements : nullptr;
    }
    auto BinaryHeap_elementSize(const BinaryHeap* self) -> usize {
        return self ? self->elementSize : 0;
    }
    auto BinaryHeap_capacity(const BinaryHeap* self) -> usize {
        return self ? self->capacity : 0;
    }
    auto BinaryHeap_size(const BinaryHeap* self) -> usize {
        return self ? self->size : 0;
    }

    auto BinaryHeap_isEmpty(const BinaryHeap* self) -> bool {
        return self ? self->size == 0 : true;
    }
}