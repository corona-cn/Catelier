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
                src::foundation::concurrency::ThreadLocal_construct(&this->handle, initFunc, onThreadExit, onDestroy, userData);
            }
            ~ThreadLocal() {
                if (this->handle.valid) {
                    src::foundation::concurrency::ThreadLocal_destruct(&this->handle);
                }
            }

            ThreadLocal(const ThreadLocal& other) {
                src::foundation::concurrency::ThreadLocal_copy(&this->handle, &other.handle);
            }
            ThreadLocal(ThreadLocal&& other) noexcept {
                src::foundation::concurrency::ThreadLocal_move(&this->handle, &other.handle);
            }
            ThreadLocal& operator = (const ThreadLocal& other) {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::concurrency::ThreadLocal_destruct(&this->handle);
                    }

                    src::foundation::concurrency::ThreadLocal_copy(&this->handle, &other.handle);
                }

                return *this;
            }
            ThreadLocal& operator = (ThreadLocal&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::concurrency::ThreadLocal_destruct(&this->handle);
                    }

                    src::foundation::concurrency::ThreadLocal_move(&this->handle, &other.handle);
                }

                return *this;
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

            auto isValid() const -> bool {
                return this->handle.valid;
            }

            auto entryCount() const -> usize {
                return src::foundation::concurrency::ThreadLocal_entryCount(&this->handle);
            }

        private:
            src::foundation::concurrency::ThreadLocal handle = {};
    };
}