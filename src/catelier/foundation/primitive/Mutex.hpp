#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef struct Mutex {
        alignas(8) u8 storage[64];
        bool valid;
    } Mutex;

    auto Mutex_init(Mutex* self) -> bool;
    auto Mutex_destroy(Mutex* self) -> bool;

    auto Mutex_lock(Mutex* self) -> bool;
    auto Mutex_unlock(Mutex* self) -> bool;
    auto Mutex_tryLock(Mutex* self) -> bool;
}