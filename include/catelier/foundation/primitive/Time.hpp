#pragma once
#include "../../../../src/catelier/foundation/primitive/Time.hpp"

namespace Catelier::foundation::primitive {
    class Time {
        public:
            static auto currentMs() -> u64 {
                return src::foundation::primitive::Time_currentMs();
            }

            static auto sleepMs(const u64 inMs) -> void {
                src::foundation::primitive::Time_sleepMs(inMs);
            }

        private:
            explicit Time() = delete;
    };
}