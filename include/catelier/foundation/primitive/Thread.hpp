#pragma once
#include "../../../../src/catelier/foundation/primitive/Thread.hpp"

#include <exception>

namespace Catelier::foundation::primitive {
    class Thread {
        public:
            explicit Thread() {
                src::foundation::primitive::Thread_init(&this->handle);
            }
            ~Thread() {
                if (this->handle.valid && this->handle.started) {
                    std::terminate();
                }

                if (this->handle.valid) {
                    src::foundation::primitive::Thread_destroy(&this->handle);
                }
            }

            Thread(const Thread&) = delete;
            Thread(Thread&&) = delete;
            Thread& operator = (const Thread&) = delete;
            Thread& operator = (Thread&&) = delete;

            auto start(const src::foundation::primitive::Thread_EntryFunc entry, void* userData = nullptr) -> bool {
                return src::foundation::primitive::Thread_start(&this->handle, entry, userData);
            }

            auto join() -> bool {
                return src::foundation::primitive::Thread_join(&this->handle);
            }
            auto detach() -> bool {
                return src::foundation::primitive::Thread_detach(&this->handle);
            }

            auto isValid() const -> bool {
                return this->handle.valid;
            }
            auto isStarted() const -> bool {
                return this->handle.started;
            }

            static auto currentId() -> u64 {
                return src::foundation::primitive::Thread_currentId();
            }
            static auto yield() -> void {
                src::foundation::primitive::Thread_yield();
            }

        private:
            src::foundation::primitive::Thread handle = {};
    };
}