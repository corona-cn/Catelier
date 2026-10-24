#include "ConcurrentUnboundedQueue.hpp"

#include <cstdlib>
#include <cstring>

#include "../primitive/Atomic.hpp"
#include "../concurrency/ThreadLocalTable.hpp"

namespace Catelier::src::foundation::concurrent {
    namespace {
        constexpr usize RETIRE_THRESHOLD = 64;
    }

    typedef struct ConcurrentUnboundedQueueNode {
        void* data;
        ConcurrentUnboundedQueueNode* nextNode;
        ConcurrentUnboundedQueueNode* retiredNext;
    } ConcurrentUnboundedQueueNode;

    typedef struct ConcurrentUnboundedQueueThreadState {
        bool inDequeue;
        ConcurrentUnboundedQueueNode* retiredHead;
        usize retiredCount;
    } ConcurrentUnboundedQueueThreadState;

    typedef struct ConcurrentUnboundedQueue {
        ConcurrentUnboundedQueueNode* head;
        ConcurrentUnboundedQueueNode* tail;
        u64 size;
        usize elementSize;
        concurrency::ThreadLocalTable threadState;
        bool valid;
    } ConcurrentUnboundedQueue;

    namespace {
        auto allocateNode(const usize elementSize, const void* inElement) -> ConcurrentUnboundedQueueNode* {
            auto* const node = (ConcurrentUnboundedQueueNode*) malloc(sizeof(ConcurrentUnboundedQueueNode));
            if (!node) {
                return nullptr;
            }

            node->data = nullptr;
            node->nextNode = nullptr;
            node->retiredNext = nullptr;

            if (!inElement) {
                return node;
            }

            node->data = malloc(elementSize);
            if (!node->data) {
                free(node);
                return nullptr;
            }

            memcpy(node->data, inElement, elementSize);

            return node;
        }
        auto freeNode(ConcurrentUnboundedQueueNode* node) -> void {
            if (!node) {
                return;
            }

            if (node->data) {
                free(node->data);
            }

            free(node);
        }
        auto linkNode(ConcurrentUnboundedQueue* self, ConcurrentUnboundedQueueNode* node) -> void {
            while (true) {
                auto* const tail = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->tail, primitive::ACQUIRE);
                auto* const next = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &tail->nextNode, primitive::ACQUIRE);

                if (tail == (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->tail, primitive::ACQUIRE)) {
                    if (!next) {
                        auto* expectedNext = (void*) next;
                        if (primitive::AtomicPtr_compareExchange((void**) &tail->nextNode, &expectedNext, (void*) node, primitive::RELEASE)) {
                            auto* expectedTail = (void*) tail;
                            primitive::AtomicPtr_compareExchange((void**) &self->tail, &expectedTail, (void*) node, primitive::RELEASE);
                            primitive::Atomic64_fetchAdd(&self->size, 1, primitive::RELAXED);
                            return;
                        }
                    } else {
                        auto* expectedTail = (void*) tail;
                        primitive::AtomicPtr_compareExchange((void**) &self->tail, &expectedTail, (void*) next, primitive::RELEASE);
                    }
                }
            }
        }
    }

    namespace {
        auto freeThreadState(ConcurrentUnboundedQueueThreadState* const state) -> void {
            if (!state) {
                return;
            }

            ConcurrentUnboundedQueueNode* node = state->retiredHead;
            while (node) {
                auto* const retiredNext = node->retiredNext;
                freeNode(node);
                node = retiredNext;
            }

            free(state);
        }

        auto threadStateCreateCallback(void* hostContext) -> void* {
            (void) hostContext;

            auto* const state = (ConcurrentUnboundedQueueThreadState*) malloc(sizeof(ConcurrentUnboundedQueueThreadState));
            if (!state) {
                return nullptr;
            }

            state->inDequeue = false;
            state->retiredHead = nullptr;
            state->retiredCount = 0;

            return state;
        }
        auto threadStateThreadExitCallback(void* value, void* hostContext) -> void* {
            (void) hostContext;

            freeThreadState((ConcurrentUnboundedQueueThreadState*) value);

            return nullptr;
        }
        auto threadStateDestroyCallback(void* value, void* hostContext) -> void {
            (void) hostContext;

            freeThreadState((ConcurrentUnboundedQueueThreadState*) value);
        }

        auto scanCallback(void* value, bool isThreadExited, void* userData) -> void {
            const auto* const state = (const ConcurrentUnboundedQueueThreadState*) value;
            auto* const allIdle = (bool*) userData;

            if (!state || !allIdle) {
                return;
            }

            if (isThreadExited) {
                return;
            }

            if (primitive::AtomicBool_load(&state->inDequeue, primitive::ACQUIRE)) {
                *allIdle = false;
            }
        }
    }

    auto ConcurrentUnboundedQueue_construct(const usize inElementSize) -> ConcurrentUnboundedQueue* {
        if (inElementSize == 0) {
            return nullptr;
        }

        auto* const self = (ConcurrentUnboundedQueue*) malloc(sizeof(ConcurrentUnboundedQueue));
        if (!self) {
            return nullptr;
        }

        self->elementSize = inElementSize;
        self->valid = false;

        auto* const dummy = allocateNode(inElementSize, nullptr);
        if (!dummy) {
            free(self);
            return nullptr;
        }

        self->head = dummy;
        self->tail = dummy;
        self->size = 0;

        if (!concurrency::ThreadLocalTable_construct(&self->threadState, threadStateCreateCallback, threadStateThreadExitCallback, threadStateDestroyCallback, nullptr)) {
            freeNode(dummy);
            free(self);
            return nullptr;
        }

        self->valid = true;

        return self;
    }
    auto ConcurrentUnboundedQueue_destruct(ConcurrentUnboundedQueue* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        concurrency::ThreadLocalTable_destruct(&self->threadState);

        auto* head = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->head, primitive::RELAXED);
        while (head) {
            auto* const nextNode = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &head->nextNode, primitive::RELAXED);
            freeNode(head);
            head = nextNode;
        }

        free(self);

        return true;
    }
    auto ConcurrentUnboundedQueue_copy(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueue* {
        if (!self || !self->valid) {
            return nullptr;
        }

        auto* const newSelf = ConcurrentUnboundedQueue_construct(self->elementSize);
        if (!newSelf) {
            return nullptr;
        }

        const auto* node = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->head, primitive::RELAXED);
        if (node) {
            node = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
        }

        while (node) {
            if (!ConcurrentUnboundedQueue_enqueue(newSelf, node->data)) {
                ConcurrentUnboundedQueue_destruct(newSelf);
                return nullptr;
            }

            node = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
        }

        return newSelf;
    }
    auto ConcurrentUnboundedQueue_move(ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueue* {
        if (!self || !self->valid) {
            return nullptr;
        }

        auto* const newSelf = (ConcurrentUnboundedQueue*) malloc(sizeof(ConcurrentUnboundedQueue));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->elementSize = self->elementSize;
        newSelf->valid = false;

        newSelf->head = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->head, primitive::RELAXED);
        newSelf->tail = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->tail, primitive::RELAXED);
        newSelf->size = primitive::Atomic64_load(&self->size, primitive::RELAXED);

        if (!concurrency::ThreadLocalTable_move(&newSelf->threadState, &self->threadState)) {
            free(newSelf);
            return nullptr;
        }

        newSelf->valid = true;

        auto* const dummy = allocateNode(self->elementSize, nullptr);
        if (!dummy) {
            self->valid = false;
            return newSelf;
        }

        primitive::AtomicPtr_store((void**) &self->head, (void*) dummy, primitive::RELAXED);
        primitive::AtomicPtr_store((void**) &self->tail, (void*) dummy, primitive::RELAXED);
        primitive::Atomic64_store(&self->size, 0, primitive::RELAXED);

        if (!concurrency::ThreadLocalTable_construct(&self->threadState, threadStateCreateCallback, threadStateThreadExitCallback, threadStateDestroyCallback, nullptr)) {
            freeNode(dummy);
            self->valid = false;
            return newSelf;
        }

        return newSelf;
    }

    auto ConcurrentUnboundedQueue_enqueue(ConcurrentUnboundedQueue* self, const void* inElement) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        auto* const node = allocateNode(self->elementSize, inElement);
        if (!node) {
            return false;
        }

        linkNode(self, node);

        return true;
    }
    auto ConcurrentUnboundedQueue_enqueueSlot(ConcurrentUnboundedQueue* self) -> void* {
        if (!self || !self->valid) {
            return nullptr;
        }

        auto* const node = allocateNode(self->elementSize, nullptr);
        if (!node) {
            return nullptr;
        }

        node->data = malloc(self->elementSize);
        if (!node->data) {
            freeNode(node);
            return nullptr;
        }

        linkNode(self, node);

        return node->data;
    }

    auto ConcurrentUnboundedQueue_dequeue(ConcurrentUnboundedQueue* self) -> bool {
        void* const data = ConcurrentUnboundedQueue_dequeueSlot(self);
        if (!data) {
            return false;
        }

        free(data);

        return true;
    }
    auto ConcurrentUnboundedQueue_dequeueSlot(ConcurrentUnboundedQueue* self) -> void* {
        if (!self || !self->valid) {
            return nullptr;
        }

        auto* const state = (ConcurrentUnboundedQueueThreadState*) concurrency::ThreadLocalTable_getOrCreate(&self->threadState);
        if (!state) {
            return nullptr;
        }

        primitive::AtomicBool_store(&state->inDequeue, true, primitive::SEQUENTIAL_CONSISTENT);

        while (true) {
            auto* const head = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->head, primitive::ACQUIRE);
            auto* const tail = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->tail, primitive::ACQUIRE);
            auto* const next = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &head->nextNode, primitive::ACQUIRE);

            if (head == (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->head, primitive::ACQUIRE)) {
                if (head == tail) {
                    if (!next) {
                        primitive::AtomicBool_store(&state->inDequeue, false, primitive::SEQUENTIAL_CONSISTENT);
                        return nullptr;
                    }

                    auto* expectedTail = (void*) tail;
                    primitive::AtomicPtr_compareExchange((void**) &self->tail, &expectedTail, (void*) next, primitive::RELEASE);
                } else {
                    auto* expectedHead = (void*) head;
                    if (primitive::AtomicPtr_compareExchange((void**) &self->head, &expectedHead, (void*) next, primitive::ACQUIRE_RELEASE)) {
                        void* const data = next->data;
                        next->data = nullptr;

                        primitive::AtomicBool_store(&state->inDequeue, false, primitive::SEQUENTIAL_CONSISTENT);

                        head->retiredNext = state->retiredHead;
                        state->retiredHead = head;
                        state->retiredCount++;

                        if (state->retiredCount >= RETIRE_THRESHOLD) {
                            bool allIdle = true;
                            concurrency::ThreadLocalTable_forEach(&self->threadState, scanCallback, &allIdle);

                            if (allIdle) {
                                ConcurrentUnboundedQueueNode* retired = state->retiredHead;
                                while (retired) {
                                    auto* const retiredNext = retired->retiredNext;
                                    freeNode(retired);
                                    retired = retiredNext;
                                }

                                state->retiredHead = nullptr;
                                state->retiredCount = 0;
                            }
                        }

                        primitive::Atomic64_fetchSub(&self->size, 1, primitive::RELAXED);

                        return data;
                    }
                }
            }
        }
    }
    auto ConcurrentUnboundedQueue_clear(ConcurrentUnboundedQueue* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        bool removed = false;
        while (true) {
            void* const slot = ConcurrentUnboundedQueue_dequeueSlot(self);
            if (!slot) {
                break;
            }

            free(slot);
            removed = true;
        }

        return removed;
    }

    auto ConcurrentUnboundedQueue_head(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueueNode* {
        if (!self || !self->valid) {
            return nullptr;
        }

        const auto* const dummy = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->head, primitive::ACQUIRE);
        if (!dummy) {
            return nullptr;
        }

        return (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &dummy->nextNode, primitive::ACQUIRE);
    }
    auto ConcurrentUnboundedQueue_tail(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueueNode* {
        if (!self || !self->valid) {
            return nullptr;
        }

        return (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->tail, primitive::ACQUIRE);
    }
    auto ConcurrentUnboundedQueue_get(const ConcurrentUnboundedQueue* self, const usize index) -> ConcurrentUnboundedQueueNode* {
        if (!self || !self->valid) {
            return nullptr;
        }

        const auto* targetNode = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &self->head, primitive::ACQUIRE);

        if (targetNode) {
            targetNode = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &targetNode->nextNode, primitive::ACQUIRE);
        }

        for (usize i = 0; i < index && targetNode; ++i) {
            targetNode = (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &targetNode->nextNode, primitive::ACQUIRE);
        }

        return (ConcurrentUnboundedQueueNode*) targetNode;
    }

    auto ConcurrentUnboundedQueue_begin(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueueNode* {
        return ConcurrentUnboundedQueue_head(self);
    }
    auto ConcurrentUnboundedQueue_end() -> ConcurrentUnboundedQueueNode* {
        return nullptr;
    }

    auto ConcurrentUnboundedQueue_size(const ConcurrentUnboundedQueue* self) -> usize {
        if (!self || !self->valid) {
            return 0;
        }

        return (usize) primitive::Atomic64_load(&self->size, primitive::RELAXED);
    }

    auto ConcurrentUnboundedQueue_isEmpty(const ConcurrentUnboundedQueue* self) -> bool {
        if (!self || !self->valid) {
            return true;
        }

        return primitive::Atomic64_load(&self->size, primitive::RELAXED) == 0;
    }

    auto ConcurrentUnboundedQueueNode_next(const ConcurrentUnboundedQueueNode* node) -> ConcurrentUnboundedQueueNode* {
        if (!node) {
            return nullptr;
        }

        return (ConcurrentUnboundedQueueNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::ACQUIRE);
    }
    auto ConcurrentUnboundedQueueNode_data(const ConcurrentUnboundedQueueNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->data;
    }
}