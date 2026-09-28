#include "AdjacencyList.hpp"

#include <cstdlib>
#include <cstring>

#include "ArrayList.hpp"

namespace Catelier::src::foundation {
    typedef struct AdjacencyList {
        ArrayList** neighborLists;
        usize edgeSize;
        usize entrySize;
        usize nodeCount;
        usize edgeCount;
        bool isDirected;
    } AdjacencyList;

    namespace {
        auto findNeighborIndex(const ArrayList* list, const usize toNode, usize* indexOut) -> bool {
            if (!list) {
                return false;
            }

            const usize count = ArrayList_size(list);
            for (usize i = 0; i < count; ++i) {
                const auto* const entry = (const u8*) ArrayList_get(list, i);
                const usize foundTo = *(const usize*) entry;

                if (foundTo == toNode) {
                    *indexOut = i;
                    return true;
                }
            }

            return false;
        }
        auto alignEntrySize(const usize rawSize) -> usize {
            constexpr usize unit = sizeof(usize);
            if (rawSize % unit == 0) {
                return rawSize;
            }

            return rawSize + (unit - rawSize % unit);
        }
    }

    auto AdjacencyList_construct(const usize inNodeCount, const usize inEdgeSize, const bool inIsDirected) -> AdjacencyList* {
        if (inNodeCount == 0 || inEdgeSize == 0) {
            return nullptr;
        }

        auto* const self = (AdjacencyList*) malloc(sizeof(AdjacencyList));
        if (!self) {
            return nullptr;
        }

        self->neighborLists = (ArrayList**) malloc(sizeof(ArrayList*) * inNodeCount);
        if (!self->neighborLists) {
            free(self);
            return nullptr;
        }

        for (usize i = 0; i < inNodeCount; ++i) {
            *(self->neighborLists + i) = nullptr;
        }

        self->edgeSize = inEdgeSize;
        self->entrySize = alignEntrySize(sizeof(usize) + inEdgeSize);
        self->nodeCount = inNodeCount;
        self->edgeCount = 0;
        self->isDirected = inIsDirected;

        return self;
    }
    auto AdjacencyList_destruct(AdjacencyList* self) -> bool {
        if (!self) {
            return false;
        }

        // 释放每个已创建的邻居列表
        for (usize i = 0; i < self->nodeCount; ++i) {
            auto* const list = *(self->neighborLists + i);
            if (list) {
                ArrayList_destruct(list);
            }
        }

        free(self->neighborLists);
        free(self);

        return true;
    }

    auto AdjacencyList_copy(const AdjacencyList* self) -> AdjacencyList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = AdjacencyList_construct(self->nodeCount, self->edgeSize, self->isDirected);
        if (!newSelf) {
            return nullptr;
        }

        // 逐个深拷贝已存在的邻居列表
        for (usize i = 0; i < self->nodeCount; ++i) {
            const auto* const srcList = *(self->neighborLists + i);
            if (srcList) {
                auto* const copiedList = ArrayList_copy(srcList);
                if (!copiedList) {
                    AdjacencyList_destruct(newSelf);
                    return nullptr;
                }

                *(newSelf->neighborLists + i) = copiedList;
            }
        }

        newSelf->edgeCount = self->edgeCount;

        return newSelf;
    }
    auto AdjacencyList_move(AdjacencyList* self) -> AdjacencyList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (AdjacencyList*) malloc(sizeof(AdjacencyList));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->neighborLists = self->neighborLists;
        newSelf->edgeSize = self->edgeSize;
        newSelf->entrySize = self->entrySize;
        newSelf->nodeCount = self->nodeCount;
        newSelf->edgeCount = self->edgeCount;
        newSelf->isDirected = self->isDirected;

        self->neighborLists = nullptr;
        self->edgeSize = 0;
        self->entrySize = 0;
        self->nodeCount = 0;
        self->edgeCount = 0;
        self->isDirected = false;

        return newSelf;
    }

    auto AdjacencyList_addEdge(AdjacencyList* self, const usize fromNode, const usize toNode, const void* inEdge) -> bool {
        if (!self || !inEdge || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        // 惰性创建 fromNode 的邻居列表
        if (!*(self->neighborLists + fromNode)) {
            *(self->neighborLists + fromNode) = ArrayList_construct(0, self->entrySize);
            if (!*(self->neighborLists + fromNode)) {
                return false;
            }
        }

        auto* const fromList = *(self->neighborLists + fromNode);

        // 检查边是否已存在，已存在则只更新边数据
        usize existingIndex = 0;
        if (findNeighborIndex(fromList, toNode, &existingIndex)) {
            auto* const entry = (u8*) ArrayList_get(fromList, existingIndex);
            memcpy(entry + sizeof(usize), inEdge, self->edgeSize);
            return true;
        }

        // 在列表中追加新条目
        auto* const slot = (u8*) ArrayList_pushSlot(fromList);
        if (!slot) {
            return false;
        }

        *(usize*) slot = toNode;
        memcpy(slot + sizeof(usize), inEdge, self->edgeSize);

        self->edgeCount++;

        // 无向图：在 toNode 的列表里追加对称条目
        if (!self->isDirected && fromNode != toNode) {
            if (!*(self->neighborLists + toNode)) {
                *(self->neighborLists + toNode) = ArrayList_construct(0, self->entrySize);
                if (!*(self->neighborLists + toNode)) {
                    return false;
                }
            }

            auto* const toList = *(self->neighborLists + toNode);

            auto* const symSlot = (u8*) ArrayList_pushSlot(toList);
            if (!symSlot) {
                return false;
            }

            *(usize*) symSlot = fromNode;
            memcpy(symSlot + sizeof(usize), inEdge, self->edgeSize);
        }

        return true;
    }

    auto AdjacencyList_removeEdge(AdjacencyList* self, const usize fromNode, const usize toNode) -> bool {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        auto* const fromList = *(self->neighborLists + fromNode);
        if (!fromList) {
            return false;
        }

        usize index = 0;
        if (!findNeighborIndex(fromList, toNode, &index)) {
            return false;
        }

        ArrayList_removeAt(fromList, index);

        self->edgeCount--;

        // 无向图：同时移除对称条目
        if (!self->isDirected && fromNode != toNode) {
            auto* const toList = *(self->neighborLists + toNode);
            if (toList) {
                usize symIndex = 0;
                if (findNeighborIndex(toList, fromNode, &symIndex)) {
                    ArrayList_removeAt(toList, symIndex);
                }
            }
        }

        return true;
    }
    auto AdjacencyList_clearEdges(AdjacencyList* self) -> bool {
        if (!self || self->edgeCount == 0) {
            return false;
        }

        // 保留邻居列表的容量，只清空其内容
        for (usize i = 0; i < self->nodeCount; ++i) {
            auto* const list = *(self->neighborLists + i);
            if (list) {
                ArrayList_clear(list);
            }
        }

        self->edgeCount = 0;

        return true;
    }

    auto AdjacencyList_getEdge(const AdjacencyList* self, const usize fromNode, const usize toNode) -> void* {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return nullptr;
        }

        auto* const fromList = *(self->neighborLists + fromNode);
        if (!fromList) {
            return nullptr;
        }

        usize index = 0;
        if (!findNeighborIndex(fromList, toNode, &index)) {
            return nullptr;
        }

        u8* const entry = (u8*) ArrayList_get(fromList, index);
        return entry + sizeof(usize);
    }

    auto AdjacencyList_neighborCount(const AdjacencyList* self, const usize node) -> usize {
        if (!self || node >= self->nodeCount) {
            return 0;
        }

        const auto* const list = *(self->neighborLists + node);
        if (!list) {
            return 0;
        }

        return ArrayList_size(list);
    }
    auto AdjacencyList_neighborNodeAt(const AdjacencyList* self, const usize node, const usize index) -> usize {
        if (!self || node >= self->nodeCount) {
            return self ? self->nodeCount : 0;
        }

        const auto* const list = *(self->neighborLists + node);
        if (!list || index >= ArrayList_size(list)) {
            return self->nodeCount;
        }

        const auto* const entry = (const u8*) ArrayList_get(list, index);
        return *(const usize*) entry;
    }
    auto AdjacencyList_neighborEdgeAt(const AdjacencyList* self, const usize node, const usize index) -> void* {
        if (!self || node >= self->nodeCount) {
            return nullptr;
        }

        auto* const list = *(self->neighborLists + node);
        if (!list || index >= ArrayList_size(list)) {
            return nullptr;
        }

        auto* const entry = (u8*) ArrayList_get(list, index);
        return entry + sizeof(usize);
    }

    auto AdjacencyList_nodeCount(const AdjacencyList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->nodeCount;
    }
    auto AdjacencyList_edgeSize(const AdjacencyList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->edgeSize;
    }
    auto AdjacencyList_edgeCount(const AdjacencyList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->edgeCount;
    }
    auto AdjacencyList_isDirected(const AdjacencyList* self) -> bool {
        if (!self) {
            return false;
        }

        return self->isDirected;
    }

    auto AdjacencyList_isEmpty(const AdjacencyList* self) -> bool {
        if (!self) {
            return true;
        }

        return self->edgeCount == 0;
    }
    auto AdjacencyList_hasEdge(const AdjacencyList* self, const usize fromNode, const usize toNode) -> bool {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        const auto* const fromList = *(self->neighborLists + fromNode);
        if (!fromList) {
            return false;
        }

        usize index = 0;
        return findNeighborIndex(fromList, toNode, &index);
    }
    auto AdjacencyList_degree(const AdjacencyList* self, const usize node) -> usize {
        return AdjacencyList_neighborCount(self, node);
    }
}