#include "UnboundedQueue.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct UnboundedQueueNode {
        void* data;
        UnboundedQueueNode* nextNode;
    } UnboundedQueueNode;

    typedef struct UnboundedQueue {
        UnboundedQueueNode* headNode;
        UnboundedQueueNode* tailNode;
        usize size;
        usize elementSize;
    } UnboundedQueue;

    auto UnboundedQueue_construct(const usize inElementSize) -> UnboundedQueue* {
        if (inElementSize == 0) {
            return nullptr;
        }

        auto* const self = (UnboundedQueue*) malloc(sizeof(UnboundedQueue));
        if (!self) {
            return nullptr;
        }

        self->headNode = nullptr;
        self->tailNode = nullptr;
        self->size = 0;
        self->elementSize = inElementSize;

        return self;
    }
    auto UnboundedQueue_destruct(UnboundedQueue* self) -> bool {
        if (!self) {
            return false;
        }

        auto* currentNode = self->headNode;
        while (currentNode) {
            auto* const nextNode = currentNode->nextNode;
            free(currentNode->data);
            free(currentNode);
            currentNode = nextNode;
        }

        free(self);

        return true;
    }

    auto UnboundedQueue_copy(const UnboundedQueue* self) -> UnboundedQueue* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = UnboundedQueue_construct(self->elementSize);
        if (!newSelf) {
            return nullptr;
        }

        for (const auto* currentNode = self->headNode; currentNode; currentNode = currentNode->nextNode) {
            void* const slot = UnboundedQueue_enqueueSlot(newSelf);
            if (!slot) {
                UnboundedQueue_destruct(newSelf);
                return nullptr;
            }

            if (currentNode->data) {
                memcpy(slot, currentNode->data, self->elementSize);
            }
        }

        return newSelf;
    }
    auto UnboundedQueue_move(UnboundedQueue* self) -> UnboundedQueue* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (UnboundedQueue*) malloc(sizeof(UnboundedQueue));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->headNode = self->headNode;
        newSelf->tailNode = self->tailNode;
        newSelf->size = self->size;
        newSelf->elementSize = self->elementSize;

        self->headNode = nullptr;
        self->tailNode = nullptr;
        self->size = 0;
        self->elementSize = 0;

        return newSelf;
    }

    auto UnboundedQueue_enqueue(UnboundedQueue* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        auto* const newNode = (UnboundedQueueNode*) malloc(sizeof(UnboundedQueueNode));
        if (!newNode) {
            return false;
        }

        if (!inElement) {
            newNode->data = nullptr;
        } else {
            newNode->data = malloc(self->elementSize);
            if (!newNode->data) {
                free(newNode);
                return false;
            }
            memcpy(newNode->data, inElement, self->elementSize);
        }

        newNode->nextNode = nullptr;

        if (self->tailNode) {
            self->tailNode->nextNode = newNode;
        } else {
            self->headNode = newNode;
        }

        self->tailNode = newNode;
        self->size++;

        return true;
    }
    auto UnboundedQueue_enqueueSlot(UnboundedQueue* self) -> void* {
        if (!self) {
            return nullptr;
        }

        auto* const newNode = (UnboundedQueueNode*) malloc(sizeof(UnboundedQueueNode));
        if (!newNode) {
            return nullptr;
        }

        newNode->data = malloc(self->elementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        newNode->nextNode = nullptr;

        if (self->tailNode) {
            self->tailNode->nextNode = newNode;
        } else {
            self->headNode = newNode;
        }

        self->tailNode = newNode;
        self->size++;

        return newNode->data;
    }

    auto UnboundedQueue_dequeue(UnboundedQueue* self) -> bool {
        if (!self || self->size == 0 || !self->headNode) {
            return false;
        }

        auto* const nextNode = self->headNode->nextNode;

        if (self->headNode->data) {
            free(self->headNode->data);
        }

        free(self->headNode);

        self->headNode = nextNode;

        // 队列非空时 tail 不变，队列空时 tail 也要置空
        if (!nextNode) {
            self->tailNode = nullptr;
        }

        self->size--;

        return true;
    }
    auto UnboundedQueue_dequeueSlot(UnboundedQueue* self) -> void* {
        if (!self || self->size == 0 || !self->headNode) {
            return nullptr;
        }

        auto* const nextNode = self->headNode->nextNode;
        void* const data = self->headNode->data;

        free(self->headNode);

        self->headNode = nextNode;

        // 队列非空时 tail 不变，队列空时 tail 也要置空
        if (!nextNode) {
            self->tailNode = nullptr;
        }

        self->size--;

        return data;
    }
    auto UnboundedQueue_clear(UnboundedQueue* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        auto* currentNode = self->headNode;
        while (currentNode) {
            auto* const nextNode = currentNode->nextNode;

            if (currentNode->data) {
                free(currentNode->data);
            }

            free(currentNode);

            currentNode = nextNode;
        }

        self->headNode = nullptr;
        self->tailNode = nullptr;
        self->size = 0;

        return true;
    }

    auto UnboundedQueue_head(const UnboundedQueue* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->headNode->data;
    }
    auto UnboundedQueue_tail(const UnboundedQueue* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->tailNode->data;
    }
    auto UnboundedQueue_get(const UnboundedQueue* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        auto* targetNode = self->headNode;
        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
        }

        return targetNode->data;
    }

    auto UnboundedQueue_begin(const UnboundedQueue* self) -> UnboundedQueueNode* {
        if (!self) {
            return nullptr;
        }

        return self->headNode;
    }
    auto UnboundedQueue_end() -> UnboundedQueueNode* {
        return nullptr;
    }

    auto UnboundedQueue_size(const UnboundedQueue* self) -> usize {
        return self ? self->size : 0;
    }

    auto UnboundedQueue_isEmpty(const UnboundedQueue* self) -> bool {
        return self ? self->size == 0 : true;
    }

    auto UnboundedQueueNode_next(const UnboundedQueueNode* node) -> UnboundedQueueNode* {
        if (!node) {
            return nullptr;
        }

        return node->nextNode;
    }

    auto UnboundedQueueNode_data(const UnboundedQueueNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->data;
    }
}
