#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct DoublyLinkedListNode DoublyLinkedListNode;

    typedef struct DoublyLinkedList DoublyLinkedList;

    auto DoublyLinkedList_construct() -> DoublyLinkedList*;
    auto DoublyLinkedList_destruct(DoublyLinkedList* self) -> bool;

    auto DoublyLinkedList_copy(const DoublyLinkedList* self, usize inElementSize) -> DoublyLinkedList*;
    auto DoublyLinkedList_move(DoublyLinkedList* self) -> DoublyLinkedList*;

    auto DoublyLinkedList_pushHead(DoublyLinkedList* self, const void* inElement, usize inElementSize) -> bool;
    auto DoublyLinkedList_pushTail(DoublyLinkedList* self, const void* inElement, usize inElementSize) -> bool;
    auto DoublyLinkedList_insertAt(DoublyLinkedList* self, usize index, const void* inElement, usize inElementSize) -> bool;

    auto DoublyLinkedList_popHead(DoublyLinkedList* self) -> bool;
    auto DoublyLinkedList_popTail(DoublyLinkedList* self) -> bool;
    auto DoublyLinkedList_removeAt(DoublyLinkedList* self, usize index) -> bool;
    auto DoublyLinkedList_removeIf(DoublyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool;
    auto DoublyLinkedList_clear(DoublyLinkedList* self) -> bool;

    auto DoublyLinkedList_get(const DoublyLinkedList* self, usize index) -> DoublyLinkedListNode*;
    auto DoublyLinkedList_head(const DoublyLinkedList* self) -> DoublyLinkedListNode*;
    auto DoublyLinkedList_tail(const DoublyLinkedList* self) -> DoublyLinkedListNode*;

    auto DoublyLinkedList_set(DoublyLinkedList* self, usize index, const void* inElement, usize inElementSize) -> bool;

    auto DoublyLinkedList_reverse(DoublyLinkedList* self) -> bool;

    auto DoublyLinkedList_begin(const DoublyLinkedList* self) -> DoublyLinkedListNode*;
    auto DoublyLinkedList_end() -> DoublyLinkedListNode*;

    auto DoublyLinkedList_size(const DoublyLinkedList* self) -> usize;

    auto DoublyLinkedList_isEmpty(const DoublyLinkedList* self) -> bool;
    auto DoublyLinkedList_contains(const DoublyLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool;

    auto DoublyLinkedListNode_prev(const DoublyLinkedListNode* node) -> DoublyLinkedListNode*;
    auto DoublyLinkedListNode_next(const DoublyLinkedListNode* node) -> DoublyLinkedListNode*;
    auto DoublyLinkedListNode_data(const DoublyLinkedListNode* node) -> void*;
}