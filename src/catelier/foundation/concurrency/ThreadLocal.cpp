#include "ThreadLocal.hpp"

#include "../../foundation/primitive/ThreadKey.hpp"
#include "../../foundation/primitive/Mutex.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace Catelier::src::foundation::concurrency {
    namespace {
        constexpr usize DEFAULT_BUCKET_COUNT = 8;
        constexpr usize BUCKET_LOAD_NUMERATOR = 3;
        constexpr usize BUCKET_LOAD_DENOMINATOR = 4;
    }

    typedef struct ThreadLocalEntry {
        ThreadLocal* owner;
        void* value;
        ThreadLocalEntry* nextInBucket;
        ThreadLocalEntry* nextInOwner;
    } ThreadLocalEntry;

    typedef struct ThreadLocalRegistry {
        ThreadLocalEntry** buckets;
        usize bucketCount;
        usize entryCount;

        ThreadLocal* cachedOwner;
        u64 cachedGeneration;
        void* cachedValue;
    } ThreadLocalRegistry;

    namespace {
        primitive::ThreadKey registryKey = {};
        primitive::Mutex entryLock = {};

        u64 generationCounter = 0;
        primitive::Mutex generationLock = {};

        auto threadExitCallback(void* inRegistry) -> void;

        auto initGlobalState() -> void {
            primitive::Mutex_init(&entryLock);
            primitive::Mutex_init(&generationLock);
            primitive::ThreadKey_init(&registryKey, threadExitCallback);
        }

        #ifdef _WIN32
        INIT_ONCE globalInitOnce = INIT_ONCE_STATIC_INIT;

        auto CALLBACK initGlobalStateBridge(PINIT_ONCE, PVOID, PVOID*) -> BOOL {
            initGlobalState();
            return TRUE;
        }

        auto ensureGlobalInit() -> void {
            InitOnceExecuteOnce(&globalInitOnce, initGlobalStateBridge, nullptr, nullptr);
        }
        #else
        pthread_once_t globalInitOnce = PTHREAD_ONCE_INIT;

        auto ensureGlobalInit() -> void {
            pthread_once(&globalInitOnce, initGlobalState);
        }
        #endif
    }

    namespace {
        auto threadLocalHash(const ThreadLocal* self) -> usize {
            return ((usize) self >> 4) * (usize) 0x9E3779B97F4A7C15ULL;
        }

        auto rehashRegistry(ThreadLocalRegistry* registry) -> bool {
            const usize newBucketCount = registry->bucketCount * 2;
            auto** const newBuckets = (ThreadLocalEntry**) calloc(newBucketCount, sizeof(ThreadLocalEntry*));
            if (!newBuckets) {
                return false;
            }

            const usize oldBucketCount = registry->bucketCount;
            auto** const oldBuckets = registry->buckets;

            for (usize i = 0; i < oldBucketCount; ++i) {
                ThreadLocalEntry* entry = *(oldBuckets + i);
                while (entry) {
                    ThreadLocalEntry* const nextEntry = entry->nextInBucket;

                    if (entry->owner == nullptr) {
                        free(entry);
                        registry->entryCount--;
                    } else {
                        const usize newIndex = threadLocalHash(entry->owner) & (newBucketCount - 1);
                        entry->nextInBucket = *(newBuckets + newIndex);
                        *(newBuckets + newIndex) = entry;
                    }

                    entry = nextEntry;
                }
            }

            free(oldBuckets);

            registry->buckets = newBuckets;
            registry->bucketCount = newBucketCount;

            return true;
        }
    }

    namespace {
        auto threadExitCallback(void* inRegistry) -> void {
            auto* const registry = (ThreadLocalRegistry*) inRegistry;
            if (!registry) {
                return;
            }

            for (usize i = 0; i < registry->bucketCount; ++i) {
                ThreadLocalEntry* entry = *(registry->buckets + i);
                while (entry) {
                    ThreadLocalEntry* const nextEntry = entry->nextInBucket;

                    primitive::Mutex_lock(&entryLock);

                    ThreadLocal* const owner = entry->owner;
                    if (owner) {
                        ThreadLocalEntry** link = &owner->head;
                        while (*link) {
                            if (*link == entry) {
                                *link = entry->nextInOwner;
                                break;
                            }

                            link = &(*link)->nextInOwner;
                        }

                        owner->entryCount--;
                        entry->owner = nullptr;
                    }

                    primitive::Mutex_unlock(&entryLock);

                    if (owner && owner->onThreadExit) {
                        owner->onThreadExit(entry->value, owner->userData);
                    }

                    free(entry);
                    entry = nextEntry;
                }
            }

            free(registry->buckets);
            free(registry);
        }
    }

    auto ThreadLocal_init(ThreadLocal* self, void* (*inInitFunc)(void* userData), void (*inOnThreadExit)(void* value, void* userData), void (*inOnDestroy)(void* value, void* userData), void* inUserData) -> bool {
        if (!self || !inInitFunc) {
            return false;
        }

        ensureGlobalInit();

        primitive::Mutex_lock(&generationLock);
        generationCounter++;
        const u64 generation = generationCounter;
        primitive::Mutex_unlock(&generationLock);

        self->head = nullptr;
        self->entryCount = 0;
        self->initFunc = inInitFunc;
        self->onThreadExit = inOnThreadExit;
        self->onDestroy = inOnDestroy;
        self->userData = inUserData;
        self->generation = generation;
        self->valid = true;

        return true;
    }
    auto ThreadLocal_destroy(ThreadLocal* self) -> bool {
        if (!self || !self->valid) {
            return false;
        }

        self->valid = false;

        if (!self->head) {
            return true;
        }

        primitive::Mutex_lock(&entryLock);

        ThreadLocalEntry* const orphaned = self->head;
        for (ThreadLocalEntry* entry = orphaned; entry; entry = entry->nextInOwner) {
            entry->owner = nullptr;
        }

        self->head = nullptr;
        self->entryCount = 0;

        primitive::Mutex_unlock(&entryLock);

        if (self->onDestroy) {
            for (ThreadLocalEntry* entry = orphaned; entry; entry = entry->nextInOwner) {
                self->onDestroy(entry->value, self->userData);
            }
        }

        return true;
    }

    auto ThreadLocal_get(const ThreadLocal* self) -> void* {
        if (!self || !self->valid) {
            return nullptr;
        }

        const auto* const registry = (const ThreadLocalRegistry*) primitive::ThreadKey_get(&registryKey);
        if (!registry) {
            return nullptr;
        }

        if (registry->cachedOwner == self && registry->cachedGeneration == self->generation) {
            return registry->cachedValue;
        }

        const usize bucketIndex = threadLocalHash(self) & (registry->bucketCount - 1);

        const ThreadLocalEntry* entry = *(registry->buckets + bucketIndex);
        while (entry) {
            if (entry->owner == self) {
                return entry->value;
            }

            entry = entry->nextInBucket;
        }

        return nullptr;
    }
    auto ThreadLocal_getOrCreate(ThreadLocal* self) -> void* {
        if (!self || !self->valid) {
            return nullptr;
        }

        ensureGlobalInit();

        auto* registry = (ThreadLocalRegistry*) primitive::ThreadKey_get(&registryKey);
        if (!registry) {
            registry = (ThreadLocalRegistry*) calloc(1, sizeof(ThreadLocalRegistry));
            if (!registry) {
                return nullptr;
            }

            registry->buckets = (ThreadLocalEntry**) calloc(DEFAULT_BUCKET_COUNT, sizeof(ThreadLocalEntry*));
            if (!registry->buckets) {
                free(registry);
                return nullptr;
            }

            registry->bucketCount = DEFAULT_BUCKET_COUNT;
            registry->entryCount = 0;
            registry->cachedOwner = nullptr;
            registry->cachedGeneration = 0;
            registry->cachedValue = nullptr;

            if (!primitive::ThreadKey_set(&registryKey, registry)) {
                free(registry->buckets);
                free(registry);
                return nullptr;
            }
        }

        if (registry->cachedOwner == self && registry->cachedGeneration == self->generation) {
            return registry->cachedValue;
        }

        const usize bucketIndex = threadLocalHash(self) & (registry->bucketCount - 1);
        ThreadLocalEntry** link = &registry->buckets[bucketIndex];

        while (*link) {
            ThreadLocalEntry* const entry = *link;

            if (entry->owner == nullptr) {
                *link = entry->nextInBucket;
                free(entry);
                registry->entryCount--;
                continue;
            }

            if (entry->owner == self) {
                registry->cachedOwner = self;
                registry->cachedGeneration = self->generation;
                registry->cachedValue = entry->value;

                return entry->value;
            }

            link = &entry->nextInBucket;
        }

        if ((registry->entryCount + 1) * BUCKET_LOAD_DENOMINATOR > registry->bucketCount * BUCKET_LOAD_NUMERATOR) {
            if (!rehashRegistry(registry)) {
                return nullptr;
            }
        }

        const usize finalBucketIndex = threadLocalHash(self) & (registry->bucketCount - 1);

        auto* const entry = (ThreadLocalEntry*) malloc(sizeof(ThreadLocalEntry));
        if (!entry) {
            return nullptr;
        }

        entry->owner = self;
        entry->value = self->initFunc(self->userData);

        primitive::Mutex_lock(&entryLock);

        if (!self->valid) {
            primitive::Mutex_unlock(&entryLock);

            if (self->onDestroy) {
                self->onDestroy(entry->value, self->userData);
            }

            free(entry);
            return nullptr;
        }

        entry->nextInBucket = *(registry->buckets + finalBucketIndex);
        *(registry->buckets + finalBucketIndex) = entry;
        registry->entryCount++;

        entry->nextInOwner = self->head;
        self->head = entry;
        self->entryCount++;

        primitive::Mutex_unlock(&entryLock);

        registry->cachedOwner = self;
        registry->cachedGeneration = self->generation;
        registry->cachedValue = entry->value;

        return entry->value;
    }

    auto ThreadLocal_forEach(const ThreadLocal* self, void (*action)(void* value, void* inUserData), void* inUserData) -> void {
        if (!self || !self->valid || !action || !self->head) {
            return;
        }

        constexpr usize STACK_ENTRY_COUNT = 64;
        void* stackValues[STACK_ENTRY_COUNT];

        primitive::Mutex_lock(&entryLock);

        usize count = 0;
        for (ThreadLocalEntry* entry = self->head; entry; entry = entry->nextInOwner) {
            count++;
        }

        if (count == 0) {
            primitive::Mutex_unlock(&entryLock);
            return;
        }

        void** values = stackValues;
        const bool usesHeap = (count > STACK_ENTRY_COUNT);

        if (usesHeap) {
            values = (void**) malloc(sizeof(void*) * count);
            if (!values) {
                for (ThreadLocalEntry* entry = self->head; entry; entry = entry->nextInOwner) {
                    action(entry->value, inUserData);
                }
                primitive::Mutex_unlock(&entryLock);
                return;
            }
        }

        usize index = 0;
        for (ThreadLocalEntry* entry = self->head; entry; entry = entry->nextInOwner) {
            values[index] = entry->value;
            index++;
        }

        primitive::Mutex_unlock(&entryLock);

        for (usize i = 0; i < count; ++i) {
            action(values[i], inUserData);
        }

        if (usesHeap) {
            free(values);
        }
    }

    auto ThreadLocal_entryCount(const ThreadLocal* self) -> usize {
        if (!self || !self->valid) {
            return 0;
        }

        return self->entryCount;
    }
}