#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/BinaryHeap.hpp"

namespace Catelier::foundation {
    template<typename Type>
    struct LessComparator {
        auto operator()(const Type& a, const Type& b) const -> bool {
            return a < b;
        }
    };

    template<typename Type>
    struct GreaterComparator {
        auto operator()(const Type& a, const Type& b) const -> bool {
            return a > b;
        }
    };

    template<typename Type, typename Compare = GreaterComparator<Type>>
    class BinaryHeap {
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

            explicit BinaryHeap(const usize initialCapacity = 4) {
                this->handle = src::foundation::BinaryHeap_construct(initialCapacity, sizeof(Type), compareBridge);
            }
            ~BinaryHeap() {
                if (this->handle) {
                    Type* const element = (Type*) src::foundation::BinaryHeap_elements(this->handle);
                    for (usize i = 0; i < src::foundation::BinaryHeap_size(this->handle); ++i) {
                        (*(element + i)).~Type();
                    }

                    src::foundation::BinaryHeap_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            BinaryHeap(const BinaryHeap& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::BinaryHeap_construct(src::foundation::BinaryHeap_capacity(other.handle), sizeof(Type), compareBridge);
                if (!this->handle) {
                    return;
                }

                for (usize i = 0; i < other.size(); ++i) {
                    void* const slot = src::foundation::BinaryHeap_pushSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::BinaryHeap_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(other[i]);
                }

                src::foundation::BinaryHeap_heapify(this->handle);
            }
            BinaryHeap(BinaryHeap&& other) noexcept {
                this->handle = src::foundation::BinaryHeap_move(other.handle);
            }
            BinaryHeap& operator = (const BinaryHeap& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::BinaryHeap_construct(src::foundation::BinaryHeap_capacity(other.handle), sizeof(Type), compareBridge);
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (usize i = 0; i < other.size(); ++i) {
                        this->push(other[i]);
                    }
                }

                return *this;
            }
            BinaryHeap& operator = (BinaryHeap&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        Type* const element = (Type*) src::foundation::BinaryHeap_elements(this->handle);
                        for (usize i = 0; i < src::foundation::BinaryHeap_size(this->handle); ++i) {
                            (*(element + i)).~Type();
                        }

                        src::foundation::BinaryHeap_destruct(this->handle);
                    }

                    this->handle = src::foundation::BinaryHeap_move(other.handle);
                }

                return *this;
            }

            auto push(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BinaryHeap_pushSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                src::foundation::BinaryHeap_siftUpTail(this->handle);

                return true;
            }
            auto push(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BinaryHeap_pushSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                src::foundation::BinaryHeap_siftUpTail(this->handle);

                return true;
            }

            auto pop() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::BinaryHeap_popSlot(this->handle);
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

                Type* const element = (Type*) src::foundation::BinaryHeap_elements(this->handle);
                for (usize i = 0; i < src::foundation::BinaryHeap_size(this->handle); ++i) {
                    (*(element + i)).~Type();
                }

                return src::foundation::BinaryHeap_clear(this->handle);
            }

            auto heapify() -> bool {
                return this->handle ? src::foundation::BinaryHeap_heapify(this->handle) : false;
            }

            auto reserve(const usize newCapacity) -> bool {
                return this->handle ? src::foundation::BinaryHeap_reserve(this->handle, newCapacity) : false;
            }
            auto shrinkToFit() -> bool {
                return this->handle ? src::foundation::BinaryHeap_shrinkToFit(this->handle) : false;
            }

            auto peek() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::BinaryHeap_peek(this->handle);
            }
            auto peek() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::BinaryHeap_peek(this->handle);
            }
            auto get(const usize index) -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::BinaryHeap_get(this->handle, index);
            }
            auto get(const usize index) const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::BinaryHeap_get(this->handle, index);
            }

            auto begin() -> Iterator {
                return Iterator((Type*) src::foundation::BinaryHeap_elements(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator((const Type*) src::foundation::BinaryHeap_elements(this->handle));
            }
            auto end() -> Iterator {
                return this->begin() + this->size();
            }
            auto end() const -> ConstIterator {
                return this->begin() + this->size();
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::BinaryHeap_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::BinaryHeap_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::BinaryHeap_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::BinaryHeap* handle = nullptr;

            static auto compareBridge(const void* a, const void* b) -> bool {
                return Compare{}(*((const Type*) a), *((const Type*) b));
            }
    };
}