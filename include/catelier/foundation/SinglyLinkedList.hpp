#pragma once
#include <cstdlib>
#include <new>
#include <utility>

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
                    explicit Iterator(src::foundation::SinglyLinkedListNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::SinglyLinkedListNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::SinglyLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        this->node = src::foundation::SinglyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        this->node = src::foundation::SinglyLinkedListNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::SinglyLinkedListNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::SinglyLinkedListNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::SinglyLinkedListNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::SinglyLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        this->node = src::foundation::SinglyLinkedListNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        this->node = src::foundation::SinglyLinkedListNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->node == other.node;
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
                    for (auto node = src::foundation::SinglyLinkedList_begin(this->handle); node; node = src::foundation::SinglyLinkedListNode_next(node)) {
                        ((Type*) src::foundation::SinglyLinkedListNode_data(node))->~Type();
                    }

                    src::foundation::SinglyLinkedList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            SinglyLinkedList(const SinglyLinkedList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::SinglyLinkedList_construct();
                if (!this->handle) {
                    return;
                }

                for (auto node = src::foundation::SinglyLinkedList_begin(other.handle); node; node = src::foundation::SinglyLinkedListNode_next(node)) {
                    void* const slot = src::foundation::SinglyLinkedList_pushTailSlot(this->handle, sizeof(Type));
                    if (!slot) {
                        this->clear();

                        src::foundation::SinglyLinkedList_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::SinglyLinkedListNode_data(node)));
                }
            }
            SinglyLinkedList(SinglyLinkedList&& other) noexcept {
                this->handle = src::foundation::SinglyLinkedList_move(other.handle);
            }
            SinglyLinkedList& operator = (const SinglyLinkedList& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::SinglyLinkedList_construct();
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (auto node = src::foundation::SinglyLinkedList_begin(other.handle); node; node = src::foundation::SinglyLinkedListNode_next(node)) {
                        this->pushTail(*((const Type*) src::foundation::SinglyLinkedListNode_data(node)));
                    }
                }

                return *this;
            }
            SinglyLinkedList& operator = (SinglyLinkedList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (auto node = src::foundation::SinglyLinkedList_begin(this->handle); node; node = src::foundation::SinglyLinkedListNode_next(node)) {
                            ((Type*) src::foundation::SinglyLinkedListNode_data(node))->~Type();
                        }

                        src::foundation::SinglyLinkedList_destruct(this->handle);
                    }

                    this->handle = src::foundation::SinglyLinkedList_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::SinglyLinkedList_pushHeadSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::SinglyLinkedList_pushHeadSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::SinglyLinkedList_pushTailSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::SinglyLinkedList_pushTailSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::SinglyLinkedList_insertAtSlot(this->handle, index, sizeof(Type));
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

                void* const slot = src::foundation::SinglyLinkedList_insertAtSlot(this->handle, index, sizeof(Type));
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

                void* const slot = src::foundation::SinglyLinkedList_popHeadSlot(this->handle);
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

                void* const slot = src::foundation::SinglyLinkedList_popTailSlot(this->handle);
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

                void* const slot = src::foundation::SinglyLinkedList_removeAtSlot(this->handle, index);
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
                for (auto node = src::foundation::SinglyLinkedList_begin(this->handle); node; node = src::foundation::SinglyLinkedListNode_next(node)) {
                    if (*((const Type*) src::foundation::SinglyLinkedListNode_data(node)) == value) {
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

                for (auto node = src::foundation::SinglyLinkedList_begin(this->handle); node; node = src::foundation::SinglyLinkedListNode_next(node)) {
                    ((Type*) src::foundation::SinglyLinkedListNode_data(node))->~Type();
                }

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
                if (!this->handle) {
                    return false;
                }

                // 先析构目标位置的旧元素
                auto* const targetNode = src::foundation::SinglyLinkedList_get(this->handle, index);
                if (!targetNode) {
                    return false;
                }

                ((Type*) src::foundation::SinglyLinkedListNode_data(targetNode))->~Type();

                void* const slot = src::foundation::SinglyLinkedList_setSlot(this->handle, index, sizeof(Type));
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

                auto* const targetNode = src::foundation::SinglyLinkedList_get(this->handle, index);
                if (!targetNode) {
                    return false;
                }

                ((Type*) src::foundation::SinglyLinkedListNode_data(targetNode))->~Type();

                void* const slot = src::foundation::SinglyLinkedList_setSlot(this->handle, index, sizeof(Type));
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
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