#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef struct OnceFlag {
        alignas(8) u8 storage[16];
        bool valid;
    } OnceFlag;

    auto OnceFlag_init(OnceFlag* self) -> bool;
    auto OnceFlag_destroy(OnceFlag* self) -> bool;

    auto OnceFlag_do(const OnceFlag* self, void (*callback)()) -> bool;
}