#include "CircularLinkedList.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct CircularLinkedListNode {
        void* data;
        CircularLinkedListNode* nextNode;
    } CircularLinkedListNode;

    typedef struct CircularLinkedList {
        CircularLinkedListNode* tailNode;
        usize size;
    } CircularLinkedList;

    namespace {
        auto headOf(const CircularLinkedList* self) -> CircularLinkedListNode* {
            if (!self->tailNode) {
                return nullptr;
            }

            return self->tailNode->nextNode;
        }
    }

    auto CircularLinkedList_construct() -> CircularLinkedList* {
        auto* const self = (CircularLinkedList*) malloc(sizeof(CircularLinkedList));
        if (!self) {
            return nullptr;
        }

        self->tailNode = nullptr;
        self->size = 0;

        return self;
    }
    auto CircularLinkedList_destruct(CircularLinkedList* self) -> bool {
        if (!self) {
            return false;
        }

        // 从 head 开始遍历一圈，释放所有节点
        auto* currentNode = headOf(self);
        while (currentNode) {
            auto* const nextNode = currentNode->nextNode;

            free(currentNode->data);
            free(currentNode);

            if (nextNode == headOf(self)) {
                break;
            }

            currentNode = nextNode;
        }

        free(self);

        return true;
    }

    auto CircularLinkedList_copy(const CircularLinkedList* self, const usize inElementSize) -> CircularLinkedList* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newSelf = CircularLinkedList_construct();
        if (!newSelf) {
            return nullptr;
        }

        auto* const headNode = headOf(self);
        if (!headNode) {
            return newSelf;
        }

        auto* currentNode = headNode;
        while (currentNode) {
            void* const slot = CircularLinkedList_pushTailSlot(newSelf, inElementSize);
            if (!slot) {
                CircularLinkedList_destruct(newSelf);
                return nullptr;
            }

            if (currentNode->data) {
                memcpy(slot, currentNode->data, inElementSize);
            }

            auto* const nextNode = currentNode->nextNode;
            if (nextNode == headNode) {
                break;
            }

            currentNode = nextNode;
        }

        return newSelf;
    }
    auto CircularLinkedList_move(CircularLinkedList* self) -> CircularLinkedList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (CircularLinkedList*) malloc(sizeof(CircularLinkedList));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->tailNode = self->tailNode;
        newSelf->size = self->size;

        self->tailNode = nullptr;
        self->size = 0;

        return newSelf;
    }

    auto CircularLinkedList_pushHead(CircularLinkedList* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        auto* const newNode = (CircularLinkedListNode*) malloc(sizeof(CircularLinkedListNode));
        if (!newNode) {
            return false;
        }

        if (inElement == nullptr) {
            newNode->data = nullptr;
        } else {
            newNode->data = malloc(inElementSize);
            if (!newNode->data) {
                free(newNode);
                return false;
            }
            memcpy(newNode->data, inElement, inElementSize);
        }

        if (self->tailNode == nullptr) {
            // 空链表：新节点既是头也是尾，next 指向自己
            newNode->nextNode = newNode;
            self->tailNode = newNode;
        } else {
            // 新节点成为新的 head，插在 tail 之后
            newNode->nextNode = self->tailNode->nextNode;
            self->tailNode->nextNode = newNode;
        }

        self->size++;

        return true;
    }
    auto CircularLinkedList_pushHeadSlot(CircularLinkedList* self, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newNode = (CircularLinkedListNode*) malloc(sizeof(CircularLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        if (self->tailNode == nullptr) {
            newNode->nextNode = newNode;
            self->tailNode = newNode;
        } else {
            newNode->nextNode = self->tailNode->nextNode;
            self->tailNode->nextNode = newNode;
        }

        self->size++;

        return newNode->data;
    }
    auto CircularLinkedList_pushTail(CircularLinkedList* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        auto* const newNode = (CircularLinkedListNode*) malloc(sizeof(CircularLinkedListNode));
        if (!newNode) {
            return false;
        }

        if (inElement == nullptr) {
            newNode->data = nullptr;
        } else {
            newNode->data = malloc(inElementSize);
            if (!newNode->data) {
                free(newNode);
                return false;
            }
            memcpy(newNode->data, inElement, inElementSize);
        }

        if (self->tailNode == nullptr) {
            newNode->nextNode = newNode;
            self->tailNode = newNode;
        } else {
            // 插在 tail 之后，新节点成为新 tail
            newNode->nextNode = self->tailNode->nextNode;
            self->tailNode->nextNode = newNode;
            self->tailNode = newNode;
        }

        self->size++;

        return true;
    }
    auto CircularLinkedList_pushTailSlot(CircularLinkedList* self, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newNode = (CircularLinkedListNode*) malloc(sizeof(CircularLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        if (self->tailNode == nullptr) {
            newNode->nextNode = newNode;
            self->tailNode = newNode;
        } else {
            newNode->nextNode = self->tailNode->nextNode;
            self->tailNode->nextNode = newNode;
            self->tailNode = newNode;
        }

        self->size++;

        return newNode->data;
    }
    auto CircularLinkedList_insertAt(CircularLinkedList* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0 || index > self->size) {
            return false;
        }

        // 头插
        if (index == 0) {
            return CircularLinkedList_pushHead(self, inElement, inElementSize);
        }

        // 尾插
        if (index == self->size) {
            return CircularLinkedList_pushTail(self, inElement, inElementSize);
        }

        auto* const newNode = (CircularLinkedListNode*) malloc(sizeof(CircularLinkedListNode));
        if (!newNode) {
            return false;
        }

        if (inElement == nullptr) {
            newNode->data = nullptr;
        } else {
            newNode->data = malloc(inElementSize);
            if (!newNode->data) {
                free(newNode);
                return false;
            }
            memcpy(newNode->data, inElement, inElementSize);
        }

        // 遍历到 index - 1 位置的节点
        auto* prevNode = headOf(self);
        if (!prevNode) {
            free(newNode->data);
            free(newNode);
            return false;
        }

        for (usize i = 0; i < index - 1; ++i) {
            prevNode = prevNode->nextNode;
            if (!prevNode) {
                free(newNode->data);
                free(newNode);
                return false;
            }
        }

        newNode->nextNode = prevNode->nextNode;
        prevNode->nextNode = newNode;

        self->size++;

        return true;
    }
    auto CircularLinkedList_insertAtSlot(CircularLinkedList* self, const usize index, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0 || index > self->size) {
            return nullptr;
        }

        if (index == 0) {
            return CircularLinkedList_pushHeadSlot(self, inElementSize);
        }

        if (index == self->size) {
            return CircularLinkedList_pushTailSlot(self, inElementSize);
        }

        auto* const newNode = (CircularLinkedListNode*) malloc(sizeof(CircularLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        auto* prevNode = headOf(self);
        if (!prevNode) {
            free(newNode->data);
            free(newNode);
            return nullptr;
        }

        for (usize i = 0; i < index - 1; ++i) {
            prevNode = prevNode->nextNode;
            if (!prevNode) {
                free(newNode->data);
                free(newNode);
                return nullptr;
            }
        }

        newNode->nextNode = prevNode->nextNode;
        prevNode->nextNode = newNode;

        self->size++;

        return newNode->data;
    }

    auto CircularLinkedList_popHead(CircularLinkedList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        auto* const headNode = headOf(self);
        if (!headNode) {
            return false;
        }

        if (self->size == 1) {
            // 只有一个节点：清空
            free(headNode->data);
            free(headNode);
            self->tailNode = nullptr;
        } else {
            // 移除 head，tail->next 指向下一个
            self->tailNode->nextNode = headNode->nextNode;

            free(headNode->data);
            free(headNode);
        }

        self->size--;

        return true;
    }
    auto CircularLinkedList_popHeadSlot(CircularLinkedList* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        auto* const headNode = headOf(self);
        if (!headNode) {
            return nullptr;
        }

        void* const data = headNode->data;

        if (self->size == 1) {
            free(headNode);
            self->tailNode = nullptr;
        } else {
            self->tailNode->nextNode = headNode->nextNode;
            free(headNode);
        }

        self->size--;

        return data;
    }
    auto CircularLinkedList_popTail(CircularLinkedList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        if (self->size == 1) {
            free(self->tailNode->data);
            free(self->tailNode);
            self->tailNode = nullptr;
            self->size--;
            return true;
        }

        // 找 tail 的前驱节点——O(N)
        auto* prevNode = headOf(self);
        if (!prevNode) {
            return false;
        }

        while (prevNode->nextNode != self->tailNode) {
            prevNode = prevNode->nextNode;
            if (!prevNode) {
                return false;
            }
        }

        auto* const oldTail = self->tailNode;
        prevNode->nextNode = oldTail->nextNode;
        self->tailNode = prevNode;

        free(oldTail->data);
        free(oldTail);

        self->size--;

        return true;
    }
    auto CircularLinkedList_popTailSlot(CircularLinkedList* self) -> void* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        if (self->size == 1) {
            void* const data = self->tailNode->data;
            free(self->tailNode);
            self->tailNode = nullptr;
            self->size--;
            return data;
        }

        auto* prevNode = headOf(self);
        if (!prevNode) {
            return nullptr;
        }

        while (prevNode->nextNode != self->tailNode) {
            prevNode = prevNode->nextNode;
            if (!prevNode) {
                return nullptr;
            }
        }

        auto* const oldTail = self->tailNode;
        void* const data = oldTail->data;

        prevNode->nextNode = oldTail->nextNode;
        self->tailNode = prevNode;

        free(oldTail);

        self->size--;

        return data;
    }
    auto CircularLinkedList_removeAt(CircularLinkedList* self, const usize index) -> bool {
        if (!self || index >= self->size) {
            return false;
        }

        if (index == 0) {
            return CircularLinkedList_popHead(self);
        }

        if (index == self->size - 1) {
            return CircularLinkedList_popTail(self);
        }

        // 找到 index - 1 位置的节点
        auto* prevNode = headOf(self);
        if (!prevNode) {
            return false;
        }

        for (usize i = 0; i < index - 1; ++i) {
            prevNode = prevNode->nextNode;
            if (!prevNode) {
                return false;
            }
        }

        auto* const targetNode = prevNode->nextNode;
        if (!targetNode) {
            return false;
        }

        prevNode->nextNode = targetNode->nextNode;

        free(targetNode->data);
        free(targetNode);

        self->size--;

        return true;
    }
    auto CircularLinkedList_removeAtSlot(CircularLinkedList* self, const usize index) -> void* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        if (index == 0) {
            return CircularLinkedList_popHeadSlot(self);
        }

        if (index == self->size - 1) {
            return CircularLinkedList_popTailSlot(self);
        }

        auto* prevNode = headOf(self);
        if (!prevNode) {
            return nullptr;
        }

        for (usize i = 0; i < index - 1; ++i) {
            prevNode = prevNode->nextNode;
            if (!prevNode) {
                return nullptr;
            }
        }

        auto* const targetNode = prevNode->nextNode;
        if (!targetNode) {
            return nullptr;
        }

        void* const data = targetNode->data;

        prevNode->nextNode = targetNode->nextNode;

        free(targetNode);

        self->size--;

        return data;
    }
    auto CircularLinkedList_removeIf(CircularLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool {
        if (!self || !dataEquals || self->size == 0) {
            return false;
        }

        auto* const headNode = headOf(self);
        if (!headNode) {
            return false;
        }

        // 头节点匹配
        if (dataEquals(headNode->data, inData)) {
            return CircularLinkedList_popHead(self);
        }

        // 从第二个节点开始遍历
        auto* prevNode = headNode;
        auto* currentNode = headNode->nextNode;

        while (currentNode && currentNode != headNode) {
            if (dataEquals(currentNode->data, inData)) {
                prevNode->nextNode = currentNode->nextNode;

                // 如果删的是 tail，更新 tail
                if (currentNode == self->tailNode) {
                    self->tailNode = prevNode;
                }

                free(currentNode->data);
                free(currentNode);

                self->size--;

                return true;
            }

            prevNode = currentNode;
            currentNode = currentNode->nextNode;
        }

        return false;
    }
    auto CircularLinkedList_clear(CircularLinkedList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        auto* const headNode = headOf(self);
        if (!headNode) {
            self->tailNode = nullptr;
            self->size = 0;
            return true;
        }

        auto* currentNode = headNode;
        while (currentNode) {
            auto* const nextNode = currentNode->nextNode;

            free(currentNode->data);
            free(currentNode);

            if (nextNode == headNode) {
                break;
            }

            currentNode = nextNode;
        }

        self->tailNode = nullptr;
        self->size = 0;

        return true;
    }

    auto CircularLinkedList_get(const CircularLinkedList* self, const usize index) -> CircularLinkedListNode* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        auto* targetNode = headOf(self);
        if (!targetNode) {
            return nullptr;
        }

        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
            if (!targetNode) {
                return nullptr;
            }
        }

        return targetNode;
    }
    auto CircularLinkedList_head(const CircularLinkedList* self) -> CircularLinkedListNode* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return headOf(self);
    }
    auto CircularLinkedList_tail(const CircularLinkedList* self) -> CircularLinkedListNode* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->tailNode;
    }

    auto CircularLinkedList_set(const CircularLinkedList* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || index >= self->size || inElementSize == 0) {
            return false;
        }

        auto* targetNode = headOf(self);
        if (!targetNode) {
            return false;
        }

        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
            if (!targetNode) {
                return false;
            }
        }

        void* newData = nullptr;
        if (inElement != nullptr) {
            newData = malloc(inElementSize);
            if (!newData) {
                return false;
            }
            memcpy(newData, inElement, inElementSize);
        }

        if (targetNode->data) {
            free(targetNode->data);
        }

        targetNode->data = newData;

        return true;
    }
    auto CircularLinkedList_setSlot(const CircularLinkedList* self, const usize index, const usize inElementSize) -> void* {
        if (!self || index >= self->size || inElementSize == 0) {
            return nullptr;
        }

        auto* targetNode = headOf(self);
        if (!targetNode) {
            return nullptr;
        }

        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
            if (!targetNode) {
                return nullptr;
            }
        }

        if (targetNode->data) {
            free(targetNode->data);
        }

        targetNode->data = malloc(inElementSize);
        if (!targetNode->data) {
            return nullptr;
        }

        return targetNode->data;
    }

    auto CircularLinkedList_reverse(CircularLinkedList* self) -> bool {
        if (!self || self->size <= 1) {
            return false;
        }

        auto* const oldHead = headOf(self);
        if (!oldHead) {
            return false;
        }

        auto* prevNode = (CircularLinkedListNode*) nullptr;
        auto* currentNode = oldHead;

        // 遍历一圈，反转每个节点的 next
        while (currentNode) {
            auto* const nextNode = currentNode->nextNode;
            currentNode->nextNode = prevNode;
            prevNode = currentNode;

            if (nextNode == oldHead) {
                break;
            }

            currentNode = nextNode;
        }

        // 原来的 head 变成 tail
        self->tailNode = oldHead;

        return true;
    }

    auto CircularLinkedList_begin(const CircularLinkedList* self) -> CircularLinkedListNode* {
        if (!self) {
            return nullptr;
        }

        return headOf(self);
    }
    auto CircularLinkedList_end(const CircularLinkedList* self) -> CircularLinkedListNode* {
        if (!self) {
            return nullptr;
        }

        return headOf(self);
    }

    auto CircularLinkedList_size(const CircularLinkedList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }

    auto CircularLinkedList_isEmpty(const CircularLinkedList* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }
    auto CircularLinkedList_contains(const CircularLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool {
        if (!self || !dataEquals) {
            return false;
        }

        auto* const headNode = headOf(self);
        if (!headNode) {
            return false;
        }

        auto* currentNode = headNode;
        while (currentNode) {
            if (dataEquals(currentNode->data, inData)) {
                return true;
            }

            auto* const nextNode = currentNode->nextNode;
            if (nextNode == headNode) {
                break;
            }

            currentNode = nextNode;
        }

        return false;
    }

    auto CircularLinkedListNode_next(const CircularLinkedListNode* node) -> CircularLinkedListNode* {
        if (!node) {
            return nullptr;
        }

        return node->nextNode;
    }
    auto CircularLinkedListNode_data(const CircularLinkedListNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->data;
    }
}