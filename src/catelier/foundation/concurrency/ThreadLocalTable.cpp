#include "ThreadLocalTable.hpp"
#include "../primitive/Atomic.hpp"

#ifdef _WIN32

#include <windows.h>
#include <intrin.h>
#include <winternl.h>

#else

#include <pthread.h>

#endif

namespace Catelier::src::foundation::concurrency {
    namespace {
        constexpr usize DEFAULT_BUCKET_COUNT = 8;
        constexpr usize BUCKET_LOAD_NUMERATOR = 3;
        constexpr usize BUCKET_LOAD_DENOMINATOR = 4;
    }

    namespace {
        auto hash(const u64 generation) -> usize {
            return ((usize) generation) * ((usize) 0x9E3779B97F4A7C15ULL);
        }
    }

    namespace {
        primitive::Mutex lifetimeLock = {};
    }

    namespace {
        typedef struct Registry {
            ThreadLocalTableEntry** buckets;
            usize bucketCount;
            usize entryCount;

            ThreadLocalTable* cachedOwner;
            u64 cachedGeneration;
            void* cachedValue;
        } Registry;

        auto Registry_create() -> Registry* {
            auto* const registry = (Registry*) calloc(1, sizeof(Registry));
            if (!registry) {
                return nullptr;
            }

            registry->buckets = (ThreadLocalTableEntry**) calloc(DEFAULT_BUCKET_COUNT, sizeof(ThreadLocalTableEntry*));
            if (!registry->buckets) {
                free(registry);
                return nullptr;
            }

            registry->bucketCount = DEFAULT_BUCKET_COUNT;
            registry->entryCount = 0;
            registry->cachedOwner = nullptr;
            registry->cachedGeneration = 0;
            registry->cachedValue = nullptr;

            return registry;
        }
        auto Registry_destroy(Registry* const self) -> void {
            if (!self) {
                return;
            }

            if (self->buckets) {
                free(self->buckets);
            }

            free(self);
        }

        auto Registry_rehash(Registry* const self) -> bool {
            const usize newBucketCount = self->bucketCount * 2;
            auto** const newBuckets = (ThreadLocalTableEntry**) calloc(newBucketCount, sizeof(ThreadLocalTableEntry*));
            if (!newBuckets) {
                return false;
            }

            ThreadLocalTableEntry* zombies = nullptr;
            usize zombieCount = 0;

            const usize oldBucketCount = self->bucketCount;
            auto** const oldBuckets = self->buckets;

            for (usize i = 0; i < oldBucketCount; ++i) {
                ThreadLocalTableEntry* entry = oldBuckets[i];
                while (entry) {
                    ThreadLocalTableEntry* const next = entry->nextInBucket;

                    ThreadLocalTable* const entryOwner = (ThreadLocalTable*) primitive::AtomicPtr_load((void* const*) &entry->owner, primitive::MemoryOrder::RELAXED);

                    if (entryOwner == nullptr) {
                        entry->nextInBucket = zombies;
                        zombies = entry;
                        zombieCount++;
                    } else {
                        const usize newIndex = hash(entryOwner->generation) & (newBucketCount - 1);
                        entry->nextInBucket = newBuckets[newIndex];
                        newBuckets[newIndex] = entry;
                    }

                    entry = next;
                }
            }

            if (zombieCount > 0) {
                primitive::Mutex_lock(&lifetimeLock);

                ThreadLocalTableEntry* zombie = zombies;
                while (zombie) {
                    ThreadLocalTableEntry* const next = zombie->nextInBucket;

                    free(zombie);
                    self->entryCount--;

                    zombie = next;
                }

                primitive::Mutex_unlock(&lifetimeLock);
            }

            free(oldBuckets);
            self->buckets = newBuckets;
            self->bucketCount = newBucketCount;

            self->cachedOwner = nullptr;
            self->cachedGeneration = 0;
            self->cachedValue = nullptr;

            return true;
        }
    }

    namespace {
        u64 generationCounter = 0;
        bool globalRegistryValid = false;

        #ifdef _WIN32

        DWORD globalRegistryTlsSlot = TLS_OUT_OF_INDEXES;
        DWORD globalRegistryFlsSlot = FLS_OUT_OF_INDEXES;
        INIT_ONCE globalRegistryOnce = INIT_ONCE_STATIC_INIT;

        #else

        pthread_key_t globalRegistryKey;
        pthread_once_t globalRegistryOnce = PTHREAD_ONCE_INIT;

        #endif

        auto threadExitCallback(void* inRegistry) -> void;

        auto initGlobalState() -> void {
            primitive::Mutex_init(&lifetimeLock);

            #ifdef _WIN32

            globalRegistryTlsSlot = TlsAlloc();
            if (globalRegistryTlsSlot == TLS_OUT_OF_INDEXES) {
                return;
            }

            globalRegistryFlsSlot = FlsAlloc(threadExitCallback);
            if (globalRegistryFlsSlot == FLS_OUT_OF_INDEXES) {
                TlsFree(globalRegistryTlsSlot);
                globalRegistryTlsSlot = TLS_OUT_OF_INDEXES;
                return;
            }

            #else

            if (pthread_key_create(&globalRegistryKey, threadExitCallback) != 0) {
                return;
            }

            #endif

            globalRegistryValid = true;
        }

        #ifdef _WIN32

        auto CALLBACK initGlobalStateBridge(PINIT_ONCE, PVOID, PVOID*) -> BOOL {
            initGlobalState();
            return TRUE;
        }

        #endif

        auto getRegistry() -> Registry* {
            if (!globalRegistryValid) {
                return nullptr;
            }

            #ifdef _WIN32

            #if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))

            if (globalRegistryTlsSlot < 64) {
                return (Registry*) NtCurrentTeb()->TlsSlots[globalRegistryTlsSlot];
            }

            #endif

            return (Registry*) TlsGetValue(globalRegistryTlsSlot);

            #else

            return (Registry*) pthread_getspecific(globalRegistryKey);

            #endif
        }
    }

    namespace {
        auto threadExitCallback(void* const inRegistry) -> void {
            auto* const registry = (Registry*) inRegistry;
            if (!registry) {
                return;
            }

            for (usize i = 0; i < registry->bucketCount; ++i) {
                ThreadLocalTableEntry* entry = registry->buckets[i];
                while (entry) {
                    ThreadLocalTableEntry* const next = entry->nextInBucket;

                    primitive::Mutex_lock(&lifetimeLock);

                    ThreadLocalTable* const owner = entry->owner;
                    if (!owner) {
                        primitive::Mutex_unlock(&lifetimeLock);

                        free(entry);
                        entry = next;
                        continue;
                    }

                    entry->isThreadExited = true;

                    if (owner->valueThreadExitCallback) {
                        entry->value = owner->valueThreadExitCallback(entry->value, owner->hostContext);
                    }

                    primitive::Mutex_unlock(&lifetimeLock);

                    entry = next;
                }
            }

            Registry_destroy(registry);
        }
    }

    auto ThreadLocalTable_construct(ThreadLocalTable* const self, void* (*inCreateValueCallback)(void* hostContext), void* (*inValueThreadExitCallback)(void* value, void* hostContext), void (*inDestroyValueCallback)(void* value, void* hostContext), void* const inHostContext) -> bool {
        if (!self || !inCreateValueCallback) {
            return false;
        }

        #ifdef _WIN32

        InitOnceExecuteOnce(&globalRegistryOnce, initGlobalStateBridge, nullptr, nullptr);

        #else

        pthread_once(&globalRegistryOnce, initGlobalState);

        #endif

        if (!globalRegistryValid) {
            return false;
        }

        self->headEntry = nullptr;
        self->entryCount = 0;
        self->createValueCallback = inCreateValueCallback;
        self->valueThreadExitCallback = inValueThreadExitCallback;
        self->destroyValueCallback = inDestroyValueCallback;
        self->hostContext = inHostContext;

        self->generation = primitive::Atomic64_fetchAdd(&generationCounter, 1, primitive::MemoryOrder::RELAXED) + 1;

        if (!primitive::Mutex_init(&self->lock)) {
            return false;
        }

        self->isValid = true;

        return true;
    }
    auto ThreadLocalTable_destruct(ThreadLocalTable* const self) -> bool {
        if (!self || !self->isValid) {
            return false;
        }

        primitive::Mutex_lock(&lifetimeLock);

        primitive::Mutex_lock(&self->lock);

        self->isValid = false;

        ThreadLocalTableEntry* const orphaned = self->headEntry;

        self->headEntry = nullptr;
        self->entryCount = 0;

        primitive::Mutex_unlock(&self->lock);

        ThreadLocalTableEntry* entry = orphaned;
        while (entry) {
            ThreadLocalTableEntry* const next = entry->nextInOwner;

            if (self->destroyValueCallback && entry->value) {
                self->destroyValueCallback(entry->value, self->hostContext);
                entry->value = nullptr;
            }

            if (entry->isThreadExited) {
                free(entry);
            } else {
                primitive::AtomicPtr_store((void**) &entry->owner, nullptr, primitive::MemoryOrder::RELEASE);
            }

            entry = next;
        }

        primitive::Mutex_destroy(&self->lock);

        primitive::Mutex_unlock(&lifetimeLock);

        return true;
    }
    auto ThreadLocalTable_copy(ThreadLocalTable* const self, const ThreadLocalTable* const source) -> bool {
        if (!self || !source || !source->isValid) {
            return false;
        }

        self->headEntry = nullptr;
        self->entryCount = 0;
        self->createValueCallback = source->createValueCallback;
        self->valueThreadExitCallback = source->valueThreadExitCallback;
        self->destroyValueCallback = source->destroyValueCallback;
        self->hostContext = source->hostContext;

        self->generation = primitive::Atomic64_fetchAdd(&generationCounter, 1, primitive::MemoryOrder::RELAXED) + 1;

        if (!primitive::Mutex_init(&self->lock)) {
            return false;
        }

        self->isValid = true;

        return true;
    }
    auto ThreadLocalTable_move(ThreadLocalTable* const self, ThreadLocalTable* source) -> bool {
        if (!self || !source || !source->isValid) {
            return false;
        }

        if (!primitive::Mutex_init(&self->lock)) {
            return false;
        }

        self->headEntry = source->headEntry;
        self->entryCount = source->entryCount;
        self->createValueCallback = source->createValueCallback;
        self->valueThreadExitCallback = source->valueThreadExitCallback;
        self->destroyValueCallback = source->destroyValueCallback;
        self->hostContext = source->hostContext;
        self->generation = source->generation;
        self->isValid = true;

        primitive::Mutex_lock(&lifetimeLock);

        for (ThreadLocalTableEntry* entry = self->headEntry; entry; entry = entry->nextInOwner) {
            primitive::AtomicPtr_store((void**) &entry->owner, (void*) self, primitive::MemoryOrder::RELEASE);
        }

        primitive::Mutex_unlock(&lifetimeLock);

        source->headEntry = nullptr;
        source->entryCount = 0;
        source->createValueCallback = nullptr;
        source->valueThreadExitCallback = nullptr;
        source->destroyValueCallback = nullptr;
        source->hostContext = nullptr;
        source->generation = 0;

        primitive::Mutex_destroy(&source->lock);

        source->isValid = false;

        return true;
    }

    auto ThreadLocalTable_entryCount(const ThreadLocalTable* const self) -> usize {
        if (!self || !self->isValid) {
            return 0;
        }

        return self->entryCount;
    }

    auto ThreadLocalTable_get(const ThreadLocalTable* const self) -> void* {
        if (!self || !self->isValid) {
            return nullptr;
        }

        const auto* const registry = getRegistry();
        if (!registry) {
            return nullptr;
        }

        if (registry->cachedOwner == self && registry->cachedGeneration == self->generation) {
            return registry->cachedValue;
        }

        const usize bucketIndex = hash(self->generation) & (registry->bucketCount - 1);
        const ThreadLocalTableEntry* entry = registry->buckets[bucketIndex];
        while (entry) {
            if ((const ThreadLocalTable*) primitive::AtomicPtr_load((void* const*) &entry->owner, primitive::MemoryOrder::RELAXED) == self) {
                return entry->value;
            }

            entry = entry->nextInBucket;
        }

        return nullptr;
    }
    auto ThreadLocalTable_getOrCreate(ThreadLocalTable* const self) -> void* {
        if (!self || !self->isValid) {
            return nullptr;
        }

        auto* registry = getRegistry();
        if (!registry) {
            registry = Registry_create();
            if (!registry) {
                return nullptr;
            }

            #ifdef _WIN32

            if (!TlsSetValue(globalRegistryTlsSlot, registry)) {
                Registry_destroy(registry);
                return nullptr;
            }

            if (!FlsSetValue(globalRegistryFlsSlot, registry)) {
                TlsSetValue(globalRegistryTlsSlot, nullptr);
                Registry_destroy(registry);
                return nullptr;
            }

            #else

            if (pthread_setspecific(globalRegistryKey, registry) != 0) {
                Registry_destroy(registry);
                return nullptr;
            }

            #endif
        }

        if (registry->cachedOwner == self && registry->cachedGeneration == self->generation) {
            return registry->cachedValue;
        }

        const usize bucketIndex = hash(self->generation) & (registry->bucketCount - 1);
        ThreadLocalTableEntry** link = &registry->buckets[bucketIndex];
        while (*link) {
            ThreadLocalTableEntry* const entry = *link;
            ThreadLocalTable* const currentOwner = (ThreadLocalTable*) primitive::AtomicPtr_load((void* const*) &entry->owner, primitive::MemoryOrder::RELAXED);

            if (currentOwner == self) {
                registry->cachedOwner = self;
                registry->cachedGeneration = self->generation;
                registry->cachedValue = entry->value;

                return entry->value;
            }

            if (currentOwner == nullptr) {
                primitive::Mutex_lock(&lifetimeLock);

                ThreadLocalTableEntry** current = &registry->buckets[bucketIndex];
                while (*current) {
                    ThreadLocalTableEntry* const candidate = *current;

                    if (candidate->owner == nullptr) {
                        *current = candidate->nextInBucket;
                        free(candidate);
                        registry->entryCount--;
                    } else {
                        current = &candidate->nextInBucket;
                    }
                }

                ThreadLocalTableEntry* scan = registry->buckets[bucketIndex];
                while (scan) {
                    if (scan->owner == self) {
                        registry->cachedOwner = self;
                        registry->cachedGeneration = self->generation;
                        registry->cachedValue = scan->value;

                        primitive::Mutex_unlock(&lifetimeLock);

                        return scan->value;
                    }

                    scan = scan->nextInBucket;
                }

                primitive::Mutex_unlock(&lifetimeLock);

                break;
            }

            link = &entry->nextInBucket;
        }

        if ((registry->entryCount + 1) * BUCKET_LOAD_DENOMINATOR > registry->bucketCount * BUCKET_LOAD_NUMERATOR) {
            if (!Registry_rehash(registry)) {
                return nullptr;
            }
        }

        const usize finalBucketIndex = hash(self->generation) & (registry->bucketCount - 1);

        auto* const entry = (ThreadLocalTableEntry*) malloc(sizeof(ThreadLocalTableEntry));
        if (!entry) {
            return nullptr;
        }

        entry->owner = self;
        entry->value = self->createValueCallback(self->hostContext);
        entry->isThreadExited = false;

        primitive::Mutex_lock(&lifetimeLock);

        if (!self->isValid) {
            primitive::Mutex_unlock(&lifetimeLock);

            if (self->destroyValueCallback && entry->value) {
                self->destroyValueCallback(entry->value, self->hostContext);
            }

            free(entry);
            return nullptr;
        }

        primitive::Mutex_lock(&self->lock);

        entry->nextInBucket = registry->buckets[finalBucketIndex];
        registry->buckets[finalBucketIndex] = entry;
        registry->entryCount++;

        entry->nextInOwner = self->headEntry;
        self->headEntry = entry;
        self->entryCount++;

        primitive::Mutex_unlock(&self->lock);

        primitive::Mutex_unlock(&lifetimeLock);

        registry->cachedOwner = self;
        registry->cachedGeneration = self->generation;
        registry->cachedValue = entry->value;

        return entry->value;
    }

    auto ThreadLocalTable_forEach(const ThreadLocalTable* const self, void (*actionCallback)(void* value, bool isThreadExited, void* actionContext), void* const actionContext) -> void {
        if (!self || !self->isValid || !actionCallback) {
            return;
        }

        primitive::Mutex_lock(&lifetimeLock);

        primitive::Mutex_lock(&((ThreadLocalTable*) self)->lock);

        for (ThreadLocalTableEntry* entry = self->headEntry; entry; entry = entry->nextInOwner) {
            actionCallback(entry->value, entry->isThreadExited, actionContext);
        }

        primitive::Mutex_unlock(&((ThreadLocalTable*) self)->lock);

        primitive::Mutex_unlock(&lifetimeLock);
    }
}