#pragma once
#include "../../../../src/catelier/foundation/sync/OnceFlag.hpp"

namespace Catelier::include::foundation::sync {
    class OnceFlag {
        public:
            auto isValid() const -> bool {
                return this->handle.valid;
            }

            explicit OnceFlag() {
                src::foundation::sync::OnceFlag_init(&this->handle);
            }
            ~OnceFlag() {
                if (this->handle.valid) {
                    src::foundation::sync::OnceFlag_destroy(&this->handle);
                }
            }
            OnceFlag(const OnceFlag&) = delete;
            OnceFlag(OnceFlag&& other) noexcept {
                this->handle = other.handle;

                other.handle.valid = false;
            }
            OnceFlag& operator = (const OnceFlag&) = delete;
            OnceFlag& operator = (OnceFlag&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::sync::OnceFlag_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto run(void (*callback)()) -> bool {
                return src::foundation::sync::OnceFlag_do(&this->handle, callback);
            }

        private:
            src::foundation::sync::OnceFlag handle = {};
    };
}