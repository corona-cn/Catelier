#pragma once
#include <cstdlib>
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/UnboundedDeque.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class UnboundedDeque {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::UnboundedDequeNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::UnboundedDequeNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::UnboundedDequeNode_data(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return temp;
                    }
                    auto operator -- () -> Iterator& {
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> Iterator {
                        Iterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::UnboundedDequeNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::UnboundedDequeNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::UnboundedDequeNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::UnboundedDequeNode_data(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return temp;
                    }
                    auto operator -- () -> ConstIterator& {
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::UnboundedDequeNode* node;
            };

            class ReverseIterator {
                public:
                    explicit ReverseIterator(src::foundation::UnboundedDequeNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::UnboundedDequeNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::UnboundedDequeNode_data(this->node);
                    }

                    auto operator ++ () -> ReverseIterator& {
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ReverseIterator {
                        ReverseIterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return temp;
                    }
                    auto operator -- () -> ReverseIterator& {
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> ReverseIterator {
                        ReverseIterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const ReverseIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ReverseIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::UnboundedDequeNode* node;
            };

            class ConstReverseIterator {
                public:
                    explicit ConstReverseIterator(const src::foundation::UnboundedDequeNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::UnboundedDequeNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::UnboundedDequeNode_data(this->node);
                    }

                    auto operator ++ () -> ConstReverseIterator& {
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ConstReverseIterator {
                        ConstReverseIterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_prev(this->node);
                        return temp;
                    }
                    auto operator -- () -> ConstReverseIterator& {
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> ConstReverseIterator {
                        ConstReverseIterator temp = *this;
                        this->node = src::foundation::UnboundedDequeNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const ConstReverseIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ConstReverseIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::UnboundedDequeNode* node;
            };

            explicit UnboundedDeque() {
                this->handle = src::foundation::UnboundedDeque_construct(sizeof(Type));
            }
            ~UnboundedDeque() {
                if (this->handle) {
                    for (auto node = src::foundation::UnboundedDeque_begin(this->handle); node; node = src::foundation::UnboundedDequeNode_next(node)) {
                        ((Type*) src::foundation::UnboundedDequeNode_data(node))->~Type();
                    }

                    src::foundation::UnboundedDeque_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            UnboundedDeque(const UnboundedDeque& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::UnboundedDeque_construct(sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (auto node = src::foundation::UnboundedDeque_begin(other.handle); node; node = src::foundation::UnboundedDequeNode_next(node)) {
                    void* const slot = src::foundation::UnboundedDeque_pushTailSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::UnboundedDeque_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::UnboundedDequeNode_data(node)));
                }
            }
            UnboundedDeque(UnboundedDeque&& other) noexcept {
                this->handle = src::foundation::UnboundedDeque_move(other.handle);
            }
            UnboundedDeque& operator = (const UnboundedDeque& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::UnboundedDeque_construct(sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (auto node = src::foundation::UnboundedDeque_begin(other.handle); node; node = src::foundation::UnboundedDequeNode_next(node)) {
                        this->pushTail(*((const Type*) src::foundation::UnboundedDequeNode_data(node)));
                    }
                }

                return *this;
            }
            UnboundedDeque& operator = (UnboundedDeque&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (auto node = src::foundation::UnboundedDeque_begin(this->handle); node; node = src::foundation::UnboundedDequeNode_next(node)) {
                            ((Type*) src::foundation::UnboundedDequeNode_data(node))->~Type();
                        }

                        src::foundation::UnboundedDeque_destruct(this->handle);
                    }

                    this->handle = src::foundation::UnboundedDeque_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::UnboundedDeque_pushHeadSlot(this->handle);
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

                void* const slot = src::foundation::UnboundedDeque_pushHeadSlot(this->handle);
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

                void* const slot = src::foundation::UnboundedDeque_pushTailSlot(this->handle);
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

                void* const slot = src::foundation::UnboundedDeque_pushTailSlot(this->handle);
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

                void* const slot = src::foundation::UnboundedDeque_popHeadSlot(this->handle);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                free(slot);

                return true;
            }
            auto popTail() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::UnboundedDeque_popTailSlot(this->handle);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                free(slot);

                return true;
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                for (auto node = src::foundation::UnboundedDeque_begin(this->handle); node; node = src::foundation::UnboundedDequeNode_next(node)) {
                    ((Type*) src::foundation::UnboundedDequeNode_data(node))->~Type();
                }

                return src::foundation::UnboundedDeque_clear(this->handle);
            }

            auto head() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::UnboundedDeque_head(this->handle);
            }
            auto head() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::UnboundedDeque_head(this->handle);
            }
            auto tail() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::UnboundedDeque_tail(this->handle);
            }
            auto tail() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::UnboundedDeque_tail(this->handle);
            }
            auto get(const usize index) -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::UnboundedDeque_get(this->handle, index);
            }
            auto get(const usize index) const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::UnboundedDeque_get(this->handle, index);
            }

            auto begin() -> Iterator {
                return Iterator(src::foundation::UnboundedDeque_begin(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(src::foundation::UnboundedDeque_begin(this->handle));
            }
            auto end() -> Iterator {
                return Iterator(src::foundation::UnboundedDeque_end());
            }
            auto end() const -> ConstIterator {
                return ConstIterator(src::foundation::UnboundedDeque_end());
            }

            auto reverseBegin() -> ReverseIterator {
                if (!this->handle) {
                    return ReverseIterator(nullptr);
                }

                return ReverseIterator(src::foundation::UnboundedDeque_tail(this->handle));
            }
            auto reverseBegin() const -> ConstReverseIterator {
                if (!this->handle) {
                    return ConstReverseIterator(nullptr);
                }

                return ConstReverseIterator(src::foundation::UnboundedDeque_tail(this->handle));
            }
            auto reverseEnd() -> ReverseIterator {
                return ReverseIterator(src::foundation::UnboundedDeque_end());
            }
            auto reverseEnd() const -> ConstReverseIterator {
                return ConstReverseIterator(src::foundation::UnboundedDeque_end());
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::UnboundedDeque_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::UnboundedDeque_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::UnboundedDeque* handle = nullptr;
    };
}