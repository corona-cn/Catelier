#include "BoundedDeque.hpp"

#include <cstdlib>

#include "RingBuffer.hpp"

namespace Catelier::src::foundation {
    // 有界双端队列结构
    // 内部持有 RingBuffer，所有操作都转发给它
    // 本层只负责语义收窄，不重复实现环形逻辑
    typedef struct BoundedDeque {
        RingBuffer* buffer;
    } BoundedDeque;

    auto BoundedDeque_construct(const usize initialCapacity, const usize inElementSize) -> BoundedDeque* {
        if (inElementSize == 0) {
            return nullptr;
        }

        auto* const self = (BoundedDeque*) malloc(sizeof(BoundedDeque));
        if (!self) {
            return nullptr;
        }

        self->buffer = RingBuffer_construct(initialCapacity, inElementSize);
        if (!self->buffer) {
            free(self);
            return nullptr;
        }

        return self;
    }
    auto BoundedDeque_destruct(BoundedDeque* self) -> bool {
        if (!self) {
            return false;
        }

        RingBuffer_destruct(self->buffer);
        free(self);

        return true;
    }

    auto BoundedDeque_copy(const BoundedDeque* self) -> BoundedDeque* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BoundedDeque*) malloc(sizeof(BoundedDeque));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->buffer = RingBuffer_copy(self->buffer);
        if (!newSelf->buffer) {
            free(newSelf);
            return nullptr;
        }

        return newSelf;
    }
    auto BoundedDeque_move(BoundedDeque* self) -> BoundedDeque* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BoundedDeque*) malloc(sizeof(BoundedDeque));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->buffer = RingBuffer_move(self->buffer);

        self->buffer = nullptr;

        return newSelf;
    }

    auto BoundedDeque_pushHead(const BoundedDeque* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        // 头部追加，等价于 RingBuffer 的头部追加
        return RingBuffer_pushHead(self->buffer, inElement);
    }
    auto BoundedDeque_pushHeadSlot(const BoundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 返回头部的新可写槽位，供上层做 placement new
        return RingBuffer_pushHeadSlot(self->buffer);
    }
    auto BoundedDeque_pushTail(const BoundedDeque* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        // 尾部追加，等价于 RingBuffer 的尾部追加
        return RingBuffer_pushTail(self->buffer, inElement);
    }
    auto BoundedDeque_pushTailSlot(const BoundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 返回尾部的新可写槽位，供上层做 placement new
        return RingBuffer_pushTailSlot(self->buffer);
    }

    auto BoundedDeque_popHead(const BoundedDeque* self) -> bool {
        if (!self) {
            return false;
        }

        // 头部弹出，等价于 RingBuffer 的头部弹出
        return RingBuffer_popHead(self->buffer);
    }
    auto BoundedDeque_popHeadSlot(const BoundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 返回被移除的头部槽位，供上层做析构
        return RingBuffer_popHeadSlot(self->buffer);
    }
    auto BoundedDeque_popTail(const BoundedDeque* self) -> bool {
        if (!self) {
            return false;
        }

        // 尾部弹出，等价于 RingBuffer 的尾部弹出
        return RingBuffer_popTail(self->buffer);
    }
    auto BoundedDeque_popTailSlot(const BoundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 返回被移除的尾部槽位，供上层做析构
        return RingBuffer_popTailSlot(self->buffer);
    }
    auto BoundedDeque_clear(const BoundedDeque* self) -> bool {
        if (!self) {
            return false;
        }

        return RingBuffer_clear(self->buffer);
    }

    auto BoundedDeque_head(const BoundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 队头就是最旧元素，将被下一个 popHead 移除
        return RingBuffer_head(self->buffer);
    }
    auto BoundedDeque_tail(const BoundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 队尾就是最新元素，刚被 pushTail 加入
        return RingBuffer_tail(self->buffer);
    }
    auto BoundedDeque_get(const BoundedDeque* self, const usize index) -> void* {
        if (!self) {
            return nullptr;
        }

        // 逻辑索引 0 表示队头，逻辑索引 size - 1 表示队尾
        return RingBuffer_get(self->buffer, index);
    }

    auto BoundedDeque_capacity(const BoundedDeque* self) -> usize {
        return self ? RingBuffer_capacity(self->buffer) : 0;
    }
    auto BoundedDeque_size(const BoundedDeque* self) -> usize {
        return self ? RingBuffer_size(self->buffer) : 0;
    }

    auto BoundedDeque_isEmpty(const BoundedDeque* self) -> bool {
        return self ? RingBuffer_isEmpty(self->buffer) : true;
    }
    auto BoundedDeque_isFull(const BoundedDeque* self) -> bool {
        return self ? RingBuffer_isFull(self->buffer) : false;
    }
}
