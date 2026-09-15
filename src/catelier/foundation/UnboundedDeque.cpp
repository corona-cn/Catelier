#include "UnboundedDeque.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct UnboundedDequeNode {
        void* data;
        UnboundedDequeNode* prevNode;
        UnboundedDequeNode* nextNode;
    } UnboundedDequeNode;

    typedef struct UnboundedDeque {
        UnboundedDequeNode* headNode;
        UnboundedDequeNode* tailNode;
        usize size;
        usize elementSize;
    } UnboundedDeque;

    auto UnboundedDeque_construct(const usize inElementSize) -> UnboundedDeque* {
        if (inElementSize == 0) {
            return nullptr;
        }

        auto* const self = (UnboundedDeque*) malloc(sizeof(UnboundedDeque));
        if (!self) {
            return nullptr;
        }

        self->headNode = nullptr;
        self->tailNode = nullptr;
        self->size = 0;
        self->elementSize = inElementSize;

        return self;
    }
    auto UnboundedDeque_destruct(UnboundedDeque* self) -> bool {
        if (!self) {
            return false;
        }

        // 从头节点开始，遍历释放每个节点的数据与节点本身
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

    auto UnboundedDeque_copy(const UnboundedDeque* self) -> UnboundedDeque* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = UnboundedDeque_construct(self->elementSize);
        if (!newSelf) {
            return nullptr;
        }

        for (const auto* currentNode = self->headNode; currentNode; currentNode = currentNode->nextNode) {
            void* const slot = UnboundedDeque_pushTailSlot(newSelf);
            if (!slot) {
                UnboundedDeque_destruct(newSelf);
                return nullptr;
            }

            if (currentNode->data) {
                memcpy(slot, currentNode->data, self->elementSize);
            }
        }

        return newSelf;
    }
    auto UnboundedDeque_move(UnboundedDeque* self) -> UnboundedDeque* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (UnboundedDeque*) malloc(sizeof(UnboundedDeque));
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

    auto UnboundedDeque_pushHead(UnboundedDeque* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        auto* const newNode = (UnboundedDequeNode*) malloc(sizeof(UnboundedDequeNode));
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

        // 新节点插入到头部，prev 为 nullptr，next 指向原头节点
        newNode->prevNode = nullptr;
        newNode->nextNode = self->headNode;

        if (self->headNode) {
            self->headNode->prevNode = newNode;
        } else {
            self->tailNode = newNode;
        }

        self->headNode = newNode;
        self->size++;

        return true;
    }
    auto UnboundedDeque_pushHeadSlot(UnboundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        auto* const newNode = (UnboundedDequeNode*) malloc(sizeof(UnboundedDequeNode));
        if (!newNode) {
            return nullptr;
        }

        newNode->data = malloc(self->elementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        // 新节点插入到头部，prev 为 nullptr，next 指向原头节点
        newNode->prevNode = nullptr;
        newNode->nextNode = self->headNode;

        if (self->headNode) {
            self->headNode->prevNode = newNode;
        } else {
            self->tailNode = newNode;
        }

        self->headNode = newNode;
        self->size++;

        return newNode->data;
    }
    auto UnboundedDeque_pushTail(UnboundedDeque* self, const void* inElement) -> bool {
        if (!self) {
            return false;
        }

        auto* const newNode = (UnboundedDequeNode*) malloc(sizeof(UnboundedDequeNode));
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

        // 新节点插入到尾部，next 为 nullptr，prev 指向原尾节点
        newNode->nextNode = nullptr;
        newNode->prevNode = self->tailNode;

        if (self->tailNode) {
            self->tailNode->nextNode = newNode;
        } else {
            self->headNode = newNode;
        }

        self->tailNode = newNode;
        self->size++;

        return true;
    }
    auto UnboundedDeque_pushTailSlot(UnboundedDeque* self) -> void* {
        if (!self) {
            return nullptr;
        }

        auto* const newNode = (UnboundedDequeNode*) malloc(sizeof(UnboundedDequeNode));
        if (!newNode) {
            return nullptr;
        }

        newNode->data = malloc(self->elementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        // 新节点插入到尾部，next 为 nullptr，prev 指向原尾节点
        newNode->nextNode = nullptr;
        newNode->prevNode = self->tailNode;

        if (self->tailNode) {
            self->tailNode->nextNode = newNode;
        } else {
            self->headNode = newNode;
        }

        self->tailNode = newNode;
        self->size++;

        return newNode->data;
    }

    auto UnboundedDeque_popHead(UnboundedDeque* self) -> bool {
        if (!self || self->size == 0 || !self->headNode) {
            return false;
        }

        // 先保存下一个节点的指针
        auto* const nextNode = self->headNode->nextNode;

        // 释放头节点的数据
        if (self->headNode->data) {
            free(self->headNode->data);
        }

        // 释放头节点
        free(self->headNode);

        // 更新头节点为下一个节点
        self->headNode = nextNode;

        if (nextNode) {
            // 新头节点的 prev 置空
            nextNode->prevNode = nullptr;
        } else {
            // 队列空了，tail 也要置空
            self->tailNode = nullptr;
        }

        self->size--;

        return true;
    }
    auto UnboundedDeque_popHeadSlot(UnboundedDeque* self) -> void* {
        if (!self || self->size == 0 || !self->headNode) {
            return nullptr;
        }

        // 先保存下一个节点的指针
        auto* const nextNode = self->headNode->nextNode;

        // 保存头节点的数据指针，稍后返回给调用者
        void* const data = self->headNode->data;

        // 释放头节点结构体本身，但不释放数据内存
        free(self->headNode);

        // 更新头节点为下一个节点
        self->headNode = nextNode;

        if (nextNode) {
            // 新头节点的 prev 置空
            nextNode->prevNode = nullptr;
        } else {
            // 队列空了，tail 也要置空
            self->tailNode = nullptr;
        }

        self->size--;

        return data;
    }
    auto UnboundedDeque_popTail(UnboundedDeque* self) -> bool {
        if (!self || self->size == 0 || !self->tailNode) {
            return false;
        }

        // 先保存前一个节点的指针
        auto* const prevNode = self->tailNode->prevNode;

        // 释放尾节点的数据
        if (self->tailNode->data) {
            free(self->tailNode->data);
        }

        // 释放尾节点
        free(self->tailNode);

        // 更新尾节点为前一个节点
        self->tailNode = prevNode;

        if (prevNode) {
            // 新尾节点的 next 置空
            prevNode->nextNode = nullptr;
        } else {
            // 队列空了，head 也要置空
            self->headNode = nullptr;
        }

        self->size--;

        return true;
    }
    auto UnboundedDeque_popTailSlot(UnboundedDeque* self) -> void* {
        if (!self || self->size == 0 || !self->tailNode) {
            return nullptr;
        }

        // 先保存前一个节点的指针
        auto* const prevNode = self->tailNode->prevNode;

        // 保存尾节点的数据指针，稍后返回给调用者
        void* const data = self->tailNode->data;

        // 释放尾节点结构体本身，但不释放数据内存
        free(self->tailNode);

        // 更新尾节点为前一个节点
        self->tailNode = prevNode;

        if (prevNode) {
            // 新尾节点的 next 置空
            prevNode->nextNode = nullptr;
        } else {
            // 队列空了，head 也要置空
            self->headNode = nullptr;
        }

        self->size--;

        return data;
    }
    auto UnboundedDeque_clear(UnboundedDeque* self) -> bool {
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

    auto UnboundedDeque_head(const UnboundedDeque* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->headNode->data;
    }
    auto UnboundedDeque_tail(const UnboundedDeque* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->tailNode->data;
    }
    auto UnboundedDeque_get(const UnboundedDeque* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        // 从头或尾出发，取近的一端遍历
        UnboundedDequeNode* targetNode = nullptr;
        if (index <= self->size / 2) {
            // 从头部出发
            targetNode = self->headNode;
            for (usize i = 0; i < index; ++i) {
                targetNode = targetNode->nextNode;
            }
        } else {
            // 从尾部出发
            targetNode = self->tailNode;
            for (usize i = self->size - 1; i > index; --i) {
                targetNode = targetNode->prevNode;
            }
        }

        return targetNode->data;
    }

    auto UnboundedDeque_begin(const UnboundedDeque* self) -> UnboundedDequeNode* {
        if (!self) {
            return nullptr;
        }

        return self->headNode;
    }
    auto UnboundedDeque_end() -> UnboundedDequeNode* {
        return nullptr;
    }

    auto UnboundedDeque_size(const UnboundedDeque* self) -> usize {
        return self ? self->size : 0;
    }

    auto UnboundedDeque_isEmpty(const UnboundedDeque* self) -> bool {
        return self ? self->size == 0 : true;
    }

    auto UnboundedDequeNode_prev(const UnboundedDequeNode* node) -> UnboundedDequeNode* {
        if (!node) {
            return nullptr;
        }

        return node->prevNode;
    }
    auto UnboundedDequeNode_next(const UnboundedDequeNode* node) -> UnboundedDequeNode* {
        if (!node) {
            return nullptr;
        }

        return node->nextNode;
    }
    auto UnboundedDequeNode_data(const UnboundedDequeNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->data;
    }
}
