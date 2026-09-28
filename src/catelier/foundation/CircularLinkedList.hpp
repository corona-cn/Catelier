#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::foundation {
    typedef struct CircularLinkedList CircularLinkedList;

    typedef struct CircularLinkedListNode CircularLinkedListNode;

    auto CircularLinkedList_construct() -> CircularLinkedList*;
    auto CircularLinkedList_destruct(CircularLinkedList* self) -> bool;

    auto CircularLinkedList_copy(const CircularLinkedList* self, usize inElementSize) -> CircularLinkedList*;
    auto CircularLinkedList_move(CircularLinkedList* self) -> CircularLinkedList*;

    auto CircularLinkedList_pushHead(CircularLinkedList* self, const void* inElement, usize inElementSize) -> bool;
    auto CircularLinkedList_pushHeadSlot(CircularLinkedList* self, usize inElementSize) -> void*;
    auto CircularLinkedList_pushTail(CircularLinkedList* self, const void* inElement, usize inElementSize) -> bool;
    auto CircularLinkedList_pushTailSlot(CircularLinkedList* self, usize inElementSize) -> void*;
    auto CircularLinkedList_insertAt(CircularLinkedList* self, usize index, const void* inElement, usize inElementSize) -> bool;
    auto CircularLinkedList_insertAtSlot(CircularLinkedList* self, usize index, usize inElementSize) -> void*;

    auto CircularLinkedList_popHead(CircularLinkedList* self) -> bool;
    auto CircularLinkedList_popHeadSlot(CircularLinkedList* self) -> void*;
    auto CircularLinkedList_popTail(CircularLinkedList* self) -> bool;
    auto CircularLinkedList_popTailSlot(CircularLinkedList* self) -> void*;
    auto CircularLinkedList_removeAt(CircularLinkedList* self, usize index) -> bool;
    auto CircularLinkedList_removeAtSlot(CircularLinkedList* self, usize index) -> void*;
    auto CircularLinkedList_removeIf(CircularLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool;
    auto CircularLinkedList_clear(CircularLinkedList* self) -> bool;

    auto CircularLinkedList_get(const CircularLinkedList* self, usize index) -> CircularLinkedListNode*;
    auto CircularLinkedList_head(const CircularLinkedList* self) -> CircularLinkedListNode*;
    auto CircularLinkedList_tail(const CircularLinkedList* self) -> CircularLinkedListNode*;

    auto CircularLinkedList_set(const CircularLinkedList* self, usize index, const void* inElement, usize inElementSize) -> bool;
    auto CircularLinkedList_setSlot(const CircularLinkedList* self, usize index, usize inElementSize) -> void*;

    auto CircularLinkedList_reverse(CircularLinkedList* self) -> bool;

    auto CircularLinkedList_begin(const CircularLinkedList* self) -> CircularLinkedListNode*;
    auto CircularLinkedList_end(const CircularLinkedList* self) -> CircularLinkedListNode*;

    auto CircularLinkedList_size(const CircularLinkedList* self) -> usize;

    auto CircularLinkedList_isEmpty(const CircularLinkedList* self) -> bool;
    auto CircularLinkedList_contains(const CircularLinkedList* self, const void* inData, bool (*dataEquals)(const void*, const void*)) -> bool;

    auto CircularLinkedListNode_next(const CircularLinkedListNode* node) -> CircularLinkedListNode*;
    auto CircularLinkedListNode_data(const CircularLinkedListNode* node) -> void*;
}