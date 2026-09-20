#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BinarySearchTree BinarySearchTree;

    typedef struct BinarySearchTreeNode BinarySearchTreeNode;

    auto BinarySearchTree_construct(usize inKeySize, usize inValueSize, i32 (*compare)(const void*, const void*)) -> BinarySearchTree*;
    auto BinarySearchTree_destruct(BinarySearchTree* self) -> bool;

    auto BinarySearchTree_copy(const BinarySearchTree* self) -> BinarySearchTree*;
    auto BinarySearchTree_move(BinarySearchTree* self) -> BinarySearchTree*;

    auto BinarySearchTree_insert(BinarySearchTree* self, const void* inKey, const void* inValue) -> bool;
    auto BinarySearchTree_insertSlot(BinarySearchTree* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool;

    auto BinarySearchTree_remove(BinarySearchTree* self, const void* inKey) -> bool;
    auto BinarySearchTree_removeSlot(BinarySearchTree* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto BinarySearchTree_clear(BinarySearchTree* self) -> bool;

    auto BinarySearchTree_find(const BinarySearchTree* self, const void* inKey) -> void*;
    auto BinarySearchTree_findNode(const BinarySearchTree* self, const void* inKey) -> BinarySearchTreeNode*;
    auto BinarySearchTree_findSuccessorNode(const BinarySearchTree* self, const void* inKey) -> BinarySearchTreeNode*;
    auto BinarySearchTree_findPredecessorNode(const BinarySearchTree* self, const void* inKey) -> BinarySearchTreeNode*;

    auto BinarySearchTree_rootNode(const BinarySearchTree* self) -> BinarySearchTreeNode*;
    auto BinarySearchTree_minNode(const BinarySearchTree* self) -> BinarySearchTreeNode*;
    auto BinarySearchTree_maxNode(const BinarySearchTree* self) -> BinarySearchTreeNode*;

    auto BinarySearchTree_keySize(const BinarySearchTree* self) -> usize;
    auto BinarySearchTree_valueSize(const BinarySearchTree* self) -> usize;
    auto BinarySearchTree_size(const BinarySearchTree* self) -> usize;

    auto BinarySearchTree_isEmpty(const BinarySearchTree* self) -> bool;
    auto BinarySearchTree_contains(const BinarySearchTree* self, const void* inKey) -> bool;

    auto BinarySearchTreeNode_key(const BinarySearchTreeNode* node) -> void*;
    auto BinarySearchTreeNode_value(const BinarySearchTreeNode* node) -> void*;
    auto BinarySearchTreeNode_left(const BinarySearchTreeNode* node) -> BinarySearchTreeNode*;
    auto BinarySearchTreeNode_right(const BinarySearchTreeNode* node) -> BinarySearchTreeNode*;
}