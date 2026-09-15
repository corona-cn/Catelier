#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/BoundedDeque.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class BoundedDeque {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::BoundedDeque* handle = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->index = index;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::BoundedDeque_get(this->handle, this->index));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::BoundedDeque_get(this->handle, this->index);
                    }
                    auto operator [] (const usize offset) -> Type& {
                        return *((Type*) src::foundation::BoundedDeque_get(this->handle, this->index + offset));
                    }

                    auto operator ++ () -> Iterator& {
                        ++this->index;
                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        ++this->index;
                        return temp;
                    }
                    auto operator -- () -> Iterator& {
                        --this->index;
                        return *this;
                    }
                    auto operator -- (int) -> Iterator {
                        Iterator temp = *this;
                        --this->index;
                        return temp;
                    }

                    auto operator += (const usize offset) -> Iterator& {
                        this->index += offset;
                        return *this;
                    }
                    auto operator -= (const usize offset) -> Iterator& {
                        this->index -= offset;
                        return *this;
                    }

                    auto operator + (const usize offset) const -> Iterator {
                        return Iterator(this->handle, this->index + offset);
                    }
                    auto operator - (const usize offset) const -> Iterator {
                        return Iterator(this->handle, this->index - offset);
                    }
                    auto operator - (const Iterator& other) const -> ssize {
                        return (ssize) this->index - (ssize) other.index;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->handle == other.handle && this->index == other.index;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->handle != other.handle || this->index != other.index;
                    }
                    auto operator < (const Iterator& other) const -> bool {
                        return this->index < other.index;
                    }
                    auto operator <= (const Iterator& other) const -> bool {
                        return this->index <= other.index;
                    }
                    auto operator > (const Iterator& other) const -> bool {
                        return this->index > other.index;
                    }
                    auto operator >= (const Iterator& other) const -> bool {
                        return this->index >= other.index;
                    }

                private:
                    src::foundation::BoundedDeque* handle;
                    usize index;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::BoundedDeque* handle = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->index = index;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::BoundedDeque_get(this->handle, this->index));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::BoundedDeque_get(this->handle, this->index);
                    }
                    auto operator [] (const usize offset) const -> const Type& {
                        return *((const Type*) src::foundation::BoundedDeque_get(this->handle, this->index + offset));
                    }

                    auto operator ++ () -> ConstIterator& {
                        ++this->index;
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        ++this->index;
                        return temp;
                    }
                    auto operator -- () -> ConstIterator& {
                        --this->index;
                        return *this;
                    }
                    auto operator -- (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        --this->index;
                        return temp;
                    }

                    auto operator += (const usize offset) -> ConstIterator& {
                        this->index += offset;
                        return *this;
                    }
                    auto operator -= (const usize offset) -> ConstIterator& {
                        this->index -= offset;
                        return *this;
                    }

                    auto operator + (const usize offset) const -> ConstIterator {
                        return ConstIterator(this->handle, this->index + offset);
                    }
                    auto operator - (const usize offset) const -> ConstIterator {
                        return ConstIterator(this->handle, this->index - offset);
                    }
                    auto operator - (const ConstIterator& other) const -> ssize {
                        return (ssize) this->index - (ssize) other.index;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->handle == other.handle && this->index == other.index;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->handle != other.handle || this->index != other.index;
                    }
                    auto operator < (const ConstIterator& other) const -> bool {
                        return this->index < other.index;
                    }
                    auto operator <= (const ConstIterator& other) const -> bool {
                        return this->index <= other.index;
                    }
                    auto operator > (const ConstIterator& other) const -> bool {
                        return this->index > other.index;
                    }
                    auto operator >= (const ConstIterator& other) const -> bool {
                        return this->index >= other.index;
                    }

                private:
                    const src::foundation::BoundedDeque* handle;
                    usize index;
            };

            explicit BoundedDeque(const usize initialCapacity = 4) {
                this->handle = src::foundation::BoundedDeque_construct(initialCapacity, sizeof(Type));
            }
            ~BoundedDeque() {
                if (this->handle) {
                    for (usize i = 0; i < src::foundation::BoundedDeque_size(this->handle); ++i) {
                        ((Type*) src::foundation::BoundedDeque_get(this->handle, i))->~Type();
                    }

                    src::foundation::BoundedDeque_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            BoundedDeque(const BoundedDeque& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::BoundedDeque_construct(src::foundation::BoundedDeque_capacity(other.handle), sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (usize i = 0; i < src::foundation::BoundedDeque_size(other.handle); ++i) {
                    void* const slot = src::foundation::BoundedDeque_pushTailSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::BoundedDeque_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::BoundedDeque_get(other.handle, i)));
                }
            }
            BoundedDeque(BoundedDeque&& other) noexcept {
                this->handle = src::foundation::BoundedDeque_move(other.handle);
            }
            BoundedDeque& operator = (const BoundedDeque& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::BoundedDeque_construct(src::foundation::BoundedDeque_capacity(other.handle), sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (usize i = 0; i < src::foundation::BoundedDeque_size(other.handle); ++i) {
                        this->pushTail(*((const Type*) src::foundation::BoundedDeque_get(other.handle, i)));
                    }
                }

                return *this;
            }
            BoundedDeque& operator = (BoundedDeque&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (usize i = 0; i < src::foundation::BoundedDeque_size(this->handle); ++i) {
                            ((Type*) src::foundation::BoundedDeque_get(this->handle, i))->~Type();
                        }

                        src::foundation::BoundedDeque_destruct(this->handle);
                    }

                    this->handle = src::foundation::BoundedDeque_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedDeque_pushHeadSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto pushHead(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedDeque_pushHeadSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }
            auto pushTail(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedDeque_pushTailSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto pushTail(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedDeque_pushTailSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }

            auto popHead() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedDeque_popHeadSlot(this->handle);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                return true;
            }
            auto popTail() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BoundedDeque_popTailSlot(this->handle);
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

                for (usize i = 0; i < src::foundation::BoundedDeque_size(this->handle); ++i) {
                    ((Type*) src::foundation::BoundedDeque_get(this->handle, i))->~Type();
                }

                return src::foundation::BoundedDeque_clear(this->handle);
            }

            auto head() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::BoundedDeque_head(this->handle);
            }
            auto head() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::BoundedDeque_head(this->handle);
            }
            auto tail() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::BoundedDeque_tail(this->handle);
            }
            auto tail() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::BoundedDeque_tail(this->handle);
            }

            auto begin() -> Iterator {
                return Iterator(this->handle, 0);
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(this->handle, 0);
            }
            auto end() -> Iterator {
                return Iterator(this->handle, this->size());
            }
            auto end() const -> ConstIterator {
                return ConstIterator(this->handle, this->size());
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::BoundedDeque_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::BoundedDeque_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::BoundedDeque_isEmpty(this->handle) : true;
            }
            auto isFull() const -> bool {
                return this->handle ? src::foundation::BoundedDeque_isFull(this->handle) : false;
            }

        private:
            src::foundation::BoundedDeque* handle = nullptr;
    };
}