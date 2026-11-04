#pragma once
#include "../../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::sync::lock {
    typedef enum BlockingLock_Policy {
        PLAIN,
        ADAPTIVE,
    } BlockingLock_Policy;

    typedef struct BlockingLock {
        alignas(16) u8 storage[64];
        BlockingLock_Policy policy;
        bool valid;
    } BlockingLock;

    auto BlockingLock_init(BlockingLock* self, BlockingLock_Policy inPolicy = BlockingLock_Policy::PLAIN) -> bool;
    auto BlockingLock_destroy(BlockingLock* self) -> bool;

    auto BlockingLock_lock(BlockingLock* self) -> bool;
    auto BlockingLock_unlock(BlockingLock* self) -> bool;
    auto BlockingLock_tryLock(BlockingLock* self) -> bool;
}