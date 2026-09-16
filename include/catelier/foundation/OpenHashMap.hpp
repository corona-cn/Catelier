#pragma once
#include <new>
#include <utility>

#include "../../../src/catelier/foundation/OpenHashMap.hpp"
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
    class OpenHashMap {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::OpenHashMap* handle = nullptr, const usize fromIndex = 0) {
                        this->handle = handle;
                        this->keyPtr = nullptr;
                        this->valuePtr = nullptr;
                        this->index = fromIndex;

                        if (!this->handle) {
                            return;
                        }

                        if (fromIndex >= src::foundation::OpenHashMap_capacity(this->handle)) {
                            return;
                        }

                        usize nextIndex = 0;
                        if (!src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, fromIndex, &this->keyPtr, &this->valuePtr, &nextIndex)) {
                            this->index = src::foundation::OpenHashMap_capacity(this->handle);
                            return;
                        }

                        this->index = nextIndex;
                    }

                    auto key() -> Key& {
                        return *((Key*) this->keyPtr);
                    }
                    auto value() -> Value& {
                        return *((Value*) this->valuePtr);
                    }

                    auto operator * () -> Value& {
                        return *((Value*) this->valuePtr);
                    }
                    auto operator -> () -> Value* {
                        return (Value*) this->valuePtr;
                    }

                    auto operator ++ () -> Iterator& {
                        if (!this->handle) {
                            return *this;
                        }

                        const usize capacity = src::foundation::OpenHashMap_capacity(this->handle);
                        if (this->index + 1 >= capacity) {
                            this->index = capacity;
                            this->keyPtr = nullptr;
                            this->valuePtr = nullptr;

                            return *this;
                        }

                        usize nextIndex = 0;
                        if (!src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, this->index + 1, &this->keyPtr, &this->valuePtr, &nextIndex)) {
                            this->index = capacity;
                            this->keyPtr = nullptr;
                            this->valuePtr = nullptr;

                            return *this;
                        }

                        this->index = nextIndex;

                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        ++(*this);

                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->handle == other.handle && this->index == other.index;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->handle != other.handle || this->index != other.index;
                    }

                private:
                    src::foundation::OpenHashMap* handle;
                    usize index;
                    void* keyPtr;
                    void* valuePtr;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::OpenHashMap* handle = nullptr, const usize fromIndex = 0) {
                        this->handle = handle;
                        this->keyPtr = nullptr;
                        this->valuePtr = nullptr;
                        this->index = fromIndex;

                        if (!this->handle) {
                            return;
                        }

                        if (fromIndex >= src::foundation::OpenHashMap_capacity(this->handle)) {
                            return;
                        }

                        usize nextIndex = 0;
                        if (!src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, fromIndex, &this->keyPtr, &this->valuePtr, &nextIndex)) {
                            this->index = src::foundation::OpenHashMap_capacity(this->handle);
                            return;
                        }

                        this->index = nextIndex;
                    }

                    auto key() const -> const Key& {
                        return *((const Key*) this->keyPtr);
                    }
                    auto value() const -> const Value& {
                        return *((const Value*) this->valuePtr);
                    }

                    auto operator * () const -> const Value& {
                        return *((const Value*) this->valuePtr);
                    }
                    auto operator -> () const -> const Value* {
                        return (const Value*) this->valuePtr;
                    }

                    auto operator ++ () -> ConstIterator& {
                        if (!this->handle) {
                            return *this;
                        }

                        const usize capacity = src::foundation::OpenHashMap_capacity(this->handle);
                        if (this->index + 1 >= capacity) {
                            this->index = capacity;
                            this->keyPtr = nullptr;
                            this->valuePtr = nullptr;

                            return *this;
                        }

                        usize nextIndex = 0;
                        if (!src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, this->index + 1, &this->keyPtr, &this->valuePtr, &nextIndex)) {
                            this->index = capacity;
                            this->keyPtr = nullptr;
                            this->valuePtr = nullptr;

                            return *this;
                        }

                        this->index = nextIndex;

                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        ++(*this);

                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->handle == other.handle && this->index == other.index;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->handle != other.handle || this->index != other.index;
                    }

                private:
                    const src::foundation::OpenHashMap* handle;
                    usize index;
                    void* keyPtr;
                    void* valuePtr;
            };

            explicit OpenHashMap(const usize initialCapacity = 4, u64 (*hash)(const void*) = defaultHash<Key>, bool (*keyEquals)(const void*, const void*) = defaultKeyEquals<Key>) {
                this->handle = src::foundation::OpenHashMap_construct(initialCapacity, hash, keyEquals);
            }
            ~OpenHashMap() {
                if (this->handle) {
                    usize index = 0;
                    void* key = nullptr;
                    void* value = nullptr;

                    while (src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, index, &key, &value, &index)) {
                        ((Key*) key)->~Key();
                        ((Value*) value)->~Value();

                        index++;
                    }

                    src::foundation::OpenHashMap_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            OpenHashMap(const OpenHashMap& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::OpenHashMap_construct(src::foundation::OpenHashMap_capacity(other.handle), defaultHash<Key>, defaultKeyEquals<Key>);
                if (!this->handle) {
                    return;
                }

                other.forEach([this](const Key& key, const Value& value) {
                    this->insert(key, value);
                });
            }
            OpenHashMap(OpenHashMap&& other) noexcept {
                this->handle = src::foundation::OpenHashMap_move(other.handle);
            }
            OpenHashMap& operator = (const OpenHashMap& other) {
                if (this != &other) {
                    this->eraseAll();

                    if (!this->handle) {
                        this->handle = src::foundation::OpenHashMap_construct(src::foundation::OpenHashMap_capacity(other.handle), defaultHash<Key>, defaultKeyEquals<Key>);
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
            OpenHashMap& operator = (OpenHashMap&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        usize index = 0;
                        void* key = nullptr;
                        void* value = nullptr;

                        while (src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, index, &key, &value, &index)) {
                            ((Key*) key)->~Key();
                            ((Value*) value)->~Value();

                            index++;
                        }

                        src::foundation::OpenHashMap_destruct(this->handle);
                    }

                    this->handle = src::foundation::OpenHashMap_move(other.handle);
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

                if (!src::foundation::OpenHashMap_insertSlot(this->handle, &key, sizeof(Key), sizeof(Value), &keySlot, &oldValueSlot, &newValueSlot)) {
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

                if (!src::foundation::OpenHashMap_insertSlot(this->handle, &key, sizeof(Key), sizeof(Value), &keySlot, &oldValueSlot, &newValueSlot)) {
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
                if (!this->handle) {
                    return false;
                }

                void* keySlot = nullptr;
                void* valueSlot = nullptr;

                if (!src::foundation::OpenHashMap_vacateSlot(this->handle, &key, &keySlot, &valueSlot)) {
                    return false;
                }

                ((Key*) keySlot)->~Key();
                free(keySlot);

                ((Value*) valueSlot)->~Value();
                free(valueSlot);

                return true;
            }
            auto vacateAll() -> bool {
                if (!this->handle) {
                    return false;
                }

                usize index = 0;
                void* key = nullptr;
                void* value = nullptr;

                while (src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, index, &key, &value, &index)) {
                    ((Key*) key)->~Key();
                    ((Value*) value)->~Value();

                    index++;
                }

                return src::foundation::OpenHashMap_vacateAll(this->handle);
            }
            auto erase(const Key& key) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* keySlot = nullptr;
                void* valueSlot = nullptr;

                if (!src::foundation::OpenHashMap_eraseSlot(this->handle, &key, &keySlot, &valueSlot)) {
                    return false;
                }

                ((Key*) keySlot)->~Key();
                free(keySlot);

                ((Value*) valueSlot)->~Value();
                free(valueSlot);

                return true;
            }
            auto eraseAll() -> bool {
                if (!this->handle) {
                    return false;
                }

                usize index = 0;
                void* key = nullptr;
                void* value = nullptr;

                while (src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, index, &key, &value, &index)) {
                    ((Key*) key)->~Key();
                    ((Value*) value)->~Value();

                    index++;
                }

                return src::foundation::OpenHashMap_eraseAll(this->handle);
            }

            template<typename Function>
            auto forEach(Function function) const -> void {
                if (!this->handle) {
                    return;
                }

                usize index = 0;
                void* key = nullptr;
                void* value = nullptr;

                while (src::foundation::OpenHashMap_nextOccupiedSlot(this->handle, index, &key, &value, &index)) {
                    function(*((const Key*) key), *((const Value*) value));
                    index++;
                }
            }

            auto reserve(const usize newCapacity) -> bool {
                return src::foundation::OpenHashMap_reserve(this->handle, newCapacity);
            }

            auto begin() -> Iterator {
                return Iterator(this->handle, 0);
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(this->handle, 0);
            }
            auto end() -> Iterator {
                return Iterator(this->handle, src::foundation::OpenHashMap_capacity(this->handle));
            }
            auto end() const -> ConstIterator {
                return ConstIterator(this->handle, src::foundation::OpenHashMap_capacity(this->handle));
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::OpenHashMap_capacity(this->handle) : 0;
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

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::OpenHashMap_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::OpenHashMap* handle = nullptr;
    };
}