#include "SinglyLinkedList.hpp"

#include <cstdlib>
#include <cstring>

#include "../CommonMacro.hpp"

namespace Catelier::src::foundation {
    typedef struct SinglyLinkedListNode {
        void* data;
        SinglyLinkedListNode* nextNode;
    } SinglyLinkedListNode;

    typedef struct SinglyLinkedList {
        SinglyLinkedListNode* headNode;
        usize size;
    } SinglyLinkedList;

    auto SinglyLinkedList_construct() -> SinglyLinkedList* {
        auto* const self = (SinglyLinkedList*) malloc(sizeof(SinglyLinkedList));
        if (!self) {
            return nullptr;
        }

        self->headNode = nullptr;
        self->size = 0;

        return self;
    }
    auto SinglyLinkedList_destruct(SinglyLinkedList* self) -> bool {
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

    auto SinglyLinkedList_copy(const SinglyLinkedList* self, const usize inElementSize) -> SinglyLinkedList* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newSelf = SinglyLinkedList_construct();
        if (!newSelf) {
            return nullptr;
        }

        // 遍历当前自身的节点链，准备尾插法构建新链表（保持顺序）
        SinglyLinkedListNode* tailNode = nullptr;
        for (const auto* currentNode = self->headNode; currentNode; currentNode = currentNode->nextNode) {
            // 先分配新节点
            auto* newNode = (SinglyLinkedListNode*) malloc(sizeof(SinglyLinkedListNode));
            if (!newNode) {
                SinglyLinkedList_destruct(newSelf);
                return nullptr;
            }

            // 分配并复制节点数据内存：当前节点 -> 新节点
            if (currentNode->data == nullptr) {
                newNode->data = nullptr;
            } else {
                newNode->data = malloc(inElementSize);
                if (!newNode->data) {
                    free(newNode);
                    SinglyLinkedList_destruct(newSelf);
                    return nullptr;
                }
                memcpy(newNode->data, currentNode->data, inElementSize);
            }

            // 置空新节点的下一节点指针，即暂时认为新节点已经是尾节点了
            // 由于下一步尾节点判断会把暂时是尾节点但实际非尾节点的节点的下一节点指针正确指向新节点
            // 所以这一步不会出现问题，且保证整个构建好的新链表是节点链连续、顺序不变、尾节点状态正确的
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

            newSelf->size++;
        }

        return newSelf;
    }
    auto SinglyLinkedList_move(SinglyLinkedList* self) -> SinglyLinkedList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (SinglyLinkedList*) malloc(sizeof(SinglyLinkedList));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->headNode = self->headNode;
        newSelf->size = self->size;

        self->headNode = nullptr;
        self->size = 0;

        return newSelf;
    }

    auto SinglyLinkedList_pushHead(SinglyLinkedList* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        auto* const newNode = (SinglyLinkedListNode*) malloc(sizeof(SinglyLinkedListNode));
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

        newNode->nextNode = self->headNode;
        self->headNode = newNode;
        self->size++;

        return true;
    }
    auto SinglyLinkedList_pushHeadSlot(SinglyLinkedList* self, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newNode = (SinglyLinkedListNode*) malloc(sizeof(SinglyLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        // 分配未初始化的数据内存槽位，返回给调用者做 placement new
        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        newNode->nextNode = self->headNode;
        self->headNode = newNode;
        self->size++;

        return newNode->data;
    }
    auto SinglyLinkedList_pushTail(SinglyLinkedList* self, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0) {
            return false;
        }

        auto* const newNode = (SinglyLinkedListNode*) malloc(sizeof(SinglyLinkedListNode));
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

        // 尾节点，没有下一节点
        newNode->nextNode = nullptr;

        if (self->headNode == nullptr) {
            // 节点链不存在，新节点就是头节点
            self->headNode = newNode;
        } else {
            // 节点链存在，遍历到最后一个节点
            auto* lastNode = self->headNode;
            while (lastNode->nextNode) {
                lastNode = lastNode->nextNode;
            }

            lastNode->nextNode = newNode;
        }

        self->size++;

        return true;
    }
    auto SinglyLinkedList_pushTailSlot(SinglyLinkedList* self, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0) {
            return nullptr;
        }

        auto* const newNode = (SinglyLinkedListNode*) malloc(sizeof(SinglyLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        // 分配未初始化的数据内存槽位，返回给调用者做 placement new
        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        newNode->nextNode = nullptr;

        if (self->headNode == nullptr) {
            // 节点链不存在，新节点就是头节点
            self->headNode = newNode;
        } else {
            // 节点链存在，遍历到最后一个节点
            auto* lastNode = self->headNode;
            while (lastNode->nextNode) {
                lastNode = lastNode->nextNode;
            }

            lastNode->nextNode = newNode;
        }

        self->size++;

        return newNode->data;
    }
    auto SinglyLinkedList_insertAt(SinglyLinkedList* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || inElementSize == 0 || index > self->size) {
            return false;
        }

        auto* const newNode = (SinglyLinkedListNode*) malloc(sizeof(SinglyLinkedListNode));
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

        if (index == 0) {
            // 插入到头部
            newNode->nextNode = self->headNode;
            self->headNode = newNode;
        } else {
            // 遍历到 index - 1 的位置
            auto* prevNode = self->headNode;
            for (usize i = 0; i < index - 1; ++i) {
                prevNode = prevNode->nextNode;
            }

            newNode->nextNode = prevNode->nextNode;
            prevNode->nextNode = newNode;
        }

        self->size++;

        return true;
    }
    auto SinglyLinkedList_insertAtSlot(SinglyLinkedList* self, const usize index, const usize inElementSize) -> void* {
        if (!self || inElementSize == 0 || index > self->size) {
            return nullptr;
        }

        auto* const newNode = (SinglyLinkedListNode*) malloc(sizeof(SinglyLinkedListNode));
        if (!newNode) {
            return nullptr;
        }

        // 分配未初始化的数据内存槽位，返回给调用者做 placement new
        newNode->data = malloc(inElementSize);
        if (!newNode->data) {
            free(newNode);
            return nullptr;
        }

        if (index == 0) {
            // 插入到头部
            newNode->nextNode = self->headNode;
            self->headNode = newNode;
        } else {
            // 遍历到 index - 1 的位置
            auto* prevNode = self->headNode;
            for (usize i = 0; i < index - 1; ++i) {
                prevNode = prevNode->nextNode;
            }

            newNode->nextNode = prevNode->nextNode;
            prevNode->nextNode = newNode;
        }

        self->size++;

        return newNode->data;
    }

    auto SinglyLinkedList_popHead(SinglyLinkedList* self) -> bool {
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

        // 更新头节点为下一个节点，即使 nextNode 是 nullptr
        self->headNode = nextNode;
        self->size--;

        return true;
    }
    auto SinglyLinkedList_popHeadSlot(SinglyLinkedList* self) -> void* {
        if (!self || self->size == 0 || !self->headNode) {
            return nullptr;
        }

        // 保存下一个节点的指针
        auto* const nextNode = self->headNode->nextNode;

        // 保存头节点的数据指针，稍后返回给调用者
        void* const data = self->headNode->data;

        // 释放头节点结构体本身，但不释放数据内存
        free(self->headNode);

        // 更新头节点为下一个节点，即使 nextNode 是 nullptr
        self->headNode = nextNode;
        self->size--;

        return data;
    }
    auto SinglyLinkedList_popTail(SinglyLinkedList* self) -> bool {
        if (!self || self->size == 0 || !self->headNode) {
            return false;
        }

        // 节点链不存在，只有头节点存在，则弹出头节点
        if (!self->headNode->nextNode) {
            // 释放头节点的数据
            if (self->headNode->data) {
                free(self->headNode->data);
            }

            // 释放并置空头节点
            PTR_FREE_AND_NULL(self->headNode);

            self->size--;

            return true;
        }

        // 节点链存在，遍历查找尾节点和未来尾节点
        auto* tailNode = self->headNode->nextNode;
        auto* futureTailNode = self->headNode;
        while (tailNode->nextNode) {
            // 始终先将未来尾节点指向尾节点，然后再向后更新尾节点，语义更明确
            futureTailNode = tailNode;
            tailNode = tailNode->nextNode;
        }

        // 释放尾节点的数据
        if (tailNode->data) {
            free(tailNode->data);
        }

        // 释放尾节点
        free(tailNode);

        // 将未来尾节点的下一节点指针置空，即认为未来尾节点是当前尾节点
        futureTailNode->nextNode = nullptr;

        self->size--;

        return true;
    }
    auto SinglyLinkedList_popTailSlot(SinglyLinkedList* self) -> void* {
        if (!self || self->size == 0 || !self->headNode) {
            return nullptr;
        }

        // 节点链不存在，只有头节点存在，直接复用头节点弹出槽位的逻辑
        if (!self->headNode->nextNode) {
            void* const data = self->headNode->data;

            free(self->headNode);
            self->headNode = nullptr;
            self->size--;

            return data;
        }

        // 节点链存在，遍历查找尾节点和未来尾节点
        auto* tailNode = self->headNode->nextNode;
        auto* futureTailNode = self->headNode;
        while (tailNode->nextNode) {
            futureTailNode = tailNode;
            tailNode = tailNode->nextNode;
        }

        // 保存尾节点的数据指针，稍后返回给调用者
        void* const data = tailNode->data;

        // 释放尾节点结构体本身，但不释放数据内存
        free(tailNode);

        // 将未来尾节点的下一节点指针置空
        futureTailNode->nextNode = nullptr;
        self->size--;

        return data;
    }
    auto SinglyLinkedList_removeAt(SinglyLinkedList* self, const usize index) -> bool {
        if (!self || index >= self->size || !self->headNode) {
            return false;
        }

        // 目标头节点，直接复用头节点弹出函数
        if (index == 0) {
            return SinglyLinkedList_popHead(self);
        }

        // 目标尾节点，直接复用尾节点弹出函数
        if (index == self->size - 1) {
            return SinglyLinkedList_popTail(self);
        }

        // 找到 index - 1 位置的节点（目标节点的前一个）
        auto* prevNode = self->headNode;
        for (usize i = 0; i < index - 1; ++i) {
            prevNode = prevNode->nextNode;
        }

        // 目标节点
        auto* targetNode = prevNode->nextNode;

        // 让前一个节点跳过目标节点，直接指向目标节点的下一个节点
        prevNode->nextNode = targetNode->nextNode;

        // 释放目标节点的数据
        if (targetNode->data) {
            free(targetNode->data);
        }

        // 释放目标节点本身
        free(targetNode);

        self->size--;

        return true;
    }
    auto SinglyLinkedList_removeAtSlot(SinglyLinkedList* self, const usize index) -> void* {
        if (!self || index >= self->size || !self->headNode) {
            return nullptr;
        }

        // 目标头节点，直接复用头节点弹出槽位函数
        if (index == 0) {
            return SinglyLinkedList_popHeadSlot(self);
        }

        // 目标尾节点，直接复用尾节点弹出槽位函数
        if (index == self->size - 1) {
            return SinglyLinkedList_popTailSlot(self);
        }

        // 找到 index - 1 位置的节点（目标节点的前一个）
        auto* prevNode = self->headNode;
        for (usize i = 0; i < index - 1; ++i) {
            prevNode = prevNode->nextNode;
        }

        // 目标节点
        auto* targetNode = prevNode->nextNode;

        // 保存目标节点的数据指针，稍后返回给调用者
        void* const data = targetNode->data;

        // 让前一个节点跳过目标节点，直接指向目标节点的下一个节点
        prevNode->nextNode = targetNode->nextNode;

        // 释放目标节点结构体本身，但不释放数据内存
        free(targetNode);
        self->size--;

        return data;
    }
    auto SinglyLinkedList_removeIf(SinglyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool {
        if (!self || !dataEquals || self->size == 0) {
            return false;
        }

        // 头节点匹配，直接弹出头节点
        if (dataEquals(self->headNode->data, inData)) {
            return SinglyLinkedList_popHead(self);
        }

        // 从头节点的下一个节点开始，遍历查找匹配节点
        auto* prevNode = self->headNode;
        auto* currentNode = self->headNode->nextNode;
        while (currentNode) {
            // 找到匹配节点
            if (dataEquals(currentNode->data, inData)) {
                // 前驱跳过当前节点
                prevNode->nextNode = currentNode->nextNode;

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
            prevNode = currentNode;
            currentNode = currentNode->nextNode;
        }

        return false;
    }
    auto SinglyLinkedList_clear(SinglyLinkedList* self) -> bool {
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
        self->size = 0;

        return true;
    }

    auto SinglyLinkedList_get(const SinglyLinkedList* self, const usize index) -> SinglyLinkedListNode* {
        if (!self || index >= self->size) {
            return nullptr;
        }

        // 遍历查找目标节点，O(N) 复杂度
        auto* targetNode = self->headNode;
        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
        }

        return targetNode;
    }
    auto SinglyLinkedList_head(const SinglyLinkedList* self) -> SinglyLinkedListNode* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        return self->headNode;
    }
    auto SinglyLinkedList_tail(const SinglyLinkedList* self) -> SinglyLinkedListNode* {
        if (!self || self->size == 0) {
            return nullptr;
        }

        // 遍历到尾部节点，O(N) 复杂度
        auto* targetNode = self->headNode;
        while (targetNode->nextNode) {
            targetNode = targetNode->nextNode;
        }

        return targetNode;
    }

    auto SinglyLinkedList_set(const SinglyLinkedList* self, const usize index, const void* inElement, const usize inElementSize) -> bool {
        if (!self || index >= self->size || inElementSize == 0) {
            return false;
        }

        // 遍历到目标节点
        auto* targetNode = self->headNode;
        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
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
    auto SinglyLinkedList_setSlot(const SinglyLinkedList* self, const usize index, const usize inElementSize) -> void* {
        if (!self || index >= self->size || inElementSize == 0) {
            return nullptr;
        }

        // 遍历到目标节点
        auto* targetNode = self->headNode;
        for (usize i = 0; i < index; ++i) {
            targetNode = targetNode->nextNode;
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

    auto SinglyLinkedList_reverse(SinglyLinkedList* self) -> bool {
        if (!self || self->size <= 1) {
            return false;
        }

        auto* prevNode = (SinglyLinkedListNode*) nullptr;
        auto* currentNode = self->headNode;
        while (currentNode) {
            // 暂存下一个节点
            auto* const nextNode = currentNode->nextNode;

            // 反转当前指针方向
            currentNode->nextNode = prevNode;

            // 将上一节点和当前节点向前移动，继续下一轮反转
            prevNode = currentNode;
            currentNode = nextNode;
        }

        // 更新头节点为上一节点
        self->headNode = prevNode;

        return true;
    }

    auto SinglyLinkedList_begin(const SinglyLinkedList* self) -> SinglyLinkedListNode* {
        if (!self) {
            return nullptr;
        }

        return self->headNode;
    }
    auto SinglyLinkedList_end() -> SinglyLinkedListNode* {
        return nullptr;
    }

    auto SinglyLinkedList_size(const SinglyLinkedList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }

    auto SinglyLinkedList_isEmpty(const SinglyLinkedList* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }
    auto SinglyLinkedList_contains(const SinglyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool {
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

    auto SinglyLinkedListNode_next(const SinglyLinkedListNode* node) -> SinglyLinkedListNode* {
        if (!node) {
            return nullptr;
        }

        return node->nextNode;
    }
    auto SinglyLinkedListNode_data(const SinglyLinkedListNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->data;
    }
}
