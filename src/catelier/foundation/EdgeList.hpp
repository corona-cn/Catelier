#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct EdgeList EdgeList;

    auto EdgeList_construct(usize inNodeCount, usize inEdgeSize, bool inIsDirected) -> EdgeList*;
    auto EdgeList_destruct(EdgeList* self) -> bool;

    auto EdgeList_copy(const EdgeList* self) -> EdgeList*;
    auto EdgeList_move(EdgeList* self) -> EdgeList*;

    auto EdgeList_addEdge(EdgeList* self, usize fromNode, usize toNode, const void* inEdge) -> bool;

    auto EdgeList_removeEdge(EdgeList* self, usize fromNode, usize toNode) -> bool;
    auto EdgeList_removeAt(EdgeList* self, usize index) -> bool;
    auto EdgeList_clearEdges(EdgeList* self) -> bool;

    auto EdgeList_getEdge(const EdgeList* self, usize fromNode, usize toNode) -> void*;

    auto EdgeList_nodeFrom(const EdgeList* self, usize index) -> usize;
    auto EdgeList_nodeTo(const EdgeList* self, usize index) -> usize;
    auto EdgeList_edgeAt(const EdgeList* self, usize index) -> void*;

    auto EdgeList_reserve(EdgeList* self, usize newCapacity) -> bool;
    auto EdgeList_shrinkToFit(EdgeList* self) -> bool;

    auto EdgeList_nodeCount(const EdgeList* self) -> usize;
    auto EdgeList_edgeSize(const EdgeList* self) -> usize;
    auto EdgeList_edgeCount(const EdgeList* self) -> usize;
    auto EdgeList_capacity(const EdgeList* self) -> usize;
    auto EdgeList_isDirected(const EdgeList* self) -> bool;

    auto EdgeList_isEmpty(const EdgeList* self) -> bool;
    auto EdgeList_hasEdge(const EdgeList* self, usize fromNode, usize toNode) -> bool;
}