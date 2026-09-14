#pragma once
#include "../../../src/catelier/foundation/SinglyLinkedList.hpp"

namespace Catelier::foundation {
    template<typename Type>
    auto defaultDataEquals(const void* a, const void* b) -> bool {
        return *((const Type*) a) == *((const Type*) b);
    }

    template<typename Type>
    class SinglyLinkedList {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::SinglyLinkedListNode* node) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::SinglyLinkedListNode_data(this->node));
                    }
                    auto operator ++ () -> Iterator& {
                        this->node = src::foundation::SinglyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::SinglyLinkedListNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::SinglyLinkedListNode* node) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::SinglyLinkedListNode_data(this->node));
                    }
                    auto operator ++ () -> ConstIterator& {
                        this->node = src::foundation::SinglyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::SinglyLinkedListNode* node;
            };

            explicit SinglyLinkedList() {
                this->handle = src::foundation::SinglyLinkedList_construct();
            }
            ~SinglyLinkedList() {
                if (this->handle) {
                    src::foundation::SinglyLinkedList_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            SinglyLinkedList(const SinglyLinkedList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::SinglyLinkedList_copy(other.handle, sizeof(Type));
            }
            SinglyLinkedList(SinglyLinkedList&& other) noexcept {
                this->handle = src::foundation::SinglyLinkedList_move(other.handle);
            }
            SinglyLinkedList& operator = (const SinglyLinkedList& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::SinglyLinkedList_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::SinglyLinkedList_copy(other.handle, sizeof(Type));
                    }
                }

                return *this;
            }
            SinglyLinkedList& operator = (SinglyLinkedList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::SinglyLinkedList_destruct(this->handle);
                    }

                    this->handle = src::foundation::SinglyLinkedList_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                return src::foundation::SinglyLinkedList_pushHead(this->handle, &value, sizeof(Type));
            }
            auto pushTail(const Type& value) -> bool {
                return src::foundation::SinglyLinkedList_pushTail(this->handle, &value, sizeof(Type));
            }
            auto insertAt(const usize index, const Type& value) -> bool {
                return src::foundation::SinglyLinkedList_insertAt(this->handle, index, &value, sizeof(Type));
            }

            auto popHead() -> bool {
                return src::foundation::SinglyLinkedList_popHead(this->handle);
            }
            auto popTail() -> bool {
                return src::foundation::SinglyLinkedList_popTail(this->handle);
            }
            auto removeAt(const usize index) -> bool {
                return src::foundation::SinglyLinkedList_removeAt(this->handle, index);
            }
            auto removeIf(const Type& value) -> bool {
                return src::foundation::SinglyLinkedList_removeIf(this->handle, &value, defaultDataEquals<Type>);
            }
            auto clear() -> bool {
                return src::foundation::SinglyLinkedList_clear(this->handle);
            }

            auto get(const usize index) -> Type* {
                const auto* node = src::foundation::SinglyLinkedList_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::SinglyLinkedListNode_data(node);
            }
            auto get(const usize index) const -> const Type* {
                const auto* node = src::foundation::SinglyLinkedList_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::SinglyLinkedListNode_data(node);
            }
            auto head() -> Type* {
                const auto* node = src::foundation::SinglyLinkedList_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::SinglyLinkedListNode_data(node);
            }
            auto head() const -> const Type* {
                const auto* node = src::foundation::SinglyLinkedList_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::SinglyLinkedListNode_data(node);
            }
            auto tail() -> Type* {
                const auto* node = src::foundation::SinglyLinkedList_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::SinglyLinkedListNode_data(node);
            }
            auto tail() const -> const Type* {
                const auto* node = src::foundation::SinglyLinkedList_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::SinglyLinkedListNode_data(node);
            }

            auto set(const usize index, const Type& value) -> bool {
                return src::foundation::SinglyLinkedList_set(this->handle, index, &value, sizeof(Type));
            }

            auto reverse() -> bool {
                return src::foundation::SinglyLinkedList_reverse(this->handle);
            }

            auto begin() -> Iterator {
                return Iterator(src::foundation::SinglyLinkedList_begin(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(src::foundation::SinglyLinkedList_begin(this->handle));
            }
            auto end() -> Iterator {
                return Iterator(src::foundation::SinglyLinkedList_end());
            }
            auto end() const -> ConstIterator {
                return ConstIterator(src::foundation::SinglyLinkedList_end());
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::SinglyLinkedList_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::SinglyLinkedList_isEmpty(this->handle) : true;
            }
            auto contains(const Type& value) const -> bool {
                return src::foundation::SinglyLinkedList_contains(this->handle, &value, defaultDataEquals<Type>);
            }

        private:
            src::foundation::SinglyLinkedList* handle = nullptr;
    };
}