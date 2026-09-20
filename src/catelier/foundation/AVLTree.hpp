#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct AVLTree AVLTree;

    typedef struct AVLTreeNode AVLTreeNode;

    auto AVLTree_construct(usize inKeySize, usize inValueSize, i32 (*compare)(const void*, const void*)) -> AVLTree*;
    auto AVLTree_destruct(AVLTree* self) -> bool;

    auto AVLTree_copy(const AVLTree* self) -> AVLTree*;
    auto AVLTree_move(AVLTree* self) -> AVLTree*;

    auto AVLTree_insert(AVLTree* self, const void* inKey, const void* inValue) -> bool;
    auto AVLTree_insertSlot(AVLTree* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool;

    auto AVLTree_remove(AVLTree* self, const void* inKey) -> bool;
    auto AVLTree_removeSlot(AVLTree* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto AVLTree_clear(AVLTree* self) -> bool;

    auto AVLTree_find(const AVLTree* self, const void* inKey) -> void*;
    auto AVLTree_findNode(const AVLTree* self, const void* inKey) -> AVLTreeNode*;
    auto AVLTree_findSuccessorNode(const AVLTree* self, const void* inKey) -> AVLTreeNode*;
    auto AVLTree_findPredecessorNode(const AVLTree* self, const void* inKey) -> AVLTreeNode*;

    auto AVLTree_rootNode(const AVLTree* self) -> AVLTreeNode*;
    auto AVLTree_minNode(const AVLTree* self) -> AVLTreeNode*;
    auto AVLTree_maxNode(const AVLTree* self) -> AVLTreeNode*;

    auto AVLTree_height(const AVLTree* self) -> i32;
    auto AVLTree_keySize(const AVLTree* self) -> usize;
    auto AVLTree_valueSize(const AVLTree* self) -> usize;
    auto AVLTree_size(const AVLTree* self) -> usize;

    auto AVLTree_isEmpty(const AVLTree* self) -> bool;
    auto AVLTree_contains(const AVLTree* self, const void* inKey) -> bool;

    auto AVLTreeNode_key(const AVLTreeNode* node) -> void*;
    auto AVLTreeNode_value(const AVLTreeNode* node) -> void*;
    auto AVLTreeNode_left(const AVLTreeNode* node) -> AVLTreeNode*;
    auto AVLTreeNode_right(const AVLTreeNode* node) -> AVLTreeNode*;
    auto AVLTreeNode_height(const AVLTreeNode* node) -> i32;
}