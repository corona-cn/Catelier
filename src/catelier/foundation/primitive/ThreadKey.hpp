#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    typedef struct ThreadKey {
        u32 handle;
        bool valid;
    } ThreadKey;

    auto ThreadKey_init(ThreadKey* self, void (*destructor)(void* value)) -> bool;
    auto ThreadKey_destroy(ThreadKey* self) -> bool;

    auto ThreadKey_get(const ThreadKey* self) -> void*;
    auto ThreadKey_set(const ThreadKey* self, void* value) -> bool;
}