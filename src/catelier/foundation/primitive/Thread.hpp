#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef struct Thread {
        alignas(8) u8 storage[16];
        bool valid;
        bool started;
    } Thread;

    typedef void (*Thread_EntryFunc)(void* userData);

    auto Thread_init(Thread* self) -> bool;
    auto Thread_destroy(Thread* self) -> bool;

    auto Thread_start(Thread* self, Thread_EntryFunc entry, void* userData) -> bool;
    auto Thread_join(Thread* self) -> bool;
    auto Thread_detach(Thread* self) -> bool;

    auto Thread_currentId() -> u64;
    auto Thread_yield() -> void;
}