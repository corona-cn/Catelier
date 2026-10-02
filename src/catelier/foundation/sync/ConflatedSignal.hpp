#pragma once
#include "../../CommonPrimitives.hpp"
#include "../primitive/Mutex.hpp"
#include "ConditionalVar.hpp"

namespace Catelier::src::foundation::sync {
    typedef struct ConflatedSignal {
        primitive::Mutex mutex;
        ConditionalVar condition;
        bool pending;
        bool valid;
    } ConflatedSignal;

    auto ConflatedSignal_init(ConflatedSignal* self) -> bool;
    auto ConflatedSignal_destroy(ConflatedSignal* self) -> bool;

    auto ConflatedSignal_signal(ConflatedSignal* self) -> void;
    auto ConflatedSignal_wait(ConflatedSignal* self) -> void;
    auto ConflatedSignal_timedWait(ConflatedSignal* self, u64 timeoutMs) -> bool;
}