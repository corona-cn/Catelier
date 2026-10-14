#include "ConcurrentChainHashMap.hpp"

#include <cstdlib>
#include <cstring>

#include "../primitive/Atomic.hpp"
#include "../primitive/Mutex.hpp"

namespace Catelier::src::foundation::concurrent {
    namespace {
        constexpr usize DEFAULT_SEGMENT_COUNT = 16;
        constexpr usize DEFAULT_INITIAL_CAPACITY = 4;
        constexpr usize LOAD_FACTOR_NUMERATOR = 3;
        constexpr usize LOAD_FACTOR_DENOMINATOR = 4;
        constexpr usize HISTORY_SIZE = 4;
    }

    typedef struct ConcurrentChainHashMapNode {
        void* key;
        void* value;
        ConcurrentChainHashMapNode* nextNode;
    } ConcurrentChainHashMapNode;

    typedef struct ConcurrentChainHashMapBucketArray {
        ConcurrentChainHashMapNode** buckets;
        usize bucketCount;
    } ConcurrentChainHashMapBucketArray;

    typedef struct ConcurrentChainHashMapSegment {
        primitive::Mutex mutex;
        ConcurrentChainHashMapBucketArray* bucketArray;
        ConcurrentChainHashMapBucketArray* history[HISTORY_SIZE];
        usize historyCount;
        u64 size;
    } ConcurrentChainHashMapSegment;

    typedef struct ConcurrentChainHashMap {
        ConcurrentChainHashMapSegment* segments;
        usize segmentCount;
        usize keySize;
        usize valueSize;
        u64 (*hash)(const void*);
        bool (*keyEquals)(const void*, const void*);
        bool valid;
    } ConcurrentChainHashMap;

    auto ConcurrentChainHashMap_construct(const usize segmentCount, const usize initialCapacity, const usize expectedElementCount, const usize inKeySize, const usize inValueSize, u64 (*hash)(const void*), bool (*keyEquals)(const void*, const void*)) -> ConcurrentChainHashMap* {
        if (inKeySize == 0 || inValueSize == 0 || !hash || !keyEquals) {
            return nullptr;
        }

        // 段数向上取整到 2 的幂
        const usize actualSegmentCount = segmentCount == 0 ? DEFAULT_SEGMENT_COUNT : segmentCount;
        usize effectiveSegmentCount = 1;
        while (effectiveSegmentCount < actualSegmentCount) {
            effectiveSegmentCount <<= 1;
        }

        // 基础有效容量
        usize effectiveCapacity = initialCapacity == 0 ? DEFAULT_INITIAL_CAPACITY : initialCapacity;

        // 每段承载 expectedElementCount / segmentCount 个元素，向上取整
        // 需要的桶数 = 每段元素数 / 负载因子
        if (expectedElementCount > 0) {
            usize perSegmentElement = expectedElementCount / effectiveSegmentCount;
            if (perSegmentElement * effectiveSegmentCount < expectedElementCount) {
                perSegmentElement++;
            }

            usize neededBucketCount = perSegmentElement * LOAD_FACTOR_DENOMINATOR / LOAD_FACTOR_NUMERATOR;
            if (neededBucketCount < DEFAULT_INITIAL_CAPACITY) {
                neededBucketCount = DEFAULT_INITIAL_CAPACITY;
            }

            if (neededBucketCount > effectiveCapacity) {
                effectiveCapacity = neededBucketCount;
            }
        }

        // 每段初始桶数向上取整到 2 的幂
        usize effectiveInitialBucketCount = 1;
        while (effectiveInitialBucketCount < effectiveCapacity) {
            effectiveInitialBucketCount <<= 1;
        }

        auto* const self = (ConcurrentChainHashMap*) malloc(sizeof(ConcurrentChainHashMap));
        if (!self) {
            return nullptr;
        }

        self->segments = (ConcurrentChainHashMapSegment*) malloc(sizeof(ConcurrentChainHashMapSegment) * effectiveSegmentCount);
        if (!self->segments) {
            free(self);
            return nullptr;
        }

        self->segmentCount = effectiveSegmentCount;
        self->keySize = inKeySize;
        self->valueSize = inValueSize;
        self->hash = hash;
        self->keyEquals = keyEquals;
        self->valid = false;

        // 逐段初始化
        for (usize i = 0; i < effectiveSegmentCount; ++i) {
            auto* const segment = &self->segments[i];

            for (usize h = 0; h < HISTORY_SIZE; ++h) {
                segment->history[h] = nullptr;
            }
            segment->historyCount = 0;

            // 初始化段互斥锁
            if (!primitive::Mutex_init(&segment->mutex)) {
                for (usize j = 0; j < i; ++j) {
                    auto* const prevSegment = &self->segments[j];
                    auto* const prevBucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &prevSegment->bucketArray, primitive::RELAXED);

                    free(prevBucketArray->buckets);
                    free(prevBucketArray);

                    primitive::Mutex_destroy(&prevSegment->mutex);
                }

                free(self->segments);
                free(self);

                return nullptr;
            }

            segment->size = 0;

            // 分配桶数组结构体
            auto* const bucketArray = (ConcurrentChainHashMapBucketArray*) malloc(sizeof(ConcurrentChainHashMapBucketArray));
            if (!bucketArray) {
                primitive::Mutex_destroy(&segment->mutex);

                for (usize j = 0; j < i; ++j) {
                    auto* const prevSegment = &self->segments[j];
                    auto* const prevBucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &prevSegment->bucketArray, primitive::RELAXED);

                    free(prevBucketArray->buckets);
                    free(prevBucketArray);

                    primitive::Mutex_destroy(&prevSegment->mutex);
                }

                free(self->segments);
                free(self);

                return nullptr;
            }

            // 分配桶数组本体
            bucketArray->buckets = (ConcurrentChainHashMapNode**) calloc(effectiveInitialBucketCount, sizeof(ConcurrentChainHashMapNode*));
            if (!bucketArray->buckets) {
                free(bucketArray);

                primitive::Mutex_destroy(&segment->mutex);

                for (usize j = 0; j < i; ++j) {
                    auto* const prevSegment = &self->segments[j];
                    auto* const prevBucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &prevSegment->bucketArray, primitive::RELAXED);

                    free(prevBucketArray->buckets);
                    free(prevBucketArray);

                    primitive::Mutex_destroy(&prevSegment->mutex);
                }

                free(self->segments);
                free(self);

                return nullptr;
            }

            bucketArray->bucketCount = effectiveInitialBucketCount;

            // 把桶数组发布到段上，后续所有读写都通过这个原子指针
            segment->bucketArray = bucketArray;
        }

        self->valid = true;

        return self;
    }
    auto ConcurrentChainHashMap_destruct(ConcurrentChainHashMap* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        auto* const segments = self->segments;
        const usize segmentCount = self->segmentCount;

        self->segments = nullptr;
        self->segmentCount = 0;

        for (usize i = 0; i < segmentCount; ++i) {
            auto* const segment = &segments[i];

            auto* const currentArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);

            for (usize j = 0; j < currentArray->bucketCount; ++j) {
                auto* node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &currentArray->buckets[j], primitive::RELAXED);
                while (node) {
                    auto* const nextNode = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);

                    if (node->key) {
                        free(node->key);
                    }

                    if (node->value) {
                        free(node->value);
                    }

                    free(node);

                    node = nextNode;
                }
            }

            free(currentArray->buckets);
            free(currentArray);

            for (usize h = 0; h < segment->historyCount; ++h) {
                free(segment->history[h]->buckets);
                free(segment->history[h]);
            }

            primitive::Mutex_destroy(&segment->mutex);
        }

        free(segments);
        free(self);

        return true;
    }

    auto ConcurrentChainHashMap_copy(const ConcurrentChainHashMap* self) -> ConcurrentChainHashMap* {
        if (!self || !self->valid) {
            return nullptr;
        }

        const auto* const firstArray = (const ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &self->segments[0].bucketArray, primitive::RELAXED);
        auto* const newSelf = ConcurrentChainHashMap_construct(self->segmentCount, firstArray->bucketCount, 0, self->keySize, self->valueSize, self->hash, self->keyEquals);
        if (!newSelf) {
            return nullptr;
        }

        for (usize i = 0; i < self->segmentCount; ++i) {
            auto* const srcSegment = const_cast<ConcurrentChainHashMapSegment*>(&self->segments[i]);
            auto* const dstSegment = &newSelf->segments[i];

            primitive::Mutex_lock(&srcSegment->mutex);

            const auto* const srcArray = (const ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &srcSegment->bucketArray, primitive::RELAXED);
            auto* const dstArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &dstSegment->bucketArray, primitive::RELAXED);

            for (usize j = 0; j < srcArray->bucketCount; ++j) {
                const auto* node = (const ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &srcArray->buckets[j], primitive::RELAXED);
                while (node) {
                    if (node->value) {
                        const u64 nodeHash = self->hash(node->key);
                        const usize dstIndex = nodeHash & (dstArray->bucketCount - 1);

                        auto* const newNode = (ConcurrentChainHashMapNode*) malloc(sizeof(ConcurrentChainHashMapNode));
                        if (!newNode) {
                            primitive::Mutex_unlock(&srcSegment->mutex);
                            ConcurrentChainHashMap_destruct(newSelf);
                            return nullptr;
                        }

                        newNode->key = malloc(self->keySize);
                        if (!newNode->key) {
                            free(newNode);
                            primitive::Mutex_unlock(&srcSegment->mutex);
                            ConcurrentChainHashMap_destruct(newSelf);
                            return nullptr;
                        }
                        memcpy(newNode->key, node->key, self->keySize);

                        newNode->value = malloc(self->valueSize);
                        if (!newNode->value) {
                            free(newNode->key);
                            free(newNode);
                            primitive::Mutex_unlock(&srcSegment->mutex);
                            ConcurrentChainHashMap_destruct(newSelf);
                            return nullptr;
                        }
                        memcpy(newNode->value, node->value, self->valueSize);

                        auto* const oldHead = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &dstArray->buckets[dstIndex], primitive::RELAXED);
                        newNode->nextNode = oldHead;
                        primitive::AtomicPtr_store((void**) &dstArray->buckets[dstIndex], (void*) newNode, primitive::RELEASE);

                        primitive::Atomic64_fetchAdd(&dstSegment->size, 1, primitive::RELAXED);
                    }
                    node = (const ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
                }
            }

            primitive::Mutex_unlock(&srcSegment->mutex);
        }

        return newSelf;
    }
    auto ConcurrentChainHashMap_move(ConcurrentChainHashMap* self) -> ConcurrentChainHashMap* {
        if (!self || !self->valid) {
            return nullptr;
        }

        auto* const fresh = ConcurrentChainHashMap_construct(self->segmentCount, DEFAULT_INITIAL_CAPACITY, 0, self->keySize, self->valueSize, self->hash, self->keyEquals);
        if (!fresh) {
            return nullptr;
        }

        auto* const newSelf = (ConcurrentChainHashMap*) malloc(sizeof(ConcurrentChainHashMap));
        if (!newSelf) {
            ConcurrentChainHashMap_destruct(fresh);
            return nullptr;
        }

        newSelf->segments = self->segments;
        newSelf->segmentCount = self->segmentCount;
        newSelf->keySize = self->keySize;
        newSelf->valueSize = self->valueSize;
        newSelf->hash = self->hash;
        newSelf->keyEquals = self->keyEquals;
        newSelf->valid = true;

        self->segments = fresh->segments;
        self->segmentCount = fresh->segmentCount;
        self->keySize = fresh->keySize;
        self->valueSize = fresh->valueSize;
        self->hash = fresh->hash;
        self->keyEquals = fresh->keyEquals;
        self->valid = true;

        free(fresh);

        return newSelf;
    }

    auto ConcurrentChainHashMap_insert(ConcurrentChainHashMap* self, const void* inKey, const void* inValue) -> bool {
        if (!self || !self->valid || !inKey || !inValue) {
            return false;
        }

        const u64 hashValue = self->hash(inKey);
        const usize segmentIndex = (hashValue >> 32) & (self->segmentCount - 1);
        auto* const segment = &self->segments[segmentIndex];

        primitive::Mutex_lock(&segment->mutex);

        auto* bucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);
        const usize bucketIndex = hashValue & (bucketArray->bucketCount - 1);

        auto* node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::RELAXED);
        while (node) {
            if (self->keyEquals(node->key, inKey)) {
                if (node->value) {
                    free(node->value);
                }

                node->value = malloc(self->valueSize);
                if (!node->value) {
                    primitive::Mutex_unlock(&segment->mutex);
                    return false;
                }
                memcpy(node->value, inValue, self->valueSize);

                primitive::Mutex_unlock(&segment->mutex);
                return true;
            }
            node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
        }

        auto* const newNode = (ConcurrentChainHashMapNode*) malloc(sizeof(ConcurrentChainHashMapNode));
        if (!newNode) {
            primitive::Mutex_unlock(&segment->mutex);
            return false;
        }

        newNode->key = malloc(self->keySize);
        if (!newNode->key) {
            free(newNode);
            primitive::Mutex_unlock(&segment->mutex);
            return false;
        }
        memcpy(newNode->key, inKey, self->keySize);

        newNode->value = malloc(self->valueSize);
        if (!newNode->value) {
            free(newNode->key);
            free(newNode);
            primitive::Mutex_unlock(&segment->mutex);
            return false;
        }
        memcpy(newNode->value, inValue, self->valueSize);

        auto* const oldHead = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::RELAXED);
        newNode->nextNode = oldHead;
        primitive::AtomicPtr_store((void**) &bucketArray->buckets[bucketIndex], (void*) newNode, primitive::RELEASE);

        primitive::Atomic64_fetchAdd(&segment->size, 1, primitive::RELAXED);

        const usize currentSize = (usize) primitive::Atomic64_load(&segment->size, primitive::RELAXED);
        if (currentSize * LOAD_FACTOR_DENOMINATOR > bucketArray->bucketCount * LOAD_FACTOR_NUMERATOR) {
            const usize newBucketCount = bucketArray->bucketCount * 2;
            auto* const newArray = (ConcurrentChainHashMapBucketArray*) malloc(sizeof(ConcurrentChainHashMapBucketArray));
            if (newArray) {
                newArray->buckets = (ConcurrentChainHashMapNode**) calloc(newBucketCount, sizeof(ConcurrentChainHashMapNode*));
                if (newArray->buckets) {
                    newArray->bucketCount = newBucketCount;

                    for (usize i = 0; i < bucketArray->bucketCount; ++i) {
                        auto* n = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[i], primitive::RELAXED);
                        while (n) {
                            auto* const nextN = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &n->nextNode, primitive::RELAXED);
                            const u64 nodeHash = self->hash(n->key);
                            const usize newIndex = nodeHash & (newBucketCount - 1);

                            auto* const newHead = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &newArray->buckets[newIndex], primitive::RELAXED);
                            primitive::AtomicPtr_store((void**) &n->nextNode, (void*) newHead, primitive::RELEASE);
                            primitive::AtomicPtr_store((void**) &newArray->buckets[newIndex], (void*) n, primitive::RELEASE);

                            n = nextN;
                        }
                        primitive::AtomicPtr_store((void**) &bucketArray->buckets[i], nullptr, primitive::RELEASE);
                    }

                    primitive::AtomicPtr_store((void**) &segment->bucketArray, (void*) newArray, primitive::RELEASE);

                    if (segment->historyCount < HISTORY_SIZE) {
                        segment->history[segment->historyCount] = bucketArray;
                        segment->historyCount++;
                    } else {
                        free(segment->history[0]->buckets);
                        free(segment->history[0]);

                        for (usize h = 1; h < HISTORY_SIZE; ++h) {
                            segment->history[h - 1] = segment->history[h];
                        }
                        segment->history[HISTORY_SIZE - 1] = bucketArray;
                    }
                } else {
                    free(newArray);
                }
            }
        }

        primitive::Mutex_unlock(&segment->mutex);

        return true;
    }
    auto ConcurrentChainHashMap_insertSlot(ConcurrentChainHashMap* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool {
        if (!self || !self->valid || !inKey || !keySlotOut || !oldValueSlotOut || !newValueSlotOut) {
            return false;
        }

        *keySlotOut = nullptr;
        *oldValueSlotOut = nullptr;
        *newValueSlotOut = nullptr;

        const u64 hashValue = self->hash(inKey);
        const usize segmentIndex = (hashValue >> 32) & (self->segmentCount - 1);
        auto* const segment = &self->segments[segmentIndex];

        primitive::Mutex_lock(&segment->mutex);

        auto* bucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);
        const usize bucketIndex = hashValue & (bucketArray->bucketCount - 1);

        auto* node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::RELAXED);
        while (node) {
            if (self->keyEquals(node->key, inKey)) {
                *oldValueSlotOut = node->value;

                node->value = malloc(self->valueSize);
                if (!node->value) {
                    primitive::Mutex_unlock(&segment->mutex);
                    return false;
                }

                *newValueSlotOut = node->value;

                primitive::Mutex_unlock(&segment->mutex);
                return true;
            }
            node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
        }

        auto* const newNode = (ConcurrentChainHashMapNode*) malloc(sizeof(ConcurrentChainHashMapNode));
        if (!newNode) {
            primitive::Mutex_unlock(&segment->mutex);
            return false;
        }

        newNode->key = malloc(self->keySize);
        if (!newNode->key) {
            free(newNode);
            primitive::Mutex_unlock(&segment->mutex);
            return false;
        }

        newNode->value = malloc(self->valueSize);
        if (!newNode->value) {
            free(newNode->key);
            free(newNode);
            primitive::Mutex_unlock(&segment->mutex);
            return false;
        }

        auto* const oldHead = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::RELAXED);
        newNode->nextNode = oldHead;
        primitive::AtomicPtr_store((void**) &bucketArray->buckets[bucketIndex], (void*) newNode, primitive::RELEASE);

        primitive::Atomic64_fetchAdd(&segment->size, 1, primitive::RELAXED);

        *keySlotOut = newNode->key;
        *newValueSlotOut = newNode->value;

        primitive::Mutex_unlock(&segment->mutex);

        return true;
    }

    auto ConcurrentChainHashMap_remove(ConcurrentChainHashMap* self, const void* inKey) -> bool {
        if (!self || !self->valid || !inKey) {
            return false;
        }

        const u64 hashValue = self->hash(inKey);
        const usize segmentIndex = (hashValue >> 32) & (self->segmentCount - 1);
        auto* const segment = &self->segments[segmentIndex];

        primitive::Mutex_lock(&segment->mutex);

        auto* const bucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);
        const usize bucketIndex = hashValue & (bucketArray->bucketCount - 1);

        auto* node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::RELAXED);
        while (node) {
            if (self->keyEquals(node->key, inKey)) {
                if (!node->value) {
                    primitive::Mutex_unlock(&segment->mutex);
                    return false;
                }

                free(node->value);
                node->value = nullptr;

                primitive::Atomic64_fetchSub(&segment->size, 1, primitive::RELAXED);

                primitive::Mutex_unlock(&segment->mutex);
                return true;
            }
            node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
        }

        primitive::Mutex_unlock(&segment->mutex);

        return false;
    }
    auto ConcurrentChainHashMap_removeSlot(ConcurrentChainHashMap* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool {
        if (!self || !self->valid || !inKey || !keySlotOut || !valueSlotOut) {
            return false;
        }

        *keySlotOut = nullptr;
        *valueSlotOut = nullptr;

        const u64 hashValue = self->hash(inKey);
        const usize segmentIndex = (hashValue >> 32) & (self->segmentCount - 1);
        auto* const segment = &self->segments[segmentIndex];

        primitive::Mutex_lock(&segment->mutex);

        auto* const bucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);
        const usize bucketIndex = hashValue & (bucketArray->bucketCount - 1);

        auto* node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::RELAXED);
        while (node) {
            if (self->keyEquals(node->key, inKey)) {
                if (!node->value) {
                    primitive::Mutex_unlock(&segment->mutex);
                    return false;
                }

                auto* const keyCopy = (void*) malloc(self->keySize);
                if (!keyCopy) {
                    primitive::Mutex_unlock(&segment->mutex);
                    return false;
                }
                memcpy(keyCopy, node->key, self->keySize);

                *keySlotOut = keyCopy;
                *valueSlotOut = node->value;

                node->value = nullptr;

                primitive::Atomic64_fetchSub(&segment->size, 1, primitive::RELAXED);

                primitive::Mutex_unlock(&segment->mutex);
                return true;
            }
            node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
        }

        primitive::Mutex_unlock(&segment->mutex);

        return false;
    }
    auto ConcurrentChainHashMap_clear(ConcurrentChainHashMap* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        bool removed = false;

        for (usize i = 0; i < self->segmentCount; ++i) {
            auto* const segment = &self->segments[i];

            primitive::Mutex_lock(&segment->mutex);

            auto* const bucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);

            for (usize j = 0; j < bucketArray->bucketCount; ++j) {
                auto* node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[j], primitive::RELAXED);
                while (node) {
                    auto* const nextNode = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);

                    if (node->key) {
                        free(node->key);
                    }
                    if (node->value) {
                        free(node->value);
                    }
                    free(node);

                    node = nextNode;
                    removed = true;
                }
                primitive::AtomicPtr_store((void**) &bucketArray->buckets[j], nullptr, primitive::RELEASE);
            }

            primitive::Atomic64_store(&segment->size, 0, primitive::RELAXED);

            primitive::Mutex_unlock(&segment->mutex);
        }

        return removed;
    }

    auto ConcurrentChainHashMap_find(const ConcurrentChainHashMap* self, const void* inKey, void** valueOut) -> bool {
        if (!self || !self->valid || !inKey || !valueOut) {
            return false;
        }

        const u64 hashValue = self->hash(inKey);
        const usize segmentIndex = (hashValue >> 32) & (self->segmentCount - 1);
        const auto* const segment = &self->segments[segmentIndex];

        while (true) {
            const auto* const bucketArray = (const ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::ACQUIRE);
            const usize bucketIndex = hashValue & (bucketArray->bucketCount - 1);

            const auto* node = (const ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::ACQUIRE);
            while (node) {
                if (self->keyEquals(node->key, inKey)) {
                    if (!node->value) {
                        return false;
                    }

                    *valueOut = node->value;
                    return true;
                }
                node = (const ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::ACQUIRE);
            }

            const auto* const bucketArrayAfter = (const ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::ACQUIRE);
            if (bucketArray == bucketArrayAfter) {
                return false;
            }
        }
    }

    auto ConcurrentChainHashMap_forEach(const ConcurrentChainHashMap* self, void (*action)(const void* key, void* value, void* userData), void* userData) -> void {
        if (!self || !self->valid || !action) {
            return;
        }

        for (usize i = 0; i < self->segmentCount; ++i) {
            auto* const segment = const_cast<ConcurrentChainHashMapSegment*>(&self->segments[i]);

            primitive::Mutex_lock(&segment->mutex);

            const auto* const bucketArray = (const ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);

            for (usize j = 0; j < bucketArray->bucketCount; ++j) {
                const auto* node = (const ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[j], primitive::RELAXED);
                while (node) {
                    if (node->value) {
                        action(node->key, node->value, userData);
                    }
                    node = (const ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
                }
            }

            primitive::Mutex_unlock(&segment->mutex);
        }
    }

    auto ConcurrentChainHashMap_headAt(const ConcurrentChainHashMap* self, const usize segmentIndex, const usize bucketIndex) -> ConcurrentChainHashMapNode* {
        if (!self || !self->valid || segmentIndex >= self->segmentCount) {
            return nullptr;
        }

        const auto* const segment = &self->segments[segmentIndex];

        const auto* const bucketArray = (const ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::ACQUIRE);
        if (bucketIndex >= bucketArray->bucketCount) {
            return nullptr;
        }

        return (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[bucketIndex], primitive::ACQUIRE);
    }

    auto ConcurrentChainHashMap_reserve(ConcurrentChainHashMap* self, const usize newCapacity) -> bool {
        if (!self || !self->valid || newCapacity == 0) {
            return false;
        }

        bool anyResize = false;

        for (usize i = 0; i < self->segmentCount; ++i) {
            auto* const segment = &self->segments[i];

            primitive::Mutex_lock(&segment->mutex);

            auto* bucketArray = (ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);

            if (bucketArray->bucketCount < newCapacity) {
                usize newBucketCount = bucketArray->bucketCount;
                while (newBucketCount < newCapacity) {
                    newBucketCount <<= 1;
                }

                auto* const newArray = (ConcurrentChainHashMapBucketArray*) malloc(sizeof(ConcurrentChainHashMapBucketArray));
                if (newArray) {
                    newArray->buckets = (ConcurrentChainHashMapNode**) calloc(newBucketCount, sizeof(ConcurrentChainHashMapNode*));
                    if (newArray->buckets) {
                        newArray->bucketCount = newBucketCount;

                        for (usize j = 0; j < bucketArray->bucketCount; ++j) {
                            auto* node = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &bucketArray->buckets[j], primitive::RELAXED);
                            while (node) {
                                auto* const nextNode = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::RELAXED);
                                const u64 nodeHash = self->hash(node->key);
                                const usize newIndex = nodeHash & (newBucketCount - 1);

                                auto* const newHead = (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &newArray->buckets[newIndex], primitive::RELAXED);
                                primitive::AtomicPtr_store((void**) &node->nextNode, (void*) newHead, primitive::RELEASE);
                                primitive::AtomicPtr_store((void**) &newArray->buckets[newIndex], (void*) node, primitive::RELEASE);

                                node = nextNode;
                            }
                            primitive::AtomicPtr_store((void**) &bucketArray->buckets[j], nullptr, primitive::RELEASE);
                        }

                        primitive::AtomicPtr_store((void**) &segment->bucketArray, (void*) newArray, primitive::RELEASE);

                        if (segment->historyCount < HISTORY_SIZE) {
                            segment->history[segment->historyCount] = bucketArray;
                            segment->historyCount++;
                        } else {
                            free(segment->history[0]->buckets);
                            free(segment->history[0]);

                            for (usize h = 1; h < HISTORY_SIZE; ++h) {
                                segment->history[h - 1] = segment->history[h];
                            }
                            segment->history[HISTORY_SIZE - 1] = bucketArray;
                        }

                        anyResize = true;
                    } else {
                        free(newArray);
                    }
                }
            }

            primitive::Mutex_unlock(&segment->mutex);
        }

        return anyResize;
    }

    auto ConcurrentChainHashMap_segmentCount(const ConcurrentChainHashMap* self) -> usize {
        if (!self || !self->valid) {
            return 0;
        }

        return self->segmentCount;
    }
    auto ConcurrentChainHashMap_capacity(const ConcurrentChainHashMap* self) -> usize {
        if (!self || !self->valid) {
            return 0;
        }

        usize totalCapacity = 0;

        for (usize i = 0; i < self->segmentCount; ++i) {
            const auto* const segment = &self->segments[i];
            const auto* const bucketArray = (const ConcurrentChainHashMapBucketArray*) primitive::AtomicPtr_load((void* const*) &segment->bucketArray, primitive::RELAXED);

            totalCapacity += bucketArray->bucketCount;
        }

        return totalCapacity;
    }
    auto ConcurrentChainHashMap_size(const ConcurrentChainHashMap* self) -> usize {
        if (!self || !self->valid) {
            return 0;
        }

        usize totalSize = 0;

        for (usize i = 0; i < self->segmentCount; ++i) {
            totalSize += (usize) primitive::Atomic64_load(&self->segments[i].size, primitive::RELAXED);
        }

        return totalSize;
    }
    auto ConcurrentChainHashMap_keySize(const ConcurrentChainHashMap* self) -> usize {
        if (!self || !self->valid) {
            return 0;
        }

        return self->keySize;
    }
    auto ConcurrentChainHashMap_valueSize(const ConcurrentChainHashMap* self) -> usize {
        if (!self || !self->valid) {
            return 0;
        }

        return self->valueSize;
    }

    auto ConcurrentChainHashMap_isEmpty(const ConcurrentChainHashMap* self) -> bool {
        if (!self || !self->valid) {
            return true;
        }

        return ConcurrentChainHashMap_size(self) == 0;
    }

    auto ConcurrentChainHashMapNode_key(const ConcurrentChainHashMapNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->key;
    }
    auto ConcurrentChainHashMapNode_value(const ConcurrentChainHashMapNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->value;
    }
    auto ConcurrentChainHashMapNode_next(const ConcurrentChainHashMapNode* node) -> ConcurrentChainHashMapNode* {
        if (!node) {
            return nullptr;
        }

        return (ConcurrentChainHashMapNode*) primitive::AtomicPtr_load((void* const*) &node->nextNode, primitive::ACQUIRE);
    }
}