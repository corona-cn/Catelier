#pragma once
#include "../../../../src/catelier/foundation/concurrent/ConcurrentChainHashMap.hpp"
#include "../../../../src/catelier/util/HashUtils.hpp"

namespace Catelier::foundation::concurrent {
    template<typename Key>
    auto ConcurrentChainHashMap_defaultHash(const void* key) -> u64 {
        return src::util::hash::FNV1a::hash64Mem(key, sizeof(Key));
    }

    template<typename Key>
    auto ConcurrentChainHashMap_defaultKeyEquals(const void* a, const void* b) -> bool {
        return *((const Key*) a) == *((const Key*) b);
    }

    template<typename Key, typename Value>
    class ConcurrentChainHashMap {
        public:
            class Iterator {
                public:
                    explicit Iterator(
                        src::foundation::concurrent::ConcurrentChainHashMap* handle = nullptr,
                        const usize segmentIndex = 0,
                        const usize bucketIndex = 0,
                        src::foundation::concurrent::ConcurrentChainHashMapNode* node = nullptr
                    ) {
                        this->handle = handle;
                        this->segmentIndex = segmentIndex;
                        this->bucketIndex = bucketIndex;
                        this->node = node;

                        if (this->handle) {
                            this->totalSegments = src::foundation::concurrent::ConcurrentChainHashMap_segmentCount(this->handle);
                            const usize totalCapacity = src::foundation::concurrent::ConcurrentChainHashMap_capacity(this->handle);
                            this->perSegmentBuckets = this->totalSegments > 0 ? totalCapacity / this->totalSegments : 0;
                        } else {
                            this->totalSegments = 0;
                            this->perSegmentBuckets = 0;
                        }
                    }

                    auto key() -> Key& {
                        return *((Key*) src::foundation::concurrent::ConcurrentChainHashMapNode_key(this->node));
                    }
                    auto value() -> Value& {
                        return *((Value*) src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node));
                    }

                    auto operator * () -> Value& {
                        return *((Value*) src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node));
                    }
                    auto operator -> () -> Value* {
                        return (Value*) src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        this->node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(this->node);
                        while (this->node && src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node) == nullptr) {
                            this->node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(this->node);
                        }

                        if (this->node) {
                            return *this;
                        }

                        while (true) {
                            ++this->bucketIndex;

                            if (this->bucketIndex >= this->perSegmentBuckets) {
                                ++this->segmentIndex;
                                this->bucketIndex = 0;
                            }

                            if (this->segmentIndex >= this->totalSegments) {
                                this->node = nullptr;
                                return *this;
                            }

                            this->node = src::foundation::concurrent::ConcurrentChainHashMap_headAt(this->handle, this->segmentIndex, this->bucketIndex);

                            while (this->node && src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node) == nullptr) {
                                this->node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(this->node);
                            }

                            if (this->node) {
                                return *this;
                            }
                        }
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
                    src::foundation::concurrent::ConcurrentChainHashMap* handle;
                    usize segmentIndex;
                    usize bucketIndex;
                    usize totalSegments;
                    usize perSegmentBuckets;
                    src::foundation::concurrent::ConcurrentChainHashMapNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(
                        const src::foundation::concurrent::ConcurrentChainHashMap* handle = nullptr,
                        const usize segmentIndex = 0,
                        const usize bucketIndex = 0,
                        const src::foundation::concurrent::ConcurrentChainHashMapNode* node = nullptr
                    ) {
                        this->handle = handle;
                        this->segmentIndex = segmentIndex;
                        this->bucketIndex = bucketIndex;
                        this->node = node;

                        if (this->handle) {
                            this->totalSegments = src::foundation::concurrent::ConcurrentChainHashMap_segmentCount(this->handle);
                            const usize totalCapacity = src::foundation::concurrent::ConcurrentChainHashMap_capacity(this->handle);
                            this->perSegmentBuckets = this->totalSegments > 0 ? totalCapacity / this->totalSegments : 0;
                        } else {
                            this->totalSegments = 0;
                            this->perSegmentBuckets = 0;
                        }
                    }

                    auto key() const -> const Key& {
                        return *((const Key*) src::foundation::concurrent::ConcurrentChainHashMapNode_key(this->node));
                    }
                    auto value() const -> const Value& {
                        return *((const Value*) src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node));
                    }

                    auto operator * () const -> const Value& {
                        return *((const Value*) src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node));
                    }
                    auto operator -> () const -> const Value* {
                        return (const Value*) src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        if (!this->handle || !this->node) {
                            return *this;
                        }

                        this->node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(this->node);
                        while (this->node && src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node) == nullptr) {
                            this->node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(this->node);
                        }

                        if (this->node) {
                            return *this;
                        }

                        while (true) {
                            ++this->bucketIndex;

                            if (this->bucketIndex >= this->perSegmentBuckets) {
                                ++this->segmentIndex;
                                this->bucketIndex = 0;
                            }

                            if (this->segmentIndex >= this->totalSegments) {
                                this->node = nullptr;
                                return *this;
                            }

                            this->node = src::foundation::concurrent::ConcurrentChainHashMap_headAt(this->handle, this->segmentIndex, this->bucketIndex);

                            while (this->node && src::foundation::concurrent::ConcurrentChainHashMapNode_value(this->node) == nullptr) {
                                this->node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(this->node);
                            }

                            if (this->node) {
                                return *this;
                            }
                        }
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
                    const src::foundation::concurrent::ConcurrentChainHashMap* handle;
                    usize segmentIndex;
                    usize bucketIndex;
                    usize totalSegments;
                    usize perSegmentBuckets;
                    const src::foundation::concurrent::ConcurrentChainHashMapNode* node;
            };

            explicit ConcurrentChainHashMap(
                const usize segmentCount = 16,
                const usize initialCapacity = 4,
                const usize expectedElementCount = 0,
                u64 (*hash)(const void*) = ConcurrentChainHashMap_defaultHash<Key>,
                bool (*keyEquals)(const void*, const void*) = ConcurrentChainHashMap_defaultKeyEquals<Key>
            ) {
                this->handle = src::foundation::concurrent::ConcurrentChainHashMap_construct(segmentCount, initialCapacity, expectedElementCount, sizeof(Key), sizeof(Value), hash, keyEquals);
            }
            ~ConcurrentChainHashMap() {
                if (this->handle) {
                    src::foundation::concurrent::ConcurrentChainHashMap_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            ConcurrentChainHashMap(const ConcurrentChainHashMap& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::concurrent::ConcurrentChainHashMap_copy(other.handle);
            }
            ConcurrentChainHashMap(ConcurrentChainHashMap&& other) noexcept {
                this->handle = src::foundation::concurrent::ConcurrentChainHashMap_move(other.handle);
            }
            ConcurrentChainHashMap& operator = (const ConcurrentChainHashMap& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::concurrent::ConcurrentChainHashMap_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::concurrent::ConcurrentChainHashMap_copy(other.handle);
                    }
                }

                return *this;
            }
            ConcurrentChainHashMap& operator = (ConcurrentChainHashMap&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::concurrent::ConcurrentChainHashMap_destruct(this->handle);
                    }

                    this->handle = src::foundation::concurrent::ConcurrentChainHashMap_move(other.handle);
                }

                return *this;
            }

            auto insert(const Key& key, const Value& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::concurrent::ConcurrentChainHashMap_insert(this->handle, &key, &value);
            }

            auto remove(const Key& key) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::concurrent::ConcurrentChainHashMap_remove(this->handle, &key);
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::concurrent::ConcurrentChainHashMap_clear(this->handle);
            }

            auto find(const Key& key) -> Value* {
                if (!this->handle) {
                    return nullptr;
                }

                void* value = nullptr;
                if (src::foundation::concurrent::ConcurrentChainHashMap_find(this->handle, &key, &value)) {
                    return (Value*) value;
                }

                return nullptr;
            }
            auto find(const Key& key) const -> const Value* {
                if (!this->handle) {
                    return nullptr;
                }

                void* value = nullptr;
                if (src::foundation::concurrent::ConcurrentChainHashMap_find(this->handle, &key, &value)) {
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

                src::foundation::concurrent::ConcurrentChainHashMap_forEach(this->handle, forEachBridge<Function>, &function);
            }

            auto reserve(const usize newCapacity) -> bool {
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_reserve(this->handle, newCapacity) : false;
            }

            auto begin() -> Iterator {
                if (!this->handle) {
                    return Iterator(nullptr);
                }

                const usize totalSegments = src::foundation::concurrent::ConcurrentChainHashMap_segmentCount(this->handle);
                const usize totalCapacity = src::foundation::concurrent::ConcurrentChainHashMap_capacity(this->handle);
                const usize perSegmentBuckets = totalSegments > 0 ? totalCapacity / totalSegments : 0;

                for (usize s = 0; s < totalSegments; ++s) {
                    for (usize b = 0; b < perSegmentBuckets; ++b) {
                        auto* node = src::foundation::concurrent::ConcurrentChainHashMap_headAt(this->handle, s, b);
                        while (node && src::foundation::concurrent::ConcurrentChainHashMapNode_value(node) == nullptr) {
                            node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(node);
                        }

                        if (node) {
                            return Iterator(this->handle, s, b, node);
                        }
                    }
                }

                return Iterator(this->handle, totalSegments, 0, nullptr);
            }
            auto begin() const -> ConstIterator {
                if (!this->handle) {
                    return ConstIterator(nullptr);
                }

                const usize totalSegments = src::foundation::concurrent::ConcurrentChainHashMap_segmentCount(this->handle);
                const usize totalCapacity = src::foundation::concurrent::ConcurrentChainHashMap_capacity(this->handle);
                const usize perSegmentBuckets = totalSegments > 0 ? totalCapacity / totalSegments : 0;

                for (usize s = 0; s < totalSegments; ++s) {
                    for (usize b = 0; b < perSegmentBuckets; ++b) {
                        auto* node = src::foundation::concurrent::ConcurrentChainHashMap_headAt(this->handle, s, b);
                        while (node && src::foundation::concurrent::ConcurrentChainHashMapNode_value(node) == nullptr) {
                            node = src::foundation::concurrent::ConcurrentChainHashMapNode_next(node);
                        }

                        if (node) {
                            return ConstIterator(this->handle, s, b, node);
                        }
                    }
                }

                return ConstIterator(this->handle, totalSegments, 0, nullptr);
            }
            auto end() -> Iterator {
                return Iterator(this->handle, src::foundation::concurrent::ConcurrentChainHashMap_segmentCount(this->handle), 0, nullptr);
            }
            auto end() const -> ConstIterator {
                return ConstIterator(this->handle, src::foundation::concurrent::ConcurrentChainHashMap_segmentCount(this->handle), 0, nullptr);
            }

            auto segmentCount() const -> usize {
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_segmentCount(this->handle) : 0;
            }
            auto capacity() const -> usize {
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_capacity(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_size(this->handle) : 0;
            }
            auto keySize() const -> usize {
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_keySize(this->handle) : 0;
            }
            auto valueSize() const -> usize {
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_valueSize(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_isEmpty(this->handle) : true;
            }
            auto contains(const Key& key) const -> bool {
                void* value = nullptr;
                return this->handle ? src::foundation::concurrent::ConcurrentChainHashMap_find(this->handle, &key, &value) : false;
            }

        private:
            src::foundation::concurrent::ConcurrentChainHashMap* handle = nullptr;

            template<typename Function>
            static auto forEachBridge(const void* key, void* value, void* userData) -> void {
                auto* const function = (Function*) userData;
                (*function)(*((const Key*) key), *((Value*) value));
            }
    };
}