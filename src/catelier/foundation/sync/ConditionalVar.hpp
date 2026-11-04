#pragma once
#include "../../CommonPrimitives.hpp"
#include "lock/BlockingLock.hpp"

namespace Catelier::src::foundation::sync {
    typedef struct ConditionalVar {
        alignas(16) u8 storage[128];
        bool valid;
    } ConditionalVar;

    auto ConditionalVar_init(ConditionalVar* self) -> bool;
    auto ConditionalVar_destroy(ConditionalVar* self) -> bool;

    auto ConditionalVar_wait(ConditionalVar* self, sync::lock::BlockingLock* mutex) -> bool;
    auto ConditionalVar_timedWait(ConditionalVar* self, sync::lock::BlockingLock* mutex, u64 timeoutMs) -> bool;
    auto ConditionalVar_signal(ConditionalVar* self) -> bool;
    auto ConditionalVar_broadcast(ConditionalVar* self) -> bool;
}
