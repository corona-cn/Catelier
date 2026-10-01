#pragma once
#include "../../../../src/catelier/foundation/primitive/OnceFlag.hpp"

namespace Catelier::foundation::primitive {
    class OnceFlag {
        public:
            explicit OnceFlag() {
                src::foundation::primitive::OnceFlag_init(&this->handle);
            }
            ~OnceFlag() {
                if (this->handle.valid) {
                    src::foundation::primitive::OnceFlag_destroy(&this->handle);
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
                        src::foundation::primitive::OnceFlag_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto run(void (*callback)()) -> bool {
                return src::foundation::primitive::OnceFlag_do(&this->handle, callback);
            }

            auto isValid() const -> bool {
                return this->handle.valid;
            }

        private:
            src::foundation::primitive::OnceFlag handle = {};
    };
}