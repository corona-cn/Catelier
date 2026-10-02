#pragma once
#include "../../../../src/catelier/foundation/concurrent/ConcurrentUnboundedQueue.hpp"

#include <cstdlib>

namespace Catelier::foundation::concurrent {
    template<typename Type>
    class ConcurrentUnboundedQueue {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::concurrent::ConcurrentUnboundedQueueNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        this->node = src::foundation::concurrent::ConcurrentUnboundedQueueNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        this->node = src::foundation::concurrent::ConcurrentUnboundedQueueNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::concurrent::ConcurrentUnboundedQueueNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::concurrent::ConcurrentUnboundedQueueNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        this->node = src::foundation::concurrent::ConcurrentUnboundedQueueNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        this->node = src::foundation::concurrent::ConcurrentUnboundedQueueNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::concurrent::ConcurrentUnboundedQueueNode* node;
            };

            explicit ConcurrentUnboundedQueue() {
                this->handle = src::foundation::concurrent::ConcurrentUnboundedQueue_construct(sizeof(Type));
            }
            ~ConcurrentUnboundedQueue() {
                if (this->handle) {
                    src::foundation::concurrent::ConcurrentUnboundedQueue_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            ConcurrentUnboundedQueue(const ConcurrentUnboundedQueue& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::concurrent::ConcurrentUnboundedQueue_copy(other.handle);
            }
            ConcurrentUnboundedQueue(ConcurrentUnboundedQueue&& other) noexcept {
                this->handle = src::foundation::concurrent::ConcurrentUnboundedQueue_move(other.handle);
            }
            ConcurrentUnboundedQueue& operator = (const ConcurrentUnboundedQueue& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::concurrent::ConcurrentUnboundedQueue_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::concurrent::ConcurrentUnboundedQueue_copy(other.handle);
                    }
                }

                return *this;
            }
            ConcurrentUnboundedQueue& operator = (ConcurrentUnboundedQueue&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::concurrent::ConcurrentUnboundedQueue_destruct(this->handle);
                    }

                    this->handle = src::foundation::concurrent::ConcurrentUnboundedQueue_move(other.handle);
                }

                return *this;
            }

            auto enqueue(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::concurrent::ConcurrentUnboundedQueue_enqueue(this->handle, &value);
            }
            auto enqueue(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::concurrent::ConcurrentUnboundedQueue_enqueue(this->handle, &value);
            }

            auto dequeue() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::concurrent::ConcurrentUnboundedQueue_dequeueSlot(this->handle);
                if (!slot) {
                    return false;
                }

                ((Type*) slot)->~Type();

                free(slot);

                return true;
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                bool removed = false;
                while (true) {
                    void* const slot = src::foundation::concurrent::ConcurrentUnboundedQueue_dequeueSlot(this->handle);
                    if (!slot) {
                        break;
                    }

                    ((Type*) slot)->~Type();

                    free(slot);

                    removed = true;
                }

                return removed;
            }

            auto head() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* const node = src::foundation::concurrent::ConcurrentUnboundedQueue_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
            }
            auto head() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* const node = src::foundation::concurrent::ConcurrentUnboundedQueue_head(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
            }
            auto tail() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* const node = src::foundation::concurrent::ConcurrentUnboundedQueue_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
            }
            auto tail() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* const node = src::foundation::concurrent::ConcurrentUnboundedQueue_tail(this->handle);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
            }
            auto get(const usize index) -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* const node = src::foundation::concurrent::ConcurrentUnboundedQueue_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
            }
            auto get(const usize index) const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                const auto* const node = src::foundation::concurrent::ConcurrentUnboundedQueue_get(this->handle, index);
                if (!node) {
                    return nullptr;
                }

                return (const Type*) src::foundation::concurrent::ConcurrentUnboundedQueueNode_data(node);
            }

            auto begin() -> Iterator {
                if (!this->handle) {
                    return Iterator(nullptr);
                }

                return Iterator(src::foundation::concurrent::ConcurrentUnboundedQueue_begin(this->handle));
            }
            auto begin() const -> ConstIterator {
                if (!this->handle) {
                    return ConstIterator(nullptr);
                }

                return ConstIterator(src::foundation::concurrent::ConcurrentUnboundedQueue_begin(this->handle));
            }
            auto end() -> Iterator {
                return Iterator(src::foundation::concurrent::ConcurrentUnboundedQueue_end());
            }
            auto end() const -> ConstIterator {
                return ConstIterator(src::foundation::concurrent::ConcurrentUnboundedQueue_end());
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::concurrent::ConcurrentUnboundedQueue_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::concurrent::ConcurrentUnboundedQueue_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::concurrent::ConcurrentUnboundedQueue* handle = nullptr;
    };
}