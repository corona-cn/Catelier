#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct UnionFind UnionFind;

    auto UnionFind_construct(usize inCapacity) -> UnionFind*;
    auto UnionFind_destruct(UnionFind* self) -> bool;

    auto UnionFind_copy(const UnionFind* self) -> UnionFind*;
    auto UnionFind_move(UnionFind* self) -> UnionFind*;

    auto UnionFind_find(UnionFind* self, usize element) -> usize;
    auto UnionFind_union(UnionFind* self, usize elementA, usize elementB) -> bool;

    auto UnionFind_connected(UnionFind* self, usize elementA, usize elementB) -> bool;

    auto UnionFind_capacity(const UnionFind* self) -> usize;
    auto UnionFind_setCount(const UnionFind* self) -> usize;

    auto UnionFind_isEmpty(const UnionFind* self) -> bool;
}