#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct BTree BTree;

    typedef struct BTreeNode BTreeNode;

    auto BTree_construct(usize inOrder, usize inKeySize, usize inValueSize, i32 (*compare)(const void*, const void*)) -> BTree*;
    auto BTree_destruct(BTree* self) -> bool;

    auto BTree_copy(const BTree* self) -> BTree*;
    auto BTree_move(BTree* self) -> BTree*;

    auto BTree_insert(BTree* self, const void* inKey, const void* inValue) -> bool;
    auto BTree_insertSlot(BTree* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool;

    auto BTree_remove(BTree* self, const void* inKey) -> bool;
    auto BTree_removeSlot(BTree* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool;
    auto BTree_clear(BTree* self) -> bool;

    auto BTree_find(const BTree* self, const void* inKey) -> void*;
    auto BTree_findSlot(const BTree* self, const void* inKey, BTreeNode** nodeOut, usize* indexOut) -> bool;
    auto BTree_findSuccessorSlot(const BTree* self, const void* inKey, BTreeNode** nodeOut, usize* indexOut) -> bool;
    auto BTree_findPredecessorSlot(const BTree* self, const void* inKey, BTreeNode** nodeOut, usize* indexOut) -> bool;

    auto BTree_rootNode(const BTree* self) -> BTreeNode*;
    auto BTree_minSlot(const BTree* self, BTreeNode** nodeOut, usize* indexOut) -> bool;
    auto BTree_maxSlot(const BTree* self, BTreeNode** nodeOut, usize* indexOut) -> bool;

    auto BTree_order(const BTree* self) -> usize;
    auto BTree_keySize(const BTree* self) -> usize;
    auto BTree_valueSize(const BTree* self) -> usize;
    auto BTree_size(const BTree* self) -> usize;

    auto BTree_isEmpty(const BTree* self) -> bool;
    auto BTree_contains(const BTree* self, const void* inKey) -> bool;

    auto BTreeNode_key(const BTreeNode* node, usize index, usize inKeySize) -> void*;
    auto BTreeNode_value(const BTreeNode* node, usize index, usize inValueSize) -> void*;
    auto BTreeNode_child(const BTreeNode* node, usize index) -> BTreeNode*;
    auto BTreeNode_keyCount(const BTreeNode* node) -> usize;
    auto BTreeNode_isLeaf(const BTreeNode* node) -> bool;
}