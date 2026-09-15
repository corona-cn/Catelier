#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/RingBuffer.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class RingBuffer {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::RingBuffer* handle = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->index = index;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::RingBuffer_get(this->handle, this->index));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::RingBuffer_get(this->handle, this->index);
                    }
                    auto operator [] (const usize offset) -> Type& {
                        return *((Type*) src::foundation::RingBuffer_get(this->handle, this->index + offset));
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
                    src::foundation::RingBuffer* handle;
                    usize index;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::RingBuffer* handle = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->index = index;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::RingBuffer_get(this->handle, this->index));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::RingBuffer_get(this->handle, this->index);
                    }
                    auto operator [] (const usize offset) const -> const Type& {
                        return *((const Type*) src::foundation::RingBuffer_get(this->handle, this->index + offset));
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
                    const src::foundation::RingBuffer* handle;
                    usize index;
            };

            explicit RingBuffer(const usize initialCapacity = 4) {
                this->handle = src::foundation::RingBuffer_construct(initialCapacity, sizeof(Type));
            }
            ~RingBuffer() {
                if (this->handle) {
                    for (usize i = 0; i < src::foundation::RingBuffer_size(this->handle); ++i) {
                        ((Type*) src::foundation::RingBuffer_get(this->handle, i))->~Type();
                    }

                    src::foundation::RingBuffer_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            RingBuffer(const RingBuffer& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::RingBuffer_construct(src::foundation::RingBuffer_capacity(other.handle), sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (usize i = 0; i < src::foundation::RingBuffer_size(other.handle); ++i) {
                    void* const slot = src::foundation::RingBuffer_pushTailSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::RingBuffer_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::RingBuffer_get(other.handle, i)));
                }
            }
            RingBuffer(RingBuffer&& other) noexcept {
                this->handle = src::foundation::RingBuffer_move(other.handle);
            }
            RingBuffer& operator = (const RingBuffer& other) {
                if (this != &other) {
                    this->clear();

                    // 句柄为空，或现有容量不足以容纳 other 的全部元素时，重新构造
                    if (!this->handle || src::foundation::RingBuffer_capacity(this->handle) < src::foundation::RingBuffer_size(other.handle)) {
                        if (this->handle) {
                            src::foundation::RingBuffer_destruct(this->handle);
                        }

                        this->handle = src::foundation::RingBuffer_construct(src::foundation::RingBuffer_capacity(other.handle), sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (usize i = 0; i < src::foundation::RingBuffer_size(other.handle); ++i) {
                        void* const slot = src::foundation::RingBuffer_pushTailSlot(this->handle);
                        if (!slot) {
                            return *this;
                        }

                        new(slot) Type(*((const Type*) src::foundation::RingBuffer_get(other.handle, i)));
                    }
                }

                return *this;
            }
            RingBuffer& operator = (RingBuffer&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (usize i = 0; i < src::foundation::RingBuffer_size(this->handle); ++i) {
                            ((Type*) src::foundation::RingBuffer_get(this->handle, i))->~Type();
                        }

                        src::foundation::RingBuffer_destruct(this->handle);
                    }

                    this->handle = src::foundation::RingBuffer_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::RingBuffer_pushHeadSlot(this->handle);
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

                void* const slot = src::foundation::RingBuffer_pushHeadSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }
            auto overwriteHead(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                if (src::foundation::RingBuffer_isFull(this->handle)) {
                    ((Type*) src::foundation::RingBuffer_head(this->handle))->~Type();
                    src::foundation::RingBuffer_popHead(this->handle);
                }

                void* const slot = src::foundation::RingBuffer_pushHeadSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto overwriteHead(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                if (src::foundation::RingBuffer_isFull(this->handle)) {
                    ((Type*) src::foundation::RingBuffer_head(this->handle))->~Type();
                    src::foundation::RingBuffer_popHead(this->handle);
                }

                void* const slot = src::foundation::RingBuffer_pushHeadSlot(this->handle);
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

                void* const slot = src::foundation::RingBuffer_pushTailSlot(this->handle);
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

                void* const slot = src::foundation::RingBuffer_pushTailSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }
            auto overwriteTail(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                if (src::foundation::RingBuffer_isFull(this->handle)) {
                    ((Type*) src::foundation::RingBuffer_head(this->handle))->~Type();
                    src::foundation::RingBuffer_popHead(this->handle);
                }

                void* const slot = src::foundation::RingBuffer_pushTailSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto overwriteTail(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                if (src::foundation::RingBuffer_isFull(this->handle)) {
                    ((Type*) src::foundation::RingBuffer_head(this->handle))->~Type();
                    src::foundation::RingBuffer_popHead(this->handle);
                }

                void* const slot = src::foundation::RingBuffer_pushTailSlot(this->handle);
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

                void* const slot = src::foundation::RingBuffer_popHeadSlot(this->handle);
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

                void* const slot = src::foundation::RingBuffer_popTailSlot(this->handle);
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

                for (usize i = 0; i < src::foundation::RingBuffer_size(this->handle); ++i) {
                    ((Type*) src::foundation::RingBuffer_get(this->handle, i))->~Type();
                }

                return src::foundation::RingBuffer_clear(this->handle);
            }

            auto get(const usize index) -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::RingBuffer_get(this->handle, index);
            }
            auto get(const usize index) const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::RingBuffer_get(this->handle, index);
            }
            auto head() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::RingBuffer_head(this->handle);
            }
            auto head() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::RingBuffer_head(this->handle);
            }
            auto tail() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::RingBuffer_tail(this->handle);
            }
            auto tail() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::RingBuffer_tail(this->handle);
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
                return this->handle ? src::foundation::RingBuffer_capacity(this->handle) : 0;
            }
            auto physicalCapacity() const -> usize {
                return this->handle ? src::foundation::RingBuffer_physicalCapacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::RingBuffer_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::RingBuffer_isEmpty(this->handle) : true;
            }
            auto isFull() const -> bool {
                return this->handle ? src::foundation::RingBuffer_isFull(this->handle) : false;
            }

        private:
            src::foundation::RingBuffer* handle = nullptr;
    };
}