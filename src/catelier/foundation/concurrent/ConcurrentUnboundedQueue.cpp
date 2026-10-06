#include "ConcurrentUnboundedQueue.hpp"

#include <cstdlib>
#include <cstring>

#include "../primitive/Atomic.hpp"
#include "../concurrency/ThreadLocal.hpp"

namespace Catelier::src::foundation::concurrent {
    namespace {
        constexpr usize RETIRE_THRESHOLD = 64;
    }

    typedef struct ConcurrentUnboundedQueueNode {
        void* data;
        primitive::Atomic nextNode;
        ConcurrentUnboundedQueueNode* retiredNext;
    } ConcurrentUnboundedQueueNode;

    typedef struct ConcurrentUnboundedQueueThreadState {
        primitive::Atomic inDequeue;
        ConcurrentUnboundedQueueNode* retiredHead;
        usize retiredCount;
    } ConcurrentUnboundedQueueThreadState;

    typedef struct ConcurrentUnboundedQueue {
        primitive::Atomic head;
        primitive::Atomic tail;
        primitive::Atomic size;
        usize elementSize;
        concurrency::ThreadLocal threadState;
        bool valid;
    } ConcurrentUnboundedQueue;

    namespace {
        auto allocateNode(const usize elementSize, const void* inElement) -> ConcurrentUnboundedQueueNode* {
            auto* const node = (ConcurrentUnboundedQueueNode*) malloc(sizeof(ConcurrentUnboundedQueueNode));
            if (!node) {
                return nullptr;
            }

            node->data = nullptr;
            node->retiredNext = nullptr;

            if (!primitive::Atomic_init(&node->nextNode, 0)) {
                free(node);
                return nullptr;
            }

            if (!inElement) {
                return node;
            }

            node->data = malloc(elementSize);
            if (!node->data) {
                primitive::Atomic_destroy(&node->nextNode);
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

            primitive::Atomic_destroy(&node->nextNode);
            free(node);
        }
        auto linkNode(ConcurrentUnboundedQueue* self, ConcurrentUnboundedQueueNode* node) -> void {
            while (true) {
                const u64 tailValue = primitive::Atomic_load(&self->tail, primitive::MEMORY_ORDER_ACQUIRE);
                auto* const tail = (ConcurrentUnboundedQueueNode*) (addr) tailValue;
                const u64 nextValue = primitive::Atomic_load(&tail->nextNode, primitive::MEMORY_ORDER_ACQUIRE);
                auto* const next = (ConcurrentUnboundedQueueNode*) (addr) nextValue;

                if (tailValue == primitive::Atomic_load(&self->tail, primitive::MEMORY_ORDER_ACQUIRE)) {
                    if (!next) {
                        u64 expectedNext = nextValue;
                        if (primitive::Atomic_compareExchange(&tail->nextNode, &expectedNext, (u64) (addr) node, primitive::MEMORY_ORDER_RELEASE)) {
                            u64 expectedTail = tailValue;
                            primitive::Atomic_compareExchange(&self->tail, &expectedTail, (u64) (addr) node, primitive::MEMORY_ORDER_RELEASE);
                            primitive::Atomic_fetchAdd(&self->size, 1, primitive::MEMORY_ORDER_RELAXED);

                            return;
                        }
                    } else {
                        u64 expectedTail = tailValue;
                        primitive::Atomic_compareExchange(&self->tail, &expectedTail, nextValue, primitive::MEMORY_ORDER_RELEASE);
                    }
                }
            }
        }

        auto threadStateInit(void* userData) -> void* {
            (void) userData;

            auto* const state = (ConcurrentUnboundedQueueThreadState*) malloc(sizeof(ConcurrentUnboundedQueueThreadState));
            if (!state) {
                return nullptr;
            }

            state->retiredHead = nullptr;
            state->retiredCount = 0;

            if (!primitive::Atomic_init(&state->inDequeue, 0)) {
                free(state);
                return nullptr;
            }

            return state;
        }
        auto threadStateExit(void* value, void* userData) -> void {
            (void) userData;

            auto* const state = (ConcurrentUnboundedQueueThreadState*) value;
            if (!state) {
                return;
            }

            ConcurrentUnboundedQueueNode* node = state->retiredHead;
            while (node) {
                auto* const retiredNext = node->retiredNext;
                freeNode(node);
                node = retiredNext;
            }

            primitive::Atomic_destroy(&state->inDequeue);
            free(state);
        }

        auto scanCallback(void* value, void* userData) -> void {
            const auto* const state = (const ConcurrentUnboundedQueueThreadState*) value;
            auto* const allIdle = (bool*) userData;

            if (!state || !allIdle) {
                return;
            }

            if (primitive::Atomic_load(&state->inDequeue, primitive::MEMORY_ORDER_ACQUIRE) != 0) {
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

        if (!primitive::Atomic_init(&self->head, (u64) (addr) dummy)) {
            freeNode(dummy);
            free(self);
            return nullptr;
        }

        if (!primitive::Atomic_init(&self->tail, (u64) (addr) dummy)) {
            primitive::Atomic_destroy(&self->head);
            freeNode(dummy);
            free(self);
            return nullptr;
        }

        if (!primitive::Atomic_init(&self->size, 0)) {
            primitive::Atomic_destroy(&self->tail);
            primitive::Atomic_destroy(&self->head);
            freeNode(dummy);
            free(self);
            return nullptr;
        }

        if (!concurrency::ThreadLocal_construct(&self->threadState, threadStateInit, threadStateExit, threadStateExit, nullptr)) {
            primitive::Atomic_destroy(&self->size);
            primitive::Atomic_destroy(&self->tail);
            primitive::Atomic_destroy(&self->head);
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

        concurrency::ThreadLocal_destruct(&self->threadState);

        auto* head = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&self->head, primitive::MEMORY_ORDER_RELAXED);
        while (head) {
            auto* const nextNode = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&head->nextNode, primitive::MEMORY_ORDER_RELAXED);
            freeNode(head);
            head = nextNode;
        }

        primitive::Atomic_destroy(&self->size);
        primitive::Atomic_destroy(&self->tail);
        primitive::Atomic_destroy(&self->head);

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

        const auto* node = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&self->head, primitive::MEMORY_ORDER_RELAXED);
        if (node) {
            node = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&node->nextNode, primitive::MEMORY_ORDER_RELAXED);
        }

        while (node) {
            if (!ConcurrentUnboundedQueue_enqueue(newSelf, node->data)) {
                ConcurrentUnboundedQueue_destruct(newSelf);
                return nullptr;
            }

            node = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&node->nextNode, primitive::MEMORY_ORDER_RELAXED);
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

        if (!primitive::Atomic_init(&newSelf->head, primitive::Atomic_load(&self->head, primitive::MEMORY_ORDER_RELAXED))) {
            free(newSelf);
            return nullptr;
        }

        if (!primitive::Atomic_init(&newSelf->tail, primitive::Atomic_load(&self->tail, primitive::MEMORY_ORDER_RELAXED))) {
            primitive::Atomic_destroy(&newSelf->head);
            free(newSelf);
            return nullptr;
        }

        if (!primitive::Atomic_init(&newSelf->size, primitive::Atomic_load(&self->size, primitive::MEMORY_ORDER_RELAXED))) {
            primitive::Atomic_destroy(&newSelf->tail);
            primitive::Atomic_destroy(&newSelf->head);
            free(newSelf);
            return nullptr;
        }

        if (!concurrency::ThreadLocal_move(&newSelf->threadState, &self->threadState)) {
            primitive::Atomic_destroy(&newSelf->size);
            primitive::Atomic_destroy(&newSelf->tail);
            primitive::Atomic_destroy(&newSelf->head);
            free(newSelf);
            return nullptr;
        }

        newSelf->valid = true;

        auto* const dummy = allocateNode(self->elementSize, nullptr);
        if (!dummy) {
            self->valid = false;
            return newSelf;
        }

        primitive::Atomic_store(&self->head, (u64) (addr) dummy, primitive::MEMORY_ORDER_RELAXED);
        primitive::Atomic_store(&self->tail, (u64) (addr) dummy, primitive::MEMORY_ORDER_RELAXED);
        primitive::Atomic_store(&self->size, 0, primitive::MEMORY_ORDER_RELAXED);

        if (!concurrency::ThreadLocal_construct(&self->threadState, threadStateInit, threadStateExit, threadStateExit, nullptr)) {
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

        auto* const state = (ConcurrentUnboundedQueueThreadState*) concurrency::ThreadLocal_getOrCreate(&self->threadState);
        if (!state) {
            return nullptr;
        }

        primitive::Atomic_store(&state->inDequeue, 1, primitive::MEMORY_ORDER_SEQ_CST);

        while (true) {
            const u64 headValue = primitive::Atomic_load(&self->head, primitive::MEMORY_ORDER_ACQUIRE);
            auto* const head = (ConcurrentUnboundedQueueNode*) (addr) headValue;
            const u64 tailValue = primitive::Atomic_load(&self->tail, primitive::MEMORY_ORDER_ACQUIRE);
            auto* const tail = (ConcurrentUnboundedQueueNode*) (addr) tailValue;
            const u64 nextValue = primitive::Atomic_load(&head->nextNode, primitive::MEMORY_ORDER_ACQUIRE);
            auto* const next = (ConcurrentUnboundedQueueNode*) (addr) nextValue;

            if (headValue == primitive::Atomic_load(&self->head, primitive::MEMORY_ORDER_ACQUIRE)) {
                if (head == tail) {
                    if (!next) {
                        primitive::Atomic_store(&state->inDequeue, 0, primitive::MEMORY_ORDER_SEQ_CST);
                        return nullptr;
                    }

                    u64 expectedTail = tailValue;
                    primitive::Atomic_compareExchange(&self->tail, &expectedTail, nextValue, primitive::MEMORY_ORDER_RELEASE);
                } else {
                    u64 expectedHead = headValue;
                    if (primitive::Atomic_compareExchange(&self->head, &expectedHead, nextValue, primitive::MEMORY_ORDER_ACQ_REL)) {
                        void* const data = next->data;
                        next->data = nullptr;

                        primitive::Atomic_store(&state->inDequeue, 0, primitive::MEMORY_ORDER_SEQ_CST);

                        head->retiredNext = state->retiredHead;
                        state->retiredHead = head;
                        state->retiredCount++;

                        if (state->retiredCount >= RETIRE_THRESHOLD) {
                            bool allIdle = true;
                            concurrency::ThreadLocal_forEach(&self->threadState, scanCallback, &allIdle);

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

                        primitive::Atomic_fetchSub(&self->size, 1, primitive::MEMORY_ORDER_RELAXED);

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

        const auto* const dummy = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&self->head, primitive::MEMORY_ORDER_ACQUIRE);
        if (!dummy) {
            return nullptr;
        }

        return (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&dummy->nextNode, primitive::MEMORY_ORDER_ACQUIRE);
    }
    auto ConcurrentUnboundedQueue_tail(const ConcurrentUnboundedQueue* self) -> ConcurrentUnboundedQueueNode* {
        if (!self || !self->valid) {
            return nullptr;
        }

        return (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&self->tail, primitive::MEMORY_ORDER_ACQUIRE);
    }
    auto ConcurrentUnboundedQueue_get(const ConcurrentUnboundedQueue* self, const usize index) -> ConcurrentUnboundedQueueNode* {
        if (!self || !self->valid) {
            return nullptr;
        }

        const auto* targetNode = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&self->head, primitive::MEMORY_ORDER_ACQUIRE);

        if (targetNode) {
            targetNode = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&targetNode->nextNode, primitive::MEMORY_ORDER_ACQUIRE);
        }

        for (usize i = 0; i < index && targetNode; ++i) {
            targetNode = (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&targetNode->nextNode, primitive::MEMORY_ORDER_ACQUIRE);
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

        return (usize) primitive::Atomic_load(&self->size, primitive::MEMORY_ORDER_RELAXED);
    }

    auto ConcurrentUnboundedQueue_isEmpty(const ConcurrentUnboundedQueue* self) -> bool {
        if (!self || !self->valid) {
            return true;
        }

        return primitive::Atomic_load(&self->size, primitive::MEMORY_ORDER_RELAXED) == 0;
    }

    auto ConcurrentUnboundedQueueNode_next(const ConcurrentUnboundedQueueNode* node) -> ConcurrentUnboundedQueueNode* {
        if (!node) {
            return nullptr;
        }

        return (ConcurrentUnboundedQueueNode*) (addr) primitive::Atomic_load(&node->nextNode, primitive::MEMORY_ORDER_ACQUIRE);
    }
    auto ConcurrentUnboundedQueueNode_data(const ConcurrentUnboundedQueueNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->data;
    }
}