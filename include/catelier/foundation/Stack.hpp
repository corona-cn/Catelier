#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/Stack.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class Stack {
        public:
            explicit Stack(const usize initialCapacity = 4) {
                this->handle = src::foundation::Stack_construct(initialCapacity, sizeof(Type));
            }
            ~Stack() {
                if (this->handle) {
                    Type* const element = (Type*) src::foundation::Stack_elements(this->handle);
                    for (usize i = 0; i < src::foundation::Stack_size(this->handle); ++i) {
                        (*(element + i)).~Type();
                    }

                    src::foundation::Stack_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            Stack(const Stack& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::Stack_construct(src::foundation::Stack_capacity(other.handle), sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (usize i = 0; i < src::foundation::Stack_size(other.handle); ++i) {
                    void* const slot = src::foundation::Stack_pushSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::Stack_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::Stack_get(other.handle, i)));
                }
            }
            Stack(Stack&& other) noexcept {
                this->handle = src::foundation::Stack_move(other.handle);
            }
            Stack& operator = (const Stack& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::Stack_construct(src::foundation::Stack_capacity(other.handle), sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (usize i = 0; i < src::foundation::Stack_size(other.handle); ++i) {
                        this->push(*((const Type*) src::foundation::Stack_get(other.handle, i)));
                    }
                }

                return *this;
            }
            Stack& operator = (Stack&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        Type* const element = (Type*) src::foundation::Stack_elements(this->handle);
                        for (usize i = 0; i < src::foundation::Stack_size(this->handle); ++i) {
                            (*(element + i)).~Type();
                        }

                        src::foundation::Stack_destruct(this->handle);
                    }

                    this->handle = src::foundation::Stack_move(other.handle);
                }

                return *this;
            }

            auto push(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::Stack_pushSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto push(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::Stack_pushSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }
            auto pop() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::Stack_popSlot(this->handle);
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

                Type* const element = (Type*) src::foundation::Stack_elements(this->handle);
                for (usize i = 0; i < src::foundation::Stack_size(this->handle); ++i) {
                    (*(element + i)).~Type();
                }

                return src::foundation::Stack_clear(this->handle);
            }

            auto peek() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::Stack_peek(this->handle);
            }
            auto peek() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::Stack_peek(this->handle);
            }

            auto reserve(const usize newCapacity) -> bool {
                return this->handle ? src::foundation::Stack_reserve(this->handle, newCapacity) : false;
            }
            auto shrinkToFit() -> bool {
                return this->handle ? src::foundation::Stack_shrinkToFit(this->handle) : false;
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::Stack_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::Stack_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::Stack_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::Stack* handle = nullptr;
    };
}