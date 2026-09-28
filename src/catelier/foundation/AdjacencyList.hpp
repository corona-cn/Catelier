#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct AdjacencyList AdjacencyList;

    auto AdjacencyList_construct(usize inNodeCount, usize inEdgeSize, bool inIsDirected) -> AdjacencyList*;
    auto AdjacencyList_destruct(AdjacencyList* self) -> bool;

    auto AdjacencyList_copy(const AdjacencyList* self) -> AdjacencyList*;
    auto AdjacencyList_move(AdjacencyList* self) -> AdjacencyList*;

    auto AdjacencyList_addEdge(AdjacencyList* self, usize fromNode, usize toNode, const void* inEdge) -> bool;

    auto AdjacencyList_removeEdge(AdjacencyList* self, usize fromNode, usize toNode) -> bool;
    auto AdjacencyList_clearEdges(AdjacencyList* self) -> bool;

    auto AdjacencyList_getEdge(const AdjacencyList* self, usize fromNode, usize toNode) -> void*;

    auto AdjacencyList_neighborCount(const AdjacencyList* self, usize node) -> usize;
    auto AdjacencyList_neighborNodeAt(const AdjacencyList* self, usize node, usize index) -> usize;
    auto AdjacencyList_neighborEdgeAt(const AdjacencyList* self, usize node, usize index) -> void*;

    auto AdjacencyList_nodeCount(const AdjacencyList* self) -> usize;
    auto AdjacencyList_edgeSize(const AdjacencyList* self) -> usize;
    auto AdjacencyList_edgeCount(const AdjacencyList* self) -> usize;
    auto AdjacencyList_isDirected(const AdjacencyList* self) -> bool;

    auto AdjacencyList_isEmpty(const AdjacencyList* self) -> bool;
    auto AdjacencyList_hasEdge(const AdjacencyList* self, usize fromNode, usize toNode) -> bool;
    auto AdjacencyList_degree(const AdjacencyList* self, usize node) -> usize;
}