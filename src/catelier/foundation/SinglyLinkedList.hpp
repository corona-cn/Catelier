#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct SinglyLinkedListNode SinglyLinkedListNode;

    typedef struct SinglyLinkedList SinglyLinkedList;

    auto SinglyLinkedList_construct() -> SinglyLinkedList*;
    auto SinglyLinkedList_destruct(SinglyLinkedList* self) -> bool;

    auto SinglyLinkedList_copy(const SinglyLinkedList* self, usize inElementSize) -> SinglyLinkedList*;
    auto SinglyLinkedList_move(SinglyLinkedList* self) -> SinglyLinkedList*;

    auto SinglyLinkedList_pushHead(SinglyLinkedList* self, const void* inElement, usize inElementSize) -> bool;
    auto SinglyLinkedList_pushTail(SinglyLinkedList* self, const void* inElement, usize inElementSize) -> bool;
    auto SinglyLinkedList_insertAt(SinglyLinkedList* self, usize index, const void* inElement, usize inElementSize) -> bool;

    auto SinglyLinkedList_popHead(SinglyLinkedList* self) -> bool;
    auto SinglyLinkedList_popTail(SinglyLinkedList* self) -> bool;
    auto SinglyLinkedList_removeAt(SinglyLinkedList* self, usize index) -> bool;
    auto SinglyLinkedList_removeIf(SinglyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool;
    auto SinglyLinkedList_clear(SinglyLinkedList* self) -> bool;

    auto SinglyLinkedList_get(const SinglyLinkedList* self, usize index) -> SinglyLinkedListNode*;
    auto SinglyLinkedList_head(const SinglyLinkedList* self) -> SinglyLinkedListNode*;
    auto SinglyLinkedList_tail(const SinglyLinkedList* self) -> SinglyLinkedListNode*;

    auto SinglyLinkedList_set(SinglyLinkedList* self, usize index, const void* inElement, usize inElementSize) -> bool;

    auto SinglyLinkedList_reverse(SinglyLinkedList* self) -> bool;

    auto SinglyLinkedList_begin(const SinglyLinkedList* self) -> SinglyLinkedListNode*;
    auto SinglyLinkedList_end() -> SinglyLinkedListNode*;

    auto SinglyLinkedList_size(const SinglyLinkedList* self) -> usize;

    auto SinglyLinkedList_isEmpty(const SinglyLinkedList* self) -> bool;
    auto SinglyLinkedList_contains(const SinglyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool;

    auto SinglyLinkedListNode_next(const SinglyLinkedListNode* node) -> SinglyLinkedListNode*;
    auto SinglyLinkedListNode_data(const SinglyLinkedListNode* node) -> void*;
}