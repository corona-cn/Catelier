#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct AdjacencyMatrix AdjacencyMatrix;

    auto AdjacencyMatrix_construct(usize inNodeCount, usize inEdgeSize, bool inIsDirected, const void* inNoEdgeSentinel) -> AdjacencyMatrix*;
    auto AdjacencyMatrix_destruct(AdjacencyMatrix* self) -> bool;

    auto AdjacencyMatrix_copy(const AdjacencyMatrix* self) -> AdjacencyMatrix*;
    auto AdjacencyMatrix_move(AdjacencyMatrix* self) -> AdjacencyMatrix*;

    auto AdjacencyMatrix_setEdge(AdjacencyMatrix* self, usize fromNode, usize toNode, const void* inEdge) -> bool;

    auto AdjacencyMatrix_removeEdge(AdjacencyMatrix* self, usize fromNode, usize toNode) -> bool;
    auto AdjacencyMatrix_clearEdges(AdjacencyMatrix* self) -> bool;

    auto AdjacencyMatrix_getEdge(const AdjacencyMatrix* self, usize fromNode, usize toNode) -> void*;

    auto AdjacencyMatrix_nodeCount(const AdjacencyMatrix* self) -> usize;
    auto AdjacencyMatrix_edgeSize(const AdjacencyMatrix* self) -> usize;
    auto AdjacencyMatrix_edgeCount(const AdjacencyMatrix* self) -> usize;
    auto AdjacencyMatrix_isDirected(const AdjacencyMatrix* self) -> bool;

    auto AdjacencyMatrix_isEmpty(const AdjacencyMatrix* self) -> bool;
    auto AdjacencyMatrix_hasEdge(const AdjacencyMatrix* self, usize fromNode, usize toNode) -> bool;
    auto AdjacencyMatrix_degree(const AdjacencyMatrix* self, usize node) -> usize;
}