#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BinaryTree BinaryTree;

    typedef struct BinaryTreeNode BinaryTreeNode;

    auto BinaryTree_construct(usize inOrder, usize inKeySize, usize inValueSize, i32 (*compare)(const void*, const void*)) -> BinaryTree*;
    auto BinaryTree_destruct(BinaryTree* self) -> bool;

    auto BinaryTree_copy(const BinaryTree* self) -> BinaryTree*;
    auto BinaryTree_move(BinaryTree* self) -> BinaryTree*;

    auto BinaryTree_insert(BinaryTree* self, const void* inKey, const void* inValue) -> bool;
    auto BinaryTree_insertSlot(BinaryTree* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool;

    auto BinaryTree_remove(BinaryTree* self, const void* inKey) -> bool;
    auto BinaryTree_removeSlot(BinaryTree* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto BinaryTree_clear(BinaryTree* self) -> bool;

    auto BinaryTree_find(const BinaryTree* self, const void* inKey) -> void*;
    auto BinaryTree_findSlot(const BinaryTree* self, const void* inKey, BinaryTreeNode** nodeOut, usize* indexOut) -> bool;
    auto BinaryTree_findSuccessorSlot(const BinaryTree* self, const void* inKey, BinaryTreeNode** nodeOut, usize* indexOut) -> bool;
    auto BinaryTree_findPredecessorSlot(const BinaryTree* self, const void* inKey, BinaryTreeNode** nodeOut, usize* indexOut) -> bool;

    auto BinaryTree_rootNode(const BinaryTree* self) -> BinaryTreeNode*;
    auto BinaryTree_minSlot(const BinaryTree* self, BinaryTreeNode** nodeOut, usize* indexOut) -> bool;
    auto BinaryTree_maxSlot(const BinaryTree* self, BinaryTreeNode** nodeOut, usize* indexOut) -> bool;

    auto BinaryTree_order(const BinaryTree* self) -> usize;
    auto BinaryTree_keySize(const BinaryTree* self) -> usize;
    auto BinaryTree_valueSize(const BinaryTree* self) -> usize;
    auto BinaryTree_size(const BinaryTree* self) -> usize;

    auto BinaryTree_isEmpty(const BinaryTree* self) -> bool;
    auto BinaryTree_contains(const BinaryTree* self, const void* inKey) -> bool;

    auto BinaryTreeNode_key(const BinaryTreeNode* node, usize index, usize inKeySize) -> void*;
    auto BinaryTreeNode_value(const BinaryTreeNode* node, usize index, usize inValueSize) -> void*;
    auto BinaryTreeNode_child(const BinaryTreeNode* node, usize index) -> BinaryTreeNode*;
    auto BinaryTreeNode_keyCount(const BinaryTreeNode* node) -> usize;
    auto BinaryTreeNode_isLeaf(const BinaryTreeNode* node) -> bool;
}