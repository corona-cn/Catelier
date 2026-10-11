#pragma once
#include "../../CommonPrimitives.hpp"

namespace Catelier::src::foundation::primitive {
    auto Time_currentMs() -> u64;

    auto Time_sleepMs(u64 inMs) -> void;
}