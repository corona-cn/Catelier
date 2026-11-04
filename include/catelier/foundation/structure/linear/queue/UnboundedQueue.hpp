#pragma once
#include <new>
#include <utility>

#include "../../../../../../src/catelier/foundation/structure/linear/queue/UnboundedQueue.hpp"

namespace Catelier::include::foundation::structure::linear::queue {
    template<typename Type>
    class UnboundedQueue {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::structure::linear::queue::UnboundedQueueNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(this->node));
                    }
                    auto operator -> () -> Type* {
                        return (Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(this->node);
                    }

                    auto operator ++ () -> Iterator& {
                        this->node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> Iterator {
                        Iterator temp = *this;
                        this->node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const Iterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    src::foundation::structure::linear::queue::UnboundedQueueNode* node;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::structure::linear::queue::UnboundedQueueNode* node = nullptr) {
                        this->node = node;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(this->node));
                    }
                    auto operator -> () const -> const Type* {
                        return (const Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(this->node);
                    }

                    auto operator ++ () -> ConstIterator& {
                        this->node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(this->node);
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        this->node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(this->node);
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->node == other.node;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->node != other.node;
                    }

                private:
                    const src::foundation::structure::linear::queue::UnboundedQueueNode* node;
            };

            explicit UnboundedQueue() {
                this->handle = src::foundation::structure::linear::queue::UnboundedQueue_construct(sizeof(Type));
            }
            ~UnboundedQueue() {
                if (this->handle) {
                    for (auto node = src::foundation::structure::linear::queue::UnboundedQueue_begin(this->handle); node; node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(node)) {
                        ((Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(node))->~Type();
                    }

                    src::foundation::structure::linear::queue::UnboundedQueue_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            UnboundedQueue(const UnboundedQueue& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::structure::linear::queue::UnboundedQueue_construct(sizeof(Type));
                if (!this->handle) {
                    return;
                }

                for (auto node = src::foundation::structure::linear::queue::UnboundedQueue_begin(other.handle); node; node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(node)) {
                    void* const slot = src::foundation::structure::linear::queue::UnboundedQueue_enqueueSlot(this->handle);
                    if (!slot) {
                        this->clear();

                        src::foundation::structure::linear::queue::UnboundedQueue_destruct(this->handle);

                        this->handle = nullptr;

                        return;
                    }

                    new(slot) Type(*((const Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(node)));
                }
            }
            UnboundedQueue(UnboundedQueue&& other) noexcept {
                this->handle = src::foundation::structure::linear::queue::UnboundedQueue_move(other.handle);
            }
            UnboundedQueue& operator = (const UnboundedQueue& other) {
                if (this != &other) {
                    this->clear();

                    if (!this->handle) {
                        this->handle = src::foundation::structure::linear::queue::UnboundedQueue_construct(sizeof(Type));
                        if (!this->handle) {
                            return *this;
                        }
                    }

                    for (auto node = src::foundation::structure::linear::queue::UnboundedQueue_begin(other.handle); node; node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(node)) {
                        this->enqueue(*((const Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(node)));
                    }
                }

                return *this;
            }
            UnboundedQueue& operator = (UnboundedQueue&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        for (auto node = src::foundation::structure::linear::queue::UnboundedQueue_begin(this->handle); node; node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(node)) {
                            ((Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(node))->~Type();
                        }

                        src::foundation::structure::linear::queue::UnboundedQueue_destruct(this->handle);
                    }

                    this->handle = src::foundation::structure::linear::queue::UnboundedQueue_move(other.handle);
                }

                return *this;
            }

            auto enqueue(const Type& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::structure::linear::queue::UnboundedQueue_enqueueSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(value);

                return true;
            }
            auto enqueue(Type&& value) -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::structure::linear::queue::UnboundedQueue_enqueueSlot(this->handle);
                if (!slot) {
                    return false;
                }

                new(slot) Type(std::move(value));

                return true;
            }

            auto dequeue() -> bool {
                if (!this->handle) {
                    return false;
                }

                void* const slot = src::foundation::structure::linear::queue::UnboundedQueue_dequeueSlot(this->handle);
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

                for (auto node = src::foundation::structure::linear::queue::UnboundedQueue_begin(this->handle); node; node = src::foundation::structure::linear::queue::UnboundedQueueNode_next(node)) {
                    ((Type*) src::foundation::structure::linear::queue::UnboundedQueueNode_data(node))->~Type();
                }

                return src::foundation::structure::linear::queue::UnboundedQueue_clear(this->handle);
            }

            auto head() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::structure::linear::queue::UnboundedQueue_head(this->handle);
            }
            auto head() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::structure::linear::queue::UnboundedQueue_head(this->handle);
            }
            auto tail() -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::structure::linear::queue::UnboundedQueue_tail(this->handle);
            }
            auto tail() const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::structure::linear::queue::UnboundedQueue_tail(this->handle);
            }
            auto get(const usize index) -> Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Type*) src::foundation::structure::linear::queue::UnboundedQueue_get(this->handle, index);
            }
            auto get(const usize index) const -> const Type* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Type*) src::foundation::structure::linear::queue::UnboundedQueue_get(this->handle, index);
            }

            auto begin() -> Iterator {
                return Iterator(src::foundation::structure::linear::queue::UnboundedQueue_begin(this->handle));
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(src::foundation::structure::linear::queue::UnboundedQueue_begin(this->handle));
            }
            auto end() -> Iterator {
                return Iterator(src::foundation::structure::linear::queue::UnboundedQueue_end());
            }
            auto end() const -> ConstIterator {
                return ConstIterator(src::foundation::structure::linear::queue::UnboundedQueue_end());
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::structure::linear::queue::UnboundedQueue_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::structure::linear::queue::UnboundedQueue_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::structure::linear::queue::UnboundedQueue* handle = nullptr;
    };
}