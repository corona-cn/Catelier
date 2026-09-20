#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/BinarySearchTree.hpp"

namespace Catelier::foundation {
    template<typename Type>
    struct AscendingComparator {
        auto operator()(const Type& a, const Type& b) const -> bool {
            return a < b;
        }
    };

    template<typename Type>
    struct DescendingComparator {
        auto operator()(const Type& a, const Type& b) const -> bool {
            return a > b;
        }
    };

    template<typename Key, typename Value, typename Compare = AscendingComparator<Key>>
    class BinarySearchTree {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::BinarySearchTree* handle = nullptr, src::foundation::BinarySearchTreeNode* node = nullptr) {
                        this->handle = handle;
                        this->node = node;
                    }

                    auto key() -> Key& {
                        return *((Key*) src::foundation::BinarySearchTreeNode_key(this->node));
                    }
                    auto value() -> Value& {
                        return *((Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }

                    auto operator * () -> Value& {
                        return *((Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }
                    auto operator -> () -> Value* {
                        return (Value*) src::foundation::BinarySearchTreeNode_value(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findSuccessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        ++(*this);
                        return temp;
                    }
                    auto operator -- () -> Iterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findPredecessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator -- (int) -> Iterator {
                        Iterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node;
                    }

                private:
                    src::foundation::BinarySearchTree* handle;
                    src::foundation::BinarySearchTreeNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::BinarySearchTree* handle = nullptr, const src::foundation::BinarySearchTreeNode* node = nullptr) {
                        this->handle = handle;
                        this->node = node;
                    }

                    auto key() const -> const Key& {
                        return *((const Key*) src::foundation::BinarySearchTreeNode_key(this->node));
                    }
                    auto value() const -> const Value& {
                        return *((const Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }

                    auto operator * () const -> const Value& {
                        return *((const Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }
                    auto operator -> () const -> const Value* {
                        return (const Value*) src::foundation::BinarySearchTreeNode_value(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findSuccessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        ++(*this);
                        return temp;
                    }
                    auto operator -- () -> ConstIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findPredecessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator -- (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node;
                    }

                private:
                    const src::foundation::BinarySearchTree* handle;
                    const src::foundation::BinarySearchTreeNode* node;
            };

            class ReverseIterator {
                public:
                    explicit ReverseIterator(src::foundation::BinarySearchTree* handle = nullptr, src::foundation::BinarySearchTreeNode* node = nullptr) {
                        this->handle = handle;
                        this->node = node;
                    }

                    auto key() -> Key& {
                        return *((Key*) src::foundation::BinarySearchTreeNode_key(this->node));
                    }
                    auto value() -> Value& {
                        return *((Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }

                    auto operator * () -> Value& {
                        return *((Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }
                    auto operator -> () -> Value* {
                        return (Value*) src::foundation::BinarySearchTreeNode_value(this->node);
                    }

                    auto operator ++ () -> ReverseIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findPredecessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator ++ (int) -> ReverseIterator {
                        ReverseIterator temp = *this;
                        ++(*this);
                        return temp;
                    }
                    auto operator -- () -> ReverseIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findSuccessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator -- (int) -> ReverseIterator {
                        ReverseIterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const ReverseIterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node;
                    }
                    auto operator != (const ReverseIterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node;
                    }

                private:
                    src::foundation::BinarySearchTree* handle;
                    src::foundation::BinarySearchTreeNode* node;
            };

            class ConstReverseIterator {
                public:
                    explicit ConstReverseIterator(const src::foundation::BinarySearchTree* handle = nullptr, const src::foundation::BinarySearchTreeNode* node = nullptr) {
                        this->handle = handle;
                        this->node = node;
                    }

                    auto key() const -> const Key& {
                        return *((const Key*) src::foundation::BinarySearchTreeNode_key(this->node));
                    }
                    auto value() const -> const Value& {
                        return *((const Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }

                    auto operator * () const -> const Value& {
                        return *((const Value*) src::foundation::BinarySearchTreeNode_value(this->node));
                    }
                    auto operator -> () const -> const Value* {
                        return (const Value*) src::foundation::BinarySearchTreeNode_value(this->node);
                    }

                    auto operator ++ () -> ConstReverseIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findPredecessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator ++ (int) -> ConstReverseIterator {
                        ConstReverseIterator temp = *this;
                        ++(*this);
                        return temp;
                    }
                    auto operator -- () -> ConstReverseIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BinarySearchTreeNode_key(this->node);
                        this->node = src::foundation::BinarySearchTree_findSuccessorNode(this->handle, currentKey);

                        return *this;
                    }
                    auto operator -- (int) -> ConstReverseIterator {
                        ConstReverseIterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const ConstReverseIterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node;
                    }
                    auto operator != (const ConstReverseIterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node;
                    }

                private:
                    const src::foundation::BinarySearchTree* handle;
                    const src::foundation::BinarySearchTreeNode* node;
            };

            explicit BinarySearchTree() {
                this->handle = src::foundation::BinarySearchTree_construct(sizeof(Key), sizeof(Value), compareBridge);
            }
            ~BinarySearchTree() {
                if (this->handle) {
                    destructSubtreeElements(src::foundation::BinarySearchTree_rootNode(this->handle));

                    src::foundation::BinarySearchTree_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            BinarySearchTree(const BinarySearchTree& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::BinarySearchTree_construct(sizeof(Key), sizeof(Value), compareBridge);
                if (!this->handle) {
                    return;
                }

                other.forEach([this](const Key& key, const Value& value) {
                    this->insert(key, value);
                });
            }
            BinarySearchTree(BinarySearchTree&& other) noexcept {
                this->handle = src::foundation::BinarySearchTree_move(other.handle);
            }
            BinarySearchTree& operator = (const BinarySearchTree& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::BinarySearchTree_construct(sizeof(Key), sizeof(Value), compareBridge);
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    other.forEach([this](const Key& key, const Value& value) {
                        this->insert(key, value);
                    });
                }

                return *this;
            }
            BinarySearchTree& operator = (BinarySearchTree&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        destructSubtreeElements(src::foundation::BinarySearchTree_rootNode(this->handle));

                        src::foundation::BinarySearchTree_destruct(this->handle);
                    }

                    this->handle = src::foundation::BinarySearchTree_move(other.handle);
                }

                return *this;
            }

            auto insert(const Key& key, const Value& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* keySlot = nullptr;
                void* oldValueSlot = nullptr;
                void* newValueSlot = nullptr;

                if (!src::foundation::BinarySearchTree_insertSlot(this->handle, &key, &keySlot, &oldValueSlot, &newValueSlot)) {
                    return false;
                }

                if (oldValueSlot) {
                    ((Value*) oldValueSlot)->~Value();
                    free(oldValueSlot);
                }

                if (keySlot) {
                    new(keySlot) Key(key);
                }

                new(newValueSlot) Value(value);

                return true;
            }
            auto insert(const Key& key, Value&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* keySlot = nullptr;
                void* oldValueSlot = nullptr;
                void* newValueSlot = nullptr;

                if (!src::foundation::BinarySearchTree_insertSlot(this->handle, &key, &keySlot, &oldValueSlot, &newValueSlot)) {
                    return false;
                }

                if (oldValueSlot) {
                    ((Value*) oldValueSlot)->~Value();
                    free(oldValueSlot);
                }

                if (keySlot) {
                    new(keySlot) Key(key);
                }

                new(newValueSlot) Value(std::move(value));

                return true;
            }

            auto remove(const Key& key) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* keySlot = nullptr;
                void* valueSlot = nullptr;

                if (!src::foundation::BinarySearchTree_removeSlot(this->handle, &key, &keySlot, &valueSlot)) {
                    return false;
                }

                ((Key*) keySlot)->~Key();
                free(keySlot);

                ((Value*) valueSlot)->~Value();
                free(valueSlot);

                return true;
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                destructSubtreeElements(src::foundation::BinarySearchTree_rootNode(this->handle));

                return src::foundation::BinarySearchTree_clear(this->handle);
            }

            auto find(const Key& key) -> Value* {
                return (Value*) src::foundation::BinarySearchTree_find(this->handle, &key);
            }
            auto find(const Key& key) const -> const Value* {
                return (const Value*) src::foundation::BinarySearchTree_find(this->handle, &key);
            }
            auto operator[](const Key& key) -> Value& {
                Value* const value = find(key);
                if (value) {
                    return *value;
                }

                insert(key, Value{});

                return *find(key);
            }
            auto operator[](const Key& key) const -> const Value& {
                return *find(key);
            }

            template<typename Function>
            auto forEach(Function function) const -> void {
                if (!this->handle) {
                    return;
                }

                for (auto iterator = this->begin(); iterator != this->end(); ++iterator) {
                    function(iterator.key(), iterator.value());
                }
            }

            auto begin() -> Iterator {
                return Iterator(this->handle, src::foundation::BinarySearchTree_minNode(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(this->handle, src::foundation::BinarySearchTree_minNode(this->handle));
            }
            auto end() -> Iterator {
                return Iterator(this->handle, nullptr);
            }
            auto end() const -> ConstIterator {
                return ConstIterator(this->handle, nullptr);
            }

            auto reverseBegin() -> ReverseIterator {
                return ReverseIterator(this->handle, src::foundation::BinarySearchTree_maxNode(this->handle));
            }
            auto reverseBegin() const -> ConstReverseIterator {
                return ConstReverseIterator(this->handle, src::foundation::BinarySearchTree_maxNode(this->handle));
            }
            auto reverseEnd() -> ReverseIterator {
                return ReverseIterator(this->handle, nullptr);
            }
            auto reverseEnd() const -> ConstReverseIterator {
                return ConstReverseIterator(this->handle, nullptr);
            }

            auto keySize() const -> usize {
                return this->handle ? src::foundation::BinarySearchTree_keySize(this->handle) : 0;
            }
            auto valueSize() const -> usize {
                return this->handle ? src::foundation::BinarySearchTree_valueSize(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::BinarySearchTree_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::BinarySearchTree_isEmpty(this->handle) : true;
            }
            auto contains(const Key& key) const -> bool {
                return src::foundation::BinarySearchTree_contains(this->handle, &key);
            }

        private:
            src::foundation::BinarySearchTree* handle = nullptr;

            template<typename Type>
            static auto compareBridge(const void* a, const void* b) -> i32 {
                if (Compare{}(*((const Type*) a), *((const Type*) b))) {
                    return -1;
                }

                if (Compare{}(*((const Type*) b), *((const Type*) a))) {
                    return 1;
                }

                return 0;
            }

            static auto destructSubtreeElements(const src::foundation::BinarySearchTreeNode* node) -> void {
                if (!node) {
                    return;
                }

                destructSubtreeElements(src::foundation::BinarySearchTreeNode_left(node));
                destructSubtreeElements(src::foundation::BinarySearchTreeNode_right(node));

                ((Key*) src::foundation::BinarySearchTreeNode_key(node))->~Key();
                ((Value*) src::foundation::BinarySearchTreeNode_value(node))->~Value();
            }
    };
}