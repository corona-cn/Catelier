#include "RingBuffer.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    // 固定容量的环形缓冲区
    // 逻辑容量由用户指定，物理容量（隐式）是大于等于逻辑容量的最小 2 的幂
    // 内部所有环形索引运算都用物理容量做掩码，对外只暴露逻辑容量
    typedef struct RingBuffer {
        u8* elements;
        usize elementSize;
        usize logicalCapacity;
        usize physicalCapacity;
        usize size;
        usize headIndex;
    } RingBuffer;

    auto RingBuffer_construct(const usize initialCapacity, const usize inElementSize) -> RingBuffer* {
        if (inElementSize == 0) {
            return nullptr;
        }

        usize logicalCapacity = initialCapacity;
        if (logicalCapacity < DEFAULT_CAPACITY) {
            logicalCapacity = DEFAULT_CAPACITY;
        }

        // 物理容量是大于等于逻辑容量的最小 2 的幂
        // 保证内部环形索引运算可以用位与代替取模
        usize physicalCapacity = 1;
        while (physicalCapacity < logicalCapacity) {
            physicalCapacity <<= 1;
        }

        auto* const self = (RingBuffer*) malloc(sizeof(RingBuffer));
        if (!self) {
            return nullptr;
        }

        // 分配连续元素缓冲区
        self->elements = (u8*) malloc(inElementSize * physicalCapacity);
        if (!self->elements) {
            free(self);
            return nullptr;
        }

        self->elementSize = inElementSize;
        self->logicalCapacity = logicalCapacity;
        self->physicalCapacity = physicalCapacity;
        self->size = 0;
        self->headIndex = 0;

        return self;
    }
    auto RingBuffer_destruct(RingBuffer* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->elements);
        free(self);

        return true;
    }

    auto RingBuffer_copy(const RingBuffer* self) -> RingBuffer* {
        if (!self) {
            return nullptr;
        }

        // 用相同的逻辑容量与元素大小构造新对象
        // 使新对象的物理容量与原对象保持一致
        auto* const newSelf = RingBuffer_construct(self->logicalCapacity, self->elementSize);
        if (!newSelf) {
            return nullptr;
        }

        // 直接复制整个物理缓冲区，保持 headIndex 与元素数量一致
        // 物理布局完全一致，因此无需逐个搬运有效元素
        memcpy(newSelf->elements, self->elements, self->elementSize * self->physicalCapacity);
        newSelf->size = self->size;
        newSelf->headIndex = self->headIndex;

        return newSelf;
    }
    auto RingBuffer_move(RingBuffer* self) -> RingBuffer* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (RingBuffer*) malloc(sizeof(RingBuffer));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->elements = self->elements;
        newSelf->elementSize = self->elementSize;
        newSelf->logicalCapacity = self->logicalCapacity;
        newSelf->physicalCapacity = self->physicalCapacity;
        newSelf->size = self->size;
        newSelf->headIndex = self->headIndex;

        self->elements = nullptr;
        self->elementSize = 0;
        self->logicalCapacity = 0;
        self->physicalCapacity = 0;
        self->size = 0;
        self->headIndex = 0;

        return newSelf;
    }

    auto RingBuffer_pushHead(RingBuffer* self, const void* inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        // 拒绝模式：环形缓冲区满了直接返回失败
        if (self->size >= self->logicalCapacity) {
            return false;
        }

        // 头部插入：headIndex 前移一位（环形绕回）
        self->headIndex = (self->headIndex - 1) & (self->physicalCapacity - 1);

        // 将新元素写入新的头部位置
        memcpy(self->elements + self->headIndex * self->elementSize, inElement, self->elementSize);

        self->size++;

        return true;
    }
    auto RingBuffer_overwriteHead(RingBuffer* self, const void* inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        // 未满时等价于普通头部插入
        if (self->size < self->logicalCapacity) {
            self->headIndex = (self->headIndex - 1) & (self->physicalCapacity - 1);

            memcpy(self->elements + self->headIndex * self->elementSize, inElement, self->elementSize);

            self->size++;

            return true;
        }

        // 覆盖模式：环形缓冲区已满时直接覆盖最旧元素（即当前 headIndex 位置）
        // 新元素取代最旧元素，headIndex 保持不变，size 保持不变
        memcpy(self->elements + self->headIndex * self->elementSize, inElement, self->elementSize);

        return true;
    }
    auto RingBuffer_pushHeadSlot(RingBuffer* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 拒绝模式：环形缓冲区满了直接返回空指针
        if (self->size >= self->logicalCapacity) {
            return nullptr;
        }

        // 头部插入：headIndex 前移一位（环形绕回）
        self->headIndex = (self->headIndex - 1) & (self->physicalCapacity - 1);

        // 新槽位就是新的头部位置，返回给调用者做 placement new
        void* const slot = self->elements + self->headIndex * self->elementSize;

        self->size++;

        return slot;
    }
    auto RingBuffer_pushTail(RingBuffer* self, const void* inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        // 拒绝模式：环形缓冲区满了直接返回失败
        if (self->size >= self->logicalCapacity) {
            return false;
        }

        // 尾部插入：写入位置为 headIndex + size（环形绕回）
        const usize index = (self->headIndex + self->size) & (self->physicalCapacity - 1);
        memcpy(self->elements + index * self->elementSize, inElement, self->elementSize);

        self->size++;

        return true;
    }
    auto RingBuffer_overwriteTail(RingBuffer* self, const void* inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        // 环形缓冲区未满时等价于普通尾部插入
        if (self->size < self->logicalCapacity) {
            const usize index = (self->headIndex + self->size) & (self->physicalCapacity - 1);
            memcpy(self->elements + index * self->elementSize, inElement, self->elementSize);

            self->size++;

            return true;
        }

        // 覆盖模式：环形缓冲区已满时先丢弃最旧元素（headIndex 后移一位）
        self->headIndex = (self->headIndex + 1) & (self->physicalCapacity - 1);

        // 上面丢弃了最旧元素，这里 size 减 1 确保下面的尾部插入正确执行
        self->size--;

        // 尾部插入（此时 size 减 1，写入位置正好是原来的尾部位置）
        const usize index = (self->headIndex + self->size) & (self->physicalCapacity - 1);
        memcpy(self->elements + index * self->elementSize, inElement, self->elementSize);

        // 尾部插入执行成功，size 再加 1，整体来看 size 没有发生变化
        self->size++;

        return true;
    }
    auto RingBuffer_pushTailSlot(RingBuffer* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 拒绝模式：满了直接返回空指针
        if (self->size >= self->logicalCapacity) {
            return nullptr;
        }

        // 尾部插入：新槽位在 headIndex + size 处（环形绕回）
        const usize index = (self->headIndex + self->size) & (self->physicalCapacity - 1);

        // 新槽位，返回给调用者做 placement new
        void* const slot = self->elements + index * self->elementSize;

        self->size++;

        return slot;
    }

    auto RingBuffer_popHead(RingBuffer* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 弹出头部：headIndex 前移一位（环形绕回）
        // 旧头部位置不再被视作有效元素，其字节内容保持不变
        self->headIndex = (self->headIndex + 1) & (self->physicalCapacity - 1);

        self->size--;

        return true;
    }
    auto RingBuffer_popHeadSlot(RingBuffer* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 记录被移除的头部位置，返回给调用者做析构
        void* const slot = self->elements + self->headIndex * self->elementSize;

        // 更新状态：headIndex 后移一位（环形绕回）
        self->headIndex = (self->headIndex + 1) & (self->physicalCapacity - 1);
        self->size--;

        return slot;
    }
    auto RingBuffer_popTail(RingBuffer* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 弹出尾部：只需将 size 减一
        // 尾部元素的物理位置由 headIndex + size 隐式确定
        // 不需要修改 headIndex，也不需要考虑绕回
        self->size--;

        return true;
    }
    auto RingBuffer_popTailSlot(RingBuffer* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 记录被移除的尾部位置，返回给调用者做析构
        const usize index = (self->headIndex + self->size - 1) & (self->physicalCapacity - 1);
        void* const slot = self->elements + index * self->elementSize;

        // 更新状态：size 递减，headIndex 不变
        self->size--;

        return slot;
    }
    auto RingBuffer_clear(RingBuffer* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 清空语义：将 size 归零，并把 headIndex 复位到物理索引 0
        // 所有有效元素位置都被视为无效，但字节内容保持不变
        self->size = 0;
        self->headIndex = 0;

        return true;
    }

    auto RingBuffer_get(const RingBuffer* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        // 逻辑索引到物理索引的映射：
        // 逻辑索引 0 对应最旧元素，即 headIndex 位置
        // 逻辑索引 i 对应物理位置 (headIndex + i) 环形绕回后的位置
        // 对物理容量减一按位与，保证物理索引绕回到正确位置
        const usize physicalIndex = (self->headIndex + index) & (self->physicalCapacity - 1);

        // 物理索引与元素大小相乘得到最终偏移量，返回正确的目标元素指针
        return self->elements + physicalIndex * self->elementSize;
    }
    auto RingBuffer_head(const RingBuffer* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 头部就是最旧元素，物理位置直接是 headIndex
        // 语义上等价于 RingBuffer_get(self, 0)
        return self->elements + self->headIndex * self->elementSize;
    }
    auto RingBuffer_tail(const RingBuffer* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 尾部是最新元素，物理位置是 headIndex + size - 1 环形绕回后的位置
        // 语义上等价于 RingBuffer_get(self, size - 1)
        // 对物理容量减一按位与，保证物理索引绕回到正确位置
        const usize physicalIndex = (self->headIndex + self->size - 1) & (self->physicalCapacity - 1);

        // 物理索引与元素大小相乘得到最终偏移量，返回正确的尾部元素指针
        return self->elements + physicalIndex * self->elementSize;
    }

    auto RingBuffer_elements(const RingBuffer* self) -> u8* {
        return self ? self->elements : nullptr;
    }
    auto RingBuffer_capacity(const RingBuffer* self) -> usize {
        return self ? self->logicalCapacity : 0;
    }
    auto RingBuffer_physicalCapacity(const RingBuffer* self) -> usize {
        return self ? self->physicalCapacity : 0;
    }
    auto RingBuffer_size(const RingBuffer* self) -> usize {
        return self ? self->size : 0;
    }

    auto RingBuffer_isEmpty(const RingBuffer* self) -> bool {
        return self ? self->size == 0 : true;
    }
    auto RingBuffer_isFull(const RingBuffer* self) -> bool {
        return self ? self->size >= self->logicalCapacity : false;
    }
}
