#pragma once
#include <cstdlib>
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/ChainHashMap.hpp"
#include "../../../src/catelier/util/HashUtils.hpp"

namespace Catelier::foundation {
    template<typename Key>
    auto defaultHash(const void* key) -> u64 {
        return src::util::hash::FNV1a::hash64Mem(key, sizeof(Key));
    }

    template<typename Key>
    auto defaultKeyEquals(const void* a, const void* b) -> bool {
        return *((const Key*) a) == *((const Key*) b);
    }

    template<typename Key, typename Value>
    class ChainHashMap {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::ChainHashMap* handle = nullptr, const usize bucketIndex = 0, src::foundation::ChainHashMapNode* node = nullptr) {
                        this->handle = handle;
                        this->bucketIndex = bucketIndex;
                        this->node = node;
                    }

                    auto key() -> Key& {
                        return *((Key*) src::foundation::ChainHashMapNode_key(this->node));
                    }
                    auto value() -> Value& {
                        return *((Value*) src::foundation::ChainHashMapNode_value(this->node));
                    }

                    auto operator * () -> Value& {
                        return *((Value*) src::foundation::ChainHashMapNode_value(this->node));
                    }
                    auto operator -> () -> Value* {
                        return (Value*) src::foundation::ChainHashMapNode_value(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        // 先尝试当前桶链表内的下一个节点
                        auto* const nextNode = src::foundation::ChainHashMapNode_next(this->node);
                        if (nextNode) {
                            this->node = nextNode;
                            return *this;
                        }

                        // 当前桶链表已走完，扫描后续桶寻找下一个非空桶
                        const usize capacity = src::foundation::ChainHashMap_capacity(this->handle);
                        for (usize i = this->bucketIndex + 1; i < capacity; ++i) {
                            auto* const headNode = src::foundation::ChainHashMap_headAt(this->handle, i);
                            if (headNode) {
                                this->bucketIndex = i;
                                this->node = headNode;
                                return *this;
                            }
                        }

                        // 没有更多节点，置为尾部迭代器
                        this->bucketIndex = capacity;
                        this->node = nullptr;

                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        ++(*this);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node;
                    }

                private:
                    src::foundation::ChainHashMap* handle;
                    usize bucketIndex;
                    src::foundation::ChainHashMapNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::ChainHashMap* handle = nullptr, const usize bucketIndex = 0, src::foundation::ChainHashMapNode* node = nullptr) {
                        this->handle = handle;
                        this->bucketIndex = bucketIndex;
                        this->node = node;
                    }

                    auto key() const -> const Key& {
                        return *((const Key*) src::foundation::ChainHashMapNode_key(this->node));
                    }
                    auto value() const -> const Value& {
                        return *((const Value*) src::foundation::ChainHashMapNode_value(this->node));
                    }

                    auto operator * () const -> const Value& {
                        return *((const Value*) src::foundation::ChainHashMapNode_value(this->node));
                    }
                    auto operator -> () const -> const Value* {
                        return (const Value*) src::foundation::ChainHashMapNode_value(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        // 先尝试当前桶链表内的下一个节点
                        auto* const nextNode = src::foundation::ChainHashMapNode_next(this->node);
                        if (nextNode) {
                            this->node = nextNode;
                            return *this;
                        }

                        // 当前桶链表已走完，扫描后续桶寻找下一个非空桶
                        const usize capacity = src::foundation::ChainHashMap_capacity(this->handle);
                        for (usize i = this->bucketIndex + 1; i < capacity; ++i) {
                            auto* const headNode = src::foundation::ChainHashMap_headAt(this->handle, i);
                            if (headNode) {
                                this->bucketIndex = i;
                                this->node = headNode;
                                return *this;
                            }
                        }

                        // 没有更多节点，置为尾部迭代器
                        this->bucketIndex = capacity;
                        this->node = nullptr;

                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        ++(*this);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->handle == other.handle && this->node == other.node;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->handle != other.handle || this->node != other.node;
                    }

                private:
                    const src::foundation::ChainHashMap* handle;
                    usize bucketIndex;
                    src::foundation::ChainHashMapNode* node;
            };

            explicit ChainHashMap(const usize initialCapacity = 4, u64 (*hash)(const void*) = defaultHash<Key>, bool (*keyEquals)(const void*, const void*) = defaultKeyEquals<Key>) {
                this->handle = src::foundation::ChainHashMap_construct(initialCapacity, sizeof(Key), sizeof(Value), hash, keyEquals);
            }
            ~ChainHashMap() {
                if (this->handle) {
                    for (usize i = 0; i < src::foundation::ChainHashMap_capacity(this->handle); ++i) {
                        for (auto node = src::foundation::ChainHashMap_headAt(this->handle, i); node; node = src::foundation::ChainHashMapNode_next(node)) {
                            ((Key*) src::foundation::ChainHashMapNode_key(node))->~Key();
                            ((Value*) src::foundation::ChainHashMapNode_value(node))->~Value();
                        }
                    }

                    src::foundation::ChainHashMap_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            ChainHashMap(const ChainHashMap& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::ChainHashMap_construct(src::foundation::ChainHashMap_capacity(other.handle), sizeof(Key), sizeof(Value), defaultHash<Key>, defaultKeyEquals<Key>);
                if (!this->handle) {
                    return;
                }

                other.forEach([this](const Key& key, const Value& value) {
                    this->insert(key, value);
                });
            }
            ChainHashMap(ChainHashMap&& other) noexcept {
                this->handle = src::foundation::ChainHashMap_move(other.handle);
            }
            ChainHashMap& operator = (const ChainHashMap& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::ChainHashMap_construct(src::foundation::ChainHashMap_capacity(other.handle), sizeof(Key), sizeof(Value), defaultHash<Key>, defaultKeyEquals<Key>);
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
            ChainHashMap& operator = (ChainHashMap&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (usize i = 0; i < src::foundation::ChainHashMap_capacity(this->handle); ++i) {
                            for (auto node = src::foundation::ChainHashMap_headAt(this->handle, i); node; node = src::foundation::ChainHashMapNode_next(node)) {
                                ((Key*) src::foundation::ChainHashMapNode_key(node))->~Key();
                                ((Value*) src::foundation::ChainHashMapNode_value(node))->~Value();
                            }
                        }

                        src::foundation::ChainHashMap_destruct(this->handle);
                    }

                    this->handle = src::foundation::ChainHashMap_move(other.handle);
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

                if (!src::foundation::ChainHashMap_insertSlot(this->handle, &key, &keySlot, &oldValueSlot, &newValueSlot)) {
                    return false;
                }

                // 如果 key 已存在，先析构并释放旧 value
                if (oldValueSlot) {
                    ((Value*) oldValueSlot)->~Value();
                    free(oldValueSlot);
                }

                // 如果是新 key，构造 key
                if (keySlot) {
                    new(keySlot) Key(key);
                }

                // 构造新 value
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

                if (!src::foundation::ChainHashMap_insertSlot(this->handle, &key, &keySlot, &oldValueSlot, &newValueSlot)) {
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

                if (!src::foundation::ChainHashMap_removeSlot(this->handle, &key, &keySlot, &valueSlot)) {
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

                for (usize i = 0; i < src::foundation::ChainHashMap_capacity(this->handle); ++i) {
                    for (auto node = src::foundation::ChainHashMap_headAt(this->handle, i); node; node = src::foundation::ChainHashMapNode_next(node)) {
                        ((Key*) src::foundation::ChainHashMapNode_key(node))->~Key();
                        ((Value*) src::foundation::ChainHashMapNode_value(node))->~Value();
                    }
                }

                return src::foundation::ChainHashMap_clear(this->handle);
            }

            auto find(const Key& key) -> Value* {
                void* value = nullptr;
                if (src::foundation::ChainHashMap_find(this->handle, &key, &value)) {
                    return (Value*) value;
                }

                return nullptr;
            }
            auto find(const Key& key) const -> const Value* {
                void* value = nullptr;
                if (src::foundation::ChainHashMap_find(this->handle, &key, &value)) {
                    return (const Value*) value;
                }

                return nullptr;
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

                for (usize i = 0; i < src::foundation::ChainHashMap_capacity(this->handle); ++i) {
                    for (auto node = src::foundation::ChainHashMap_headAt(this->handle, i); node; node = src::foundation::ChainHashMapNode_next(node)) {
                        function(*((const Key*) src::foundation::ChainHashMapNode_key(node)), *((const Value*) src::foundation::ChainHashMapNode_value(node)));
                    }
                }
            }

            auto reserve(const usize newCapacity) -> bool {
                return src::foundation::ChainHashMap_reserve(this->handle, newCapacity);
            }

            auto begin() -> Iterator {
                if (!this->handle) {
                    return Iterator(nullptr, 0, nullptr);
                }

                const usize capacity = src::foundation::ChainHashMap_capacity(this->handle);
                for (usize i = 0; i < capacity; ++i) {
                    auto* const headNode = src::foundation::ChainHashMap_headAt(this->handle, i);
                    if (headNode) {
                        return Iterator(this->handle, i, headNode);
                    }
                }

                return Iterator(this->handle, capacity, nullptr);
            }
            auto begin() const -> ConstIterator {
                if (!this->handle) {
                    return ConstIterator(nullptr, 0, nullptr);
                }

                const usize capacity = src::foundation::ChainHashMap_capacity(this->handle);
                for (usize i = 0; i < capacity; ++i) {
                    auto* const headNode = src::foundation::ChainHashMap_headAt(this->handle, i);
                    if (headNode) {
                        return ConstIterator(this->handle, i, headNode);
                    }
                }

                return ConstIterator(this->handle, capacity, nullptr);
            }
            auto end() -> Iterator {
                return Iterator(this->handle, src::foundation::ChainHashMap_capacity(this->handle), nullptr);
            }
            auto end() const -> ConstIterator {
                return ConstIterator(this->handle, src::foundation::ChainHashMap_capacity(this->handle), nullptr);
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::ChainHashMap_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::ChainHashMap_size(this->handle) : 0;
            }
            auto keySize() const -> usize {
                return this->handle ? src::foundation::ChainHashMap_keySize(this->handle) : 0;
            }
            auto valueSize() const -> usize {
                return this->handle ? src::foundation::ChainHashMap_valueSize(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::ChainHashMap_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::ChainHashMap* handle = nullptr;
    };
}