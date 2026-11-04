#pragma once
#include "../../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::sync::lock {
    typedef enum SpinLock_Policy {
        TAS,
        TTAS,
    } SpinLock_Policy;

    typedef struct SpinLock {
        alignas(64) u32 state;
        SpinLock_Policy policy;
        bool valid;
    } SpinLock;

    auto SpinLock_init(SpinLock* self, SpinLock_Policy inPolicy = SpinLock_Policy::TTAS) -> bool;
    auto SpinLock_destroy(SpinLock* self) -> bool;

    auto SpinLock_lock(SpinLock* self) -> bool;
    auto SpinLock_unlock(SpinLock* self) -> bool;
    auto SpinLock_tryLock(SpinLock* self) -> bool;
}