#pragma once
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
                    src::foundation::ChainHashMap_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            ChainHashMap(const ChainHashMap& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::ChainHashMap_copy(other.handle);
            }
            ChainHashMap(ChainHashMap&& other) noexcept {
                this->handle = src::foundation::ChainHashMap_move(other.handle);
            }
            ChainHashMap& operator = (const ChainHashMap& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::ChainHashMap_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::ChainHashMap_copy(other.handle);
                    }
                }

                return *this;
            }
            ChainHashMap& operator = (ChainHashMap&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::ChainHashMap_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    this->handle = src::foundation::ChainHashMap_move(other.handle);
                }

                return *this;
            }

            auto insert(const Key& key, const Value& value) -> bool {
                return src::foundation::ChainHashMap_insert(this->handle, &key, sizeof(Key), &value, sizeof(Value));
            }

            auto remove(const Key& key) -> bool {
                return src::foundation::ChainHashMap_remove(this->handle, &key);
            }
            auto clear() -> bool {
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