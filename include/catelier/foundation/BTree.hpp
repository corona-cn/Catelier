#pragma once
#include <cstdlib>
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/BTree.hpp"

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
    class BTree {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::BTree* handle = nullptr, src::foundation::BTreeNode* node = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->node = node;
                        this->index = index;
                    }

                    auto key() -> Key& {
                        return *((Key*) src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle)));
                    }
                    auto value() -> Value& {
                        return *((Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }

                    auto operator * () -> Value& {
                        return *((Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }
                    auto operator -> () -> Value* {
                        return (Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle));
                    }

                    auto operator ++ () -> Iterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* successorNode = (src::foundation::BTreeNode*) nullptr;
                        usize successorIndex = 0;

                        if (!src::foundation::BTree_findSuccessorSlot(this->handle, currentKey, &successorNode, &successorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = successorNode;
                        this->index = successorIndex;

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

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* predecessorNode = (src::foundation::BTreeNode*) nullptr;
                        usize predecessorIndex = 0;

                        if (!src::foundation::BTree_findPredecessorSlot(this->handle, currentKey, &predecessorNode, &predecessorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = predecessorNode;
                        this->index = predecessorIndex;

                        return *this;
                    }
                    auto operator -- (int) -> Iterator {
                        Iterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node && this->index == other.index;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node || this->index != other.index;
                    }

                private:
                    src::foundation::BTree* handle;
                    src::foundation::BTreeNode* node;
                    usize index;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::BTree* handle = nullptr, const src::foundation::BTreeNode* node = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->node = node;
                        this->index = index;
                    }

                    auto key() const -> const Key& {
                        return *((const Key*) src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle)));
                    }
                    auto value() const -> const Value& {
                        return *((const Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }

                    auto operator * () const -> const Value& {
                        return *((const Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }
                    auto operator -> () const -> const Value* {
                        return (const Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle));
                    }

                    auto operator ++ () -> ConstIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* successorNode = (src::foundation::BTreeNode*) nullptr;
                        usize successorIndex = 0;

                        if (!src::foundation::BTree_findSuccessorSlot(this->handle, currentKey, &successorNode, &successorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = successorNode;
                        this->index = successorIndex;

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

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* predecessorNode = (src::foundation::BTreeNode*) nullptr;
                        usize predecessorIndex = 0;

                        if (!src::foundation::BTree_findPredecessorSlot(this->handle, currentKey, &predecessorNode, &predecessorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = predecessorNode;
                        this->index = predecessorIndex;

                        return *this;
                    }
                    auto operator -- (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node && this->index == other.index;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node || this->index != other.index;
                    }

                private:
                    const src::foundation::BTree* handle;
                    const src::foundation::BTreeNode* node;
                    usize index;
            };

            class ReverseIterator {
                public:
                    explicit ReverseIterator(src::foundation::BTree* handle = nullptr, src::foundation::BTreeNode* node = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->node = node;
                        this->index = index;
                    }

                    auto key() -> Key& {
                        return *((Key*) src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle)));
                    }
                    auto value() -> Value& {
                        return *((Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }

                    auto operator * () -> Value& {
                        return *((Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }
                    auto operator -> () -> Value* {
                        return (Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle));
                    }

                    auto operator ++ () -> ReverseIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* predecessorNode = (src::foundation::BTreeNode*) nullptr;
                        usize predecessorIndex = 0;

                        if (!src::foundation::BTree_findPredecessorSlot(this->handle, currentKey, &predecessorNode, &predecessorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = predecessorNode;
                        this->index = predecessorIndex;

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

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* successorNode = (src::foundation::BTreeNode*) nullptr;
                        usize successorIndex = 0;

                        if (!src::foundation::BTree_findSuccessorSlot(this->handle, currentKey, &successorNode, &successorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = successorNode;
                        this->index = successorIndex;

                        return *this;
                    }
                    auto operator -- (int) -> ReverseIterator {
                        ReverseIterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const ReverseIterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node && this->index == other.index;
                    }
                    auto operator != (const ReverseIterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node || this->index != other.index;
                    }

                private:
                    src::foundation::BTree* handle;
                    src::foundation::BTreeNode* node;
                    usize index;
            };

            class ConstReverseIterator {
                public:
                    explicit ConstReverseIterator(const src::foundation::BTree* handle = nullptr, const src::foundation::BTreeNode* node = nullptr, const usize index = 0) {
                        this->handle = handle;
                        this->node = node;
                        this->index = index;
                    }

                    auto key() const -> const Key& {
                        return *((const Key*) src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle)));
                    }
                    auto value() const -> const Value& {
                        return *((const Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }

                    auto operator * () const -> const Value& {
                        return *((const Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle)));
                    }
                    auto operator -> () const -> const Value* {
                        return (const Value*) src::foundation::BTreeNode_value(this->node, this->index, src::foundation::BTree_valueSize(this->handle));
                    }

                    auto operator ++ () -> ConstReverseIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* predecessorNode = (src::foundation::BTreeNode*) nullptr;
                        usize predecessorIndex = 0;

                        if (!src::foundation::BTree_findPredecessorSlot(this->handle, currentKey, &predecessorNode, &predecessorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = predecessorNode;
                        this->index = predecessorIndex;

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

                        const void* const currentKey = src::foundation::BTreeNode_key(this->node, this->index, src::foundation::BTree_keySize(this->handle));

                        auto* successorNode = (src::foundation::BTreeNode*) nullptr;
                        usize successorIndex = 0;

                        if (!src::foundation::BTree_findSuccessorSlot(this->handle, currentKey, &successorNode, &successorIndex)) {
                            this->node = nullptr;
                            this->index = 0;
                            return *this;
                        }

                        this->node = successorNode;
                        this->index = successorIndex;

                        return *this;
                    }
                    auto operator -- (int) -> ConstReverseIterator {
                        ConstReverseIterator temp = *this;
                        --(*this);
                        return temp;
                    }

                    auto operator == (const ConstReverseIterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node && this->index == other.index;
                    }
                    auto operator != (const ConstReverseIterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node || this->index != other.index;
                    }

                private:
                    const src::foundation::BTree* handle;
                    const src::foundation::BTreeNode* node;
                    usize index;
            };

            explicit BTree(const usize inOrder = 4) {
                this->handle = src::foundation::BTree_construct(inOrder, sizeof(Key), sizeof(Value), compareBridge);
            }
            ~BTree() {
                if (this->handle) {
                    destructSubtreeElements(this->handle, src::foundation::BTree_rootNode(this->handle));

                    src::foundation::BTree_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            BTree(const BTree& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::BTree_construct(src::foundation::BTree_order(other.handle), sizeof(Key), sizeof(Value), compareBridge);
                if (!this->handle) {
                    return;
                }

                other.forEach([this](const Key& key, const Value& value) {
                    this->insert(key, value);
                });
            }
            BTree(BTree&& other) noexcept {
                this->handle = src::foundation::BTree_move(other.handle);
            }
            BTree& operator = (const BTree& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::BTree_construct(src::foundation::BTree_order(other.handle), sizeof(Key), sizeof(Value), compareBridge);
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
            BTree& operator = (BTree&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        destructSubtreeElements(this->handle, src::foundation::BTree_rootNode(this->handle));

                        src::foundation::BTree_destruct(this->handle);
                    }

                    this->handle = src::foundation::BTree_move(other.handle);
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

                if (!src::foundation::BTree_insertSlot(this->handle, &key, &keySlot, &oldValueSlot, &newValueSlot)) {
                    return false;
                }

                // 如果键已存在，先析构旧值
                // B 树 value 是内联存储，槽位在节点内，不需要 free
                if (oldValueSlot) {
                    ((Value*) oldValueSlot)->~Value();
                }

                // 如果是新键，构造键
                if (keySlot) {
                    new(keySlot) Key(key);
                }

                // 构造新值
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

                if (!src::foundation::BTree_insertSlot(this->handle, &key, &keySlot, &oldValueSlot, &newValueSlot)) {
                    return false;
                }

                if (oldValueSlot) {
                    ((Value*) oldValueSlot)->~Value();
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

                if (!src::foundation::BTree_removeSlot(this->handle, &key, &keySlot, &valueSlot)) {
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

                destructSubtreeElements(this->handle, src::foundation::BTree_rootNode(this->handle));

                return src::foundation::BTree_clear(this->handle);
            }

            auto find(const Key& key) -> Value* {
                return (Value*) src::foundation::BTree_find(this->handle, &key);
            }
            auto find(const Key& key) const -> const Value* {
                return (const Value*) src::foundation::BTree_find(this->handle, &key);
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
                if (!this->handle) {
                    return Iterator(nullptr, nullptr, 0);
                }

                src::foundation::BTreeNode* node = nullptr;
                usize index = 0;

                if (!src::foundation::BTree_minSlot(this->handle, &node, &index)) {
                    return Iterator(this->handle, nullptr, 0);
                }

                return Iterator(this->handle, node, index);
            }
            auto begin() const -> ConstIterator {
                if (!this->handle) {
                    return ConstIterator(nullptr, nullptr, 0);
                }

                src::foundation::BTreeNode* node = nullptr;
                usize index = 0;

                if (!src::foundation::BTree_minSlot(this->handle, &node, &index)) {
                    return ConstIterator(this->handle, nullptr, 0);
                }

                return ConstIterator(this->handle, node, index);
            }
            auto end() -> Iterator {
                return Iterator(this->handle, nullptr, 0);
            }
            auto end() const -> ConstIterator {
                return ConstIterator(this->handle, nullptr, 0);
            }

            auto reverseBegin() -> ReverseIterator {
                if (!this->handle) {
                    return ReverseIterator(nullptr, nullptr, 0);
                }

                src::foundation::BTreeNode* node = nullptr;
                usize index = 0;

                if (!src::foundation::BTree_maxSlot(this->handle, &node, &index)) {
                    return ReverseIterator(this->handle, nullptr, 0);
                }

                return ReverseIterator(this->handle, node, index);
            }
            auto reverseBegin() const -> ConstReverseIterator {
                if (!this->handle) {
                    return ConstReverseIterator(nullptr, nullptr, 0);
                }

                src::foundation::BTreeNode* node = nullptr;
                usize index = 0;

                if (!src::foundation::BTree_maxSlot(this->handle, &node, &index)) {
                    return ConstReverseIterator(this->handle, nullptr, 0);
                }

                return ConstReverseIterator(this->handle, node, index);
            }
            auto reverseEnd() -> ReverseIterator {
                return ReverseIterator(this->handle, nullptr, 0);
            }
            auto reverseEnd() const -> ConstReverseIterator {
                return ConstReverseIterator(this->handle, nullptr, 0);
            }

            auto height() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                auto* node = src::foundation::BTree_rootNode(this->handle);
                usize height = 0;

                while (node) {
                    height++;

                    if (src::foundation::BTreeNode_isLeaf(node)) {
                        break;
                    }

                    node = src::foundation::BTreeNode_child(node, 0);
                }

                return height;
            }
            auto order() const -> usize {
                return this->handle ? src::foundation::BTree_order(this->handle) : 0;
            }
            auto keySize() const -> usize {
                return this->handle ? src::foundation::BTree_keySize(this->handle) : 0;
            }
            auto valueSize() const -> usize {
                return this->handle ? src::foundation::BTree_valueSize(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::BTree_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::BTree_isEmpty(this->handle) : true;
            }
            auto contains(const Key& key) const -> bool {
                return src::foundation::BTree_contains(this->handle, &key);
            }

        private:
            src::foundation::BTree* handle = nullptr;

            static auto compareBridge(const void* a, const void* b) -> i32 {
                if (Compare{}(*((const Key*) a), *((const Key*) b))) {
                    return -1;
                }

                if (Compare{}(*((const Key*) b), *((const Key*) a))) {
                    return 1;
                }

                return 0;
            }

            static auto destructSubtreeElements(const src::foundation::BTree* self, const src::foundation::BTreeNode* node) -> void {
                if (!node) {
                    return;
                }

                const usize keyCount = src::foundation::BTreeNode_keyCount(node);

                if (!src::foundation::BTreeNode_isLeaf(node)) {
                    for (usize i = 0; i <= keyCount; ++i) {
                        destructSubtreeElements(self, src::foundation::BTreeNode_child(node, i));
                    }
                }

                for (usize i = 0; i < keyCount; ++i) {
                    ((Key*) src::foundation::BTreeNode_key(node, i, src::foundation::BTree_keySize(self)))->~Key();
                    ((Value*) src::foundation::BTreeNode_value(node, i, src::foundation::BTree_valueSize(self)))->~Value();
                }
            }
    };
}