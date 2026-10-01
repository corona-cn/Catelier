#pragma once
#include "../../../../src/catelier/foundation/concurrency/ThreadLocal.hpp"

namespace Catelier::foundation::concurrency {
    class ThreadLocal {
        public:
            explicit ThreadLocal(
                void* (*initFunc)(void* userData) = nullptr,
                void (*onThreadExit)(void* value, void* userData) = nullptr,
                void (*onDestroy)(void* value, void* userData) = nullptr,
                void* userData = nullptr
            ) {
                src::foundation::concurrency::ThreadLocal_init(&this->handle, initFunc, onThreadExit, onDestroy, userData);
            }
            ~ThreadLocal() {
                if (this->handle.valid) {
                    src::foundation::concurrency::ThreadLocal_destroy(&this->handle);
                }
            }

            ThreadLocal(const ThreadLocal&) = delete;
            ThreadLocal(ThreadLocal&& other) noexcept {
                this->handle = other.handle;

                other.handle.valid = false;
            }
            ThreadLocal& operator = (const ThreadLocal&) = delete;
            ThreadLocal& operator = (ThreadLocal&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::concurrency::ThreadLocal_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto isValid() const -> bool {
                return this->handle.valid;
            }

            auto get() const -> void* {
                return src::foundation::concurrency::ThreadLocal_get(&this->handle);
            }
            auto getOrCreate() -> void* {
                return src::foundation::concurrency::ThreadLocal_getOrCreate(&this->handle);
            }

            auto forEach(void (*action)(void* value, void* inUserData), void* inUserData) const -> void {
                src::foundation::concurrency::ThreadLocal_forEach(&this->handle, action, inUserData);
            }

            auto entryCount() const -> usize {
                return src::foundation::concurrency::ThreadLocal_entryCount(&this->handle);
            }

        private:
            src::foundation::concurrency::ThreadLocal handle = {};
    };
}