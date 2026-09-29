#pragma once
#include <cstdlib>
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/CircularLinkedList.hpp"

namespace Catelier::foundation {
    template<typename Type>
    auto CircularLinkedList_defaultDataEquals(const void* a, const void* b) -> bool {
        return *((const Type*) a) == *((const Type*) b);
    }

    template<typename Type>
    class CircularLinkedList {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::CircularLinkedListNode* node = nullptr, src::foundation::CircularLinkedListNode* endNode = nullptr) {
                        this->node = node;
                        this->endNode = endNode;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::CircularLinkedListNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::CircularLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        if (!this->node) {
                            return *this;
                        }

                        auto* const nextNode = src::foundation::CircularLinkedListNode_next(this->node);
                        if (!nextNode || nextNode == this->endNode) {
                            this->node = nullptr;
                            return *this;
                        }

                        this->node = nextNode;

                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        ++(*this);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::CircularLinkedListNode* node;
                    src::foundation::CircularLinkedListNode* endNode;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::CircularLinkedListNode* node = nullptr, const src::foundation::CircularLinkedListNode* endNode = nullptr) {
                        this->node = node;
                        this->endNode = endNode;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::CircularLinkedListNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::CircularLinkedListNode_data(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        if (!this->node) {
                            return *this;
                        }

                        auto* const nextNode = src::foundation::CircularLinkedListNode_next(this->node);
                        if (!nextNode || nextNode == this->endNode) {
                            this->node = nullptr;
                            return *this;
                        }

                        this->node = nextNode;

                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        ++(*this);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::CircularLinkedListNode* node;
                    const src::foundation::CircularLinkedListNode* endNode;
            };

            explicit CircularLinkedList() {
                this->handle = src::foundation::CircularLinkedList_construct();
            }
            ~CircularLinkedList() {
                if (this->handle) {
                    for (auto* node = src::foundation::CircularLinkedList_begin(this->handle);
                         node != nullptr;
                         node = nextNodeOf(this->handle, node)) {
                        ((Type*) src::foundation::CircularLinkedListNode_data(node))->~Type();
                    }

                    src::foundation::CircularLinkedList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            CircularLinkedList(const CircularLinkedList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::CircularLinkedList_construct();
                if (!this->handle) {
                    return;
                }

                for (auto* node = src::foundation::CircularLinkedList_begin(other.handle);
                     node != nullptr;
                     node = nextNodeOf(other.handle, node)) {
                    void* const slot = src::foundation::CircularLinkedList_pushTailSlot(this->handle, sizeof(Type));
                    if (!slot) {
                        this->clear();

                        src::foundation::CircularLinkedList_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::CircularLinkedListNode_data(node)));
                }
            }
            CircularLinkedList(CircularLinkedList&& other) noexcept {
                this->handle = src::foundation::CircularLinkedList_move(other.handle);
            }
            CircularLinkedList& operator = (const CircularLinkedList& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::CircularLinkedList_construct();
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (auto* node = src::foundation::CircularLinkedList_begin(other.handle);
                         node != nullptr;
                         node = nextNodeOf(other.handle, node)) {
                        this->pushTail(*((const Type*) src::foundation::CircularLinkedListNode_data(node)));
                    }
                }

                return *this;
            }
            CircularLinkedList& operator = (CircularLinkedList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (auto* node = src::foundation::CircularLinkedList_begin(this->handle);
                             node != nullptr;
                             node = nextNodeOf(this->handle, node)) {
                            ((Type*) src::foundation::CircularLinkedListNode_data(node))->~Type();
                        }

                        src::foundation::CircularLinkedList_destruct(this->handle);
                    }

                    this->handle = src::foundation::CircularLinkedList_move(other.handle);
                }

                return *this;
            }

            auto pushHead(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::CircularLinkedList_pushHeadSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::CircularLinkedList_pushHeadSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::CircularLinkedList_pushTailSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::CircularLinkedList_pushTailSlot(this->handle, sizeof(Type));
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

                void* const slot = src::foundation::CircularLinkedList_insertAtSlot(this->handle, index, sizeof(Type));
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

                void* const slot = src::foundation::CircularLinkedList_insertAtSlot(this->handle, index, sizeof(Type));
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

                void* const slot = src::foundation::CircularLinkedList_popHeadSlot(this->handle);
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

                void* const slot = src::foundation::CircularLinkedList_popTailSlot(this->handle);
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

                void* const slot = src::foundation::CircularLinkedList_removeAtSlot(this->handle, index);
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

                return src::foundation::CircularLinkedList_removeIf(this->handle, &value, CircularLinkedList_defaultDataEquals<Type>);
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                for (auto* node = src::foundation::CircularLinkedList_begin(this->handle);
                     node != nullptr;
                     node = nextNodeOf(this->handle, node)) {
                    ((Type*) src::foundation::CircularLinkedListNode_data(node))->~Type();
                }

                return src::foundation::CircularLinkedList_clear(this->handle);
            }

            auto get(const usize index) -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* node = src::foundation::CircularLinkedList_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::CircularLinkedListNode_data(node);
            }
            auto get(const usize index) const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* node = src::foundation::CircularLinkedList_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::CircularLinkedListNode_data(node);
            }
            auto head() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* node = src::foundation::CircularLinkedList_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::CircularLinkedListNode_data(node);
            }
            auto head() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* node = src::foundation::CircularLinkedList_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::CircularLinkedListNode_data(node);
            }
            auto tail() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* node = src::foundation::CircularLinkedList_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::CircularLinkedListNode_data(node);
            }
            auto tail() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* node = src::foundation::CircularLinkedList_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::CircularLinkedListNode_data(node);
            }

            auto set(const usize index, const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                auto* const targetNode = src::foundation::CircularLinkedList_get(this->handle, index);
                if (!targetNode) {
                    return false;
                }

                ((Type*) src::foundation::CircularLinkedListNode_data(targetNode))->~Type();

                void* const slot = src::foundation::CircularLinkedList_setSlot(this->handle, index, sizeof(Type));
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

                auto* const targetNode = src::foundation::CircularLinkedList_get(this->handle, index);
                if (!targetNode) {
                    return false;
                }

                ((Type*) src::foundation::CircularLinkedListNode_data(targetNode))->~Type();

                void* const slot = src::foundation::CircularLinkedList_setSlot(this->handle, index, sizeof(Type));
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }

            auto reverse() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::CircularLinkedList_reverse(this->handle);
            }

            auto begin() -> Iterator {
                if (!this->handle) {
                    return Iterator(nullptr, nullptr);
                }

                auto* const headNode = src::foundation::CircularLinkedList_head(this->handle);
                if (!headNode) {
                    return Iterator(nullptr, nullptr);
                }

                return Iterator(headNode, headNode);
            }
            auto begin() const -> ConstIterator {
                if (!this->handle) {
                    return ConstIterator(nullptr, nullptr);
                }

                const auto* const headNode = src::foundation::CircularLinkedList_head(this->handle);
                if (!headNode) {
                    return ConstIterator(nullptr, nullptr);
                }

                return ConstIterator(headNode, headNode);
            }
            auto end() -> Iterator {
                return Iterator(nullptr, nullptr);
            }
            auto end() const -> ConstIterator {
                return ConstIterator(nullptr, nullptr);
            }

            auto size() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::CircularLinkedList_size(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::CircularLinkedList_isEmpty(this->handle);
            }
            auto contains(const Type& value) const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::CircularLinkedList_contains(this->handle, &value, CircularLinkedList_defaultDataEquals<Type>);
            }

        private:
            src::foundation::CircularLinkedList* handle = nullptr;

            static auto nextNodeOf(const src::foundation::CircularLinkedList* handle, const src::foundation::CircularLinkedListNode* node) -> src::foundation::CircularLinkedListNode* {
                if (!handle || !node) {
                    return nullptr;
                }

                const auto* const nextNode = src::foundation::CircularLinkedListNode_next(node);
                if (!nextNode) {
                    return nullptr;
                }

                const auto* const headNode = src::foundation::CircularLinkedList_head(handle);
                if (!headNode) {
                    return nullptr;
                }

                if (nextNode == headNode) {
                    return nullptr;
                }

                return const_cast<src::foundation::CircularLinkedListNode*>(nextNode);
            }
    };
}