#pragma once
#include "../../../../src/catelier/foundation/sync/ConflatedSignal.hpp"

namespace Catelier::foundation::sync {
    class ConflatedSignal {
        public:
            explicit ConflatedSignal() {
                src::foundation::sync::ConflatedSignal_init(&this->handle);
            }
            ~ConflatedSignal() {
                if (this->handle.valid) {
                    src::foundation::sync::ConflatedSignal_destroy(&this->handle);
                }
            }

            ConflatedSignal(const ConflatedSignal&) = delete;
            ConflatedSignal(ConflatedSignal&&) = delete;
            ConflatedSignal& operator = (const ConflatedSignal&) = delete;
            ConflatedSignal& operator = (ConflatedSignal&&) = delete;

            auto signal() -> void {
                src::foundation::sync::ConflatedSignal_signal(&this->handle);
            }
            auto wait() -> void {
                src::foundation::sync::ConflatedSignal_wait(&this->handle);
            }
            auto timedWait(u64 timeoutMs) -> bool {
                return src::foundation::sync::ConflatedSignal_timedWait(&this->handle, timeoutMs);
            }

            auto isValid() const -> bool {
                return this->handle.valid;
            }

        private:
            src::foundation::sync::ConflatedSignal handle = {};
    };
}