#include "DoublyLinkedList.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct DoublyLinkedListNode {
        void* data;
        DoublyLinkedListNode* prevNode;
        DoublyLinkedListNode* nextNode;
    } DoublyLinkedListNode;

    typedef struct DoublyLinkedList {
        DoublyLinkedListNode* headNode;
        DoublyLinkedListNode* tailNode;
        usize size;
    } DoublyLinkedList;

    auto DoublyLinkedList_construct() -> DoublyLinkedList* {
        auto* const self = (DoublyLinkedList*) malloc(sizeof(DoublyLinkedList));
        if (!self) {
            return nullptr;
        }

        self->headNode = nullptr;
        self->tailNode = nullptr;
        self->size = 0;

        return self;
    }
    auto DoublyLinkedList_destruct(DoublyLinkedList* self) -> bool {
        if (!self) {
            return false;
        }

        // 遍历释放每一个节点的内部数据，并释放自身
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

    auto DoublyLinkedList_copy(const DoublyLinkedList* self, const usize inElementSize) -> DoublyLinkedList* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newSelf = DoublyLinkedList_construct();
        if (!newSelf) {
            return nullptr;
        }

        // 遍历当前自身的节点链，准备尾插法构建新链表（保持顺序）
        DoublyLinkedListNode* tailNode = nullptr;
        for (const auto* currentNode = self->headNode; currentNode; currentNode = currentNode->nextNode) {
            // 先分配新节点
            auto* newNode = (DoublyLinkedListNode*) malloc(sizeof(DoublyLinkedListNode));
            if (!newNode) {
                DoublyLinkedList_destruct(newSelf);
                return nullptr;
            }

            // 分配并复制节点数据内存：当前节点 -> 新节点
            if (currentNode->data == nullptr) {
                newNode->data = nullptr;
            } else {
                newNode->data = malloc(inElementSize);
                if (!newNode->data) {
                    free(newNode);
                    DoublyLinkedList_destruct(newSelf);
                    return nullptr;
                }
                memcpy(newNode->data, currentNode->data, inElementSize);
            }

            // 挂到新链表尾部（双向链接）
            newNode->prevNode = tailNode;
            newNode->nextNode = nullptr;

            if (tailNode) {
                // 尾节点已存在，则将新节点挂到尾节点的后面
                tailNode->nextNode = newNode;
            } else {
                // 尾节点还不存在，则将新节点直接挂到头部
                newSelf->headNode = newNode;
            }

            // 节点拷贝完毕，更新尾节点为新节点，继续下一轮拷贝
            tailNode = newNode;
            newSelf->tailNode = newNode;

            newSelf->size++;
        }

        return newSelf;
    }
    auto DoublyLinkedList_move(DoublyLinkedList* self) -> DoublyLinkedList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (DoublyLinkedList*) malloc(sizeof(DoublyLinkedList));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->headNode = self->headNode;
        newSelf->tailNode = self->tailNode;
        newSelf->size = self->size;

        self->headNode = nullptr;
        self->tailNode = nullptr;
        self->size = 0;

        return newSelf;
    }

    auto DoublyLinkedList_pushHead(DoublyLinkedList* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        auto* const newNode = (DoublyLinkedListNode*) malloc(sizeof(DoublyLinkedListNode));
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

        // 新节点插入到头部，prev 为 nullptr，next 指向原头节点
        newNode->prevNode = nullptr;
        newNode->nextNode = self->headNode;

        if (self->headNode) {
            // 原头节点的 prev 指向新节点
            self->headNode->prevNode = newNode;
        } else {
            // 空链表时，新节点也是尾节点
            self->tailNode = newNode;
        }

        self->headNode = newNode;
        self->size++;

        return true;
    }
    auto DoublyLinkedList_pushHeadSlot(DoublyLinkedList* self, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newNode = (DoublyLinkedListNode*) malloc(sizeof(DoublyLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        // 分配未初始化的数据内存槽位，返回给调用者做 placement new
        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        // 新节点插入到头部，prev 为 nullptr，next 指向原头节点
        newNode->prevNode = nullptr;
        newNode->nextNode = self->headNode;

        if (self->headNode) {
            // 原头节点的 prev 指向新节点
            self->headNode->prevNode = newNode;
        } else {
            // 空链表时，新节点也是尾节点
            self->tailNode = newNode;
        }

        self->headNode = newNode;
        self->size++;

        return newNode->data;
    }
    auto DoublyLinkedList_pushTail(DoublyLinkedList* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        auto* const newNode = (DoublyLinkedListNode*) malloc(sizeof(DoublyLinkedListNode));
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

        // 新节点插入到尾部，next 为 nullptr，prev 指向原尾节点
        newNode->nextNode = nullptr;
        newNode->prevNode = self->tailNode;

        if (self->tailNode) {
            // 原尾节点的 next 指向新节点
            self->tailNode->nextNode = newNode;
        } else {
            // 空链表时，新节点也是头节点
            self->headNode = newNode;
        }

        self->tailNode = newNode;
        self->size++;

        return true;
    }
    auto DoublyLinkedList_pushTailSlot(DoublyLinkedList* self, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newNode = (DoublyLinkedListNode*) malloc(sizeof(DoublyLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        // 分配未初始化的数据内存槽位，返回给调用者做 placement new
        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        // 新节点插入到尾部，next 为 nullptr，prev 指向原尾节点
        newNode->nextNode = nullptr;
        newNode->prevNode = self->tailNode;

        if (self->tailNode) {
            // 原尾节点的 next 指向新节点
            self->tailNode->nextNode = newNode;
        } else {
            // 空链表时，新节点也是头节点
            self->headNode = newNode;
        }

        self->tailNode = newNode;
        self->size++;

        return newNode->data;
    }
    auto DoublyLinkedList_insertAt(DoublyLinkedList* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0 || index > self->size) {
            return false;
        }

        // 头插，复用 pushHead
        if (index == 0) {
            return DoublyLinkedList_pushHead(self, inElement, inElementSize);
        }

        // 尾插，复用 pushTail
        if (index == self->size) {
            return DoublyLinkedList_pushTail(self, inElement, inElementSize);
        }

        auto* const newNode = (DoublyLinkedListNode*) malloc(sizeof(DoublyLinkedListNode));
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

        // 遍历到 index 位置的节点（目标节点）
        auto* targetNode = self->headNode;
        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
        }

        // 新节点插入到 targetNode 之前
        newNode->prevNode = targetNode->prevNode;
        newNode->nextNode = targetNode;

        targetNode->prevNode->nextNode = newNode;
        targetNode->prevNode = newNode;

        self->size++;

        return true;
    }
    auto DoublyLinkedList_insertAtSlot(DoublyLinkedList* self, const usize index, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0 || index > self->size) {
            return nullptr;
        }

        // 头插，复用 pushHeadSlot
        if (index == 0) {
            return DoublyLinkedList_pushHeadSlot(self, inElementSize);
        }

        // 尾插，复用 pushTailSlot
        if (index == self->size) {
            return DoublyLinkedList_pushTailSlot(self, inElementSize);
        }

        auto* const newNode = (DoublyLinkedListNode*) malloc(sizeof(DoublyLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        // 分配未初始化的数据内存槽位，返回给调用者做 placement new
        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        // 遍历到 index 位置的节点（目标节点）
        auto* targetNode = self->headNode;
        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
        }

        // 新节点插入到 targetNode 之前
        newNode->prevNode = targetNode->prevNode;
        newNode->nextNode = targetNode;

        targetNode->prevNode->nextNode = newNode;
        targetNode->prevNode = newNode;

        self->size++;

        return newNode->data;
    }

    auto DoublyLinkedList_popHead(DoublyLinkedList* self) -> bool {
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
            // 链表空了，tail 也要置空
            self->tailNode = nullptr;
        }

        self->size--;

        return true;
    }
    auto DoublyLinkedList_popHeadSlot(DoublyLinkedList* self) -> void* {
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
            // 链表空了，tail 也要置空
            self->tailNode = nullptr;
        }

        self->size--;

        return data;
    }
    auto DoublyLinkedList_popTail(DoublyLinkedList* self) -> bool {
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
            // 链表空了，head 也要置空
            self->headNode = nullptr;
        }

        self->size--;

        return true;
    }
    auto DoublyLinkedList_popTailSlot(DoublyLinkedList* self) -> void* {
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
            // 链表空了，head 也要置空
            self->headNode = nullptr;
        }

        self->size--;

        return data;
    }
    auto DoublyLinkedList_removeAt(DoublyLinkedList* self, const usize index) -> bool {
        if (!self || index >= self->size || !self->headNode) {
            return false;
        }

        // 目标头节点，直接复用头节点弹出函数
        if (index == 0) {
            return DoublyLinkedList_popHead(self);
        }

        // 目标尾节点，直接复用尾节点弹出函数
        if (index == self->size - 1) {
            return DoublyLinkedList_popTail(self);
        }

        // 遍历到目标节点
        auto* targetNode = self->headNode;
        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
        }

        // 让前驱和后继互相指向，跳过目标节点
        targetNode->prevNode->nextNode = targetNode->nextNode;
        targetNode->nextNode->prevNode = targetNode->prevNode;

        // 释放目标节点的数据
        if (targetNode->data) {
            free(targetNode->data);
        }

        // 释放目标节点本身
        free(targetNode);

        self->size--;

        return true;
    }
    auto DoublyLinkedList_removeAtSlot(DoublyLinkedList* self, const usize index) -> void* {
        if (!self || index >= self->size || !self->headNode) {
            return nullptr;
        }

        // 目标头节点，直接复用头节点弹出槽位函数
        if (index == 0) {
            return DoublyLinkedList_popHeadSlot(self);
        }

        // 目标尾节点，直接复用尾节点弹出槽位函数
        if (index == self->size - 1) {
            return DoublyLinkedList_popTailSlot(self);
        }

        // 从头或尾出发，取近的一端遍历
        DoublyLinkedListNode* targetNode = nullptr;
        if (index <= self->size / 2) {
            targetNode = self->headNode;
            for (usize i = 0; i < index; ++i) {
                targetNode = targetNode->nextNode;
            }
        } else {
            targetNode = self->tailNode;
            for (usize i = self->size - 1; i > index; --i) {
                targetNode = targetNode->prevNode;
            }
        }

        // 保存目标节点的数据指针，稍后返回给调用者
        void* const data = targetNode->data;

        // 让前驱和后继互相指向，跳过目标节点
        targetNode->prevNode->nextNode = targetNode->nextNode;
        targetNode->nextNode->prevNode = targetNode->prevNode;

        // 释放目标节点结构体本身，但不释放数据内存
        free(targetNode);
        self->size--;

        return data;
    }
    auto DoublyLinkedList_removeIf(DoublyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool {
        if (!self || !dataEquals || self->size == 0) {
            return false;
        }

        // 头节点匹配，直接弹出头节点
        if (dataEquals(self->headNode->data, inData)) {
            return DoublyLinkedList_popHead(self);
        }

        // 尾节点匹配，直接弹出尾节点
        if (dataEquals(self->tailNode->data, inData)) {
            return DoublyLinkedList_popTail(self);
        }

        // 从中间节点遍历查找匹配节点
        auto* currentNode = self->headNode->nextNode;
        while (currentNode != self->tailNode) {
            // 找到匹配节点
            if (dataEquals(currentNode->data, inData)) {
                // 前驱和后继互相指向，跳过当前节点
                currentNode->prevNode->nextNode = currentNode->nextNode;
                currentNode->nextNode->prevNode = currentNode->prevNode;

                // 释放当前节点的数据
                if (currentNode->data) {
                    free(currentNode->data);
                }

                // 释放当前节点
                free(currentNode);

                self->size--;

                return true;
            }

            // 继续下一轮查找匹配
            currentNode = currentNode->nextNode;
        }

        return false;
    }
    auto DoublyLinkedList_clear(DoublyLinkedList* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 遍历所有节点
        auto* currentNode = self->headNode;
        while (currentNode) {
            auto* nextNode = currentNode->nextNode;

            // 释放当前节点的数据
            if (currentNode->data) {
                free(currentNode->data);
            }

            // 释放当前节点
            free(currentNode);

            // 继续遍历下一个节点
            currentNode = nextNode;
        }

        self->headNode = nullptr;
        self->tailNode = nullptr;
        self->size = 0;

        return true;
    }

    auto DoublyLinkedList_get(const DoublyLinkedList* self, const usize index) -> DoublyLinkedListNode* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        // 从头或尾出发，取近的一端遍历
        if (index <= self->size / 2) {
            // 从头部出发
            auto* targetNode = self->headNode;
            for (usize i = 0; i < index; ++i) {
                targetNode = targetNode->nextNode;
            }

            return targetNode;
        }

        // 从尾部出发
        auto* targetNode = self->tailNode;
        for (usize i = self->size - 1; i > index; --i) {
            targetNode = targetNode->prevNode;
        }

        return targetNode;
    }
    auto DoublyLinkedList_head(const DoublyLinkedList* self) -> DoublyLinkedListNode* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->headNode;
    }
    auto DoublyLinkedList_tail(const DoublyLinkedList* self) -> DoublyLinkedListNode* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->tailNode;
    }

    auto DoublyLinkedList_set(const DoublyLinkedList* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || index >= self->size || inElementSize == 0) {
            return false;
        }

        // 从头或尾出发，取近的一端遍历
        DoublyLinkedListNode* targetNode = nullptr;
        if (index <= self->size / 2) {
            targetNode = self->headNode;
            for (usize i = 0; i < index; ++i) {
                targetNode = targetNode->nextNode;
            }
        } else {
            targetNode = self->tailNode;
            for (usize i = self->size - 1; i > index; --i) {
                targetNode = targetNode->prevNode;
            }
        }

        // 分配并复制新数据内存
        void* newData = nullptr;
        if (inElement != nullptr) {
            newData = malloc(inElementSize);
            if (!newData) {
                return false;
            }
            memcpy(newData, inElement, inElementSize);
        }

        // 释放旧数据
        if (targetNode->data) {
            free(targetNode->data);
        }

        // 替换为新数据
        targetNode->data = newData;

        return true;
    }
    auto DoublyLinkedList_setSlot(const DoublyLinkedList* self, const usize index, const usize inElementSize) -> void* {
        if (!self || index >= self->size || inElementSize == 0) {
            return nullptr;
        }

        // 从头或尾出发，取近的一端遍历
        DoublyLinkedListNode* targetNode = nullptr;
        if (index <= self->size / 2) {
            targetNode = self->headNode;
            for (usize i = 0; i < index; ++i) {
                targetNode = targetNode->nextNode;
            }
        } else {
            targetNode = self->tailNode;
            for (usize i = self->size - 1; i > index; --i) {
                targetNode = targetNode->prevNode;
            }
        }

        // 释放旧数据内存（调用者已在此之前析构旧元素）
        if (targetNode->data) {
            free(targetNode->data);
        }

        // 分配未初始化的新数据内存槽位，返回给调用者做 placement new
        targetNode->data = malloc(inElementSize);
        if (!targetNode->data) {
            return nullptr;
        }

        return targetNode->data;
    }

    auto DoublyLinkedList_reverse(DoublyLinkedList* self) -> bool {
        if (!self || self->size <= 1) {
            return false;
        }

        // 遍历所有节点，交换每个节点的上一个节点和下一个节点
        auto* currentNode = self->headNode;
        while (currentNode) {
            auto* const tempNode = currentNode->prevNode;
            currentNode->prevNode = currentNode->nextNode;
            currentNode->nextNode = tempNode;

            // 继续处理下一个节点，注意此时上一个节点已经是原来的下一个节点
            currentNode = currentNode->prevNode;
        }

        // 交换头节点和尾节点
        auto* const tempNode = self->headNode;
        self->headNode = self->tailNode;
        self->tailNode = tempNode;

        return true;
    }

    auto DoublyLinkedList_begin(const DoublyLinkedList* self) -> DoublyLinkedListNode* {
        if (!self) {
            return nullptr;
        }

        return self->headNode;
    }
    auto DoublyLinkedList_end() -> DoublyLinkedListNode* {
        return nullptr;
    }

    auto DoublyLinkedList_size(const DoublyLinkedList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }

    auto DoublyLinkedList_isEmpty(const DoublyLinkedList* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }
    auto DoublyLinkedList_contains(const DoublyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool {
        if (!self || !dataEquals) {
            return false;
        }

        const auto* currentNode = self->headNode;
        while (currentNode) {
            // 任一节点数据与目标数据相同，认为节点链包含目标数据
            if (dataEquals(currentNode->data, inData)) {
                return true;
            }

            currentNode = currentNode->nextNode;
        }

        return false;
    }

    auto DoublyLinkedListNode_prev(const DoublyLinkedListNode* node) -> DoublyLinkedListNode* {
        if (!node) {
            return nullptr;
        }

        return node->prevNode;
    }
    auto DoublyLinkedListNode_next(const DoublyLinkedListNode* node) -> DoublyLinkedListNode* {
        if (!node) {
            return nullptr;
        }

        return node->nextNode;
    }
    auto DoublyLinkedListNode_data(const DoublyLinkedListNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->data;
    }
}