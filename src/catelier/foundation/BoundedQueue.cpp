#include "BoundedQueue.hpp"

#include <cstdlib>

#include "RingBuffer.hpp"

namespace Catelier::src::foundation {
    // 先进先出的队列结构
    // 内部持有 RingBuffer，所有操作都转发给它
    // 本层只负责语义收窄，不重复实现环形逻辑
    typedef struct BoundedQueue {
        RingBuffer* buffer;
    } BoundedQueue;

    auto BoundedQueue_construct(const usize initialCapacity, const usize inElementSize) -> BoundedQueue* {
        if (inElementSize == 0) {
            return nullptr;
        }

        auto* const self = (BoundedQueue*) malloc(sizeof(BoundedQueue));
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
    auto BoundedQueue_destruct(BoundedQueue* self) -> bool {
        if (!self) {
            return false;
        }

        RingBuffer_destruct(self->buffer);
        free(self);

        return true;
    }

    auto BoundedQueue_copy(const BoundedQueue* self) -> BoundedQueue* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BoundedQueue*) malloc(sizeof(BoundedQueue));
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
    auto BoundedQueue_move(BoundedQueue* self) -> BoundedQueue* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BoundedQueue*) malloc(sizeof(BoundedQueue));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->buffer = RingBuffer_move(self->buffer);

        self->buffer = nullptr;

        return newSelf;
    }

    auto BoundedQueue_enqueue(const BoundedQueue* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        // 入队：写入队列尾部，等价于 RingBuffer 的尾部追加
        return RingBuffer_pushTail(self->buffer, inElement);
    }
    auto BoundedQueue_enqueueSlot(const BoundedQueue* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 返回尾部的新可写槽位，供上层做 placement new
        return RingBuffer_pushTailSlot(self->buffer);
    }

    auto BoundedQueue_dequeue(const BoundedQueue* self) -> bool {
        if (!self) {
            return false;
        }

        // 出队：从队列头部弹出，等价于 RingBuffer 的头部弹出
        return RingBuffer_popHead(self->buffer);
    }
    auto BoundedQueue_dequeueSlot(const BoundedQueue* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 返回被移除的头部槽位，供上层做析构
        return RingBuffer_popHeadSlot(self->buffer);
    }
    auto BoundedQueue_clear(const BoundedQueue* self) -> bool {
        if (!self) {
            return false;
        }

        return RingBuffer_clear(self->buffer);
    }

    auto BoundedQueue_head(const BoundedQueue* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 队头就是最旧元素，将被下一个 dequeue 移除
        return RingBuffer_head(self->buffer);
    }
    auto BoundedQueue_tail(const BoundedQueue* self) -> void* {
        if (!self) {
            return nullptr;
        }

        // 队尾就是最新元素，刚被 enqueue 加入
        return RingBuffer_tail(self->buffer);
    }
    auto BoundedQueue_get(const BoundedQueue* self, const usize index) -> void* {
        if (!self) {
            return nullptr;
        }

        // 逻辑索引 0 表示队头，逻辑索引 size - 1 表示队尾
        return RingBuffer_get(self->buffer, index);
    }

    auto BoundedQueue_capacity(const BoundedQueue* self) -> usize {
        return self ? RingBuffer_capacity(self->buffer) : 0;
    }
    auto BoundedQueue_size(const BoundedQueue* self) -> usize {
        return self ? RingBuffer_size(self->buffer) : 0;
    }

    auto BoundedQueue_isEmpty(const BoundedQueue* self) -> bool {
        return self ? RingBuffer_isEmpty(self->buffer) : true;
    }
    auto BoundedQueue_isFull(const BoundedQueue* self) -> bool {
        return self ? RingBuffer_isFull(self->buffer) : false;
    }
}
