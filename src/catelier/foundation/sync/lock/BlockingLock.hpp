#pragma once
#include "../../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::sync::lock {
    typedef struct BlockingLock {
        alignas(16) u8 storage[64];
        bool valid;
    } BlockingLock;

    auto BlockingLock_init(BlockingLock* self) -> bool;
    auto BlockingLock_destroy(BlockingLock* self) -> bool;

    auto BlockingLock_lock(BlockingLock* self) -> bool;
    auto BlockingLock_unlock(BlockingLock* self) -> bool;
    auto BlockingLock_tryLock(BlockingLock* self) -> bool;
}