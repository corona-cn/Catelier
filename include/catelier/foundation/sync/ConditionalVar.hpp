#pragma once
#include "../../../../src/catelier/foundation/sync/ConditionalVar.hpp"
#include "../primitive/Mutex.hpp"

namespace Catelier::foundation::sync {
    class ConditionalVar {
        public:
            explicit ConditionalVar() {
                src::foundation::sync::ConditionalVar_init(&this->handle);
            }
            ~ConditionalVar() {
                if (this->handle.valid) {
                    src::foundation::sync::ConditionalVar_destroy(&this->handle);
                }
            }

            ConditionalVar(const ConditionalVar&) = delete;
            ConditionalVar(ConditionalVar&& other) noexcept {
                this->handle = other.handle;

                other.handle.valid = false;
            }
            ConditionalVar& operator = (const ConditionalVar&) = delete;
            ConditionalVar& operator = (ConditionalVar&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::sync::ConditionalVar_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto wait(primitive::Mutex& mutex) -> bool {
                return ConditionalVar_wait(&this->handle, mutex.rawHandle());
            }
            auto timedWait(primitive::Mutex& mutex, const u64 timeoutMs) -> bool {
                return src::foundation::sync::ConditionalVar_timedWait(&this->handle, mutex.rawHandle(), timeoutMs);
            }
            auto signal() -> bool {
                return src::foundation::sync::ConditionalVar_signal(&this->handle);
            }
            auto broadcast() -> bool {
                return src::foundation::sync::ConditionalVar_broadcast(&this->handle);
            }

            auto isValid() const -> bool {
                return this->handle.valid;
            }

        private:
            src::foundation::sync::ConditionalVar handle = {};
    };
}