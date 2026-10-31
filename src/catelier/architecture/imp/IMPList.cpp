#include <cstdlib>
#include <cstring>

#include "IMPList.hpp"
#include "../../foundation/primitive/Atomic.hpp"
#include "../../foundation/primitive/Thread.hpp"
#include "../../foundation/primitive/Mutex.hpp"
#include "../../foundation/primitive/Time.hpp"
#include "../../foundation/sync/ConditionalVar.hpp"
#include "../../foundation/sync/ConflatedSignal.hpp"
#include "../../foundation/concurrent/ConcurrentUnboundedQueue.hpp"
#include "../../foundation/concurrency/ThreadLocalTable.hpp"

namespace Catelier::src::architecture::imp {
    namespace {
        namespace Internal {
            constexpr u64 DEFAULT_PROPAGATION_POLLING_INTERVAL_MS = 1;
            constexpr u64 DEFAULT_PROPAGATION_BACKOFF_INTERVAL_MS = 1;

            namespace {
                auto hashElementKey(const u64 inElementKey) -> usize {
                    return ((usize) inElementKey) * ((usize) 0x9E3779B97F4A7C15ULL);
                }
            }
        }
    }

    namespace {
        typedef struct alignas(64) ElementChunk {
            typedef enum ElementState : u8 {
                EMPTY = 0,
                VALID = 1,
                TOMBSTONE = 2,
            } ElementState;

            u8* elements;
            ElementState* elementStates;
            u64 elementCount;

            // 污染标记，用于标记这个块是否存在非 VALID 元素状态的元素
            bool isTainted;

            // 有效标记，用于标记这个块是否出于某些原因被认定为无效，即需要被封装
            bool isValid;
        } ElementChunk;

        auto ElementChunk_construct(const usize inElementSize, const usize inElementChunkSize) -> ElementChunk* {
            auto* const self = (ElementChunk*) malloc(sizeof(ElementChunk));
            if (!self) {
                return nullptr;
            }

            const usize elementsByteSize = inElementSize * inElementChunkSize;
            const usize statesByteSize = sizeof(ElementChunk::ElementState) * inElementChunkSize;

            auto* const statefulElement = (u8*) malloc(elementsByteSize + statesByteSize);
            if (!statefulElement) {
                free(self);
                return nullptr;
            }

            memset(statefulElement, 0, elementsByteSize + statesByteSize);

            self->elements = statefulElement;
            self->elementStates = (ElementChunk::ElementState*) (statefulElement + elementsByteSize);
            self->elementCount = 0;
            self->isTainted = false;
            self->isValid = true;

            return self;
        }
        auto ElementChunk_destruct(ElementChunk* const self) -> bool {
            if (!self) {
                return false;
            }

            if (self->elements) {
                free(self->elements);
            }

            free(self);

            return true;
        }
    }

    namespace {
        auto IMPList_threadExitCallback(void* inThreadExitCallbackContext) -> void;

        typedef struct alignas(64) ElementChunkBundle {
            ElementChunk* currentChunk;
            foundation::concurrent::ConcurrentUnboundedQueue* sealedChunks;

            // 损坏标记，用于标记这个块包是否发生了破坏性突变（比如 remove），导致呈现数据作废，必须立刻被重新合并
            // 具体表现为合并结束后立刻重新触发一次合并
            bool isCorrupted;
        } ElementChunkBundle;

        auto ElementChunkBundle_createValueCallback(void* const hostContext) -> void* {
            using namespace foundation::concurrent;

            (void) hostContext;

            auto* const self = (ElementChunkBundle*) malloc(sizeof(ElementChunkBundle));
            if (!self) {
                return nullptr;
            }

            self->currentChunk = nullptr;

            self->sealedChunks = ConcurrentUnboundedQueue_construct(sizeof(ElementChunk*));
            if (!self->sealedChunks) {
                free(self);
                return nullptr;
            }

            self->isCorrupted = false;

            return self;
        }
        auto ElementChunkBundle_destroyValueCallback(void* const value, void* const hostContext) -> void {
            using namespace foundation::primitive;
            using namespace foundation::concurrent;

            (void) hostContext;

            auto* const self = (ElementChunkBundle*) value;
            if (!self) {
                return;
            }

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &self->currentChunk, MemoryOrder::RELAXED);
            if (currentChunk) {
                ElementChunk_destruct(currentChunk);
            }

            if (self->sealedChunks) {
                auto* node = ConcurrentUnboundedQueue_begin(self->sealedChunks);
                while (node) {
                    auto* const chunk = *(ElementChunk**) ConcurrentUnboundedQueueNode_data(node);
                    if (chunk) {
                        ElementChunk_destruct(chunk);
                    }

                    node = ConcurrentUnboundedQueueNode_next(node);
                }

                ConcurrentUnboundedQueue_destruct(self->sealedChunks);
                self->sealedChunks = nullptr;
            }

            free(self);
        }

        auto ElementChunkBundle_valueThreadExitCallback(void* const value, void* const hostContext) -> void* {
            using namespace foundation::primitive;
            using namespace foundation::concurrent;

            auto* const bundle = (ElementChunkBundle*) value;

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::ACQUIRE);
            if (currentChunk) {
                auto* expected = (void*) currentChunk;
                if (AtomicPtr_compareExchange((void**) &bundle->currentChunk, &expected, nullptr, MemoryOrder::ACQUIRE_RELEASE)) {
                    AtomicBool_store(&currentChunk->isValid, false, MemoryOrder::RELEASE);
                    ConcurrentUnboundedQueue_enqueue(bundle->sealedChunks, &currentChunk);

                    if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                        AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                    }
                }
            }

            IMPList_threadExitCallback(hostContext);

            return value;
        }
    }

    namespace {
        typedef struct alignas(64) DiscardBuffer {
            u8* elements;
            u64 elementCount;

            // 有效标记，用于标记这个缓存是否出于某些原因被认定为无效，即需要被封装
            bool isValid;
        } DiscardBuffer;

        auto DiscardBuffer_construct(const usize inElementSize, const usize inDiscardBufferSize) -> DiscardBuffer* {
            auto* const self = (DiscardBuffer*) malloc(sizeof(DiscardBuffer));
            if (!self) {
                return nullptr;
            }

            auto* const elements = (u8*) malloc(inElementSize * inDiscardBufferSize);
            if (!elements) {
                free(self);
                return nullptr;
            }

            self->elements = elements;
            self->elementCount = 0;
            self->isValid = true;

            return self;
        }
        auto DiscardBuffer_destruct(DiscardBuffer* const self) -> bool {
            if (!self) {
                return false;
            }

            if (self->elements) {
                free(self->elements);
            }

            free(self);

            return true;
        }
    }

    namespace {
        typedef struct alignas(64) DiscardBufferBundle {
            DiscardBuffer* currentBuffer;
            foundation::concurrent::ConcurrentUnboundedQueue* sealedBuffers;
            foundation::concurrent::ConcurrentUnboundedQueue* recycledBuffers;
        } DiscardBufferBundle;

        auto DiscardBufferBundle_createValueCallback(void* const hostContext) -> void* {
            using namespace foundation::concurrent;

            (void) hostContext;

            auto* const self = (DiscardBufferBundle*) malloc(sizeof(DiscardBufferBundle));
            if (!self) {
                return nullptr;
            }

            self->currentBuffer = nullptr;

            self->sealedBuffers = ConcurrentUnboundedQueue_construct(sizeof(DiscardBuffer*));
            if (!self->sealedBuffers) {
                free(self);
                return nullptr;
            }

            self->recycledBuffers = ConcurrentUnboundedQueue_construct(sizeof(DiscardBuffer*));
            if (!self->recycledBuffers) {
                ConcurrentUnboundedQueue_destruct(self->sealedBuffers);
                free(self);
                return nullptr;
            }

            return self;
        }
        auto DiscardBufferBundle_destroyValueCallback(void* const value, void* const hostContext) -> void {
            using namespace foundation::primitive;
            using namespace foundation::concurrent;

            (void) hostContext;

            auto* const self = (DiscardBufferBundle*) value;
            if (!self) {
                return;
            }

            auto* const currentBuffer = (DiscardBuffer*) AtomicPtr_load((void* const*) &self->currentBuffer, MemoryOrder::RELAXED);
            if (currentBuffer) {
                DiscardBuffer_destruct(currentBuffer);
            }

            if (self->sealedBuffers) {
                auto* node = ConcurrentUnboundedQueue_begin(self->sealedBuffers);
                while (node) {
                    auto* const buffer = *(DiscardBuffer**) ConcurrentUnboundedQueueNode_data(node);
                    if (buffer) {
                        DiscardBuffer_destruct(buffer);
                    }

                    node = ConcurrentUnboundedQueueNode_next(node);
                }

                ConcurrentUnboundedQueue_destruct(self->sealedBuffers);
                self->sealedBuffers = nullptr;
            }

            if (self->recycledBuffers) {
                auto* node = ConcurrentUnboundedQueue_begin(self->recycledBuffers);
                while (node) {
                    auto* const buffer = *(DiscardBuffer**) ConcurrentUnboundedQueueNode_data(node);
                    if (buffer) {
                        DiscardBuffer_destruct(buffer);
                    }

                    node = ConcurrentUnboundedQueueNode_next(node);
                }

                ConcurrentUnboundedQueue_destruct(self->recycledBuffers);
                self->recycledBuffers = nullptr;
            }

            free(self);
        }

        auto DiscardBufferBundle_valueThreadExitCallback(void* const value, void* const hostContext) -> void* {
            using namespace foundation::primitive;
            using namespace foundation::concurrent;

            auto* const bundle = (DiscardBufferBundle*) value;

            auto* const currentBuffer = (DiscardBuffer*) AtomicPtr_load((void* const*) &bundle->currentBuffer, MemoryOrder::ACQUIRE);
            if (currentBuffer) {
                auto* expected = (void*) currentBuffer;
                if (AtomicPtr_compareExchange((void**) &bundle->currentBuffer, &expected, nullptr, MemoryOrder::ACQUIRE_RELEASE)) {
                    AtomicBool_store(&currentBuffer->isValid, false, MemoryOrder::RELEASE);

                    if (Atomic64_load(&currentBuffer->elementCount, MemoryOrder::RELAXED) > 0) {
                        ConcurrentUnboundedQueue_enqueue(bundle->sealedBuffers, &currentBuffer);
                    } else {
                        AtomicBool_store(&currentBuffer->isValid, true, MemoryOrder::RELEASE);
                        ConcurrentUnboundedQueue_enqueue(bundle->recycledBuffers, &currentBuffer);
                    }
                }
            }

            IMPList_threadExitCallback(hostContext);

            return value;
        }
    }

    typedef struct IMPList {
        typedef struct alignas(64) PresentElementsSpan {
            u8* elements;
            usize count;
        } PresentElementsSpan;

        usize elementSize;
        usize elementChunkSize;
        usize discardBufferSize;
        bool (*elementEquals)(const void*, const void*);
        u64 propagationPollingIntervalMs;
        u64 propagationBackoffIntervalMs;

        IMPList_ElementSortingStrategy elementSortingStrategy;
        i32 (*elementSortingComparator)(const void*, const void*);

        alignas(64) PresentElementsSpan* presentElementsSpan;

        foundation::concurrency::ThreadLocalTable chunkBundleThreadLocalTable;
        foundation::concurrency::ThreadLocalTable discardBufferBundleThreadLocalTable;

        alignas(64) PresentElementsSpan** retiredPresentElements;
        usize retiredPresentElementsCapacity;
        usize retiredPresentElementCount;
        usize retiredPresentElementIndex;

        foundation::primitive::Thread maintenanceThread;
        bool isMaintenanceThreadActive;
        foundation::sync::ConflatedSignal maintenanceTrigger;

        alignas(64) bool isMergeSignaled;
        bool isMergeExecuting;
        bool isMaintenanceMergingEnabled;
        foundation::primitive::Mutex mergeMutex;
        foundation::primitive::Mutex awaitMergingMutex;
        foundation::sync::ConditionalVar awaitMergingCondition;
        u64 mergeCount;
        u64 lastMergeDurationMs;
    } IMPList;

    namespace {
        auto IMPList_threadExitCallback(void* const inThreadExitCallbackContext) -> void {
            using namespace foundation::primitive;
            using namespace foundation::sync;

            auto* const self = (IMPList*) inThreadExitCallbackContext;
            if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
                bool expected = false;
                if (AtomicBool_compareExchange(&self->isMergeSignaled, &expected, true, MemoryOrder::ACQUIRE_RELEASE)) {
                    ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        auto IMPList_maintenanceEntry(void* const inSelf) -> void {
            using namespace foundation::primitive;
            using namespace foundation::sync;

            auto* const self = (IMPList*) inSelf;
            while (AtomicBool_load(&self->isMaintenanceThreadActive, MemoryOrder::ACQUIRE)) {
                ConflatedSignal_timedWait(&self->maintenanceTrigger, Atomic64_load(&self->propagationPollingIntervalMs, MemoryOrder::ACQUIRE));

                if (!AtomicBool_load(&self->isMaintenanceThreadActive, MemoryOrder::ACQUIRE)) {
                    break;
                }

                while (AtomicBool_load(&self->isMergeSignaled, MemoryOrder::ACQUIRE) && AtomicBool_load(&self->isMaintenanceThreadActive, MemoryOrder::ACQUIRE)) {
                    if (AtomicBool_load(&self->isMaintenanceMergingEnabled, MemoryOrder::ACQUIRE)) {
                        IMPList_tryMerging(self);
                    } else {
                        Mutex_lock(&self->awaitMergingMutex);
                        AtomicBool_store(&self->isMergeSignaled, false, MemoryOrder::RELEASE);
                        ConditionalVar_broadcast(&self->awaitMergingCondition);
                        Mutex_unlock(&self->awaitMergingMutex);
                    }

                    Time_sleepMs(Atomic64_load(&self->propagationBackoffIntervalMs, MemoryOrder::ACQUIRE));
                }
            }
        }

        auto IMPList_retirePresentElements(IMPList* const self) -> void {
            using namespace foundation::primitive;

            auto* const oldSpan = (IMPList::PresentElementsSpan*) AtomicPtr_exchange((void**) &self->presentElementsSpan, nullptr, MemoryOrder::ACQUIRE_RELEASE);
            if (!oldSpan) {
                return;
            }

            if (self->retiredPresentElementsCapacity == 0) {
                if (oldSpan->elements) {
                    free((void*) oldSpan->elements);
                }
                free(oldSpan);
                return;
            }

            if (self->retiredPresentElementCount == self->retiredPresentElementsCapacity) {
                auto* const evictedSpan = self->retiredPresentElements[self->retiredPresentElementIndex];
                self->retiredPresentElements[self->retiredPresentElementIndex] = nullptr;
                if (evictedSpan) {
                    if (evictedSpan->elements) {
                        free((void*) evictedSpan->elements);
                    }
                    free(evictedSpan);
                }
            } else {
                self->retiredPresentElementCount++;
            }

            self->retiredPresentElements[self->retiredPresentElementIndex] = oldSpan;
            self->retiredPresentElementIndex = (self->retiredPresentElementIndex + 1) % self->retiredPresentElementsCapacity;
        }
    }

    auto IMPList_construct(
        const usize inElementSize, const usize inElementChunkSize, const usize inDiscardBufferSize,
        const IMPList_ElementSortingStrategy inElementSortingStrategy, i32 (*inElementSortingComparator)(const void*, const void*),
        bool (*inElementEquals)(const void*, const void*)
    ) -> IMPList* {
        using namespace foundation::primitive;
        using namespace foundation::concurrency;
        using namespace foundation::sync;

        if (inElementSize == 0 || inElementChunkSize < 16 || inElementChunkSize > 16384 || inDiscardBufferSize < 16 || inDiscardBufferSize > 16384) {
            return nullptr;
        }

        auto* const self = (IMPList*) malloc(sizeof(IMPList));
        if (!self) {
            return nullptr;
        }

        memset(self, 0, sizeof(IMPList));

        self->elementSize = inElementSize;
        self->elementChunkSize = inElementChunkSize;
        self->discardBufferSize = inDiscardBufferSize;
        Atomic64_store(&self->propagationPollingIntervalMs, Internal::DEFAULT_PROPAGATION_POLLING_INTERVAL_MS, MemoryOrder::RELAXED);
        Atomic64_store(&self->propagationBackoffIntervalMs, Internal::DEFAULT_PROPAGATION_BACKOFF_INTERVAL_MS, MemoryOrder::RELAXED);
        self->elementEquals = inElementEquals;

        self->elementSortingStrategy = inElementSortingStrategy;
        self->elementSortingComparator = inElementSortingComparator;

        self->presentElementsSpan = nullptr;

        self->retiredPresentElements = nullptr;
        self->retiredPresentElementCount = 0;
        self->retiredPresentElementIndex = 0;
        self->retiredPresentElementsCapacity = 0;

        self->isMergeSignaled = false;
        self->isMergeExecuting = false;
        AtomicBool_store(&self->isMaintenanceMergingEnabled, true, MemoryOrder::RELAXED);
        self->isMaintenanceThreadActive = true;
        self->mergeCount = 0;
        self->lastMergeDurationMs = 0;

        if (!ConflatedSignal_init(&self->maintenanceTrigger)) {
            free(self);
            return nullptr;
        }

        if (!Mutex_init(&self->awaitMergingMutex)) {
            ConflatedSignal_destroy(&self->maintenanceTrigger);
            free(self);
            return nullptr;
        }

        if (!ConditionalVar_init(&self->awaitMergingCondition)) {
            Mutex_destroy(&self->awaitMergingMutex);
            ConflatedSignal_destroy(&self->maintenanceTrigger);
            free(self);
            return nullptr;
        }

        if (!Mutex_init(&self->mergeMutex)) {
            ConditionalVar_destroy(&self->awaitMergingCondition);
            Mutex_destroy(&self->awaitMergingMutex);
            ConflatedSignal_destroy(&self->maintenanceTrigger);
            free(self);
            return nullptr;
        }

        if (!ThreadLocalTable_construct(&self->chunkBundleThreadLocalTable, ElementChunkBundle_createValueCallback, ElementChunkBundle_valueThreadExitCallback, ElementChunkBundle_destroyValueCallback, self)) {
            Mutex_destroy(&self->mergeMutex);
            ConditionalVar_destroy(&self->awaitMergingCondition);
            Mutex_destroy(&self->awaitMergingMutex);
            ConflatedSignal_destroy(&self->maintenanceTrigger);
            free(self);
            return nullptr;
        }

        if (!ThreadLocalTable_construct(&self->discardBufferBundleThreadLocalTable, DiscardBufferBundle_createValueCallback, DiscardBufferBundle_valueThreadExitCallback, DiscardBufferBundle_destroyValueCallback, self)) {
            ThreadLocalTable_destruct(&self->chunkBundleThreadLocalTable);
            Mutex_destroy(&self->mergeMutex);
            ConditionalVar_destroy(&self->awaitMergingCondition);
            Mutex_destroy(&self->awaitMergingMutex);
            ConflatedSignal_destroy(&self->maintenanceTrigger);
            free(self);
            return nullptr;
        }

        if (!Thread_init(&self->maintenanceThread)) {
            ThreadLocalTable_destruct(&self->discardBufferBundleThreadLocalTable);
            ThreadLocalTable_destruct(&self->chunkBundleThreadLocalTable);
            Mutex_destroy(&self->mergeMutex);
            ConditionalVar_destroy(&self->awaitMergingCondition);
            Mutex_destroy(&self->awaitMergingMutex);
            ConflatedSignal_destroy(&self->maintenanceTrigger);
            free(self);
            return nullptr;
        }

        if (!Thread_start(&self->maintenanceThread, IMPList_maintenanceEntry, self)) {
            Thread_destroy(&self->maintenanceThread);
            ThreadLocalTable_destruct(&self->discardBufferBundleThreadLocalTable);
            ThreadLocalTable_destruct(&self->chunkBundleThreadLocalTable);
            Mutex_destroy(&self->mergeMutex);
            ConditionalVar_destroy(&self->awaitMergingCondition);
            Mutex_destroy(&self->awaitMergingMutex);
            ConflatedSignal_destroy(&self->maintenanceTrigger);
            free(self);
            return nullptr;
        }

        return self;
    }
    auto IMPList_destruct(IMPList* const self) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::concurrency;
        using namespace foundation::sync;

        if (!self) {
            return false;
        }

        AtomicBool_store(&self->isMaintenanceThreadActive, false, MemoryOrder::RELEASE);
        ConflatedSignal_signal(&self->maintenanceTrigger);

        Thread_join(&self->maintenanceThread);
        Thread_destroy(&self->maintenanceThread);

        ConflatedSignal_destroy(&self->maintenanceTrigger);

        ThreadLocalTable_destruct(&self->discardBufferBundleThreadLocalTable);
        ThreadLocalTable_destruct(&self->chunkBundleThreadLocalTable);

        if (self->presentElementsSpan) {
            if (self->presentElementsSpan->elements) {
                free((void*) self->presentElementsSpan->elements);
            }
            free(self->presentElementsSpan);
        }

        if (self->retiredPresentElements) {
            for (usize i = 0; i < self->retiredPresentElementCount; ++i) {
                if (self->retiredPresentElements[i]) {
                    if (self->retiredPresentElements[i]->elements) {
                        free((void*) self->retiredPresentElements[i]->elements);
                    }
                    free(self->retiredPresentElements[i]);
                }
            }

            free(self->retiredPresentElements);
        }

        ConditionalVar_destroy(&self->awaitMergingCondition);
        Mutex_destroy(&self->awaitMergingMutex);
        Mutex_destroy(&self->mergeMutex);

        free(self);

        return true;
    }
    auto IMPList_copy(const IMPList* const self) -> IMPList* {
        using namespace foundation::primitive;
        using namespace foundation::concurrency;
        using namespace foundation::concurrent;

        if (!self) {
            return nullptr;
        }

        auto* const newSelf = IMPList_construct(self->elementSize, self->elementChunkSize, self->discardBufferSize, self->elementSortingStrategy, self->elementSortingComparator, self->elementEquals);
        if (!newSelf) {
            return nullptr;
        }

        Atomic64_store(&newSelf->propagationPollingIntervalMs, Atomic64_load(&self->propagationPollingIntervalMs, MemoryOrder::ACQUIRE), MemoryOrder::RELAXED);
        Atomic64_store(&newSelf->propagationBackoffIntervalMs, Atomic64_load(&self->propagationBackoffIntervalMs, MemoryOrder::ACQUIRE), MemoryOrder::RELAXED);

        if (self->retiredPresentElementsCapacity > 0) {
            if (!IMPList_configureRetirement(newSelf, self->retiredPresentElementsCapacity)) {
                IMPList_destruct(newSelf);
                return nullptr;
            }
        }

        const auto* const sourceSpan = (const IMPList::PresentElementsSpan*) AtomicPtr_load((void* const*) &self->presentElementsSpan, MemoryOrder::ACQUIRE);
        if (sourceSpan && sourceSpan->count > 0) {
            const usize sourceElementCount = sourceSpan->count;
            const u8* const sourceElements = sourceSpan->elements;
            const usize presentElementsByteSize = sourceElementCount * self->elementSize;

            auto* const newPresentElements = (u8*) malloc(presentElementsByteSize);
            if (!newPresentElements) {
                IMPList_destruct(newSelf);
                return nullptr;
            }

            memcpy(newPresentElements, sourceElements, presentElementsByteSize);

            auto* const newSpan = (IMPList::PresentElementsSpan*) malloc(sizeof(IMPList::PresentElementsSpan));
            if (!newSpan) {
                free(newPresentElements);
                IMPList_destruct(newSelf);
                return nullptr;
            }

            newSpan->elements = newPresentElements;
            newSpan->count = sourceElementCount;
            newSelf->presentElementsSpan = newSpan;

            auto* const bundle = (ElementChunkBundle*) ThreadLocalTable_getOrCreate(&newSelf->chunkBundleThreadLocalTable);
            if (!bundle) {
                IMPList_destruct(newSelf);
                return nullptr;
            }

            const u8* chunkElementCursor = sourceElements;
            usize remainingElementCount = sourceElementCount;
            while (remainingElementCount > 0) {
                auto* const newChunk = ElementChunk_construct(self->elementSize, self->elementChunkSize);
                if (!newChunk) {
                    IMPList_destruct(newSelf);
                    return nullptr;
                }

                const usize writeElementCount = remainingElementCount < self->elementChunkSize ? remainingElementCount : self->elementChunkSize;

                memcpy(newChunk->elements, chunkElementCursor, writeElementCount * self->elementSize);
                memset(newChunk->elementStates, ElementChunk::ElementState::VALID, writeElementCount);

                Atomic64_store(&newChunk->elementCount, writeElementCount, MemoryOrder::RELEASE);

                chunkElementCursor += writeElementCount * self->elementSize;
                remainingElementCount -= writeElementCount;

                if (remainingElementCount > 0) {
                    AtomicBool_store(&newChunk->isValid, false, MemoryOrder::RELEASE);
                    ConcurrentUnboundedQueue_enqueue(bundle->sealedChunks, &newChunk);
                } else {
                    AtomicPtr_store((void**) &bundle->currentChunk, (void*) newChunk, MemoryOrder::RELEASE);
                }
            }
        }

        return newSelf;
    }

    auto IMPList_configureRetirement(IMPList* const self, const usize inRetiredPresentElementsCapacity) -> bool {
        using namespace foundation::primitive;

        if (!self) {
            return false;
        }

        Mutex_lock(&self->mergeMutex);

        if (self->retiredPresentElementsCapacity == inRetiredPresentElementsCapacity) {
            Mutex_unlock(&self->mergeMutex);
            return true;
        }

        IMPList::PresentElementsSpan** newRetiredPresentElements = nullptr;
        if (inRetiredPresentElementsCapacity > 0) {
            newRetiredPresentElements = (IMPList::PresentElementsSpan**) calloc(inRetiredPresentElementsCapacity, sizeof(IMPList::PresentElementsSpan*));
            if (!newRetiredPresentElements) {
                Mutex_unlock(&self->mergeMutex);
                return false;
            }
        }

        const usize oldCapacity = self->retiredPresentElementsCapacity;
        const usize oldCount = self->retiredPresentElementCount;
        const usize oldIndex = self->retiredPresentElementIndex;

        const usize keepCount = oldCount < inRetiredPresentElementsCapacity ? oldCount : inRetiredPresentElementsCapacity;

        if (newRetiredPresentElements && self->retiredPresentElements) {
            for (usize j = 0; j < keepCount; ++j) {
                const usize oldSlot = (oldIndex - keepCount + j + oldCapacity * 2) % oldCapacity;
                newRetiredPresentElements[j] = self->retiredPresentElements[oldSlot];
            }
        }

        if (self->retiredPresentElements && oldCount > keepCount) {
            for (usize k = 0; k < oldCount - keepCount; ++k) {
                const usize oldSlot = (oldIndex - oldCount + k + oldCapacity * 2) % oldCapacity;
                if (self->retiredPresentElements[oldSlot]) {
                    auto* const evictedSpan = self->retiredPresentElements[oldSlot];
                    self->retiredPresentElements[oldSlot] = nullptr;
                    if (evictedSpan->elements) {
                        free((void*) evictedSpan->elements);
                    }
                    free(evictedSpan);
                }
            }
        }

        if (self->retiredPresentElements) {
            free(self->retiredPresentElements);
        }

        self->retiredPresentElements = newRetiredPresentElements;
        self->retiredPresentElementsCapacity = inRetiredPresentElementsCapacity;
        self->retiredPresentElementCount = keepCount;
        self->retiredPresentElementIndex = (inRetiredPresentElementsCapacity > 0) ? (keepCount % inRetiredPresentElementsCapacity) : 0;

        Mutex_unlock(&self->mergeMutex);

        return true;
    }
    auto IMPList_configurePropagation(IMPList* const self, const u64 inPollingIntervalMs, const u64 inBackoffIntervalMs) -> bool {
        if (!self || inPollingIntervalMs == 0 || inBackoffIntervalMs == 0) {
            return false;
        }

        foundation::primitive::Atomic64_store(&self->propagationPollingIntervalMs, inPollingIntervalMs, foundation::primitive::MemoryOrder::RELEASE);
        foundation::primitive::Atomic64_store(&self->propagationBackoffIntervalMs, inBackoffIntervalMs, foundation::primitive::MemoryOrder::RELEASE);

        foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);

        return true;
    }
    auto IMPList_configureMaintenanceMerging(IMPList* const self, const bool inIsMaintenanceMergingEnabled) -> bool {
        if (!self) {
            return false;
        }

        foundation::primitive::AtomicBool_store(&self->isMaintenanceMergingEnabled, inIsMaintenanceMergingEnabled, foundation::primitive::MemoryOrder::RELEASE);

        if (inIsMaintenanceMergingEnabled) {
            foundation::sync::ConflatedSignal_signal(&self->maintenanceTrigger);
        }

        return true;
    }

    auto IMPList_presentElementsSpan(const IMPList* const self, const u8** const outElements, usize* const outElementCount) -> bool {
        if (!self || !outElements || !outElementCount) {
            return false;
        }

        *outElements = nullptr;
        *outElementCount = 0;

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (span) {
            *outElements = span->elements;
            *outElementCount = span->count;
        }

        return true;
    }
    auto IMPList_presentElementsSpanCopy(const IMPList* const self, const u8** const outElements, usize* const outElementCount) -> bool {
        if (!self || !outElements || !outElementCount) {
            return false;
        }

        *outElements = nullptr;
        *outElementCount = 0;

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return true;
        }

        const usize presentElementsByteSize = span->count * self->elementSize;

        auto* const copiedElements = (u8*) malloc(presentElementsByteSize);
        if (!copiedElements) {
            return false;
        }

        memcpy(copiedElements, span->elements, presentElementsByteSize);

        *outElements = copiedElements;
        *outElementCount = span->count;

        return true;
    }
    auto IMPList_presentElements(const IMPList* const self) -> const u8* {
        if (!self) {
            return nullptr;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);
        return span ? span->elements : nullptr;
    }
    auto IMPList_presentElementCount(const IMPList* const self) -> usize {
        if (!self) {
            return 0;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);
        return span ? span->count : 0;
    }
    auto IMPList_ingestElementCount(const IMPList* const self) -> usize {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;

        if (!self) {
            return 0;
        }

        usize total = 0;

        Mutex_lock(&((IMPList*) self)->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::RELAXED);
            if (currentChunk) {
                total += (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::RELAXED);
            }

            auto* node = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    total += (usize) Atomic64_load(&chunk->elementCount, MemoryOrder::RELAXED);
                }

                node = ConcurrentUnboundedQueueNode_next(node);
            }
        }

        Mutex_unlock(&((IMPList*) self)->chunkBundleThreadLocalTable.lock);

        return total;
    }
    auto IMPList_mergeElementCount(const IMPList* const self) -> usize {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;

        if (!self) {
            return 0;
        }

        usize total = 0;

        Mutex_lock(&((IMPList*) self)->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            auto* node = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    total += (usize) Atomic64_load(&chunk->elementCount, MemoryOrder::RELAXED);
                }

                node = ConcurrentUnboundedQueueNode_next(node);
            }
        }

        Mutex_unlock(&((IMPList*) self)->chunkBundleThreadLocalTable.lock);

        return total;
    }
    auto IMPList_mergeCount(const IMPList* const self) -> usize {
        if (!self) {
            return 0;
        }

        return (usize) foundation::primitive::Atomic64_load(&self->mergeCount, foundation::primitive::MemoryOrder::ACQUIRE);
    }
    auto IMPList_lastMergeDurationMs(const IMPList* const self) -> usize {
        if (!self) {
            return 0;
        }

        return (usize) foundation::primitive::Atomic64_load(&self->lastMergeDurationMs, foundation::primitive::MemoryOrder::ACQUIRE);
    }

    auto IMPList_get(const IMPList* const self, const usize index) -> const void* {
        if (!self) {
            return nullptr;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || index >= span->count) {
            return nullptr;
        }

        return span->elements + (index * self->elementSize);
    }
    auto IMPList_first(const IMPList* const self) -> const void* {
        if (!self) {
            return nullptr;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return nullptr;
        }

        return span->elements;
    }
    auto IMPList_last(const IMPList* const self) -> const void* {
        if (!self) {
            return nullptr;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return nullptr;
        }

        return span->elements + ((span->count - 1) * self->elementSize);
    }

    auto IMPList_add(IMPList* const self, const void* inElement) -> bool {
        using namespace foundation::primitive;
        if (!self || !inElement) {
            return false;
        }

        // 尝试获取线程本地的元素块包，否则就原地创建一个
        auto* const currentBundle = (ElementChunkBundle*) foundation::concurrency::ThreadLocalTable_getOrCreate(&self->chunkBundleThreadLocalTable);
        if (!currentBundle) {
            return false;
        }

        // 仅原子地获取当前块包中的当前块，同时创建当前块元素计数变量
        auto* currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &currentBundle->currentChunk, MemoryOrder::RELAXED);
        usize currentChunkElementCount = 0;

        // 首先判断当前块存在情况
        if (currentChunk) {
            // 仅原子地判断当前块有效状态
            if (AtomicBool_load(&currentChunk->isValid, MemoryOrder::RELAXED)) {
                // 当前块有效情况，以 Release 内存序原子地读取当前块元素计数
                currentChunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::RELAXED);

                // 当前块已满情况
                if (currentChunkElementCount >= self->elementChunkSize) {
                    // 以 Acquire-Release 内存序原子地交换当前块块有效状态为无效状态，并取回交换前的旧状态
                    // 仅当旧状态为有效状态时，当前线程成为本次封装的唯一赢家，负责将当前块入队
                    // 若旧状态已被他者抢先置为无效状态，他者已负责入队，则当前线程须跳过入队以避免重复入队
                    if (AtomicBool_exchange(&currentChunk->isValid, false, MemoryOrder::ACQUIRE_RELEASE)) {
                        // 将当前块指针入队（拷贝入队）当前块包的封装块队列
                        foundation::concurrent::ConcurrentUnboundedQueue_enqueue(currentBundle->sealedChunks, &currentChunk);
                    }

                    // 当前块指针已被拷贝入队（或已由他者入队），将当前块指针置空
                    currentChunk = nullptr;
                }
            } else {
                // 当前块无效情况，直接将当前块指针置空
                currentChunk = nullptr;
            }
        }

        // 当前块不存在情况（本来就不存在，又或者是被成功封装后被置空）
        if (!currentChunk) {
            // 当前块不存在，原地构造新块
            currentChunk = ElementChunk_construct(self->elementSize, self->elementChunkSize);
            if (!currentChunk) {
                return false;
            }

            // 将新构造的当前块以 Release 内存序原子地存储为当前块包的当前块
            AtomicPtr_store((void**) &currentBundle->currentChunk, (void*) currentChunk, MemoryOrder::RELEASE);

            // 新构造当前块，当前块元素计数为 0
            currentChunkElementCount = 0;
        }

        // 内存拷贝存储新写入的元素值，此时是单线程操作，无需特殊内存序
        // 同时仅原子地标记新写入的元素的状态为有效
        memcpy(currentChunk->elements + currentChunkElementCount * self->elementSize, inElement, self->elementSize);
        Atomic8_store((u8*) &currentChunk->elementStates[currentChunkElementCount], ElementChunk::ElementState::VALID, MemoryOrder::RELAXED);

        // 以 Release 内存序原子地存储当前块元素计数为当前块的的元素计数
        // 同时保证上面内存拷贝和状态标记写拥有一个共同的发布点
        Atomic64_store(&currentChunk->elementCount, currentChunkElementCount + 1, MemoryOrder::RELEASE);

        // 仅原子地设置合并等待标志，唤醒维护线程的工作交由定时轮询或消费者触发
        if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
            AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);
        }

        return true;
    }
    auto IMPList_addAll(IMPList* const self, const void* inElements, const usize inElementCount) -> bool {
        using namespace foundation::primitive;
        if (!self || !inElements) {
            return false;
        }

        if (inElementCount == 0) {
            return true;
        }

        // 尝试获取线程本地的元素块包，否则就原地创建一个
        auto* const currentBundle = (ElementChunkBundle*) foundation::concurrency::ThreadLocalTable_getOrCreate(&self->chunkBundleThreadLocalTable);
        if (!currentBundle) {
            return false;
        }

        // 仅原子地获取当前块包中的当前块，同时创建当前块元素计数变量
        auto* currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &currentBundle->currentChunk, MemoryOrder::RELAXED);
        usize currentChunkElementCount = 0;

        // 创建批量写入元素游标与剩余元素计数变量
        const auto* elementCursor = (const u8*) inElements;
        usize remainingElementCount = inElementCount;

        // 循环批量写入，直到剩余元素全部写完
        while (remainingElementCount > 0) {
            // 首先判断当前块存在情况
            if (currentChunk) {
                // 仅原子地判断当前块有效状态
                if (AtomicBool_load(&currentChunk->isValid, MemoryOrder::RELAXED)) {
                    // 当前块有效情况，仅原子地读取当前块元素计数
                    currentChunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::RELAXED);

                    // 当前块已满情况
                    if (currentChunkElementCount >= self->elementChunkSize) {
                        // 以 Acquire-Release 内存序原子地交换当前块块有效状态为无效状态，并取回交换前的旧状态
                        // 仅当旧状态为有效状态时，当前线程成为本次封装的唯一赢家，负责将当前块入队
                        // 若旧状态已被他者抢先置为无效状态，他者已负责入队，则当前线程须跳过入队以避免重复入队
                        if (AtomicBool_exchange(&currentChunk->isValid, false, MemoryOrder::ACQUIRE_RELEASE)) {
                            // 将当前块指针入队（拷贝入队）当前块包的封装块队列
                            foundation::concurrent::ConcurrentUnboundedQueue_enqueue(currentBundle->sealedChunks, &currentChunk);
                        }

                        // 当前块指针已被拷贝入队（或已由他者入队），将当前块指针置空
                        currentChunk = nullptr;
                    }
                } else {
                    // 当前块无效情况，直接将当前块指针置空
                    currentChunk = nullptr;
                }
            }

            // 当前块不存在情况（本来就不存在，又或者是被成功封装后被置空）
            if (!currentChunk) {
                // 当前块不存在，原地构造新块
                currentChunk = ElementChunk_construct(self->elementSize, self->elementChunkSize);
                if (!currentChunk) {
                    return false;
                }

                // 将新构造的当前块以 Release 内存序原子地存储为当前块包的当前块
                AtomicPtr_store((void**) &currentBundle->currentChunk, (void*) currentChunk, MemoryOrder::RELEASE);

                // 新构造当前块，当前块元素计数为 0
                currentChunkElementCount = 0;
            }

            // 计算当前块可用空间与本次批量写入的元素数
            const usize currentChunkAvailableSpace = self->elementChunkSize - currentChunkElementCount;
            const usize writeElementCount = remainingElementCount < currentChunkAvailableSpace ? remainingElementCount : currentChunkAvailableSpace;

            // 内存拷贝存储本次批量写入的元素值，此时是单线程操作，无需特殊内存序
            // 同时批量标记本次批量写入的元素状态为有效
            memcpy(currentChunk->elements + currentChunkElementCount * self->elementSize, elementCursor, writeElementCount * self->elementSize);
            memset(currentChunk->elementStates + currentChunkElementCount, ElementChunk::ElementState::VALID, writeElementCount);

            // 以 Release 内存序原子地存储当前块元素计数为当前块的元素计数
            // 同时保证上面内存拷贝和状态标记写拥有一个共同的发布点
            Atomic64_store(&currentChunk->elementCount, currentChunkElementCount + writeElementCount, MemoryOrder::RELEASE);

            // 推进元素游标与剩余元素计数变量
            elementCursor += writeElementCount * self->elementSize;
            remainingElementCount -= writeElementCount;
        }

        // 仅原子地设置合并等待标志，唤醒维护线程的工作交由定时轮询或消费者触发
        if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
            AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);
        }

        return true;
    }

    auto IMPList_remove(IMPList* const self, const void* const inElement) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;
        using namespace foundation::sync;

        if (!self || !inElement) {
            return false;
        }

        bool removed = false;

        Mutex_lock(&self->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::ACQUIRE);
            if (currentChunk) {
                if (AtomicBool_load(&currentChunk->isValid, MemoryOrder::ACQUIRE)) {
                    const usize currentChunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &currentChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = currentChunk->elements + (i * self->elementSize);
                        if (self->elementEquals ? self->elementEquals(candidateElement, inElement) : (memcmp(candidateElement, inElement, self->elementSize) == 0)) {
                            Atomic8_store((u8*) &currentChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&currentChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&currentChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }
            }

            auto* sealedChunkNode = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (sealedChunkNode) {
                auto* const sealedChunk = *((ElementChunk**) ConcurrentUnboundedQueueNode_data(sealedChunkNode));
                if (sealedChunk) {
                    const usize sealedChunkElementCount = (usize) Atomic64_load(&sealedChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < sealedChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &sealedChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = sealedChunk->elements + i * self->elementSize;
                        if (self->elementEquals ? self->elementEquals(candidateElement, inElement) : (memcmp(candidateElement, inElement, self->elementSize) == 0)) {
                            Atomic8_store((u8*) &sealedChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&sealedChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&sealedChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }

                sealedChunkNode = ConcurrentUnboundedQueueNode_next(sealedChunkNode);
            }
        }

        Mutex_unlock(&self->chunkBundleThreadLocalTable.lock);

        if (removed) {
            if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
                bool expected = false;
                if (AtomicBool_compareExchange(&self->isMergeSignaled, &expected, true, MemoryOrder::ACQUIRE_RELEASE)) {
                    ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return removed;
    }
    auto IMPList_removeIf(IMPList* const self, bool (*inPredicate)(const void*, void*), void* const inPredicateContext) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;
        using namespace foundation::sync;

        if (!self || !inPredicate) {
            return false;
        }

        bool removed = false;

        Mutex_lock(&self->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::ACQUIRE);
            if (currentChunk) {
                if (AtomicBool_load(&currentChunk->isValid, MemoryOrder::ACQUIRE)) {
                    const usize currentChunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &currentChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = currentChunk->elements + (i * self->elementSize);
                        if (inPredicate(candidateElement, inPredicateContext)) {
                            Atomic8_store((u8*) &currentChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&currentChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&currentChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }
            }

            auto* sealedChunkNode = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (sealedChunkNode) {
                auto* const sealedChunk = *((ElementChunk**) ConcurrentUnboundedQueueNode_data(sealedChunkNode));
                if (sealedChunk) {
                    const usize sealedChunkElementCount = (usize) Atomic64_load(&sealedChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < sealedChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &sealedChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = sealedChunk->elements + i * self->elementSize;
                        if (inPredicate(candidateElement, inPredicateContext)) {
                            Atomic8_store((u8*) &sealedChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&sealedChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&sealedChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }

                sealedChunkNode = ConcurrentUnboundedQueueNode_next(sealedChunkNode);
            }
        }

        Mutex_unlock(&self->chunkBundleThreadLocalTable.lock);

        if (removed) {
            if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
                bool expected = false;
                if (AtomicBool_compareExchange(&self->isMergeSignaled, &expected, true, MemoryOrder::ACQUIRE_RELEASE)) {
                    ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return removed;
    }
    auto IMPList_removeExcept(IMPList* const self, const void* const inElements, const usize inElementCount) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;
        using namespace foundation::sync;

        if (!self || !inElements || inElementCount == 0) {
            return false;
        }

        bool removed = false;

        Mutex_lock(&self->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::ACQUIRE);
            if (currentChunk) {
                if (AtomicBool_load(&currentChunk->isValid, MemoryOrder::ACQUIRE)) {
                    const usize currentChunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &currentChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = currentChunk->elements + (i * self->elementSize);

                        bool isFound = false;

                        for (usize j = 0; j < inElementCount; ++j) {
                            const void* const target = ((const u8*) inElements) + j * self->elementSize;
                            if (self->elementEquals ? self->elementEquals(candidateElement, target) : (memcmp(candidateElement, target, self->elementSize) == 0)) {
                                isFound = true;
                                break;
                            }
                        }

                        if (!isFound) {
                            Atomic8_store((u8*) &currentChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&currentChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&currentChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }
            }

            auto* sealedChunkNode = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (sealedChunkNode) {
                auto* const sealedChunk = *((ElementChunk**) ConcurrentUnboundedQueueNode_data(sealedChunkNode));
                if (sealedChunk) {
                    const usize sealedChunkElementCount = (usize) Atomic64_load(&sealedChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < sealedChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &sealedChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = sealedChunk->elements + i * self->elementSize;

                        bool isFound = false;

                        for (usize j = 0; j < inElementCount; ++j) {
                            const void* const target = (const u8*) inElements + j * self->elementSize;
                            if (self->elementEquals ? self->elementEquals(candidateElement, target) : (memcmp(candidateElement, target, self->elementSize) == 0)) {
                                isFound = true;
                                break;
                            }
                        }

                        if (!isFound) {
                            Atomic8_store((u8*) &sealedChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&sealedChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&sealedChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }

                sealedChunkNode = ConcurrentUnboundedQueueNode_next(sealedChunkNode);
            }
        }

        Mutex_unlock(&self->chunkBundleThreadLocalTable.lock);

        if (removed) {
            if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
                bool expected = false;
                if (AtomicBool_compareExchange(&self->isMergeSignaled, &expected, true, MemoryOrder::ACQUIRE_RELEASE)) {
                    ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return removed;
    }
    auto IMPList_removeExceptIf(IMPList* const self, bool (*inPredicate)(const void*, void*), void* const inPredicateContext) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;
        using namespace foundation::sync;

        if (!self || !inPredicate) {
            return false;
        }

        bool removed = false;

        Mutex_lock(&self->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::ACQUIRE);
            if (currentChunk) {
                if (AtomicBool_load(&currentChunk->isValid, MemoryOrder::ACQUIRE)) {
                    const usize currentChunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &currentChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = currentChunk->elements + (i * self->elementSize);
                        if (!inPredicate(candidateElement, inPredicateContext)) {
                            Atomic8_store((u8*) &currentChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&currentChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&currentChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }
            }

            auto* sealedChunkNode = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (sealedChunkNode) {
                auto* const sealedChunk = *((ElementChunk**) ConcurrentUnboundedQueueNode_data(sealedChunkNode));
                if (sealedChunk) {
                    const usize sealedChunkElementCount = (usize) Atomic64_load(&sealedChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < sealedChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &sealedChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        const void* const candidateElement = sealedChunk->elements + i * self->elementSize;
                        if (!inPredicate(candidateElement, inPredicateContext)) {
                            Atomic8_store((u8*) &sealedChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkRemoved = true;
                        }
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&sealedChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&sealedChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }

                sealedChunkNode = ConcurrentUnboundedQueueNode_next(sealedChunkNode);
            }
        }

        Mutex_unlock(&self->chunkBundleThreadLocalTable.lock);

        if (removed) {
            if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
                bool expected = false;
                if (AtomicBool_compareExchange(&self->isMergeSignaled, &expected, true, MemoryOrder::ACQUIRE_RELEASE)) {
                    ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return removed;
    }
    auto IMPList_removeAll(IMPList* const self) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;
        using namespace foundation::sync;

        if (!self) {
            return false;
        }

        bool removed = false;

        Mutex_lock(&self->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::ACQUIRE);
            if (currentChunk) {
                if (AtomicBool_load(&currentChunk->isValid, MemoryOrder::ACQUIRE)) {
                    const usize currentChunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < currentChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &currentChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        Atomic8_store((u8*) &currentChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                        chunkRemoved = true;
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&currentChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&currentChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }
            }

            auto* sealedChunkNode = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (sealedChunkNode) {
                auto* const sealedChunk = *((ElementChunk**) ConcurrentUnboundedQueueNode_data(sealedChunkNode));
                if (sealedChunk) {
                    const usize sealedChunkElementCount = (usize) Atomic64_load(&sealedChunk->elementCount, MemoryOrder::ACQUIRE);
                    bool chunkRemoved = false;
                    for (usize i = 0; i < sealedChunkElementCount; ++i) {
                        if (Atomic8_load((const u8*) &sealedChunk->elementStates[i], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            continue;
                        }

                        Atomic8_store((u8*) &sealedChunk->elementStates[i], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                        chunkRemoved = true;
                    }

                    if (chunkRemoved) {
                        if (!AtomicBool_load(&sealedChunk->isTainted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&sealedChunk->isTainted, true, MemoryOrder::RELEASE);
                        }

                        if (!AtomicBool_load(&bundle->isCorrupted, MemoryOrder::RELAXED)) {
                            AtomicBool_store(&bundle->isCorrupted, true, MemoryOrder::RELEASE);
                        }

                        removed = true;
                    }
                }

                sealedChunkNode = ConcurrentUnboundedQueueNode_next(sealedChunkNode);
            }
        }

        Mutex_unlock(&self->chunkBundleThreadLocalTable.lock);

        if (removed) {
            if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
                bool expected = false;
                if (AtomicBool_compareExchange(&self->isMergeSignaled, &expected, true, MemoryOrder::ACQUIRE_RELEASE)) {
                    ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
        }

        return true;
    }

    auto IMPList_discard(IMPList* const self, const void* inElement) -> bool {
        using namespace foundation::primitive;
        if (!self || !inElement) {
            return false;
        }

        // 尝试获取线程本地的丢弃缓存包，否则就原地创建一个
        auto* const currentBundle = (DiscardBufferBundle*) foundation::concurrency::ThreadLocalTable_getOrCreate(&self->discardBufferBundleThreadLocalTable);
        if (!currentBundle) {
            return false;
        }

        // 仅原子地获取当前缓存包中的当前缓存，同时创建当前缓存元素计数变量
        auto* currentBuffer = (DiscardBuffer*) AtomicPtr_load((void* const*) &currentBundle->currentBuffer, MemoryOrder::RELAXED);
        usize currentBufferElementCount = 0;

        // 首先判断当前缓存存在情况
        if (currentBuffer) {
            // 仅原子地判断当前缓存有效状态
            if (AtomicBool_load(&currentBuffer->isValid, MemoryOrder::RELAXED)) {
                // 当前缓存有效情况，仅原子地读取当前缓存元素计数
                currentBufferElementCount = (usize) Atomic64_load(&currentBuffer->elementCount, MemoryOrder::RELAXED);

                // 当前缓存已满情况
                if (currentBufferElementCount >= self->discardBufferSize) {
                    // 以 Acquire-Release 内存序原子地交换当前缓存有效状态为无效状态，并取回交换前的旧状态
                    // 仅当旧状态为有效状态时，当前线程成为本次封装的唯一赢家，负责将当前缓存入队
                    // 若旧状态已被他者抢先置为无效状态，他者已负责入队，则当前线程须跳过入队以避免重复入队
                    if (AtomicBool_exchange(&currentBuffer->isValid, false, MemoryOrder::ACQUIRE_RELEASE)) {
                        // 将当前缓存指针入队（拷贝入队）当前缓存包的封装缓存队列
                        foundation::concurrent::ConcurrentUnboundedQueue_enqueue(currentBundle->sealedBuffers, &currentBuffer);
                    }

                    // 当前缓存指针已被拷贝入队（或已由他者入队），将当前缓存指针置空
                    currentBuffer = nullptr;
                }
            } else {
                // 当前缓存无效情况，直接将当前缓存指针置空
                currentBuffer = nullptr;
            }
        }

        // 当前缓存不存在情况（本来就不存在，又或者是被成功封装后被置空）
        if (!currentBuffer) {
            // 尝试从回收队列复用
            void* const slot = foundation::concurrent::ConcurrentUnboundedQueue_dequeueSlot(currentBundle->recycledBuffers);
            if (slot) {
                currentBuffer = *(DiscardBuffer**) slot;
                free(slot);

                // 重置复用的缓存状态
                Atomic64_store(&currentBuffer->elementCount, 0, MemoryOrder::RELAXED);
                AtomicBool_store(&currentBuffer->isValid, true, MemoryOrder::RELAXED);
            } else {
                // 回收队列为空，原地构造新缓存
                currentBuffer = DiscardBuffer_construct(self->elementSize, self->discardBufferSize);
                if (!currentBuffer) {
                    return false;
                }
            }

            // 将新构造或复用的当前缓存以 Release 内存序原子地存储为当前缓存包的当前缓存
            AtomicPtr_store((void**) &currentBundle->currentBuffer, (void*) currentBuffer, MemoryOrder::RELEASE);

            // 新构造或复用的当前缓存，当前缓存元素计数为 0
            currentBufferElementCount = 0;
        }

        // 内存拷贝存储新写入的元素值，此时是单线程操作，无需特殊内存序
        memcpy(currentBuffer->elements + currentBufferElementCount * self->elementSize, inElement, self->elementSize);

        // 以 Release 内存序原子地存储当前缓存元素计数为当前缓存的元素计数
        // 同时保证上面内存拷贝拥有一个共同的发布点
        Atomic64_store(&currentBuffer->elementCount, currentBufferElementCount + 1, MemoryOrder::RELEASE);

        // 仅原子地设置合并等待标志，唤醒维护线程的工作交由定时轮询或消费者触发
        if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
            AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);
        }

        return true;
    }

    auto IMPList_find(const IMPList* const self, const void* const inElement) -> const void* {
        if (!self || !inElement) {
            return nullptr;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return nullptr;
        }

        const usize elementSize = self->elementSize;
        const u8* cursor = span->elements;
        for (usize i = 0; i < span->count; ++i) {
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return cursor;
            }

            cursor += elementSize;
        }

        return nullptr;
    }
    auto IMPList_contains(const IMPList* const self, const void* const inElement) -> bool {
        if (!self || !inElement) {
            return false;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return false;
        }

        const usize elementSize = self->elementSize;
        const u8* cursor = span->elements;
        for (usize i = 0; i < span->count; ++i) {
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return true;
            }

            cursor += elementSize;
        }

        return false;
    }
    auto IMPList_containsAll(const IMPList* const self, const void* const inElements, const usize inElementCount) -> bool {
        if (!self) {
            return false;
        }

        if (inElementCount == 0) {
            return true;
        }

        if (!inElements) {
            return false;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return false;
        }

        const usize elementSize = self->elementSize;
        const u8* const targets = (const u8*) inElements;
        for (usize t = 0; t < inElementCount; ++t) {
            const void* const target = targets + t * elementSize;

            bool isFound = false;

            const u8* cursor = span->elements;
            for (usize i = 0; i < span->count; ++i) {
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
    auto IMPList_firstIndexOf(const IMPList* const self, const void* const inElement) -> ssize {
        if (!self || !inElement) {
            return -1;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return -1;
        }

        const usize elementSize = self->elementSize;
        const u8* cursor = span->elements;
        for (usize i = 0; i < span->count; ++i) {
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return (ssize) i;
            }

            cursor += elementSize;
        }

        return -1;
    }
    auto IMPList_lastIndexOf(const IMPList* const self, const void* const inElement) -> ssize {
        if (!self || !inElement) {
            return -1;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);

        if (!span || span->count == 0) {
            return -1;
        }

        const usize elementSize = self->elementSize;
        usize i = span->count;
        while (i > 0) {
            i--;

            const u8* const cursor = span->elements + i * elementSize;
            const bool isMatch = self->elementEquals ? self->elementEquals(cursor, inElement) : (memcmp(cursor, inElement, elementSize) == 0);
            if (isMatch) {
                return (ssize) i;
            }
        }

        return -1;
    }
    auto IMPList_isEmpty(const IMPList* const self) -> bool {
        if (!self) {
            return true;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);
        return !span || span->count == 0;
    }
    auto IMPList_isNotEmpty(const IMPList* const self) -> bool {
        if (!self) {
            return false;
        }

        const auto* const span = (const IMPList::PresentElementsSpan*) foundation::primitive::AtomicPtr_load((void* const*) &self->presentElementsSpan, foundation::primitive::MemoryOrder::ACQUIRE);
        return span && span->count != 0;
    }

    auto IMPList_sort(
        IMPList* const self,
        const IMPList_ElementSortingStrategy inElementSortingStrategy, i32 (*inElementSortingComparator)(const void*, const void*)
    ) -> bool {
        if (!self) {
            return false;
        }

        self->elementSortingStrategy = inElementSortingStrategy;
        self->elementSortingComparator = inElementSortingComparator;

        IMPList_tryMerging(self);

        return IMPList_awaitMerging(self);
    }

    auto IMPList_tryMerging(IMPList* const self) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::concurrent;
        using namespace foundation::sync;

        if (!self) {
            return false;
        }

        bool expectedExecuting = false;
        if (!AtomicBool_compareExchange(&self->isMergeExecuting, &expectedExecuting, true, MemoryOrder::ACQUIRE_RELEASE)) {
            if (!AtomicBool_load(&self->isMergeSignaled, MemoryOrder::RELAXED)) {
                bool expectedSignaled = false;
                if (AtomicBool_compareExchange(&self->isMergeSignaled, &expectedSignaled, true, MemoryOrder::ACQUIRE_RELEASE)) {
                    ConflatedSignal_signal(&self->maintenanceTrigger);
                }
            }
            return true;
        }

        const u64 mergeStartTimeMs = Time_currentMs();

        AtomicBool_store(&self->isMergeSignaled, false, MemoryOrder::RELAXED);

        auto* sourceChunks = (ElementChunk**) malloc(sizeof(ElementChunk*) * 16);
        auto* sourceElementCounts = (usize*) malloc(sizeof(usize) * 16);
        if (!sourceChunks || !sourceElementCounts) {
            free(sourceChunks);
            free(sourceElementCounts);

            AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);

            Mutex_lock(&self->awaitMergingMutex);

            AtomicBool_store(&self->isMergeExecuting, false, MemoryOrder::RELEASE);

            ConditionalVar_broadcast(&self->awaitMergingCondition);

            Mutex_unlock(&self->awaitMergingMutex);

            ConflatedSignal_signal(&self->maintenanceTrigger);

            return false;
        }

        usize sourceCount = 0;
        usize sourceCapacity = 16;
        usize totalElementCount = 0;
        bool collectFailed = false;

        Mutex_lock(&self->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;

            AtomicBool_store(&bundle->isCorrupted, false, MemoryOrder::RELAXED);

            auto* node = ConcurrentUnboundedQueue_begin(bundle->sealedChunks);
            while (node) {
                auto* const chunk = *(ElementChunk**) ConcurrentUnboundedQueueNode_data(node);
                if (chunk) {
                    const usize chunkElementCount = (usize) Atomic64_load(&chunk->elementCount, MemoryOrder::ACQUIRE);
                    if (chunkElementCount > 0) {
                        if (sourceCount >= sourceCapacity) {
                            const usize newCapacity = sourceCapacity * 2;

                            auto* const newChunks = (ElementChunk**) realloc(sourceChunks, sizeof(ElementChunk*) * newCapacity);
                            if (!newChunks) {
                                collectFailed = true;
                                break;
                            }
                            sourceChunks = newChunks;

                            auto* const newElementCounts = (usize*) realloc(sourceElementCounts, sizeof(usize) * newCapacity);
                            if (!newElementCounts) {
                                collectFailed = true;
                                break;
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

                node = ConcurrentUnboundedQueueNode_next(node);
            }

            if (collectFailed) {
                break;
            }

            auto* const currentChunk = (ElementChunk*) AtomicPtr_load((void* const*) &bundle->currentChunk, MemoryOrder::ACQUIRE);
            if (currentChunk) {
                const bool isChunkValid = AtomicBool_load(&currentChunk->isValid, MemoryOrder::ACQUIRE);
                const usize chunkElementCount = (usize) Atomic64_load(&currentChunk->elementCount, MemoryOrder::ACQUIRE);
                if (isChunkValid && chunkElementCount > 0) {
                    if (sourceCount >= sourceCapacity) {
                        const usize newCapacity = sourceCapacity * 2;

                        auto* const newChunks = (ElementChunk**) realloc(sourceChunks, sizeof(ElementChunk*) * newCapacity);
                        if (!newChunks) {
                            collectFailed = true;
                            break;
                        }
                        sourceChunks = newChunks;

                        auto* const newElementCounts = (usize*) realloc(sourceElementCounts, sizeof(usize) * newCapacity);
                        if (!newElementCounts) {
                            collectFailed = true;
                            break;
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

        Mutex_unlock(&self->chunkBundleThreadLocalTable.lock);

        if (collectFailed) {
            free(sourceChunks);
            free(sourceElementCounts);

            AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);

            Mutex_lock(&self->awaitMergingMutex);

            AtomicBool_store(&self->isMergeExecuting, false, MemoryOrder::RELEASE);

            ConditionalVar_broadcast(&self->awaitMergingCondition);

            Mutex_unlock(&self->awaitMergingMutex);

            ConflatedSignal_signal(&self->maintenanceTrigger);

            return false;
        }

        u8* discardElements = nullptr;
        usize discardCount = 0;
        usize discardCapacity = 16;
        bool discardCollectFailed = false;

        if (self->discardBufferBundleThreadLocalTable.headEntry != nullptr) {
            discardElements = (u8*) malloc(self->elementSize * discardCapacity);
            if (!discardElements) {
                free(sourceChunks);
                free(sourceElementCounts);

                AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);

                Mutex_lock(&self->awaitMergingMutex);

                AtomicBool_store(&self->isMergeExecuting, false, MemoryOrder::RELEASE);

                ConditionalVar_broadcast(&self->awaitMergingCondition);

                Mutex_unlock(&self->awaitMergingMutex);

                ConflatedSignal_signal(&self->maintenanceTrigger);

                return false;
            }

            Mutex_lock(&self->discardBufferBundleThreadLocalTable.lock);

            for (auto* entry = self->discardBufferBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
                auto* const bundle = (DiscardBufferBundle*) entry->value;

                while (true) {
                    void* const slot = ConcurrentUnboundedQueue_dequeueSlot(bundle->sealedBuffers);
                    if (!slot) {
                        break;
                    }

                    auto* const buffer = *(DiscardBuffer**) slot;
                    free(slot);

                    const usize bufferElementCount = (usize) Atomic64_load(&buffer->elementCount, MemoryOrder::ACQUIRE);
                    bool isBufferCollected = true;

                    if (bufferElementCount > 0) {
                        if (discardCount + bufferElementCount > discardCapacity) {
                            usize newCapacity = discardCapacity;
                            while (newCapacity < discardCount + bufferElementCount) {
                                newCapacity <<= 1;
                            }

                            auto* const newDiscardElements = (u8*) realloc(discardElements, self->elementSize * newCapacity);
                            if (!newDiscardElements) {
                                isBufferCollected = false;
                            } else {
                                discardElements = newDiscardElements;
                                discardCapacity = newCapacity;
                            }
                        }

                        if (isBufferCollected) {
                            memcpy(discardElements + discardCount * self->elementSize, buffer->elements, bufferElementCount * self->elementSize);
                            discardCount += bufferElementCount;
                        }
                    }

                    if (isBufferCollected) {
                        Atomic64_store(&buffer->elementCount, 0, MemoryOrder::RELEASE);
                        AtomicBool_store(&buffer->isValid, true, MemoryOrder::RELEASE);
                        ConcurrentUnboundedQueue_enqueue(bundle->recycledBuffers, &buffer);
                    } else {
                        discardCollectFailed = true;
                        ConcurrentUnboundedQueue_enqueue(bundle->sealedBuffers, &buffer);
                        break;
                    }
                }

                if (discardCollectFailed) {
                    break;
                }

                auto* const currentBuffer = (DiscardBuffer*) AtomicPtr_load((void* const*) &bundle->currentBuffer, MemoryOrder::ACQUIRE);
                if (currentBuffer) {
                    auto* expected = (void*) currentBuffer;
                    if (AtomicPtr_compareExchange((void**) &bundle->currentBuffer, &expected, nullptr, MemoryOrder::ACQUIRE_RELEASE)) {
                        if (AtomicBool_exchange(&currentBuffer->isValid, false, MemoryOrder::ACQUIRE_RELEASE)) {
                            const usize bufferElementCount = (usize) Atomic64_load(&currentBuffer->elementCount, MemoryOrder::ACQUIRE);
                            bool isCurrentBufferCollected = true;

                            if (bufferElementCount > 0) {
                                if (discardCount + bufferElementCount > discardCapacity) {
                                    usize newCapacity = discardCapacity;
                                    while (newCapacity < discardCount + bufferElementCount) {
                                        newCapacity <<= 1;
                                    }

                                    auto* const newDiscardElements = (u8*) realloc(discardElements, self->elementSize * newCapacity);
                                    if (!newDiscardElements) {
                                        isCurrentBufferCollected = false;
                                    } else {
                                        discardElements = newDiscardElements;
                                        discardCapacity = newCapacity;
                                    }
                                }

                                if (isCurrentBufferCollected) {
                                    memcpy(discardElements + discardCount * self->elementSize, currentBuffer->elements, bufferElementCount * self->elementSize);
                                    discardCount += bufferElementCount;
                                }
                            }

                            if (isCurrentBufferCollected) {
                                Atomic64_store(&currentBuffer->elementCount, 0, MemoryOrder::RELEASE);
                                AtomicBool_store(&currentBuffer->isValid, true, MemoryOrder::RELEASE);
                                ConcurrentUnboundedQueue_enqueue(bundle->recycledBuffers, &currentBuffer);
                            } else {
                                discardCollectFailed = true;
                                ConcurrentUnboundedQueue_enqueue(bundle->sealedBuffers, &currentBuffer);
                            }
                        }
                    }
                }

                if (discardCollectFailed) {
                    break;
                }
            }

            Mutex_unlock(&self->discardBufferBundleThreadLocalTable.lock);
        }

        if (discardCollectFailed) {
            free(discardElements);
            free(sourceChunks);
            free(sourceElementCounts);

            AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);

            Mutex_lock(&self->awaitMergingMutex);

            AtomicBool_store(&self->isMergeExecuting, false, MemoryOrder::RELEASE);

            ConditionalVar_broadcast(&self->awaitMergingCondition);

            Mutex_unlock(&self->awaitMergingMutex);

            ConflatedSignal_signal(&self->maintenanceTrigger);

            return false;
        }

        u64* hashKeys = nullptr;
        u8* hashOccupied = nullptr;
        usize hashTableSize = 0;

        if (discardCount > 0 && self->elementEquals == nullptr && self->elementSize <= 8) {
            hashTableSize = 16;
            while (hashTableSize < discardCount * 2) {
                hashTableSize <<= 1;
            }
            hashKeys = (u64*) calloc(hashTableSize, sizeof(u64));
            hashOccupied = (u8*) calloc(hashTableSize, sizeof(u8));

            if (hashKeys != nullptr && hashOccupied != nullptr) {
                u64* const localHashKeys = hashKeys;
                u8* const localHashOccupied = hashOccupied;

                for (usize i = 0; i < discardCount; ++i) {
                    u64 elementKey = 0;
                    memcpy(&elementKey, discardElements + i * self->elementSize, self->elementSize);

                    usize hashSlotIndex = Internal::hashElementKey(elementKey) & (hashTableSize - 1);
                    while (localHashOccupied[hashSlotIndex]) {
                        if (localHashKeys[hashSlotIndex] == elementKey) {
                            break;
                        }

                        hashSlotIndex = (hashSlotIndex + 1) & (hashTableSize - 1);
                    }

                    localHashKeys[hashSlotIndex] = elementKey;
                    localHashOccupied[hashSlotIndex] = 1;
                }
            }
        }

        u8* newPresentElements = nullptr;
        usize newPresentElementCount = 0;

        if (totalElementCount > 0) {
            newPresentElements = (u8*) malloc(self->elementSize * totalElementCount);
            if (!newPresentElements) {
                free(discardElements);
                free(hashKeys);
                free(hashOccupied);
                free(sourceChunks);
                free(sourceElementCounts);

                AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);

                Mutex_lock(&self->awaitMergingMutex);

                AtomicBool_store(&self->isMergeExecuting, false, MemoryOrder::RELEASE);

                ConditionalVar_broadcast(&self->awaitMergingCondition);

                Mutex_unlock(&self->awaitMergingMutex);

                ConflatedSignal_signal(&self->maintenanceTrigger);

                return false;
            }

            usize writeIndex = 0;
            for (usize i = 0; i < sourceCount; ++i) {
                auto* const chunk = sourceChunks[i];
                const usize elementCount = sourceElementCounts[i];
                if (elementCount == 0) {
                    continue;
                }

                const bool chunkHasOldTombstone = AtomicBool_load(&chunk->isTainted, MemoryOrder::ACQUIRE);
                const bool needDiscardCheck = (discardCount > 0);

                if (!chunkHasOldTombstone && !needDiscardCheck) {
                    memcpy(newPresentElements + writeIndex * self->elementSize, chunk->elements, elementCount * self->elementSize);
                    writeIndex += elementCount;
                    continue;
                }

                bool chunkNewlyTainted = false;
                usize j = 0;

                while (j < elementCount) {
                    if (chunkHasOldTombstone) {
                        while (j < elementCount && Atomic8_load((const u8*) &chunk->elementStates[j], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            j++;
                        }

                        if (j == elementCount) {
                            break;
                        }
                    }

                    const usize runStart = j;
                    const u8* ptr = chunk->elements + j * self->elementSize;

                    while (j < elementCount) {
                        if (chunkHasOldTombstone && Atomic8_load((const u8*) &chunk->elementStates[j], MemoryOrder::RELAXED) != ElementChunk::ElementState::VALID) {
                            break;
                        }

                        bool isDiscarded = false;
                        if (needDiscardCheck) {
                            if (hashKeys != nullptr && hashOccupied != nullptr) {
                                u64* const localHashKeys = hashKeys;
                                u8* const localHashOccupied = hashOccupied;

                                u64 elementKey = 0;
                                memcpy(&elementKey, ptr, self->elementSize);

                                usize hashSlotIndex = Internal::hashElementKey(elementKey) & (hashTableSize - 1);
                                while (localHashOccupied[hashSlotIndex]) {
                                    if (localHashKeys[hashSlotIndex] == elementKey) {
                                        isDiscarded = true;
                                        break;
                                    }

                                    hashSlotIndex = (hashSlotIndex + 1) & (hashTableSize - 1);
                                }
                            } else {
                                for (usize k = 0; k < discardCount; ++k) {
                                    if (self->elementEquals ? self->elementEquals(ptr, discardElements + k * self->elementSize) : (memcmp(ptr, discardElements + k * self->elementSize, self->elementSize) == 0)) {
                                        isDiscarded = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (isDiscarded) {
                            Atomic8_store((u8*) &chunk->elementStates[j], ElementChunk::ElementState::TOMBSTONE, MemoryOrder::RELAXED);
                            chunkNewlyTainted = true;
                            break;
                        }

                        j++;
                        ptr += self->elementSize;
                    }

                    const usize runLength = j - runStart;
                    if (runLength > 0) {
                        memcpy(newPresentElements + writeIndex * self->elementSize, chunk->elements + runStart * self->elementSize, runLength * self->elementSize);
                        writeIndex += runLength;
                    }

                    if (j < elementCount) {
                        j++;
                    }
                }

                if (chunkNewlyTainted && !chunkHasOldTombstone) {
                    AtomicBool_store(&chunk->isTainted, true, MemoryOrder::RELEASE);
                }
            }

            if (writeIndex == 0) {
                free(newPresentElements);
                newPresentElements = nullptr;
            } else {
                if (writeIndex < totalElementCount) {
                    const usize shrunkByteSize = writeIndex * self->elementSize;
                    auto* const shrunk = (u8*) realloc(newPresentElements, shrunkByteSize);
                    if (shrunk) {
                        newPresentElements = shrunk;
                    }
                }

                if (self->elementSortingStrategy != IMPList_ElementSortingStrategy::UNORDERED && self->elementSortingComparator) {
                    qsort(newPresentElements, writeIndex, self->elementSize, self->elementSortingComparator);
                    if (self->elementSortingStrategy == IMPList_ElementSortingStrategy::DESCENDING) {
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

                newPresentElementCount = writeIndex;
            }
        }

        free(discardElements);
        free(hashKeys);
        free(hashOccupied);
        free(sourceChunks);
        free(sourceElementCounts);

        IMPList::PresentElementsSpan* newSpan = nullptr;
        if (newPresentElements) {
            newSpan = (IMPList::PresentElementsSpan*) malloc(sizeof(IMPList::PresentElementsSpan));
            if (!newSpan) {
                free(newPresentElements);

                AtomicBool_store(&self->isMergeSignaled, true, MemoryOrder::RELEASE);

                Mutex_lock(&self->awaitMergingMutex);

                AtomicBool_store(&self->isMergeExecuting, false, MemoryOrder::RELEASE);

                ConditionalVar_broadcast(&self->awaitMergingCondition);

                Mutex_unlock(&self->awaitMergingMutex);

                ConflatedSignal_signal(&self->maintenanceTrigger);

                return false;
            }
            newSpan->elements = newPresentElements;
            newSpan->count = newPresentElementCount;
        }

        Mutex_lock(&self->mergeMutex);

        IMPList_retirePresentElements(self);

        AtomicPtr_store((void**) &self->presentElementsSpan, (void*) newSpan, MemoryOrder::RELEASE);

        Mutex_unlock(&self->mergeMutex);

        Atomic64_fetchAdd(&self->mergeCount, 1, MemoryOrder::RELAXED);
        Atomic64_store(&self->lastMergeDurationMs, Time_currentMs() - mergeStartTimeMs, MemoryOrder::RELAXED);

        bool anyCorrupted = false;

        Mutex_lock(&self->chunkBundleThreadLocalTable.lock);

        for (auto* entry = self->chunkBundleThreadLocalTable.headEntry; entry; entry = entry->nextInOwner) {
            auto* const bundle = (ElementChunkBundle*) entry->value;
            if (AtomicBool_load(&bundle->isCorrupted, MemoryOrder::ACQUIRE)) {
                anyCorrupted = true;
                break;
            }
        }

        Mutex_unlock(&self->chunkBundleThreadLocalTable.lock);

        if (anyCorrupted) {
            bool expectedSignaled = false;
            if (AtomicBool_compareExchange(&self->isMergeSignaled, &expectedSignaled, true, MemoryOrder::ACQUIRE_RELEASE)) {
                ConflatedSignal_signal(&self->maintenanceTrigger);
            }
        }

        Mutex_lock(&self->awaitMergingMutex);

        AtomicBool_store(&self->isMergeExecuting, false, MemoryOrder::RELEASE);

        ConditionalVar_broadcast(&self->awaitMergingCondition);

        Mutex_unlock(&self->awaitMergingMutex);

        return true;
    }
    auto IMPList_awaitMerging(IMPList* const self) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::sync;

        if (!self) {
            return false;
        }

        ConflatedSignal_signal(&self->maintenanceTrigger);

        Mutex_lock(&self->awaitMergingMutex);

        while (AtomicBool_load(&self->isMergeExecuting, MemoryOrder::ACQUIRE) || AtomicBool_load(&self->isMergeSignaled, MemoryOrder::ACQUIRE)) {
            ConditionalVar_wait(&self->awaitMergingCondition, &self->awaitMergingMutex);
        }

        Mutex_unlock(&self->awaitMergingMutex);

        return true;
    }
    auto IMPList_timedAwaitMerging(IMPList* const self, const u64 inTimeoutMs) -> bool {
        using namespace foundation::primitive;
        using namespace foundation::sync;

        if (!self) {
            return false;
        }

        ConflatedSignal_signal(&self->maintenanceTrigger);

        Mutex_lock(&self->awaitMergingMutex);

        bool result = true;

        while (AtomicBool_load(&self->isMergeExecuting, MemoryOrder::ACQUIRE) || AtomicBool_load(&self->isMergeSignaled, MemoryOrder::ACQUIRE)) {
            if (inTimeoutMs == 0 || !ConditionalVar_timedWait(&self->awaitMergingCondition, &self->awaitMergingMutex, inTimeoutMs)) {
                result = false;
                break;
            }
        }

        Mutex_unlock(&self->awaitMergingMutex);

        return result;
    }
}