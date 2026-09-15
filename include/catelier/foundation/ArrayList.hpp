#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/ArrayList.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class ArrayList {
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

            explicit ArrayList(const usize initialCapacity = 4) {
                this->handle = src::foundation::ArrayList_construct(initialCapacity, sizeof(Type));
            }
            ~ArrayList() {
                if (this->handle) {
                    Type* const element = (Type*) src::foundation::ArrayList_elements(this->handle);
                    for (usize i = 0; i < src::foundation::ArrayList_size(this->handle); ++i) {
                        element[i].~Type();
                    }

                    src::foundation::ArrayList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            ArrayList(const ArrayList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::ArrayList_construct(other.size(), sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (usize i = 0; i < other.size(); ++i) {
                    void* const slot = src::foundation::ArrayList_pushSlot(this->handle);
                    new(slot) Type(other[i]);
                }
            }
            ArrayList(ArrayList&& other) noexcept {
                this->handle = src::foundation::ArrayList_move(other.handle);
            }
            ArrayList& operator = (const ArrayList& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::ArrayList_construct(other.size(), sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (usize i = 0; i < other.size(); ++i) {
                        void* const slot = src::foundation::ArrayList_pushSlot(this->handle);
                        new(slot) Type(other[i]);
                    }
                }

                return *this;
            }
            ArrayList& operator = (ArrayList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        Type* const element = (Type*) src::foundation::ArrayList_elements(this->handle);
                        for (usize i = 0; i < src::foundation::ArrayList_size(this->handle); ++i) {
                            (*(element + i)).~Type();
                        }

                        src::foundation::ArrayList_destruct(this->handle);
                    }

                    this->handle = src::foundation::ArrayList_move(other.handle);
                }

                return *this;
            }

            auto push(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::ArrayList_pushSlot(this->handle);
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

                void* const slot = src::foundation::ArrayList_pushSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }
            auto insertAt(const usize index, const Type& value) -> bool {
                if (!this->handle || index > this->size()) {
                    return false;
                }

                if (index == this->size()) {
                    return this->push(value);
                }

                void* const slot = src::foundation::ArrayList_pushSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move((*this)[this->size() - 2]));

                // 将 index 之后的元素逐个右移一格
                for (usize i = this->size() - 2; i > index; --i) {
                    (*this)[i] = std::move((*this)[i - 1]);
                }

                // 在 index 处写入新元素
                (*this)[index] = value;

                return true;
            }

            auto pop() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::ArrayList_popSlot(this->handle);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                return true;
            }
            auto removeAt(const usize index) -> bool {
                if (!this->handle || index >= this->size()) {
                    return false;
                }

                // 将 index 之后的元素逐个左移一格
                for (usize i = index; i < this->size() - 1; ++i) {
                    (*this)[i] = std::move((*this)[i + 1]);
                }

                return this->pop();
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                Type* const element = (Type*) src::foundation::ArrayList_elements(this->handle);
                for (usize i = 0; i < src::foundation::ArrayList_size(this->handle); ++i) {
                    (*(element + i)).~Type();
                }

                return src::foundation::ArrayList_clear(this->handle);
            }

            auto reserve(const usize newCapacity) -> bool {
                return this->handle ? src::foundation::ArrayList_reserve(this->handle, newCapacity) : false;
            }
            auto shrinkToFit() -> bool {
                return this->handle ? src::foundation::ArrayList_shrinkToFit(this->handle) : false;
            }

            auto get(const usize index) -> Type& {
                return *((Type*) src::foundation::ArrayList_get(this->handle, index));
            }
            auto get(const usize index) const -> const Type& {
                return *((const Type*) src::foundation::ArrayList_get(this->handle, index));
            }
            auto operator[](const usize index) -> Type& {
                return get(index);
            }
            auto operator[](const usize index) const -> const Type& {
                return get(index);
            }

            auto set(const usize index, const Type& value) -> bool {
                if (!this->handle || index >= this->size()) {
                    return false;
                }

                (*this)[index] = value;

                return true;
            }

            auto begin() -> Iterator {
                return Iterator((Type*) src::foundation::ArrayList_elements(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator((const Type*) src::foundation::ArrayList_elements(this->handle));
            }
            auto end() -> Iterator {
                return this->begin() + this->size();
            }
            auto end() const -> ConstIterator {
                return this->begin() + this->size();
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::ArrayList_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::ArrayList_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::ArrayList_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::ArrayList* handle = nullptr;
    };
}