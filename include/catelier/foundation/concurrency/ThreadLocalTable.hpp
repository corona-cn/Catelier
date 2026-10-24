#pragma once
#include <cassert>

#include "../../../../src/catelier/foundation/concurrency/ThreadLocalTable.hpp"

namespace Catelier::include::foundation::concurrency {
    class ThreadLocalTable {
        public:
            auto isValid() const -> bool {
                return this->handle.isValid;
            }

            explicit ThreadLocalTable(void* (*createValueCallback)(void* hostContext) = nullptr, void* (*valueThreadExitCallback)(void* value, void* hostContext) = nullptr, void (*destroyValueCallback)(void* value, void* hostContext) = nullptr, void* hostContext = nullptr) {
                src::foundation::concurrency::ThreadLocalTable_construct(&this->handle, createValueCallback, valueThreadExitCallback, destroyValueCallback, hostContext);
            }
            ~ThreadLocalTable() {
                if (this->handle.isValid) {
                    src::foundation::concurrency::ThreadLocalTable_destruct(&this->handle);
                }
            }
            ThreadLocalTable(const ThreadLocalTable& other) {
                src::foundation::concurrency::ThreadLocalTable_copy(&this->handle, &other.handle);
            }
            ThreadLocalTable(ThreadLocalTable&& other) noexcept {
                src::foundation::concurrency::ThreadLocalTable_move(&this->handle, &other.handle);
            }
            ThreadLocalTable& operator = (const ThreadLocalTable& other) {
                if (this != &other) {
                    if (this->handle.isValid) {
                        src::foundation::concurrency::ThreadLocalTable_destruct(&this->handle);
                    }

                    src::foundation::concurrency::ThreadLocalTable_copy(&this->handle, &other.handle);
                }

                return *this;
            }
            ThreadLocalTable& operator = (ThreadLocalTable&& other) noexcept {
                if (this != &other) {
                    if (this->handle.isValid) {
                        src::foundation::concurrency::ThreadLocalTable_destruct(&this->handle);
                    }

                    src::foundation::concurrency::ThreadLocalTable_move(&this->handle, &other.handle);
                }

                return *this;
            }

            auto entryCount() const -> usize {
                assert(this->handle.isValid);

                if (!this->handle.isValid) {
                    return 0;
                }

                return src::foundation::concurrency::ThreadLocalTable_entryCount(&this->handle);
            }

            auto get() const -> void* {
                assert(this->handle.isValid);

                if (!this->handle.isValid) {
                    return nullptr;
                }

                return src::foundation::concurrency::ThreadLocalTable_get(&this->handle);
            }
            auto getOrCreate() -> void* {
                assert(this->handle.isValid);

                if (!this->handle.isValid) {
                    return nullptr;
                }

                return src::foundation::concurrency::ThreadLocalTable_getOrCreate(&this->handle);
            }

            auto forEach(void (*actionCallback)(void* value, bool isThreadExited, void* actionContext), void* actionContext) const -> void {
                assert(this->handle.isValid);

                if (!this->handle.isValid || !actionCallback) {
                    return;
                }

                src::foundation::concurrency::ThreadLocalTable_forEach(&this->handle, actionCallback, actionContext);
            }

        private:
            src::foundation::concurrency::ThreadLocalTable handle = {};
    };
}