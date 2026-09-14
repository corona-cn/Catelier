#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/BoundedQueue.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class BoundedQueue {
        public:
            explicit BoundedQueue(const usize initialCapacity = 4) {
                this->handle = src::foundation::BoundedQueue_construct(initialCapacity, sizeof(Type));
            }
            ~BoundedQueue() {
                if (this->handle) {
                    for (usize i = 0; i < src::foundation::BoundedQueue_size(this->handle); ++i) {
                        ((Type*) src::foundation::BoundedQueue_get(this->handle, i))->~Type();
                    }

                    src::foundation::BoundedQueue_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            BoundedQueue(const BoundedQueue& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::BoundedQueue_construct(src::foundation::BoundedQueue_capacity(other.handle), sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (usize i = 0; i < src::foundation::BoundedQueue_size(other.handle); ++i) {
                    void* const slot = src::foundation::BoundedQueue_enqueueSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::BoundedQueue_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::BoundedQueue_get(other.handle, i)));
                }
            }
            BoundedQueue(BoundedQueue&& other) noexcept {
                this->handle = src::foundation::BoundedQueue_move(other.handle);
            }
            BoundedQueue& operator = (const BoundedQueue& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::BoundedQueue_construct(src::foundation::BoundedQueue_capacity(other.handle), sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (usize i = 0; i < src::foundation::BoundedQueue_size(other.handle); ++i) {
                        this->enqueue(*((const Type*) src::foundation::BoundedQueue_get(other.handle, i)));
                    }
                }

                return *this;
            }
            BoundedQueue& operator = (BoundedQueue&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (usize i = 0; i < src::foundation::BoundedQueue_size(this->handle); ++i) {
                            ((Type*) src::foundation::BoundedQueue_get(this->handle, i))->~Type();
                        }

                        src::foundation::BoundedQueue_destruct(this->handle);
                    }

                    this->handle = src::foundation::BoundedQueue_move(other.handle);
                }

                return *this;
            }

            auto enqueue(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedQueue_enqueueSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto enqueue(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedQueue_enqueueSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }

            auto dequeue() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedQueue_dequeueSlot(this->handle);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                return true;
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                for (usize i = 0; i < src::foundation::BoundedQueue_size(this->handle); ++i) {
                    ((Type*) src::foundation::BoundedQueue_get(this->handle, i))->~Type();
                }

                return src::foundation::BoundedQueue_clear(this->handle);
            }

            auto head() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::BoundedQueue_head(this->handle);
            }
            auto head() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::BoundedQueue_head(this->handle);
            }
            auto tail() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::BoundedQueue_tail(this->handle);
            }
            auto tail() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::BoundedQueue_tail(this->handle);
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::BoundedQueue_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::BoundedQueue_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::BoundedQueue_isEmpty(this->handle) : true;
            }
            auto isFull() const -> bool {
                return this->handle ? src::foundation::BoundedQueue_isFull(this->handle) : false;
            }

        private:
            src::foundation::BoundedQueue* handle = nullptr;
    };
}