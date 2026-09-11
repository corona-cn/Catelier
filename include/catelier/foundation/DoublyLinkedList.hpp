#pragma once
#include "../../../src/catelier/foundation/DoublyLinkedList.hpp"

namespace Catelier::foundation {
    template<typename Type>
    auto defaultDataEquals(const void* a, const void* b) -> bool {
        return *((const Type*) a) == *((const Type*) b);
    }

    template<typename Type>
    class DoublyLinkedList {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::DoublyLinkedListNode* node) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator ++ () -> Iterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator -- () -> Iterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::DoublyLinkedListNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::DoublyLinkedListNode* node) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator ++ () -> ConstIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator -- () -> ConstIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::DoublyLinkedListNode* node;
            };

            class ReverseIterator {
                public:
                    explicit ReverseIterator(src::foundation::DoublyLinkedListNode* node) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator ++ () -> ReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator -- () -> ReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator != (const ReverseIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::DoublyLinkedListNode* node;
            };

            class ConstReverseIterator {
                public:
                    explicit ConstReverseIterator(const src::foundation::DoublyLinkedListNode* node) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator ++ () -> ConstReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator -- () -> ConstReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator != (const ConstReverseIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::DoublyLinkedListNode* node;
            };

            explicit DoublyLinkedList() {
                this->handle = src::foundation::DoublyLinkedList_construct();
            }
            ~DoublyLinkedList() {
                if (this->handle) {
                    src::foundation::DoublyLinkedList_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            DoublyLinkedList(const DoublyLinkedList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::DoublyLinkedList_copy(other.handle, sizeof(Type));
            }
            DoublyLinkedList(DoublyLinkedList&& other) noexcept {
                this->handle = src::foundation::DoublyLinkedList_move(other.handle);
            }
            DoublyLinkedList& operator = (const DoublyLinkedList& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::DoublyLinkedList_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::DoublyLinkedList_copy(other.handle, sizeof(Type));
                    }
                }

                return *this;
            }
            DoublyLinkedList& operator = (DoublyLinkedList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::DoublyLinkedList_destruct(this->handle);
                    }

                    this->handle = src::foundation::DoublyLinkedList_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                return src::foundation::DoublyLinkedList_pushHead(this->handle, &value, sizeof(Type));
            }
            auto pushTail(const Type& value) -> bool {
                return src::foundation::DoublyLinkedList_pushTail(this->handle, &value, sizeof(Type));
            }
            auto insertAt(const usize index, const Type& value) -> bool {
                return src::foundation::DoublyLinkedList_insertAt(this->handle, index, &value, sizeof(Type));
            }

            auto popHead() -> bool {
                return src::foundation::DoublyLinkedList_popHead(this->handle);
            }
            auto popTail() -> bool {
                return src::foundation::DoublyLinkedList_popTail(this->handle);
            }
            auto removeAt(const usize index) -> bool {
                return src::foundation::DoublyLinkedList_removeAt(this->handle, index);
            }
            auto removeIf(const Type& value) -> bool {
                return src::foundation::DoublyLinkedList_removeIf(this->handle, &value, defaultDataEquals<Type>);
            }
            auto clear() -> bool {
                return src::foundation::DoublyLinkedList_clear(this->handle);
            }

            auto get(const usize index) -> Type* {
                const auto* node = src::foundation::DoublyLinkedList_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::DoublyLinkedListNode_data(node);
            }
            auto get(const usize index) const -> const Type* {
                const auto* node = src::foundation::DoublyLinkedList_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::DoublyLinkedListNode_data(node);
            }
            auto head() -> Type* {
                const auto* node = src::foundation::DoublyLinkedList_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::DoublyLinkedListNode_data(node);
            }
            auto head() const -> const Type* {
                const auto* node = src::foundation::DoublyLinkedList_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::DoublyLinkedListNode_data(node);
            }
            auto tail() -> Type* {
                const auto* node = src::foundation::DoublyLinkedList_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::DoublyLinkedListNode_data(node);
            }
            auto tail() const -> const Type* {
                const auto* node = src::foundation::DoublyLinkedList_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::DoublyLinkedListNode_data(node);
            }

            auto set(const usize index, const Type& value) -> bool {
                return src::foundation::DoublyLinkedList_set(this->handle, index, &value, sizeof(Type));
            }

            auto reverse() -> bool {
                return src::foundation::DoublyLinkedList_reverse(this->handle);
            }

            auto begin() -> Iterator {
                return Iterator(src::foundation::DoublyLinkedList_begin(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(src::foundation::DoublyLinkedList_begin(this->handle));
            }
            auto end() -> Iterator {
                return Iterator(src::foundation::DoublyLinkedList_end());
            }
            auto end() const -> ConstIterator {
                return ConstIterator(src::foundation::DoublyLinkedList_end());
            }

            auto reverseBegin() -> ReverseIterator {
                return ReverseIterator(src::foundation::DoublyLinkedList_tail(this->handle));
            }
            auto reverseBegin() const -> ConstReverseIterator {
                return ConstReverseIterator(src::foundation::DoublyLinkedList_tail(this->handle));
            }
            auto reverseEnd() -> ReverseIterator {
                return ReverseIterator(src::foundation::DoublyLinkedList_end());
            }
            auto reverseEnd() const -> ConstReverseIterator {
                return ConstReverseIterator(src::foundation::DoublyLinkedList_end());
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::DoublyLinkedList_size(this->handle) : 0;
            }
            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::DoublyLinkedList_isEmpty(this->handle) : true;
            }
            auto contains(const Type& value) const -> bool {
                return src::foundation::DoublyLinkedList_contains(this->handle, &value, defaultDataEquals<Type>);
            }

        private:
            src::foundation::DoublyLinkedList* handle = nullptr;
    };
}