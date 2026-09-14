#pragma once
#include "../../../src/catelier/foundation/OpenHashMap.hpp"
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
    class OpenHashMap {
        public:
            explicit OpenHashMap(const usize initialCapacity = 4, u64 (*hash)(const void*) = defaultHash<Key>, bool (*keyEquals)(const void*, const void*) = defaultKeyEquals<Key>) {
                this->handle = src::foundation::OpenHashMap_construct(initialCapacity, hash, keyEquals);
            }
            ~OpenHashMap() {
                if (this->handle) {
                    src::foundation::OpenHashMap_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            OpenHashMap(const OpenHashMap& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::OpenHashMap_copy(other.handle);
            }
            OpenHashMap(OpenHashMap&& other) noexcept {
                this->handle = src::foundation::OpenHashMap_move(other.handle);
            }
            OpenHashMap& operator = (const OpenHashMap& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::OpenHashMap_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::OpenHashMap_copy(other.handle);
                    }
                }

                return *this;
            }
            OpenHashMap& operator = (OpenHashMap&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::OpenHashMap_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    this->handle = src::foundation::OpenHashMap_move(other.handle);
                }

                return *this;
            }

            auto insert(const Key& key, const Value& value) -> bool {
                return src::foundation::OpenHashMap_insert(this->handle, &key, sizeof(Key), &value, sizeof(Value));
            }

            auto find(const Key& key) -> Value* {
                void* value = nullptr;
                if (src::foundation::OpenHashMap_find(this->handle, &key, &value)) {
                    return (Value*) value;
                }

                return nullptr;
            }
            auto find(const Key& key) const -> const Value* {
                void* value = nullptr;
                if (src::foundation::OpenHashMap_find(this->handle, &key, &value)) {
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

            auto vacate(const Key& key) -> bool {
                return src::foundation::OpenHashMap_vacate(this->handle, &key);
            }
            auto vacateAll() -> bool {
                return src::foundation::OpenHashMap_vacateAll(this->handle);
            }
            auto erase(const Key& key) -> bool {
                return src::foundation::OpenHashMap_erase(this->handle, &key);
            }
            auto eraseAll() -> bool {
                return src::foundation::OpenHashMap_eraseAll(this->handle);
            }

            auto reserve(const usize newCapacity) -> bool {
                return src::foundation::OpenHashMap_reserve(this->handle, newCapacity);
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::OpenHashMap_size(this->handle) : 0;
            }
            auto keySize() const -> usize {
                return this->handle ? src::foundation::OpenHashMap_keySize(this->handle) : 0;
            }
            auto valueSize() const -> usize {
                return this->handle ? src::foundation::OpenHashMap_valueSize(this->handle) : 0;
            }
            auto capacity() const -> usize {
                return this->handle ? src::foundation::OpenHashMap_capacity(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::OpenHashMap_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::OpenHashMap* handle = nullptr;
    };
}