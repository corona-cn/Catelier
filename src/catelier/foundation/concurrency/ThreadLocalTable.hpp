#pragma once
#include "../../CommonPrimitives.hpp"
#include "../primitive/Mutex.hpp"

namespace Catelier::src::foundation::concurrency {
    typedef struct ThreadLocalTableEntry {
        struct ThreadLocalTable* owner;
        void* value;

        ThreadLocalTableEntry* nextInBucket;
        ThreadLocalTableEntry* nextInOwner;

        bool isThreadExited;
    } ThreadLocalTableEntry;

    typedef struct ThreadLocalTable {
        ThreadLocalTableEntry* headEntry;
        usize entryCount;

        void* (*createValueCallback)(void* hostContext);
        void* (*valueThreadExitCallback)(void* value, void* hostContext);
        void (*destroyValueCallback)(void* value, void* hostContext);
        void* hostContext;

        u64 generation;

        primitive::Mutex lock;

        bool isValid;
    } ThreadLocalTable;

    auto ThreadLocalTable_construct(ThreadLocalTable* self, void* (*inCreateValueCallback)(void* hostContext), void* (*inValueThreadExitCallback)(void* value, void* hostContext), void (*inDestroyValueCallback)(void* value, void* hostContext), void* inHostContext) -> bool;
    auto ThreadLocalTable_destruct(ThreadLocalTable* self) -> bool;
    auto ThreadLocalTable_copy(ThreadLocalTable* self, const ThreadLocalTable* source) -> bool;
    auto ThreadLocalTable_move(ThreadLocalTable* self, ThreadLocalTable* source) -> bool;

    auto ThreadLocalTable_entryCount(const ThreadLocalTable* self) -> usize;

    auto ThreadLocalTable_get(const ThreadLocalTable* self) -> void*;
    auto ThreadLocalTable_getOrCreate(ThreadLocalTable* self) -> void*;

    auto ThreadLocalTable_forEach(const ThreadLocalTable* self, void (*actionCallback)(void* value, bool isThreadExited, void* actionContext), void* actionContext) -> void;
}