#include "IMPList.hpp"

#include <cstdlib>
#include <cstring>

#include "../../foundation/primitive/Atomic.hpp"
#include "../../foundation/primitive/Thread.hpp"
#include "../../foundation/primitive/ThreadKey.hpp"
#include "../../foundation/primitive/OnceFlag.hpp"
#include "../../foundation/primitive/Mutex.hpp"
#include "../../foundation/primitive/Time.hpp"
#include "../../foundation/sync/ConditionalVar.hpp"
#include "../../foundation/sync/ConflatedSignal.hpp"
#include "../../foundation/concurrent/ConcurrentUnboundedQueue.hpp"

namespace Catelier::src::architecture::imp {
    namespace {
        constexpr usize RETIRED_PRESENT_ELEMENTS_CAPACITY = 4;
    }

    namespace {
        typedef enum ElementState : u8 {
            ELEMENT_STATE_EMPTY = 0,
            ELEMENT_STATE_VALID = 1,
            ELEMENT_STATE_TOMBSTONE = 2,
        } ElementState;

        typedef struct ElementChunk {
            u8* elements;
            ElementState* elementStates;
            foundation::primitive::Atomic elementCount;

            foundation::primitive::Atomic isValid;
        } ElementChunk;

        typedef struct ElementChunkBundle {
            foundation::primitive::Atomic currentChunk;
            foundation::concurrent::ConcurrentUnboundedQueue* sealedChunks;
        } ElementChunkBundle;

        typedef struct ElementChunkBundleThreadLocalSetEntry {
            struct ElementChunkBundleThreadLocalSet* owner;
            ElementChunkBundle* bundle;
            ElementChunkBundleThreadLocalSetEntry* nextInThread;
            ElementChunkBundleThreadLocalSetEntry* nextInOwner;
            bool isThreadExited;
        } ElementChunkBundleThreadLocalSetEntry;

        typedef struct ElementChunkBundleThreadLocalSet {
            ElementChunkBundleThreadLocalSetEntry* head;
            void (*threadExitCallback)(void* inThreadExitCallbackContext);
            void* threadExitCallbackContext;
            bool isValid;
        } ElementChunkBundleThreadLocalSet;
    }

    typedef struct IMPList {
        usize elementSize;
        usize chunkSize;
        u64 propagationDurationMs;
        bool (*elementEquals)(const void*, const void*);

        IMPList_ElementSortingStrategy elementSortingStrategy;
        i32 (*elementSortingComparator)(const void*, const void*);

        u8* presentElements;
        usize presentElementCount;

        u8* retiredPresentElements[RETIRED_PRESENT_ELEMENTS_CAPACITY];
        usize retiredPresentElementCount;
        usize retiredPresentElementIndex;

        ElementChunkBundleThreadLocalSet chunkBundleThreadLocalSet;

        foundation::primitive::Atomic isMergePending;
        foundation::primitive::Thread maintenanceThread;
        foundation::primitive::Atomic isMaintenanceThreadActive;
        foundation::sync::ConflatedSignal maintenanceTrigger;

        foundation::primitive::Mutex awaitMergingMutex;
        foundation::sync::ConditionalVar awaitMergingCondition;

        foundation::primitive::Atomic mergeCount;
        foundation::primitive::Atomic lastMergeDurationMs;
    } IMPList;

    namespace {
        auto ElementChunk_construct(const usize inElementSize, const usize inChunkSize) -> ElementChunk* {
            auto* const self = (ElementChunk*) malloc(sizeof(ElementChunk));
            if (!self) {
                return nullptr;
            }

            const usize elementsByteSize = inElementSize * inChunkSize;
            const usize statesByteSize = sizeof(ElementState) * inChunkSize;

            auto* const statefulElement = (u8*) malloc(elementsByteSize + statesByteSize);
            if (!statefulElement) {
                free(self);
                return nullptr;
            }

            memset(statefulElement, 0, elementsByteSize + statesByteSize);

            self->elements = statefulElement;
            self->elementStates = (ElementState*) (statefulElement + elementsByteSize);

            if (!foundation::primitive::Atomic_init(&self->elementCount, 0)) {
                free(statefulElement);
                free(self);
                return nullptr;
            }

            if (!foundation::primitive::Atomic_init(&self->isValid, 1)) {
                foundation::primitive::Atomic_destroy(&self->elementCount);
                free(statefulElement);
                free(self);
                return nullptr;
            }

            return self;
        }
        auto ElementChunk_destruct(ElementChunk* self) -> bool {
            if (!self) {
                return false;
            }

            foundation::primitive::Atomic_destroy(&self->isValid);
            foundation::primitive::Atomic_destroy(&self->elementCount);

            if (self->elements) {
                free(self->elements);
            }

            free(self);

            return true;
        }
    }

    namespace {
        auto ElementChunkBundle_construct() -> ElementChunkBundle* {
            auto* const self = (ElementChunkBundle*) malloc(sizeof(ElementChunkBundle));
            if (!self) {
                return nullptr;
            }

            if (!foundation::primitive::Atomic_init(&self->currentChunk, 0)) {
                free(self);
                return nullptr;
            }

            self->sealedChunks = foundation::concurrent::ConcurrentUnboundedQueue_construct(sizeof(ElementChunk*));
            if (!self->sealedChunks) {
                foundation::primitive::Atomic_destroy(&self->currentChunk);
                free(self);
                return nullptr;
            }

            return self;
        }
        auto ElementChunkBundle_destruct(ElementChunkBundle* self) -> bool {
            if (!self) {
                return false;
            }

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&self->currentChunk, foundation::primitive::MEMORY_ORDER_RELAXED);
            if (currentChunk) {
                ElementChunk_destruct(currentChunk);
            }
            foundation::primitive::Atomic_destroy(&self->currentChunk);

            if (self->sealedChunks) {
                auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(self->sealedChunks);
                while (node) {
                    auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                    if (chunk) {
                        ElementChunk_destruct(chunk);
                    }

                    node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
                }

                foundation::concurrent::ConcurrentUnboundedQueue_destruct(self->sealedChunks);
                self->sealedChunks = nullptr;
            }

            free(self);

            return true;
        }
        auto ElementChunkBundle_threadExitCallback(void* inThreadExitCallbackContext) -> void {
            auto* const self = (IMPList*) inThreadExitCallbackContext;
            if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
                u64 expected = 0;
                if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                    foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }
    }

    namespace {
        foundation::primitive::ThreadKey ElementChunkBundleThreadLocalSet_entryRegistryKey = {};
        foundation::primitive::OnceFlag ElementChunkBundleThreadLocalSet_entryRegistryKeyOnceFlag = {};
        foundation::primitive::Mutex ElementChunkBundleThreadLocalSet_entryRegistryMutex = {};
        foundation::primitive::Atomic ElementChunkBundleThreadLocalSet_entryRegistryInitGuard = {};

        auto ElementChunkBundleThreadLocalSet_threadExitCallback(void* inEntry) -> void;

        auto ElementChunkBundleThreadLocalSet_initRegistryKey() -> void {
            foundation::primitive::Mutex_init(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);
            foundation::primitive::ThreadKey_init(&ElementChunkBundleThreadLocalSet_entryRegistryKey, ElementChunkBundleThreadLocalSet_threadExitCallback);
        }

        auto ElementChunkBundleThreadLocalSet_ensureRegistryInit() -> void {
            u64 expected = 0;
            if (foundation::primitive::Atomic_compareExchange(&ElementChunkBundleThreadLocalSet_entryRegistryInitGuard, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                foundation::primitive::OnceFlag_init(&ElementChunkBundleThreadLocalSet_entryRegistryKeyOnceFlag);
            }

            foundation::primitive::OnceFlag_do(&ElementChunkBundleThreadLocalSet_entryRegistryKeyOnceFlag, ElementChunkBundleThreadLocalSet_initRegistryKey);
        }

        auto ElementChunkBundleThreadLocalSet_construct(ElementChunkBundleThreadLocalSet* self, void (*inThreadExitCallback)(void* inThreadExitCallbackContext), void* inThreadExitCallbackContext) -> bool {
            if (!self) {
                return false;
            }

            ElementChunkBundleThreadLocalSet_ensureRegistryInit();

            self->head = nullptr;
            self->threadExitCallback = inThreadExitCallback;
            self->threadExitCallbackContext = inThreadExitCallbackContext;
            self->isValid = true;

            return true;
        }
        auto ElementChunkBundleThreadLocalSet_destruct(ElementChunkBundleThreadLocalSet* self) -> bool {
            if (!self || !self->isValid) {
                return false;
            }

            self->isValid = false;

            auto* entry = self->head;
            while (entry) {
                auto* const nextEntry = entry->nextInOwner;

                ElementChunkBundle_destruct(entry->bundle);

                if (entry->isThreadExited) {
                    free(entry);
                } else {
                    entry->owner = nullptr;
                }

                entry = nextEntry;
            }

            self->head = nullptr;

            return true;
        }

        auto ElementChunkBundleThreadLocalSet_getOrCreate(ElementChunkBundleThreadLocalSet* self) -> ElementChunkBundle* {
            if (!self || !self->isValid) {
                return nullptr;
            }

            auto* entry = (ElementChunkBundleThreadLocalSetEntry*) foundation::primitive::ThreadKey_get(&ElementChunkBundleThreadLocalSet_entryRegistryKey);
            if (entry) {
                if (entry->owner == self) {
                    return entry->bundle;
                }

                auto* prev = entry;
                auto* current = entry->nextInThread;
                while (current) {
                    if (current->owner == self) {
                        prev->nextInThread = current->nextInThread;
                        current->nextInThread = entry;

                        foundation::primitive::ThreadKey_set(&ElementChunkBundleThreadLocalSet_entryRegistryKey, current);

                        return current->bundle;
                    }

                    prev = current;
                    current = current->nextInThread;
                }
            }

            auto* const bundle = ElementChunkBundle_construct();
            if (!bundle) {
                return nullptr;
            }

            auto* const newEntry = (ElementChunkBundleThreadLocalSetEntry*) malloc(sizeof(ElementChunkBundleThreadLocalSetEntry));
            if (!newEntry) {
                ElementChunkBundle_destruct(bundle);
                return nullptr;
            }

            newEntry->owner = self;
            newEntry->bundle = bundle;
            newEntry->isThreadExited = false;

            newEntry->nextInThread = entry;
            if (!foundation::primitive::ThreadKey_set(&ElementChunkBundleThreadLocalSet_entryRegistryKey, newEntry)) {
                free(newEntry);
                ElementChunkBundle_destruct(bundle);
                return nullptr;
            }

            foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

            newEntry->nextInOwner = self->head;
            self->head = newEntry;

            foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

            return bundle;
        }

        auto ElementChunkBundleThreadLocalSet_remove(ElementChunkBundleThreadLocalSet* self, ElementChunkBundle* inBundle) -> bool {
            if (!self || !self->isValid || !inBundle) {
                return false;
            }

            foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

            ElementChunkBundleThreadLocalSetEntry** link = &self->head;
            ElementChunkBundleThreadLocalSetEntry* target = nullptr;
            while (*link) {
                if ((*link)->bundle == inBundle) {
                    target = *link;
                    *link = target->nextInOwner;
                    break;
                }

                link = &(*link)->nextInOwner;
            }

            foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

            if (!target) {
                return false;
            }

            ElementChunkBundle_destruct(target->bundle);
            free(target);

            return true;
        }

        auto ElementChunkBundleThreadLocalSet_forEach(const ElementChunkBundleThreadLocalSet* self, void (*action)(ElementChunkBundle* inBundle, bool inIsThreadExited, void* inActionContext), void* inActionContext) -> void {
            if (!self || !self->isValid || !action) {
                return;
            }

            foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

            for (auto* entry = self->head; entry; entry = entry->nextInOwner) {
                action(entry->bundle, entry->isThreadExited, inActionContext);
            }

            foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);
        }

        auto ElementChunkBundleThreadLocalSet_threadExitCallback(void* inEntry) -> void {
            auto* entry = (ElementChunkBundleThreadLocalSetEntry*) inEntry;

            while (entry) {
                auto* const nextEntry = entry->nextInThread;
                auto* const self = entry->owner;

                if (!self) {
                    free(entry);
                    entry = nextEntry;
                    continue;
                }

                auto* const bundle = entry->bundle;
                auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                if (currentChunk) {
                    u64 expected = (u64) (addr) currentChunk;
                    if (foundation::primitive::Atomic_compareExchange(&bundle->currentChunk, &expected, 0, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                        foundation::primitive::Atomic_store(&currentChunk->isValid, 0, foundation::primitive::MEMORY_ORDER_RELEASE);

                        ElementChunk* const chunkToSeal = currentChunk;
                        foundation::concurrent::ConcurrentUnboundedQueue_enqueue(bundle->sealedChunks, &chunkToSeal);
                    }
                }

                entry->isThreadExited = true;

                if (self->threadExitCallback) {
                    self->threadExitCallback(self->threadExitCallbackContext);
                }

                entry = nextEntry;
            }
        }
    }

    namespace {
        auto IMPList_maintenanceEntry(void* inSelf) -> void {
            auto* const self = (IMPList*) inSelf;
            while (foundation::primitive::Atomic_load(&self->isMaintenanceThreadActive, foundation::primitive::MEMORY_ORDER_ACQUIRE)) {
                foundation::sync::ConflatedSignal_timedWait(&self->maintenanceTrigger, self->propagationDurationMs);

                if (!foundation::primitive::Atomic_load(&self->isMaintenanceThreadActive, foundation::primitive::MEMORY_ORDER_ACQUIRE)) {
                    break;
                }

                if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_ACQUIRE)) {
                    IMPList_merge(self);
                }
            }
        }

        auto IMPList_countIngestElement(ElementChunkBundle* inBundle, const bool inIsThreadExited, void* inContext) -> void {
            (void) inIsThreadExited;

            auto* const total = (usize*) inContext;
            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&inBundle->currentChunk, foundation::primitive::MEMORY_ORDER_RELAXED);

            if (currentChunk) {
                *total += (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_RELAXED);
            }
        }
        auto IMPList_countMergeElement(ElementChunkBundle* inBundle, const bool inIsThreadExited, void* inContext) -> void {
            (void) inIsThreadExited;

            auto* const total = (usize*) inContext;
            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(inBundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    *total += (usize) foundation::primitive::Atomic_load(&chunk->elementCount, foundation::primitive::MEMORY_ORDER_RELAXED);
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }
        }

        auto IMPList_retirePresentElements(IMPList* self) -> void {
            if (!self->presentElements) {
                return;
            }

            if (self->retiredPresentElementCount == RETIRED_PRESENT_ELEMENTS_CAPACITY) {
                free(self->retiredPresentElements[self->retiredPresentElementIndex]);
            } else {
                self->retiredPresentElementCount++;
            }

            self->retiredPresentElements[self->retiredPresentElementIndex] = self->presentElements;
            self->retiredPresentElementIndex = (self->retiredPresentElementIndex + 1) % RETIRED_PRESENT_ELEMENTS_CAPACITY;
        }
    }

    auto IMPList_construct(const usize inElementSize, const usize inChunkSize, const u64 inPropagationDurationMs, const IMPList_ElementSortingStrategy inElementSortingStrategy, i32 (*inElementSortingComparator)(const void*, const void*), bool (*inElementEquals)(const void*, const void*)) -> IMPList* {
        if (inElementSize == 0 || inChunkSize < 16 || inChunkSize > 16384 || inPropagationDurationMs == 0) {
            return nullptr;
        }

        auto* const self = (IMPList*) malloc(sizeof(IMPList));
        if (!self) {
            return nullptr;
        }

        memset(self, 0, sizeof(IMPList));

        self->elementSize = inElementSize;
        self->chunkSize = inChunkSize;
        self->propagationDurationMs = inPropagationDurationMs;
        self->elementEquals = inElementEquals;

        self->elementSortingStrategy = inElementSortingStrategy;
        self->elementSortingComparator = inElementSortingComparator;

        self->presentElements = nullptr;
        self->presentElementCount = 0;

        self->retiredPresentElementCount = 0;
        self->retiredPresentElementIndex = 0;
        for (usize i = 0; i < RETIRED_PRESENT_ELEMENTS_CAPACITY; ++i) {
            self->retiredPresentElements[i] = nullptr;
        }

        if (!foundation::primitive::Atomic_init(&self->isMergePending, 0)) {
            free(self);
            return nullptr;
        }

        if (!foundation::primitive::Atomic_init(&self->mergeCount, 0)) {
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!foundation::primitive::Atomic_init(&self->lastMergeDurationMs, 0)) {
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!foundation::sync::ConflatedSignal_init(&self->maintenanceTrigger)) {
            foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!foundation::primitive::Atomic_init(&self->isMaintenanceThreadActive, 1)) {
            foundation::sync::ConflatedSignal_destroy(&self->maintenanceTrigger);
            foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!foundation::primitive::Mutex_init(&self->awaitMergingMutex)) {
            foundation::primitive::Atomic_destroy(&self->isMaintenanceThreadActive);
            foundation::sync::ConflatedSignal_destroy(&self->maintenanceTrigger);
            foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!foundation::sync::ConditionalVar_init(&self->awaitMergingCondition)) {
            foundation::primitive::Mutex_destroy(&self->awaitMergingMutex);
            foundation::primitive::Atomic_destroy(&self->isMaintenanceThreadActive);
            foundation::sync::ConflatedSignal_destroy(&self->maintenanceTrigger);
            foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!ElementChunkBundleThreadLocalSet_construct(&self->chunkBundleThreadLocalSet, ElementChunkBundle_threadExitCallback, self)) {
            foundation::sync::ConditionalVar_destroy(&self->awaitMergingCondition);
            foundation::primitive::Mutex_destroy(&self->awaitMergingMutex);
            foundation::primitive::Atomic_destroy(&self->isMaintenanceThreadActive);
            foundation::sync::ConflatedSignal_destroy(&self->maintenanceTrigger);
            foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!foundation::primitive::Thread_init(&self->maintenanceThread)) {
            ElementChunkBundleThreadLocalSet_destruct(&self->chunkBundleThreadLocalSet);
            foundation::sync::ConditionalVar_destroy(&self->awaitMergingCondition);
            foundation::primitive::Mutex_destroy(&self->awaitMergingMutex);
            foundation::primitive::Atomic_destroy(&self->isMaintenanceThreadActive);
            foundation::sync::ConflatedSignal_destroy(&self->maintenanceTrigger);
            foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        if (!foundation::primitive::Thread_start(&self->maintenanceThread, IMPList_maintenanceEntry, self)) {
            foundation::primitive::Thread_destroy(&self->maintenanceThread);
            ElementChunkBundleThreadLocalSet_destruct(&self->chunkBundleThreadLocalSet);
            foundation::sync::ConditionalVar_destroy(&self->awaitMergingCondition);
            foundation::primitive::Mutex_destroy(&self->awaitMergingMutex);
            foundation::primitive::Atomic_destroy(&self->isMaintenanceThreadActive);
            foundation::sync::ConflatedSignal_destroy(&self->maintenanceTrigger);
            foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);
            foundation::primitive::Atomic_destroy(&self->mergeCount);
            foundation::primitive::Atomic_destroy(&self->isMergePending);
            free(self);
            return nullptr;
        }

        return self;
    }
    auto IMPList_destruct(IMPList* self) -> bool {
        if (!self) {
            return false;
        }

        foundation::primitive::Atomic_store(&self->isMaintenanceThreadActive, 0, foundation::primitive::MEMORY_ORDER_RELEASE);
        foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);

        foundation::primitive::Thread_join(&self->maintenanceThread);
        foundation::primitive::Thread_destroy(&self->maintenanceThread);

        foundation::sync::ConflatedSignal_destroy(&self->maintenanceTrigger);

        ElementChunkBundleThreadLocalSet_destruct(&self->chunkBundleThreadLocalSet);

        if (self->presentElements) {
            free(self->presentElements);
        }

        for (usize i = 0; i < self->retiredPresentElementCount; ++i) {
            if (self->retiredPresentElements[i]) {
                free(self->retiredPresentElements[i]);
            }
        }

        foundation::primitive::Atomic_destroy(&self->isMaintenanceThreadActive);
        foundation::primitive::Atomic_destroy(&self->isMergePending);

        foundation::sync::ConditionalVar_destroy(&self->awaitMergingCondition);
        foundation::primitive::Mutex_destroy(&self->awaitMergingMutex);

        foundation::primitive::Atomic_destroy(&self->mergeCount);
        foundation::primitive::Atomic_destroy(&self->lastMergeDurationMs);

        free(self);

        return true;
    }
    auto IMPList_copy(const IMPList* self) -> IMPList* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = IMPList_construct(self->elementSize, self->chunkSize, self->propagationDurationMs, self->elementSortingStrategy, self->elementSortingComparator, self->elementEquals);
        if (!newSelf) {
            return nullptr;
        }

        const u8* const sourceElements = self->presentElements;
        const usize sourceElementCount = self->presentElementCount;
        if (sourceElements && sourceElementCount > 0) {
            const usize presentElementsByteSize = sourceElementCount * self->elementSize;

            auto* const newPresentElements = (u8*) malloc(presentElementsByteSize);
            if (!newPresentElements) {
                IMPList_destruct(newSelf);
                return nullptr;
            }

            memcpy(newPresentElements, sourceElements, presentElementsByteSize);

            newSelf->presentElements = newPresentElements;
            newSelf->presentElementCount = sourceElementCount;

            auto* const bundle = ElementChunkBundleThreadLocalSet_getOrCreate(&newSelf->chunkBundleThreadLocalSet);
            if (!bundle) {
                IMPList_destruct(newSelf);
                return nullptr;
            }

            auto* const newChunk = ElementChunk_construct(self->elementSize, self->chunkSize);
            if (!newChunk) {
                IMPList_destruct(newSelf);
                return nullptr;
            }

            memcpy(newChunk->elements, sourceElements, presentElementsByteSize);
            memset(newChunk->elementStates, ELEMENT_STATE_VALID, sourceElementCount);

            foundation::primitive::Atomic_store(&newChunk->elementCount, sourceElementCount, foundation::primitive::MEMORY_ORDER_RELEASE);
            foundation::primitive::Atomic_store(&bundle->currentChunk, (u64) (addr) newChunk, foundation::primitive::MEMORY_ORDER_RELEASE);
        }

        return newSelf;
    }

    auto IMPList_presentElements(const IMPList* self) -> const u8* {
        if (!self) {
            return nullptr;
        }

        return self->presentElements;
    }
    auto IMPList_presentElementsCopy(const IMPList* self, u8** outElements, usize* outElementCount) -> bool {
        if (!self || !outElements || !outElementCount) {
            return false;
        }

        *outElements = nullptr;
        *outElementCount = 0;

        const u8* const presentElements = self->presentElements;
        const usize presentElementCount = self->presentElementCount;

        if (!presentElements || presentElementCount == 0) {
            return true;
        }

        const usize presentElementsByteSize = presentElementCount * self->elementSize;

        auto* const copiedElements = (u8*) malloc(presentElementsByteSize);
        if (!copiedElements) {
            return false;
        }

        memcpy(copiedElements, presentElements, presentElementsByteSize);

        *outElements = copiedElements;
        *outElementCount = presentElementCount;

        return true;
    }
    auto IMPList_presentElementCount(const IMPList* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->presentElementCount;
    }
    auto IMPList_ingestElementCount(const IMPList* self) -> usize {
        if (!self) {
            return 0;
        }

        usize total = 0;
        ElementChunkBundleThreadLocalSet_forEach(&self->chunkBundleThreadLocalSet, IMPList_countIngestElement, &total);

        return total;
    }
    auto IMPList_mergeElementCount(const IMPList* self) -> usize {
        if (!self) {
            return 0;
        }

        usize total = 0;
        ElementChunkBundleThreadLocalSet_forEach(&self->chunkBundleThreadLocalSet, IMPList_countMergeElement, &total);

        return total;
    }
    auto IMPList_mergeCount(const IMPList* self) -> usize {
        if (!self) {
            return 0;
        }

        return (usize) foundation::primitive::Atomic_load(&self->mergeCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
    }
    auto IMPList_lastMergeDurationMs(const IMPList* self) -> usize {
        if (!self) {
            return 0;
        }

        return (usize) foundation::primitive::Atomic_load(&self->lastMergeDurationMs, foundation::primitive::MEMORY_ORDER_ACQUIRE);
    }

    auto IMPList_get(const IMPList* self, const usize index) -> const void* {
        if (!self || index >= self->presentElementCount) {
            return nullptr;
        }

        return self->presentElements + (index * self->elementSize);
    }
    auto IMPList_first(const IMPList* self) -> const void* {
        if (!self || self->presentElementCount == 0) {
            return nullptr;
        }

        return self->presentElements;
    }
    auto IMPList_last(const IMPList* self) -> const void* {
        if (!self || self->presentElementCount == 0) {
            return nullptr;
        }

        return self->presentElements + ((self->presentElementCount - 1) * self->elementSize);
    }

    auto IMPList_add(IMPList* self, const void* inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        auto* const bundle = ElementChunkBundleThreadLocalSet_getOrCreate(&self->chunkBundleThreadLocalSet);
        if (!bundle) {
            return false;
        }

        auto* currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_RELAXED);
        usize currentElementCount = 0;

        if (currentChunk) {
            const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_RELAXED) != 0;
            currentElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_RELAXED);

            if (!isChunkValid || currentElementCount >= self->chunkSize) {
                if (isChunkValid) {
                    foundation::primitive::Atomic_store(&currentChunk->isValid, 0, foundation::primitive::MEMORY_ORDER_RELEASE);

                    ElementChunk* const chunkToSeal = currentChunk;
                    foundation::concurrent::ConcurrentUnboundedQueue_enqueue(bundle->sealedChunks, &chunkToSeal);
                }

                currentChunk = nullptr;
            }
        }

        if (!currentChunk) {
            currentChunk = ElementChunk_construct(self->elementSize, self->chunkSize);
            if (!currentChunk) {
                return false;
            }

            foundation::primitive::Atomic_store(&bundle->currentChunk, (u64) (addr) currentChunk, foundation::primitive::MEMORY_ORDER_RELEASE);
            currentElementCount = 0;
        }

        memcpy(currentChunk->elements + currentElementCount * self->elementSize, inElement, self->elementSize);
        currentChunk->elementStates[currentElementCount] = ELEMENT_STATE_VALID;

        foundation::primitive::Atomic_store(&currentChunk->elementCount, currentElementCount + 1, foundation::primitive::MEMORY_ORDER_RELEASE);

        if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
            u64 expected = 0;
            if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
            }
        }

        return true;
    }
    auto IMPList_addAll(IMPList* self, const void* inElements, const usize inElementCount) -> bool {
        if (!self || !inElements) {
            return false;
        }

        if (inElementCount == 0) {
            return true;
        }

        auto* const bundle = ElementChunkBundleThreadLocalSet_getOrCreate(&self->chunkBundleThreadLocalSet);
        if (!bundle) {
            return false;
        }

        auto* currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_RELAXED);
        usize currentElementCount = 0;

        const u8* elementCursor = (const u8*) inElements;
        usize remainingElementCount = inElementCount;

        while (remainingElementCount > 0) {
            if (currentChunk) {
                const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_RELAXED) != 0;
                currentElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_RELAXED);

                if (!isChunkValid || currentElementCount >= self->chunkSize) {
                    if (isChunkValid) {
                        foundation::primitive::Atomic_store(&currentChunk->isValid, 0, foundation::primitive::MEMORY_ORDER_RELEASE);

                        ElementChunk* const chunkToSeal = currentChunk;
                        foundation::concurrent::ConcurrentUnboundedQueue_enqueue(bundle->sealedChunks, &chunkToSeal);
                    }

                    currentChunk = nullptr;
                }
            }

            if (!currentChunk) {
                currentChunk = ElementChunk_construct(self->elementSize, self->chunkSize);
                if (!currentChunk) {
                    return false;
                }

                foundation::primitive::Atomic_store(&bundle->currentChunk, (u64) (addr) currentChunk, foundation::primitive::MEMORY_ORDER_RELEASE);
                currentElementCount = 0;
            }

            const usize availableSpace = self->chunkSize - currentElementCount;
            const usize writeElementCount = remainingElementCount < availableSpace ? remainingElementCount : availableSpace;

            memcpy(currentChunk->elements + currentElementCount * self->elementSize, elementCursor, writeElementCount * self->elementSize);
            memset(currentChunk->elementStates + currentElementCount, ELEMENT_STATE_VALID, writeElementCount);

            foundation::primitive::Atomic_store(&currentChunk->elementCount, currentElementCount + writeElementCount, foundation::primitive::MEMORY_ORDER_RELEASE);

            elementCursor += writeElementCount * self->elementSize;
            remainingElementCount -= writeElementCount;
        }

        if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
            u64 expected = 0;
            if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
            }
        }

        return true;
    }

    auto IMPList_remove(IMPList* self, const void* inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        bool changed = false;

        foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        for (auto* entry = self->chunkBundleThreadLocalSet.head; entry; entry = entry->nextInOwner) {
            auto* const bundle = entry->bundle;

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
            if (currentChunk) {
                const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_ACQUIRE) != 0;
                if (isChunkValid) {
                    const usize currentChunkElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (currentChunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = currentChunk->elements + i * self->elementSize;
                        const bool isMatch = self->elementEquals ? self->elementEquals(candidate, inElement) : (memcmp(candidate, inElement, self->elementSize) == 0);
                        if (isMatch) {
                            currentChunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                            break;
                        }
                    }
                }
            }

            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    const usize chunkElementCount = (usize) foundation::primitive::Atomic_load(&chunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < chunkElementCount; ++i) {
                        if (chunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = chunk->elements + i * self->elementSize;
                        const bool isMatch = self->elementEquals ? self->elementEquals(candidate, inElement) : (memcmp(candidate, inElement, self->elementSize) == 0);
                        if (isMatch) {
                            chunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                            break;
                        }
                    }
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }
        }

        foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        if (changed) {
            if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
                u64 expected = 0;
                if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                    foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return changed;
    }
    auto IMPList_removeAll(IMPList* self, const void* inElements, const usize inElementCount) -> bool {
        if (!self || !inElements || inElementCount == 0) {
            return false;
        }

        bool changed = false;

        foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        for (auto* entry = self->chunkBundleThreadLocalSet.head; entry; entry = entry->nextInOwner) {
            auto* const bundle = entry->bundle;

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
            if (currentChunk) {
                const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_ACQUIRE) != 0;
                if (isChunkValid) {
                    const usize currentChunkElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (currentChunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = currentChunk->elements + i * self->elementSize;
                        for (usize j = 0; j < inElementCount; ++j) {
                            const void* const target = ((const u8*) inElements) + j * self->elementSize;
                            const bool isMatch = self->elementEquals ? self->elementEquals(candidate, target) : (memcmp(candidate, target, self->elementSize) == 0);
                            if (isMatch) {
                                currentChunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                                changed = true;
                                break;
                            }
                        }
                    }
                }
            }

            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    const usize chunkElementCount = (usize) foundation::primitive::Atomic_load(&chunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < chunkElementCount; ++i) {
                        if (chunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = chunk->elements + i * self->elementSize;
                        for (usize j = 0; j < inElementCount; ++j) {
                            const void* const target = ((const u8*) inElements) + j * self->elementSize;
                            const bool isMatch = self->elementEquals ? self->elementEquals(candidate, target) : (memcmp(candidate, target, self->elementSize) == 0);
                            if (isMatch) {
                                chunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                                changed = true;
                                break;
                            }
                        }
                    }
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }
        }

        foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        if (changed) {
            if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
                u64 expected = 0;
                if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                    foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return changed;
    }
    auto IMPList_removeIf(IMPList* self, bool (*inPredicate)(const void*, void*), void* inPredicateContext) -> bool {
        if (!self || !inPredicate) {
            return false;
        }

        bool changed = false;

        foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        for (auto* entry = self->chunkBundleThreadLocalSet.head; entry; entry = entry->nextInOwner) {
            auto* const bundle = entry->bundle;

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
            if (currentChunk) {
                const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_ACQUIRE) != 0;
                if (isChunkValid) {
                    const usize currentChunkElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (currentChunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = currentChunk->elements + i * self->elementSize;
                        if (inPredicate(candidate, inPredicateContext)) {
                            currentChunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                        }
                    }
                }
            }

            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    const usize chunkElementCount = (usize) foundation::primitive::Atomic_load(&chunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < chunkElementCount; ++i) {
                        if (chunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = chunk->elements + i * self->elementSize;
                        if (inPredicate(candidate, inPredicateContext)) {
                            chunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                        }
                    }
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }
        }

        foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        if (changed) {
            if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
                u64 expected = 0;
                if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                    foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return changed;
    }
    auto IMPList_retain(IMPList* self, const void* inElements, const usize inElementCount) -> bool {
        if (!self || !inElements || inElementCount == 0) {
            return false;
        }

        bool changed = false;

        foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        for (auto* entry = self->chunkBundleThreadLocalSet.head; entry; entry = entry->nextInOwner) {
            auto* const bundle = entry->bundle;

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
            if (currentChunk) {
                const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_ACQUIRE) != 0;
                if (isChunkValid) {
                    const usize currentChunkElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (currentChunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = currentChunk->elements + i * self->elementSize;

                        bool isFound = false;

                        for (usize j = 0; j < inElementCount; ++j) {
                            const void* const target = ((const u8*) inElements) + j * self->elementSize;
                            const bool isMatch = self->elementEquals ? self->elementEquals(candidate, target) : (memcmp(candidate, target, self->elementSize) == 0);
                            if (isMatch) {
                                isFound = true;
                                break;
                            }
                        }

                        if (!isFound) {
                            currentChunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                        }
                    }
                }
            }

            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    const usize chunkElementCount = (usize) foundation::primitive::Atomic_load(&chunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < chunkElementCount; ++i) {
                        if (chunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = chunk->elements + i * self->elementSize;

                        bool isFound = false;

                        for (usize j = 0; j < inElementCount; ++j) {
                            const void* const target = (const u8*) inElements + j * self->elementSize;
                            const bool isMatch = self->elementEquals ? self->elementEquals(candidate, target) : (memcmp(candidate, target, self->elementSize) == 0);
                            if (isMatch) {
                                isFound = true;
                                break;
                            }
                        }

                        if (!isFound) {
                            chunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                        }
                    }
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }
        }

        foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        if (changed) {
            if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
                u64 expected = 0;
                if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                    foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return changed;
    }
    auto IMPList_retainIf(IMPList* self, bool (*inPredicate)(const void*, void*), void* inPredicateContext) -> bool {
        if (!self || !inPredicate) {
            return false;
        }

        bool changed = false;

        foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        for (auto* entry = self->chunkBundleThreadLocalSet.head; entry; entry = entry->nextInOwner) {
            auto* const bundle = entry->bundle;

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
            if (currentChunk) {
                const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_ACQUIRE) != 0;
                if (isChunkValid) {
                    const usize currentChunkElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (currentChunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = currentChunk->elements + i * self->elementSize;
                        if (!inPredicate(candidate, inPredicateContext)) {
                            currentChunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                        }
                    }
                }
            }

            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    const usize chunkElementCount = (usize) foundation::primitive::Atomic_load(&chunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    for (usize i = 0; i < chunkElementCount; ++i) {
                        if (chunk->elementStates[i] != ELEMENT_STATE_VALID) {
                            continue;
                        }

                        const void* const candidate = chunk->elements + i * self->elementSize;
                        if (!inPredicate(candidate, inPredicateContext)) {
                            chunk->elementStates[i] = ELEMENT_STATE_TOMBSTONE;
                            changed = true;
                        }
                    }
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }
        }

        foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        if (changed) {
            if (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) == 0) {
                u64 expected = 0;
                if (foundation::primitive::Atomic_compareExchange(&self->isMergePending, &expected, 1, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                    foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return changed;
    }
    auto IMPList_clear(IMPList* self) -> bool {
        if (!self) {
            return false;
        }

        foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        for (auto* entry = self->chunkBundleThreadLocalSet.head; entry; entry = entry->nextInOwner) {
            auto* const bundle = entry->bundle;

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
            if (currentChunk) {
                u64 expected = (u64) (addr) currentChunk;
                if (foundation::primitive::Atomic_compareExchange(&bundle->currentChunk, &expected, 0, foundation::primitive::MEMORY_ORDER_ACQ_REL)) {
                    foundation::primitive::Atomic_store(&currentChunk->isValid, 0, foundation::primitive::MEMORY_ORDER_RELEASE);

                    ElementChunk* const chunkToSeal = currentChunk;
                    foundation::concurrent::ConcurrentUnboundedQueue_enqueue(bundle->sealedChunks, &chunkToSeal);
                }
            }

            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    foundation::primitive::Atomic_store(&chunk->elementCount, 0, foundation::primitive::MEMORY_ORDER_RELEASE);
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }
        }

        foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        IMPList_retirePresentElements(self);
        self->presentElements = nullptr;
        self->presentElementCount = 0;

        IMPList_merge(self);

        return true;
    }

    auto IMPList_find(const IMPList* self, const void* inElement) -> const void* {
        if (!self || !inElement) {
            return nullptr;
        }

        const u8* const presentElements = self->presentElements;
        const usize presentElementCount = self->presentElementCount;

        if (!presentElements || presentElementCount == 0) {
            return nullptr;
        }

        const usize elementSize = self->elementSize;
        const u8* cursor = presentElements;
        for (usize i = 0; i < presentElementCount; ++i) {
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return cursor;
            }

            cursor += elementSize;
        }

        return nullptr;
    }
    auto IMPList_contains(const IMPList* self, const void* inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        const u8* const presentElements = self->presentElements;
        const usize presentElementCount = self->presentElementCount;

        if (!presentElements || presentElementCount == 0) {
            return false;
        }

        const usize elementSize = self->elementSize;
        const u8* cursor = presentElements;
        for (usize i = 0; i < presentElementCount; ++i) {
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return true;
            }

            cursor += elementSize;
        }

        return false;
    }
    auto IMPList_containsAll(const IMPList* self, const void* inElements, const usize inElementCount) -> bool {
        if (!self) {
            return false;
        }

        if (inElementCount == 0) {
            return true;
        }

        if (!inElements) {
            return false;
        }

        const u8* const presentElements = self->presentElements;
        const usize presentElementCount = self->presentElementCount;

        if (!presentElements || presentElementCount == 0) {
            return false;
        }

        const usize elementSize = self->elementSize;
        const u8* const targets = (const u8*) inElements;
        for (usize t = 0; t < inElementCount; ++t) {
            const void* const target = targets + t * elementSize;

            bool isFound = false;

            const u8* cursor = presentElements;
            for (usize i = 0; i < presentElementCount; ++i) {
                const bool isMatch = self->elementEquals ? self->elementEquals(cursor, target) : (memcmp(cursor, target, elementSize) == 0);
                if (isMatch) {
                    isFound = true;
                    break;
                }

                cursor += elementSize;
            }

            if (!isFound) {
                return false;
            }
        }

        return true;
    }
    auto IMPList_firstIndexOf(const IMPList* self, const void* inElement) -> ssize {
        if (!self || !inElement) {
            return -1;
        }

        const u8* const presentElements = self->presentElements;
        const usize presentElementCount = self->presentElementCount;

        if (!presentElements || presentElementCount == 0) {
            return -1;
        }

        const usize elementSize = self->elementSize;
        const u8* cursor = presentElements;
        for (usize i = 0; i < presentElementCount; ++i) {
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return (ssize) i;
            }

            cursor += elementSize;
        }

        return -1;
    }
    auto IMPList_lastIndexOf(const IMPList* self, const void* inElement) -> ssize {
        if (!self || !inElement) {
            return -1;
        }

        const u8* const presentElements = self->presentElements;
        const usize presentElementCount = self->presentElementCount;

        if (!presentElements || presentElementCount == 0) {
            return -1;
        }

        const usize elementSize = self->elementSize;
        usize i = presentElementCount;
        while (i > 0) {
            i--;

            const u8* const cursor = presentElements + i * elementSize;
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return (ssize) i;
            }
        }

        return -1;
    }
    auto IMPList_isEmpty(const IMPList* self) -> bool {
        if (!self) {
            return true;
        }

        return self->presentElementCount == 0;
    }
    auto IMPList_isNotEmpty(const IMPList* self) -> bool {
        if (!self) {
            return false;
        }

        return self->presentElementCount != 0;
    }

    auto IMPList_sort(IMPList* self, const IMPList_ElementSortingStrategy inElementSortingStrategy, i32 (*inElementSortingComparator)(const void*, const void*)) -> bool {
        if (!self) {
            return false;
        }

        self->elementSortingStrategy = inElementSortingStrategy;
        self->elementSortingComparator = inElementSortingComparator;

        return IMPList_merge(self);
    }

    auto IMPList_merge(IMPList* self) -> bool {
        if (!self) {
            return false;
        }

        const u64 mergeStartTimeMs = foundation::primitive::Time_currentMs();
        const usize ingestSizeBefore = IMPList_ingestElementCount(self);

        auto* sourceChunks = (ElementChunk**) malloc(sizeof(ElementChunk*) * 16);
        auto* sourceElementCounts = (usize*) malloc(sizeof(usize) * 16);
        if (!sourceChunks || !sourceElementCounts) {
            free(sourceChunks);
            free(sourceElementCounts);
            return false;
        }

        usize sourceCount = 0;
        usize sourceCapacity = 16;
        usize totalElementCount = 0;

        foundation::primitive::Mutex_lock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        for (auto* entry = self->chunkBundleThreadLocalSet.head; entry; entry = entry->nextInOwner) {
            auto* const bundle = entry->bundle;

            auto* node = foundation::concurrent::ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    const usize chunkElementCount = (usize) foundation::primitive::Atomic_load(&chunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                    if (chunkElementCount > 0) {
                        if (sourceCount >= sourceCapacity) {
                            const usize newCapacity = sourceCapacity * 2;

                            auto* const newChunks = (ElementChunk**) realloc(sourceChunks, sizeof(ElementChunk*) * newCapacity);
                            if (!newChunks) {
                                foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);
                                free(sourceChunks);
                                free(sourceElementCounts);
                                return false;
                            }
                            sourceChunks = newChunks;

                            auto* const newElementCounts = (usize*) realloc(sourceElementCounts, sizeof(usize) * newCapacity);
                            if (!newElementCounts) {
                                foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);
                                free(sourceChunks);
                                free(sourceElementCounts);
                                return false;
                            }

                            sourceElementCounts = newElementCounts;
                            sourceCapacity = newCapacity;
                        }

                        sourceChunks[sourceCount] = chunk;
                        sourceElementCounts[sourceCount] = chunkElementCount;
                        sourceCount++;

                        totalElementCount += chunkElementCount;
                    }
                }

                node = foundation::concurrent::ConcurrentUnboundedQueueNode_next(node);
            }

            auto* const currentChunk = (ElementChunk*) (addr) foundation::primitive::Atomic_load(&bundle->currentChunk, foundation::primitive::MEMORY_ORDER_ACQUIRE);
            if (currentChunk) {
                const bool isChunkValid = foundation::primitive::Atomic_load(&currentChunk->isValid, foundation::primitive::MEMORY_ORDER_ACQUIRE) != 0;
                const usize chunkElementCount = (usize) foundation::primitive::Atomic_load(&currentChunk->elementCount, foundation::primitive::MEMORY_ORDER_ACQUIRE);
                if (isChunkValid && chunkElementCount > 0) {
                    if (sourceCount >= sourceCapacity) {
                        const usize newCapacity = sourceCapacity * 2;

                        auto* const newChunks = (ElementChunk**) realloc(sourceChunks, sizeof(ElementChunk*) * newCapacity);
                        if (!newChunks) {
                            foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);
                            free(sourceChunks);
                            free(sourceElementCounts);
                            return false;
                        }
                        sourceChunks = newChunks;

                        auto* const newElementCounts = (usize*) realloc(sourceElementCounts, sizeof(usize) * newCapacity);
                        if (!newElementCounts) {
                            foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);
                            free(sourceChunks);
                            free(sourceElementCounts);
                            return false;
                        }

                        sourceElementCounts = newElementCounts;
                        sourceCapacity = newCapacity;
                    }

                    sourceChunks[sourceCount] = currentChunk;
                    sourceElementCounts[sourceCount] = chunkElementCount;
                    sourceCount++;

                    totalElementCount += chunkElementCount;
                }
            }
        }

        foundation::primitive::Mutex_unlock(&ElementChunkBundleThreadLocalSet_entryRegistryMutex);

        if (totalElementCount == 0) {
            free(sourceChunks);
            free(sourceElementCounts);

            foundation::primitive::Atomic_fetchAdd(&self->mergeCount, 1, foundation::primitive::MEMORY_ORDER_RELAXED);
            foundation::primitive::Atomic_store(&self->lastMergeDurationMs, foundation::primitive::Time_currentMs() - mergeStartTimeMs, foundation::primitive::MEMORY_ORDER_RELAXED);

            const usize ingestSizeAfter = IMPList_ingestElementCount(self);
            if (ingestSizeAfter == ingestSizeBefore) {
                foundation::primitive::Mutex_lock(&self->awaitMergingMutex);
                foundation::primitive::Atomic_store(&self->isMergePending, 0, foundation::primitive::MEMORY_ORDER_RELEASE);
                foundation::sync::ConditionalVar_broadcast(&self->awaitMergingCondition);
                foundation::primitive::Mutex_unlock(&self->awaitMergingMutex);
            }

            return true;
        }

        auto* newPresentElements = (u8*) malloc(self->elementSize * totalElementCount);
        if (!newPresentElements) {
            free(sourceChunks);
            free(sourceElementCounts);
            return false;
        }

        usize writeIndex = 0;
        for (usize s = 0; s < sourceCount; ++s) {
            const auto* const chunk = sourceChunks[s];
            const usize elementCount = sourceElementCounts[s];
            for (usize i = 0; i < elementCount; ++i) {
                if (chunk->elementStates[i] == ELEMENT_STATE_VALID) {
                    memcpy(newPresentElements + writeIndex * self->elementSize, chunk->elements + i * self->elementSize, self->elementSize);
                    writeIndex++;
                }
            }
        }

        free(sourceChunks);
        free(sourceElementCounts);

        if (writeIndex == 0) {
            free(newPresentElements);

            foundation::primitive::Atomic_fetchAdd(&self->mergeCount, 1, foundation::primitive::MEMORY_ORDER_RELAXED);
            foundation::primitive::Atomic_store(&self->lastMergeDurationMs, foundation::primitive::Time_currentMs() - mergeStartTimeMs, foundation::primitive::MEMORY_ORDER_RELAXED);

            const usize ingestSizeAfter = IMPList_ingestElementCount(self);
            if (ingestSizeAfter == ingestSizeBefore) {
                foundation::primitive::Mutex_lock(&self->awaitMergingMutex);
                foundation::primitive::Atomic_store(&self->isMergePending, 0, foundation::primitive::MEMORY_ORDER_RELEASE);
                foundation::sync::ConditionalVar_broadcast(&self->awaitMergingCondition);
                foundation::primitive::Mutex_unlock(&self->awaitMergingMutex);
            }

            return true;
        }

        if (writeIndex < totalElementCount) {
            const usize shrunkByteSize = writeIndex * self->elementSize;
            auto* const shrunk = (u8*) realloc(newPresentElements, shrunkByteSize);
            if (shrunk) {
                newPresentElements = shrunk;
            }
        }

        if (self->elementSortingStrategy != UNORDERED && self->elementSortingComparator) {
            qsort(newPresentElements, writeIndex, self->elementSize, self->elementSortingComparator);
            if (self->elementSortingStrategy == DESCENDING) {
                for (usize pairIndex = 0; pairIndex < writeIndex / 2; ++pairIndex) {
                    u8* const leftElement = newPresentElements + pairIndex * self->elementSize;
                    u8* const rightElement = newPresentElements + (writeIndex - 1 - pairIndex) * self->elementSize;
                    for (usize byteIndex = 0; byteIndex < self->elementSize; ++byteIndex) {
                        const u8 tempByte = leftElement[byteIndex];
                        leftElement[byteIndex] = rightElement[byteIndex];
                        rightElement[byteIndex] = tempByte;
                    }
                }
            }
        }

        IMPList_retirePresentElements(self);
        self->presentElements = newPresentElements;
        self->presentElementCount = writeIndex;

        foundation::primitive::Atomic_fetchAdd(&self->mergeCount, 1, foundation::primitive::MEMORY_ORDER_RELAXED);
        foundation::primitive::Atomic_store(&self->lastMergeDurationMs, foundation::primitive::Time_currentMs() - mergeStartTimeMs, foundation::primitive::MEMORY_ORDER_RELAXED);

        const usize ingestSizeAfter = IMPList_ingestElementCount(self);
        if (ingestSizeAfter == ingestSizeBefore) {
            foundation::primitive::Mutex_lock(&self->awaitMergingMutex);
            foundation::primitive::Atomic_store(&self->isMergePending, 0, foundation::primitive::MEMORY_ORDER_RELEASE);
            foundation::sync::ConditionalVar_broadcast(&self->awaitMergingCondition);
            foundation::primitive::Mutex_unlock(&self->awaitMergingMutex);
        }

        return true;
    }
    auto IMPList_awaitMerging(IMPList* self) -> bool {
        if (!self) {
            return false;
        }

        foundation::primitive::Mutex_lock(&self->awaitMergingMutex);

        while (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) != 0) {
            foundation::sync::ConditionalVar_wait(&self->awaitMergingCondition, &self->awaitMergingMutex);
        }

        foundation::primitive::Mutex_unlock(&self->awaitMergingMutex);

        return true;
    }
    auto IMPList_timedAwaitMerging(IMPList* self, const u64 inTimeoutMs) -> bool {
        if (!self) {
            return false;
        }

        foundation::primitive::Mutex_lock(&self->awaitMergingMutex);

        bool result = true;

        while (foundation::primitive::Atomic_load(&self->isMergePending, foundation::primitive::MEMORY_ORDER_RELAXED) != 0) {
            if (inTimeoutMs == 0 || !foundation::sync::ConditionalVar_timedWait(&self->awaitMergingCondition, &self->awaitMergingMutex, inTimeoutMs)) {
                result = false;
                break;
            }
        }

        foundation::primitive::Mutex_unlock(&self->awaitMergingMutex);

        return result;
    }
}