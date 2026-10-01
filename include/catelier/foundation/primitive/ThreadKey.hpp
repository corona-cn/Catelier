#pragma once
#include "../../../../src/catelier/foundation/primitive/ThreadKey.hpp"

namespace Catelier::foundation::primitive {
    class ThreadKey {
        public:
            explicit ThreadKey(void (*destructor)(void* value) = nullptr) {
                src::foundation::primitive::ThreadKey_init(&this->handle, destructor);
            }
            ~ThreadKey() {
                if (this->handle.valid) {
                    src::foundation::primitive::ThreadKey_destroy(&this->handle);
                }
            }

            ThreadKey(const ThreadKey&) = delete;
            ThreadKey(ThreadKey&& other) noexcept {
                this->handle = other.handle;

                other.handle.handle = 0;
                other.handle.valid = false;
            }
            ThreadKey& operator = (const ThreadKey&) = delete;
            ThreadKey& operator = (ThreadKey&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::primitive::ThreadKey_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.handle = 0;
                    other.handle.valid = false;
                }

                return *this;
            }

            auto set(void* value) const -> bool {
                return src::foundation::primitive::ThreadKey_set(&this->handle, value);
            }

            auto get() const -> void* {
                return src::foundation::primitive::ThreadKey_get(&this->handle);
            }

            auto isValid() const -> bool {
                return this->handle.valid;
            }

        private:
            src::foundation::primitive::ThreadKey handle = {};
    };
}