#pragma once
#include <algorithm>

#include "../../../src/catelier/foundation/ArrayList.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class ArrayList {
        public:
            class Iterator {
                public:
                    explicit Iterator(src::foundation::ArrayList* handle, const usize index) {
                        this->handle = handle;
                        this->index = index;
                    }

                    auto operator * () -> Type& {
                        return *((Type*) src::foundation::ArrayList_get(this->handle, this->index));
                    }
                    auto operator ++ () -> Iterator& {
                        ++this->index;
                        return *this;
                    }
                    auto operator != (const Iterator& other) const -> bool {
                        return this->index != other.index;
                    }

                private:
                    src::foundation::ArrayList* handle;
                    usize index;
            };

            class ConstIterator {
                public:
                    explicit ConstIterator(const src::foundation::ArrayList* handle, const usize index) {
                        this->handle = handle;
                        this->index = index;
                    }

                    auto operator * () const -> const Type& {
                        return *((const Type*) src::foundation::ArrayList_get(this->handle, this->index));
                    }
                    auto operator ++ () -> ConstIterator& {
                        ++this->index;
                        return *this;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->index != other.index;
                    }

                private:
                    const src::foundation::ArrayList* handle;
                    usize index;
            };

            explicit ArrayList(const usize initialCapacity = 4) {
                this->handle = src::foundation::ArrayList_construct(initialCapacity);
            }
            ~ArrayList() {
                if (this->handle) {
                    src::foundation::ArrayList_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            ArrayList(const ArrayList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::ArrayList_copy(other.handle, sizeof(Type));
            }
            ArrayList(ArrayList&& other) noexcept {
                this->handle = src::foundation::ArrayList_move(other.handle);
            }
            ArrayList& operator = (const ArrayList& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::ArrayList_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::ArrayList_copy(other.handle, sizeof(Type));
                    }
                }

                return *this;
            }
            ArrayList& operator = (ArrayList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::ArrayList_destruct(this->handle);
                    }

                    this->handle = src::foundation::ArrayList_move(other.handle);
                }

                return *this;
            }

            auto push(const Type& value) -> bool {
                return src::foundation::ArrayList_push(this->handle, &value, sizeof(Type));
            }
            auto push(Type&& value) -> bool {
                Type* movedValue = new Type(std::move(value));
                if (!src::foundation::ArrayList_pushMove(this->handle, movedValue)) {
                    delete movedValue;
                    return false;
                }

                return true;
            }
            auto insertAt(const usize index, const Type& value) -> bool {
                return src::foundation::ArrayList_insertAt(this->handle, index, &value, sizeof(Type));
            }

            auto pop() -> bool {
                return src::foundation::ArrayList_pop(this->handle);
            }
            auto removeAt(const usize index) -> bool {
                return src::foundation::ArrayList_removeAt(this->handle, index);
            }
            auto clear() -> bool {
                return src::foundation::ArrayList_clear(this->handle);
            }

            auto reserve(const usize newCapacity) -> bool {
                return src::foundation::ArrayList_reserve(this->handle, newCapacity);
            }
            auto shrinkToFit() -> bool {
                return src::foundation::ArrayList_shrinkToFit(this->handle);
            }

            auto get(const usize index) -> Type& {
                return *((Type*) src::foundation::ArrayList_get(this->handle, index));
            }
            auto get(const usize index) const -> const Type& {
                return *((const Type*) src::foundation::ArrayList_get(this->handle, index));
            }
            auto operator[](const usize index) -> Type& {
                return get(index);
            }
            auto operator[](const usize index) const -> const Type& {
                return get(index);
            }

            auto set(const usize index, const Type& value) -> bool {
                return src::foundation::ArrayList_set(this->handle, index, &value);
            }

            auto begin() -> Iterator {
                return Iterator(this->handle, 0);
            }
            auto begin() const -> ConstIterator {
                return ConstIterator(this->handle, 0);
            }
            auto end() -> Iterator {
                return Iterator(this->handle, this->size());
            }
            auto end() const -> ConstIterator {
                return ConstIterator(this->handle, this->size());
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::ArrayList_size(this->handle) : 0;
            }
            auto capacity() const -> usize {
                return this->handle ? src::foundation::ArrayList_capacity(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::ArrayList_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::ArrayList* handle;
    };
}
