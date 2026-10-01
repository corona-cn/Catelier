#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef struct Mutex {
        alignas(8) u8 storage[64];
        bool valid;
    } Mutex;

    auto Mutex_init(Mutex* self) -> bool;
    auto Mutex_destroy(Mutex* self) -> bool;

    auto Mutex_lock(const Mutex* self) -> bool;
    auto Mutex_unlock(const Mutex* self) -> bool;
    auto Mutex_tryLock(const Mutex* self) -> bool;
}