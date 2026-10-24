#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::architecture::imp {
    typedef struct IMPList IMPList;

    typedef enum IMPList_ElementSortingStrategy {
        UNORDERED,
        ASCENDING,
        DESCENDING,
    } IMPList_ElementSortingStrategy;

    auto IMPList_construct(usize inElementSize, usize inChunkSize, u64 inPropagationDurationMs, IMPList_ElementSortingStrategy inElementSortingStrategy, i32 (*inElementSortingComparator)(const void*, const void*), bool (*inElementEquals)(const void*, const void*)) -> IMPList*;
    auto IMPList_destruct(IMPList* self) -> bool;
    auto IMPList_copy(const IMPList* self) -> IMPList*;

    auto IMPList_configureRetirement(IMPList* self, usize inRetiredPresentElementsCapacity) -> bool;

    auto IMPList_presentElementsSpan(const IMPList* self, const u8** outElements, usize* outElementCount) -> bool;
    auto IMPList_presentElementsSpanCopy(const IMPList* self, const u8** outElements, usize* outElementCount) -> bool;
    auto IMPList_presentElements(const IMPList* self) -> const u8*;
    auto IMPList_presentElementCount(const IMPList* self) -> usize;
    auto IMPList_ingestElementCount(const IMPList* self) -> usize;
    auto IMPList_mergeElementCount(const IMPList* self) -> usize;
    auto IMPList_mergeCount(const IMPList* self) -> usize;
    auto IMPList_lastMergeDurationMs(const IMPList* self) -> usize;

    auto IMPList_get(const IMPList* self, usize index) -> const void*;
    auto IMPList_first(const IMPList* self) -> const void*;
    auto IMPList_last(const IMPList* self) -> const void*;

    auto IMPList_add(IMPList* self, const void* inElement) -> bool;
    auto IMPList_addAll(IMPList* self, const void* inElements, usize inElementCount) -> bool;

    auto IMPList_remove(IMPList* self, const void* inElement) -> bool;
    auto IMPList_removeIf(IMPList* self, bool (*inPredicate)(const void*, void*), void* inPredicateContext) -> bool;
    auto IMPList_removeExcept(IMPList* self, const void* inElements, usize inElementCount) -> bool;
    auto IMPList_removeExceptIf(IMPList* self, bool (*inPredicate)(const void*, void*), void* inPredicateContext) -> bool;
    auto IMPList_removeAll(IMPList* self) -> bool;

    auto IMPList_find(const IMPList* self, const void* inElement) -> const void*;
    auto IMPList_contains(const IMPList* self, const void* inElement) -> bool;
    auto IMPList_containsAll(const IMPList* self, const void* inElements, usize inElementCount) -> bool;
    auto IMPList_firstIndexOf(const IMPList* self, const void* inElement) -> ssize;
    auto IMPList_lastIndexOf(const IMPList* self, const void* inElement) -> ssize;
    auto IMPList_isEmpty(const IMPList* self) -> bool;
    auto IMPList_isNotEmpty(const IMPList* self) -> bool;

    auto IMPList_sort(IMPList* self, IMPList_ElementSortingStrategy inElementSortingStrategy, i32 (*inElementSortingComparator)(const void*, const void*)) -> bool;

    auto IMPList_merge(IMPList* self) -> bool;
    auto IMPList_awaitMerging(IMPList* self) -> bool;
    auto IMPList_timedAwaitMerging(IMPList* self, u64 inTimeoutMs) -> bool;
}