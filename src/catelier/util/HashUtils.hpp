#pragma once
#include "../CommonPrimitives.hpp"

namespace Catelier::src::util::hash {
    namespace FNV1a {
        auto hash64Mem(
            const void* mem,
            usize len
        ) -> u64;
    }
}