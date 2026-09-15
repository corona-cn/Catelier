#pragma once
#include <cstdlib>
#include <new>
#include <utility>

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
                    explicit Iterator(src::foundation::DoublyLinkedListNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::DoublyLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return temp;
                    }
                    auto operator -- () -> Iterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> Iterator {
                        Iterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::DoublyLinkedListNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::DoublyLinkedListNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::DoublyLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return temp;
                    }
                    auto operator -- () -> ConstIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::DoublyLinkedListNode* node;
            };

            class ReverseIterator {
                public:
                    explicit ReverseIterator(src::foundation::DoublyLinkedListNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::DoublyLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> ReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ReverseIterator {
                        ReverseIterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return temp;
                    }
                    auto operator -- () -> ReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> ReverseIterator {
                        ReverseIterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const ReverseIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ReverseIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::DoublyLinkedListNode* node;
            };

            class ConstReverseIterator {
                public:
                    explicit ConstReverseIterator(const src::foundation::DoublyLinkedListNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::DoublyLinkedListNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::DoublyLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> ConstReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ConstReverseIterator {
                        ConstReverseIterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_prev(this->node);
                        return temp;
                    }
                    auto operator -- () -> ConstReverseIterator& {
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator -- (int) -> ConstReverseIterator {
                        ConstReverseIterator temp = *this;
                        this->node = src::foundation::DoublyLinkedListNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const ConstReverseIterator& other) const -> bool {
                        return this->node == other.node;
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
                    for (auto node = src::foundation::DoublyLinkedList_begin(this->handle); node; node = src::foundation::DoublyLinkedListNode_next(node)) {
                        ((Type*) src::foundation::DoublyLinkedListNode_data(node))->~Type();
                    }

                    src::foundation::DoublyLinkedList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            DoublyLinkedList(const DoublyLinkedList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::DoublyLinkedList_construct();
                if (!this->handle) {
                    return;
                }

                for (auto node = src::foundation::DoublyLinkedList_begin(other.handle); node; node = src::foundation::DoublyLinkedListNode_next(node)) {
                    void* const slot = src::foundation::DoublyLinkedList_pushTailSlot(this->handle, sizeof(Type));
                    if (!slot) {
                        this->clear();

                        src::foundation::DoublyLinkedList_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::DoublyLinkedListNode_data(node)));
                }
            }
            DoublyLinkedList(DoublyLinkedList&& other) noexcept {
                this->handle = src::foundation::DoublyLinkedList_move(other.handle);
            }
            DoublyLinkedList& operator = (const DoublyLinkedList& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::DoublyLinkedList_construct();
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (auto node = src::foundation::DoublyLinkedList_begin(other.handle); node; node = src::foundation::DoublyLinkedListNode_next(node)) {
                        this->pushTail(*((const Type*) src::foundation::DoublyLinkedListNode_data(node)));
                    }
                }

                return *this;
            }
            DoublyLinkedList& operator = (DoublyLinkedList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (auto node = src::foundation::DoublyLinkedList_begin(this->handle); node; node = src::foundation::DoublyLinkedListNode_next(node)) {
                            ((Type*) src::foundation::DoublyLinkedListNode_data(node))->~Type();
                        }

                        src::foundation::DoublyLinkedList_destruct(this->handle);
                    }

                    this->handle = src::foundation::DoublyLinkedList_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::DoublyLinkedList_pushHeadSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::DoublyLinkedList_pushHeadSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::DoublyLinkedList_pushTailSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::DoublyLinkedList_pushTailSlot(this->handle, sizeof(Type));
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }
            auto insertAt(const usize index, const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::DoublyLinkedList_insertAtSlot(this->handle, index, sizeof(Type));
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto insertAt(const usize index, Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::DoublyLinkedList_insertAtSlot(this->handle, index, sizeof(Type));
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

                void* const slot = src::foundation::DoublyLinkedList_popHeadSlot(this->handle);
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

                void* const slot = src::foundation::DoublyLinkedList_popTailSlot(this->handle);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                free(slot);

                return true;
            }
            auto removeAt(const usize index) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::DoublyLinkedList_removeAtSlot(this->handle, index);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                free(slot);

                return true;
            }
            auto removeIf(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                // 遍历查找第一个匹配的索引，然后走 removeAt 完成析构与移除
                usize index = 0;
                for (auto node = src::foundation::DoublyLinkedList_begin(this->handle); node; node = src::foundation::DoublyLinkedListNode_next(node)) {
                    if (*((const Type*) src::foundation::DoublyLinkedListNode_data(node)) == value) {
                        return this->removeAt(index);
                    }

                    index++;
                }

                return false;
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                for (auto node = src::foundation::DoublyLinkedList_begin(this->handle); node; node = src::foundation::DoublyLinkedListNode_next(node)) {
                    ((Type*) src::foundation::DoublyLinkedListNode_data(node))->~Type();
                }

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
                if (!this->handle) {
                    return false;
                }

                // 先析构目标位置的旧元素
                auto* const targetNode = src::foundation::DoublyLinkedList_get(this->handle, index);
                if (!targetNode) {
                    return false;
                }

                ((Type*) src::foundation::DoublyLinkedListNode_data(targetNode))->~Type();

                void* const slot = src::foundation::DoublyLinkedList_setSlot(this->handle, index, sizeof(Type));
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto set(const usize index, Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                auto* const targetNode = src::foundation::DoublyLinkedList_get(this->handle, index);
                if (!targetNode) {
                    return false;
                }

                ((Type*) src::foundation::DoublyLinkedListNode_data(targetNode))->~Type();

                void* const slot = src::foundation::DoublyLinkedList_setSlot(this->handle, index, sizeof(Type));
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
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