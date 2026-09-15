#pragma once
#include <cstdlib>
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/ChainHashMap.hpp"
#include "../../../src/catelier/util/HashUtils.hpp"

namespace Catelier::foundation {
    template<typename Key>
    u64 defaultHash(const void* key) {
        return src::util::hash::FNV1a::hash64Mem(key, sizeof(Key));
    }

    template<typename Key>
    bool defaultKeyEquals(const void* a, const void* b) {
        return *((const Key*) a) == *((const Key*) b);
    }

    template<typename Key, typename Value>
    class ChainHashMap {
        public:
            explicit ChainHashMap(const usize initialCapacity = 4, u64 (*hash)(const void*) = defaultHash<Key>, bool (*keyEquals)(const void*, const void*) = defaultKeyEquals<Key>) {
                this->handle = src::foundation::ChainHashMap_construct(initialCapacity, hash, keyEquals);
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

                this->handle = src::foundation::ChainHashMap_construct(src::foundation::ChainHashMap_capacity(other.handle), defaultHash<Key>, defaultKeyEquals<Key>);
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
                        this->handle = src::foundation::ChainHashMap_construct(src::foundation::ChainHashMap_capacity(other.handle), defaultHash<Key>, defaultKeyEquals<Key>);
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

                if (!src::foundation::ChainHashMap_insertSlot(this->handle, &key, sizeof(Key), sizeof(Value), &keySlot, &oldValueSlot, &newValueSlot)) {
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

                if (!src::foundation::ChainHashMap_insertSlot(this->handle, &key, sizeof(Key), sizeof(Value), &keySlot, &oldValueSlot, &newValueSlot)) {
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