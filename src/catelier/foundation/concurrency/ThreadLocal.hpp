#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::concurrency {
    typedef struct ThreadLocalEntry ThreadLocalEntry;

    typedef struct ThreadLocal {
        ThreadLocalEntry* head;
        usize entryCount;
        void* (*initFunc)(void* userData);
        void (*onThreadExit)(void* value, void* userData);
        void (*onDestroy)(void* value, void* userData);
        void* userData;
        u64 generation;
        bool valid;
    } ThreadLocal;

    auto ThreadLocal_init(ThreadLocal* self, void* (*inInitFunc)(void* userData), void (*inOnThreadExit)(void* value, void* userData), void (*inOnDestroy)(void* value, void* userData), void* inUserData) -> bool;
    auto ThreadLocal_destroy(ThreadLocal* self) -> bool;

    auto ThreadLocal_get(const ThreadLocal* self) -> void*;
    auto ThreadLocal_getOrCreate(ThreadLocal* self) -> void*;

    auto ThreadLocal_forEach(const ThreadLocal* self, void (*action)(void* value, void* inUserData), void* inUserData) -> void;

    auto ThreadLocal_entryCount(const ThreadLocal* self) -> usize;
}