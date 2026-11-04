#pragma once
#include "../../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::sync::lock {
    typedef struct SpinLock {
        alignas(64) u32 state;
        bool valid;
    } SpinLock;

    auto SpinLock_init(SpinLock* self) -> bool;
    auto SpinLock_destroy(SpinLock* self) -> bool;

    auto SpinLock_lock(SpinLock* self) -> bool;
    auto SpinLock_unlock(SpinLock* self) -> bool;
    auto SpinLock_tryLock(SpinLock* self) -> bool;
}