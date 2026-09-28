#include "UnionFind.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct UnionFind {
        usize* parentNodes;
        usize* rankValues;
        usize capacity;
        usize setCount;
    } UnionFind;

    auto UnionFind_construct(const usize inCapacity) -> UnionFind* {
        if (inCapacity == 0) {
            return nullptr;
        }

        auto* const self = (UnionFind*) malloc(sizeof(UnionFind));
        if (!self) {
            return nullptr;
        }

        // 分配父指针数组，每个元素初始指向自己
        self->parentNodes = (usize*) malloc(sizeof(usize) * inCapacity);
        if (!self->parentNodes) {
            free(self);
            return nullptr;
        }

        // 分配秩数组，每个元素初始秩为 0
        self->rankValues = (usize*) malloc(sizeof(usize) * inCapacity);
        if (!self->rankValues) {
            free(self->parentNodes);
            free(self);
            return nullptr;
        }

        for (usize i = 0; i < inCapacity; ++i) {
            *(self->parentNodes + i) = i;
            *(self->rankValues + i) = 0;
        }

        self->capacity = inCapacity;
        self->setCount = inCapacity;

        return self;
    }
    auto UnionFind_destruct(UnionFind* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->rankValues);
        free(self->parentNodes);

        free(self);

        return true;
    }

    auto UnionFind_copy(const UnionFind* self) -> UnionFind* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = UnionFind_construct(self->capacity);
        if (!newSelf) {
            return nullptr;
        }

        memcpy(newSelf->parentNodes, self->parentNodes, sizeof(usize) * self->capacity);
        memcpy(newSelf->rankValues, self->rankValues, sizeof(usize) * self->capacity);

        newSelf->setCount = self->setCount;

        return newSelf;
    }
    auto UnionFind_move(UnionFind* self) -> UnionFind* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (UnionFind*) malloc(sizeof(UnionFind));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->parentNodes = self->parentNodes;
        newSelf->rankValues = self->rankValues;
        newSelf->capacity = self->capacity;
        newSelf->setCount = self->setCount;

        self->parentNodes = nullptr;
        self->rankValues = nullptr;
        self->capacity = 0;
        self->setCount = 0;

        return newSelf;
    }

    auto UnionFind_find(UnionFind* self, const usize element) -> usize {
        if (!self || element >= self->capacity) {
            return element;
        }

        // 沿着父指针一路向上，找到根
        usize rootNode = element;
        while (*(self->parentNodes + rootNode) != rootNode) {
            rootNode = *(self->parentNodes + rootNode);
        }

        // 路径压缩，把路径上所有节点的父指针直接指向根
        usize currentNode = element;
        while (*(self->parentNodes + currentNode) != rootNode) {
            const usize nextNode = *(self->parentNodes + currentNode);
            *(self->parentNodes + currentNode) = rootNode;
            currentNode = nextNode;
        }

        return rootNode;
    }
    auto UnionFind_union(UnionFind* self, const usize elementA, const usize elementB) -> bool {
        if (!self || elementA >= self->capacity || elementB >= self->capacity) {
            return false;
        }

        const usize rootA = UnionFind_find(self, elementA);
        const usize rootB = UnionFind_find(self, elementB);

        if (rootA == rootB) {
            return false;
        }

        // 按秩合并：把秩小的树挂到秩大的树下
        const usize rankA = *(self->rankValues + rootA);
        const usize rankB = *(self->rankValues + rootB);

        if (rankA < rankB) {
            *(self->parentNodes + rootA) = rootB;
        } else if (rankA > rankB) {
            *(self->parentNodes + rootB) = rootA;
        } else {
            *(self->parentNodes + rootB) = rootA;
            *(self->rankValues + rootA) = rankA + 1;
        }

        self->setCount--;

        return true;
    }

    auto UnionFind_connected(UnionFind* self, const usize elementA, const usize elementB) -> bool {
        if (!self || elementA >= self->capacity || elementB >= self->capacity) {
            return false;
        }

        return UnionFind_find(self, elementA) == UnionFind_find(self, elementB);
    }

    auto UnionFind_capacity(const UnionFind* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->capacity;
    }
    auto UnionFind_setCount(const UnionFind* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->setCount;
    }

    auto UnionFind_isEmpty(const UnionFind* self) -> bool {
        if (!self) {
            return true;
        }

        return self->setCount == 0;
    }
}