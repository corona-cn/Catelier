#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct HuffmanTree HuffmanTree;

    typedef struct HuffmanTreeNode HuffmanTreeNode;

    auto HuffmanTree_construct(usize inKeySize, i32 (*compare)(const void*, const void*)) -> HuffmanTree*;
    auto HuffmanTree_destruct(HuffmanTree* self) -> bool;

    auto HuffmanTree_copy(const HuffmanTree* self) -> HuffmanTree*;
    auto HuffmanTree_move(HuffmanTree* self) -> HuffmanTree*;

    auto HuffmanTree_build(HuffmanTree* self, const void* inKeys, const u64* inWeights, usize count) -> bool;

    auto HuffmanTree_rootNode(const HuffmanTree* self) -> HuffmanTreeNode*;
    auto HuffmanTree_weight(const HuffmanTree* self) -> u64;
    auto HuffmanTree_size(const HuffmanTree* self) -> usize;

    auto HuffmanTree_isEmpty(const HuffmanTree* self) -> bool;

    auto HuffmanTreeNode_key(const HuffmanTreeNode* node) -> void*;
    auto HuffmanTreeNode_weight(const HuffmanTreeNode* node) -> u64;
    auto HuffmanTreeNode_left(const HuffmanTreeNode* node) -> HuffmanTreeNode*;
    auto HuffmanTreeNode_right(const HuffmanTreeNode* node) -> HuffmanTreeNode*;
    auto HuffmanTreeNode_isLeaf(const HuffmanTreeNode* node) -> bool;
}