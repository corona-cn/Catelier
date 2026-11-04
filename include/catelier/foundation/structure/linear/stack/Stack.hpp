#pragma once
#include <new>
#include <utility>

#include "../../../../../../src/catelier/foundation/structure/linear/stack/Stack.hpp"

namespace Catelier::include::foundation::structure::linear::stack {
    template<typename Type>
    class Stack {
        public:
            class Iterator {
                public:
                    explicit Iterator(Type* ptr = nullptr) {
                        this->ptr = ptr;
                    }

                    auto operator * () -> Type& {
                        return *this->ptr;
                    }
                    auto operator -> () -> Type* {
                        return this->ptr;
                    }
                    auto operator [] (const usize index) -> Type& {
                        return this->ptr[index];
                    }

                    auto operator ++ () -> Iterator& {
                        ++this->ptr;
                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        ++this->ptr;
                        return temp;
                    }
                    auto operator -- () -> Iterator& {
                        --this->ptr;
                        return *this;
                    }
                    auto operator -- (int) -> Iterator {
                        Iterator temp = *this;
                        --this->ptr;
                        return temp;
                    }

                    auto operator += (const usize offset) -> Iterator& {
                        this->ptr += offset;
                        return *this;
                    }
                    auto operator -= (const usize offset) -> Iterator& {
                        this->ptr -= offset;
                        return *this;
                    }

                    auto operator + (const usize offset) const -> Iterator {
                        return Iterator(this->ptr + offset);
                    }
                    auto operator - (const usize offset) const -> Iterator {
                        return Iterator(this->ptr - offset);
                    }
                    auto operator - (const Iterator& other) const -> ssize {
                        return this->ptr - other.ptr;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->ptr == other.ptr;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->ptr != other.ptr;
                    }
                    auto operator < (const Iterator& other) const -> bool {
                        return this->ptr < other.ptr;
                    }
                    auto operator <= (const Iterator& other) const -> bool {
                        return this->ptr <= other.ptr;
                    }
                    auto operator > (const Iterator& other) const -> bool {
                        return this->ptr > other.ptr;
                    }
                    auto operator >= (const Iterator& other) const -> bool {
                        return this->ptr >= other.ptr;
                    }

                private:
                    Type* ptr;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const Type* ptr = nullptr) {
                        this->ptr = ptr;
                    }

                    auto operator * () const -> const Type& {
                        return *this->ptr;
                    }
                    auto operator -> () const -> const Type* {
                        return this->ptr;
                    }
                    auto operator [] (const usize index) const -> const Type& {
                        return this->ptr[index];
                    }

                    auto operator ++ () -> ConstIterator& {
                        ++this->ptr;
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        ++this->ptr;
                        return temp;
                    }
                    auto operator -- () -> ConstIterator& {
                        --this->ptr;
                        return *this;
                    }
                    auto operator -- (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        --this->ptr;
                        return temp;
                    }

                    auto operator += (const usize offset) -> ConstIterator& {
                        this->ptr += offset;
                        return *this;
                    }
                    auto operator -= (const usize offset) -> ConstIterator& {
                        this->ptr -= offset;
                        return *this;
                    }

                    auto operator + (const usize offset) const -> ConstIterator {
                        return ConstIterator(this->ptr + offset);
                    }
                    auto operator - (const usize offset) const -> ConstIterator {
                        return ConstIterator(this->ptr - offset);
                    }
                    auto operator - (const ConstIterator& other) const -> ssize {
                        return this->ptr - other.ptr;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->ptr == other.ptr;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->ptr != other.ptr;
                    }
                    auto operator < (const ConstIterator& other) const -> bool {
                        return this->ptr < other.ptr;
                    }
                    auto operator <= (const ConstIterator& other) const -> bool {
                        return this->ptr <= other.ptr;
                    }
                    auto operator > (const ConstIterator& other) const -> bool {
                        return this->ptr > other.ptr;
                    }
                    auto operator >= (const ConstIterator& other) const -> bool {
                        return this->ptr >= other.ptr;
                    }

                private:
                    const Type* ptr;
            };

            explicit Stack(const usize initialCapacity = 4) {
                this->handle = src::foundation::structure::linear::stack::Stack_construct(initialCapacity, sizeof(Type));
            }
            ~Stack() {
                if (this->handle) {
                    Type* const element = (Type*) src::foundation::structure::linear::stack::Stack_elements(this->handle);
                    for (usize i = 0; i < src::foundation::structure::linear::stack::Stack_size(this->handle); ++i) {
                        (*(element + i)).~Type();
                    }

                    src::foundation::structure::linear::stack::Stack_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            Stack(const Stack& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::structure::linear::stack::Stack_construct(src::foundation::structure::linear::stack::Stack_capacity(other.handle), sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (usize i = 0; i < src::foundation::structure::linear::stack::Stack_size(other.handle); ++i) {
                    void* const slot = src::foundation::structure::linear::stack::Stack_pushSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::structure::linear::stack::Stack_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::structure::linear::stack::Stack_get(other.handle, i)));
                }
            }
            Stack(Stack&& other) noexcept {
                this->handle = src::foundation::structure::linear::stack::Stack_move(other.handle);
            }
            Stack& operator = (const Stack& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::structure::linear::stack::Stack_construct(src::foundation::structure::linear::stack::Stack_capacity(other.handle), sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (usize i = 0; i < src::foundation::structure::linear::stack::Stack_size(other.handle); ++i) {
                        this->push(*((const Type*) src::foundation::structure::linear::stack::Stack_get(other.handle, i)));
                    }
                }

                return *this;
            }
            Stack& operator = (Stack&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        Type* const element = (Type*) src::foundation::structure::linear::stack::Stack_elements(this->handle);
                        for (usize i = 0; i < src::foundation::structure::linear::stack::Stack_size(this->handle); ++i) {
                            (*(element + i)).~Type();
                        }

                        src::foundation::structure::linear::stack::Stack_destruct(this->handle);
                    }

                    this->handle = src::foundation::structure::linear::stack::Stack_move(other.handle);
                }

                return *this;
            }

            auto push(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::structure::linear::stack::Stack_pushSlot(this->handle);
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

                void* const slot = src::foundation::structure::linear::stack::Stack_pushSlot(this->handle);
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

                void* const slot = src::foundation::structure::linear::stack::Stack_popSlot(this->handle);
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

                Type* const element = (Type*) src::foundation::structure::linear::stack::Stack_elements(this->handle);
                for (usize i = 0; i < src::foundation::structure::linear::stack::Stack_size(this->handle); ++i) {
                    (*(element + i)).~Type();
                }

                return src::foundation::structure::linear::stack::Stack_clear(this->handle);
            }

            auto peek() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::structure::linear::stack::Stack_peek(this->handle);
            }
            auto peek() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::structure::linear::stack::Stack_peek(this->handle);
            }

            auto reserve(const usize newCapacity) -> bool {
                return this->handle ? src::foundation::structure::linear::stack::Stack_reserve(this->handle, newCapacity) : false;
            }
            auto shrinkToFit() -> bool {
                return this->handle ? src::foundation::structure::linear::stack::Stack_shrinkToFit(this->handle) : false;
            }

            auto begin() -> Iterator {
                return Iterator((Type*) src::foundation::structure::linear::stack::Stack_elements(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator((const Type*) src::foundation::structure::linear::stack::Stack_elements(this->handle));
            }
            auto end() -> Iterator {
                return this->begin() + this->size();
            }
            auto end() const -> ConstIterator {
                return this->begin() + this->size();
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::structure::linear::stack::Stack_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::structure::linear::stack::Stack_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::structure::linear::stack::Stack_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::structure::linear::stack::Stack* handle = nullptr;
    };
}