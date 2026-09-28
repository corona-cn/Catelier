#include "EdgeList.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
    }

    typedef struct EdgeList {
        u8* edgeBytes;
        usize edgeSize;
        usize entrySize;
        usize nodeCount;
        usize edgeCount;
        usize capacity;
        bool isDirected;
    } EdgeList;

    namespace {
        auto alignEntrySize(const usize rawSize) -> usize {
            const usize unit = sizeof(usize);
            if (rawSize % unit == 0) {
                return rawSize;
            }

            return rawSize + (unit - rawSize % unit);
        }
        auto entryAt(const EdgeList* self, const usize index) -> u8* {
            return self->edgeBytes + index * self->entrySize;
        }
        auto findEdgeIndex(const EdgeList* self, const usize fromNode, const usize toNode, usize* indexOut) -> bool {
            for (usize i = 0; i < self->edgeCount; ++i) {
                const u8* const entry = entryAt(self, i);
                const usize foundFrom = *(const usize*) entry;
                const usize foundTo = *(const usize*) (entry + sizeof(usize));

                if (foundFrom == fromNode && foundTo == toNode) {
                    *indexOut = i;
                    return true;
                }

                if (!self->isDirected && foundFrom == toNode && foundTo == fromNode) {
                    *indexOut = i;
                    return true;
                }
            }

            return false;
        }
    }

    auto EdgeList_construct(const usize inNodeCount, const usize inEdgeSize, const bool inIsDirected) -> EdgeList* {
        if (inNodeCount == 0 || inEdgeSize == 0) {
            return nullptr;
        }

        auto* const self = (EdgeList*) malloc(sizeof(EdgeList));
        if (!self) {
            return nullptr;
        }

        const usize entrySize = alignEntrySize(2 * sizeof(usize) + inEdgeSize);

        self->edgeBytes = (u8*) malloc(entrySize * DEFAULT_CAPACITY);
        if (!self->edgeBytes) {
            free(self);
            return nullptr;
        }

        self->edgeSize = inEdgeSize;
        self->entrySize = entrySize;
        self->nodeCount = inNodeCount;
        self->edgeCount = 0;
        self->capacity = DEFAULT_CAPACITY;
        self->isDirected = inIsDirected;

        return self;
    }
    auto EdgeList_destruct(EdgeList* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->edgeBytes);

        free(self);

        return true;
    }

    auto EdgeList_copy(const EdgeList* self) -> EdgeList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = EdgeList_construct(self->nodeCount, self->edgeSize, self->isDirected);
        if (!newSelf) {
            return nullptr;
        }

        // 容量不足时先扩容
        if (self->capacity > newSelf->capacity) {
            if (!EdgeList_reserve(newSelf, self->capacity)) {
                EdgeList_destruct(newSelf);
                return nullptr;
            }
        }

        // 复制所有边
        memcpy(newSelf->edgeBytes, self->edgeBytes, self->entrySize * self->edgeCount);
        newSelf->edgeCount = self->edgeCount;

        return newSelf;
    }
    auto EdgeList_move(EdgeList* self) -> EdgeList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (EdgeList*) malloc(sizeof(EdgeList));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->edgeBytes = self->edgeBytes;
        newSelf->edgeSize = self->edgeSize;
        newSelf->entrySize = self->entrySize;
        newSelf->nodeCount = self->nodeCount;
        newSelf->edgeCount = self->edgeCount;
        newSelf->capacity = self->capacity;
        newSelf->isDirected = self->isDirected;

        self->edgeBytes = nullptr;
        self->edgeSize = 0;
        self->entrySize = 0;
        self->nodeCount = 0;
        self->edgeCount = 0;
        self->capacity = 0;
        self->isDirected = false;

        return newSelf;
    }

    auto EdgeList_addEdge(EdgeList* self, const usize fromNode, const usize toNode, const void* inEdge) -> bool {
        if (!self || !inEdge || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        // 容量不足时翻倍扩容
        if (self->edgeCount >= self->capacity) {
            const usize newCapacity = self->capacity * 2;
            if (!EdgeList_reserve(self, newCapacity)) {
                return false;
            }
        }

        // 写入新的边
        u8* const entry = entryAt(self, self->edgeCount);
        *(usize*) entry = fromNode;
        *(usize*) (entry + sizeof(usize)) = toNode;
        memcpy(entry + 2 * sizeof(usize), inEdge, self->edgeSize);

        self->edgeCount++;

        return true;
    }

    auto EdgeList_removeEdge(EdgeList* self, const usize fromNode, const usize toNode) -> bool {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        usize index = 0;
        if (!findEdgeIndex(self, fromNode, toNode, &index)) {
            return false;
        }

        return EdgeList_removeAt(self, index);
    }
    auto EdgeList_removeAt(EdgeList* self, const usize index) -> bool {
        if (!self || index >= self->edgeCount) {
            return false;
        }

        // 把末尾的边移到被删除的位置
        if (index < self->edgeCount - 1) {
            memcpy(entryAt(self, index), entryAt(self, self->edgeCount - 1), self->entrySize);
        }

        self->edgeCount--;

        return true;
    }
    auto EdgeList_clearEdges(EdgeList* self) -> bool {
        if (!self || self->edgeCount == 0) {
            return false;
        }

        self->edgeCount = 0;

        return true;
    }

    auto EdgeList_getEdge(const EdgeList* self, const usize fromNode, const usize toNode) -> void* {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return nullptr;
        }

        usize index = 0;
        if (!findEdgeIndex(self, fromNode, toNode, &index)) {
            return nullptr;
        }

        return entryAt(self, index) + 2 * sizeof(usize);
    }

    auto EdgeList_nodeFrom(const EdgeList* self, const usize index) -> usize {
        if (!self || index >= self->edgeCount) {
            return self ? self->nodeCount : 0;
        }

        return *(const usize*) entryAt(self, index);
    }
    auto EdgeList_nodeTo(const EdgeList* self, const usize index) -> usize {
        if (!self || index >= self->edgeCount) {
            return self ? self->nodeCount : 0;
        }

        return *(const usize*) (entryAt(self, index) + sizeof(usize));
    }
    auto EdgeList_edgeAt(const EdgeList* self, const usize index) -> void* {
        if (!self || index >= self->edgeCount) {
            return nullptr;
        }

        return entryAt(self, index) + 2 * sizeof(usize);
    }

    auto EdgeList_reserve(EdgeList* self, const usize newCapacity) -> bool {
        if (!self || newCapacity < DEFAULT_CAPACITY) {
            return false;
        }

        if (newCapacity <= self->capacity) {
            return true;
        }

        auto* const newData = (u8*) realloc(self->edgeBytes, self->entrySize * newCapacity);
        if (!newData) {
            return false;
        }

        self->edgeBytes = newData;
        self->capacity = newCapacity;

        return true;
    }
    auto EdgeList_shrinkToFit(EdgeList* self) -> bool {
        if (!self) {
            return false;
        }

        if (self->edgeCount == 0) {
            free(self->edgeBytes);
            self->edgeBytes = nullptr;
            self->capacity = 0;

            return true;
        }

        if (self->edgeCount == self->capacity) {
            return false;
        }

        auto* const newData = (u8*) realloc(self->edgeBytes, self->entrySize * self->edgeCount);
        if (newData) {
            self->edgeBytes = newData;
            self->capacity = self->edgeCount;
        }

        return true;
    }

    auto EdgeList_nodeCount(const EdgeList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->nodeCount;
    }
    auto EdgeList_edgeSize(const EdgeList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->edgeSize;
    }
    auto EdgeList_edgeCount(const EdgeList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->edgeCount;
    }
    auto EdgeList_capacity(const EdgeList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->capacity;
    }
    auto EdgeList_isDirected(const EdgeList* self) -> bool {
        if (!self) {
            return false;
        }

        return self->isDirected;
    }

    auto EdgeList_isEmpty(const EdgeList* self) -> bool {
        if (!self) {
            return true;
        }

        return self->edgeCount == 0;
    }
    auto EdgeList_hasEdge(const EdgeList* self, const usize fromNode, const usize toNode) -> bool {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        usize index = 0;
        return findEdgeIndex(self, fromNode, toNode, &index);
    }
}