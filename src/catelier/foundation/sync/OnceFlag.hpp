#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::sync {
    typedef struct OnceFlag {
        alignas(16) u8 storage[32];
        bool valid;
    } OnceFlag;

    auto OnceFlag_init(OnceFlag* self) -> bool;
    auto OnceFlag_destroy(OnceFlag* self) -> bool;

    auto OnceFlag_do(const OnceFlag* self, void (*callback)()) -> bool;
}