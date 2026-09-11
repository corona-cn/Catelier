#include "HashUtils.hpp"

namespace Catelier::src::util::hash {
    namespace FNV1a {
        auto hash64Mem(
            const void* mem,
            const usize len
        ) -> u64 {
            constexpr u64 INITIAL_SEED = 14695981039346656037ULL;
            constexpr u64 PRIME_MULTIPLIER = 1099511628211ULL;

            const auto* const bytes = (const u8*) mem;
            u64 hash = INITIAL_SEED;

            for (usize i = 0; i < len; ++i) {
                hash ^= *(bytes + i);
                hash *= PRIME_MULTIPLIER;
            }

            return hash;
        }
    }
}