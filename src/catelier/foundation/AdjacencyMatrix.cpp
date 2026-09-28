#include "AdjacencyMatrix.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct AdjacencyMatrix {
        u8* edgeBytes;
        u8* noEdgeSentinel;
        usize edgeSize;
        usize nodeCount;
        usize edgeCount;
        bool isDirected;
    } AdjacencyMatrix;

    namespace {
        auto edgeSlot(const AdjacencyMatrix* self, const usize fromNode, const usize toNode) -> u8* {
            return self->edgeBytes + (fromNode * self->nodeCount + toNode) * self->edgeSize;
        }
    }

    auto AdjacencyMatrix_construct(const usize inNodeCount, const usize inEdgeSize, const bool inIsDirected, const void* inNoEdgeSentinel) -> AdjacencyMatrix* {
        if (inNodeCount == 0 || inEdgeSize == 0 || !inNoEdgeSentinel) {
            return nullptr;
        }

        auto* const self = (AdjacencyMatrix*) malloc(sizeof(AdjacencyMatrix));
        if (!self) {
            return nullptr;
        }

        // 分配矩阵数据缓冲区，全部填充为哨兵值
        const usize matrixBytes = inNodeCount * inNodeCount * inEdgeSize;
        self->edgeBytes = (u8*) malloc(matrixBytes);
        if (!self->edgeBytes) {
            free(self);
            return nullptr;
        }

        // 分配哨兵值副本
        self->noEdgeSentinel = (u8*) malloc(inEdgeSize);
        if (!self->noEdgeSentinel) {
            free(self->edgeBytes);
            free(self);
            return nullptr;
        }
        memcpy(self->noEdgeSentinel, inNoEdgeSentinel, inEdgeSize);

        // 初始化矩阵：所有位置都是"无边"
        for (usize i = 0; i < inNodeCount * inNodeCount; ++i) {
            memcpy(self->edgeBytes + i * inEdgeSize, inNoEdgeSentinel, inEdgeSize);
        }

        self->edgeSize = inEdgeSize;
        self->nodeCount = inNodeCount;
        self->edgeCount = 0;
        self->isDirected = inIsDirected;

        return self;
    }
    auto AdjacencyMatrix_destruct(AdjacencyMatrix* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->noEdgeSentinel);
        free(self->edgeBytes);

        free(self);

        return true;
    }

    auto AdjacencyMatrix_copy(const AdjacencyMatrix* self) -> AdjacencyMatrix* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = AdjacencyMatrix_construct(self->nodeCount, self->edgeSize, self->isDirected, self->noEdgeSentinel);
        if (!newSelf) {
            return nullptr;
        }

        // 直接复制整块矩阵数据
        const usize matrixBytes = self->nodeCount * self->nodeCount * self->edgeSize;
        memcpy(newSelf->edgeBytes, self->edgeBytes, matrixBytes);

        newSelf->edgeCount = self->edgeCount;

        return newSelf;
    }
    auto AdjacencyMatrix_move(AdjacencyMatrix* self) -> AdjacencyMatrix* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (AdjacencyMatrix*) malloc(sizeof(AdjacencyMatrix));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->edgeBytes = self->edgeBytes;
        newSelf->noEdgeSentinel = self->noEdgeSentinel;
        newSelf->edgeSize = self->edgeSize;
        newSelf->nodeCount = self->nodeCount;
        newSelf->edgeCount = self->edgeCount;
        newSelf->isDirected = self->isDirected;

        self->edgeBytes = nullptr;
        self->noEdgeSentinel = nullptr;
        self->edgeSize = 0;
        self->nodeCount = 0;
        self->edgeCount = 0;
        self->isDirected = false;

        return newSelf;
    }

    auto AdjacencyMatrix_setEdge(AdjacencyMatrix* self, const usize fromNode, const usize toNode, const void* inEdge) -> bool {
        if (!self || !inEdge || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        // 判断该位置原本是否有边
        const u8* const slot = edgeSlot(self, fromNode, toNode);
        const bool hadEdge = memcmp(slot, self->noEdgeSentinel, self->edgeSize) != 0;

        // 写入新的边数据
        memcpy(edgeSlot(self, fromNode, toNode), inEdge, self->edgeSize);

        // 无向图同时设置对称位置
        if (!self->isDirected && fromNode != toNode) {
            memcpy(edgeSlot(self, toNode, fromNode), inEdge, self->edgeSize);
        }

        // 更新边数量
        if (!hadEdge) {
            self->edgeCount++;
        }

        return true;
    }

    auto AdjacencyMatrix_removeEdge(AdjacencyMatrix* self, const usize fromNode, const usize toNode) -> bool {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        // 判断该位置原本是否有边
        const u8* const slot = edgeSlot(self, fromNode, toNode);
        const bool hadEdge = memcmp(slot, self->noEdgeSentinel, self->edgeSize) != 0;
        if (!hadEdge) {
            return false;
        }

        // 写回哨兵值
        memcpy(edgeSlot(self, fromNode, toNode), self->noEdgeSentinel, self->edgeSize);

        // 无向图同时清除对称位置
        if (!self->isDirected && fromNode != toNode) {
            memcpy(edgeSlot(self, toNode, fromNode), self->noEdgeSentinel, self->edgeSize);
        }

        self->edgeCount--;

        return true;
    }
    auto AdjacencyMatrix_clearEdges(AdjacencyMatrix* self) -> bool {
        if (!self || self->edgeCount == 0) {
            return false;
        }

        // 把整个矩阵重置为哨兵值
        for (usize i = 0; i < self->nodeCount * self->nodeCount; ++i) {
            memcpy(self->edgeBytes + i * self->edgeSize, self->noEdgeSentinel, self->edgeSize);
        }

        self->edgeCount = 0;

        return true;
    }

    auto AdjacencyMatrix_getEdge(const AdjacencyMatrix* self, const usize fromNode, const usize toNode) -> void* {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return nullptr;
        }

        if (memcmp(edgeSlot(self, fromNode, toNode), self->noEdgeSentinel, self->edgeSize) == 0) {
            return nullptr;
        }

        return edgeSlot(self, fromNode, toNode);
    }

    auto AdjacencyMatrix_nodeCount(const AdjacencyMatrix* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->nodeCount;
    }
    auto AdjacencyMatrix_edgeSize(const AdjacencyMatrix* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->edgeSize;
    }
    auto AdjacencyMatrix_edgeCount(const AdjacencyMatrix* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->edgeCount;
    }
    auto AdjacencyMatrix_isDirected(const AdjacencyMatrix* self) -> bool {
        if (!self) {
            return false;
        }

        return self->isDirected;
    }

    auto AdjacencyMatrix_isEmpty(const AdjacencyMatrix* self) -> bool {
        if (!self) {
            return true;
        }

        return self->edgeCount == 0;
    }
    auto AdjacencyMatrix_hasEdge(const AdjacencyMatrix* self, const usize fromNode, const usize toNode) -> bool {
        if (!self || fromNode >= self->nodeCount || toNode >= self->nodeCount) {
            return false;
        }

        return memcmp(edgeSlot(self, fromNode, toNode), self->noEdgeSentinel, self->edgeSize) != 0;
    }
    auto AdjacencyMatrix_degree(const AdjacencyMatrix* self, const usize node) -> usize {
        if (!self || node >= self->nodeCount) {
            return 0;
        }

        usize degree = 0;

        for (usize i = 0; i < self->nodeCount; ++i) {
            if (memcmp(edgeSlot(self, node, i), self->noEdgeSentinel, self->edgeSize) != 0) {
                degree++;
            }
        }

        return degree;
    }
}