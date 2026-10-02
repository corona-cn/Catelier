#pragma once
#include "../../CommonPrimitives.hpp"
#include "../primitive/Mutex.hpp"

namespace Catelier::src::foundation::sync {
    typedef struct ConditionalVar {
        alignas(8) u8 storage[64];
        bool valid;
    } ConditionalVar;

    auto ConditionalVar_init(ConditionalVar* self) -> bool;
    auto ConditionalVar_destroy(ConditionalVar* self) -> bool;

    auto ConditionalVar_wait(ConditionalVar* self, primitive::Mutex* mutex) -> bool;
    auto ConditionalVar_timedWait(ConditionalVar* self, primitive::Mutex* mutex, u64 timeoutMs) -> bool;
    auto ConditionalVar_signal(ConditionalVar* self) -> bool;
    auto ConditionalVar_broadcast(ConditionalVar* self) -> bool;
}
