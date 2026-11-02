#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef struct SpinMutex {
        alignas(64) u32 state;
        bool valid;
    } SpinMutex;

    auto SpinMutex_init(SpinMutex* self) -> bool;
    auto SpinMutex_destroy(SpinMutex* self) -> bool;

    auto SpinMutex_lock(SpinMutex* self) -> bool;
    auto SpinMutex_unlock(SpinMutex* self) -> bool;
    auto SpinMutex_tryLock(SpinMutex* self) -> bool;
}