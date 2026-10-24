#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef struct Thread {
        alignas(16) u8 storage[32];
        bool valid;
        bool started;
    } Thread;

    auto Thread_init(Thread* self) -> bool;
    auto Thread_destroy(Thread* self) -> bool;

    auto Thread_currentId() -> u64;

    auto Thread_start(Thread* self, void (*entry)(void* userData), void* userData) -> bool;

    auto Thread_join(Thread* self) -> bool;

    auto Thread_detach(Thread* self) -> bool;

    auto Thread_yield() -> void;
}