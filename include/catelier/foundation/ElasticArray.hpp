#pragma once
#include <algorithm>

#include "../../../src/catelier/foundation/ElasticArray.hpp"

namespace Catelier::foundation {
    template<typename Type>
    class ElasticArray {
        public:
            explicit ElasticArray(const usize initialCapacity = 4) {
                this->handle = src::foundation::ElasticArray_construct(initialCapacity);
            }
            ~ElasticArray() {
                if (this->handle) {
                    src::foundation::ElasticArray_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            ElasticArray(const ElasticArray& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::ElasticArray_copy(other.handle, sizeof(Type));
            }
            ElasticArray(ElasticArray&& other) noexcept {
                this->handle = src::foundation::ElasticArray_move(other.handle);
            }
            ElasticArray& operator = (const ElasticArray& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::ElasticArray_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::ElasticArray_copy(other.handle, sizeof(Type));
                    }
                }

                return *this;
            }
            ElasticArray& operator = (ElasticArray&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::ElasticArray_destruct(this->handle);
                    }

                    this->handle = src::foundation::ElasticArray_move(other.handle);
                }

                return *this;
            }

            auto push(const Type& value) -> bool {
                return src::foundation::ElasticArray_push(this->handle, &value, sizeof(Type));
            }
            auto push(Type&& value) -> bool {
                Type* movedValue = new Type(std::move(value));
                if (!src::foundation::ElasticArray_pushMove(this->handle, movedValue)) {
                    delete movedValue;
                    return false;
                }

                return true;
            }
            auto insertAt(const usize index, const Type& value) -> bool {
                return src::foundation::ElasticArray_insertAt(this->handle, index, &value, sizeof(Type));
            }

            auto pop() -> bool {
                return src::foundation::ElasticArray_pop(this->handle);
            }
            auto removeAt(const usize index) -> bool {
                return src::foundation::ElasticArray_removeAt(this->handle, index);
            }
            auto clear() -> bool {
                return src::foundation::ElasticArray_clear(this->handle);
            }

            auto reserve(const usize newCapacity) -> bool {
                return src::foundation::ElasticArray_reserve(this->handle, newCapacity);
            }
            auto shrinkToFit() -> bool {
                return src::foundation::ElasticArray_shrinkToFit(this->handle);
            }

            auto get(const usize index) -> Type& {
                return *((Type*) src::foundation::ElasticArray_get(this->handle, index));
            }
            auto get(const usize index) const -> const Type& {
                return *((const Type*) src::foundation::ElasticArray_get(this->handle, index));
            }
            auto operator[](const usize index) -> Type& {
                return get(index);
            }
            auto operator[](const usize index) const -> const Type& {
                return get(index);
            }

            auto set(const usize index, const Type& value) -> bool {
                return src::foundation::ElasticArray_set(this->handle, index, &value);
            }

            auto begin() -> Type* {
                return (Type*) src::foundation::ElasticArray_get(this->handle, 0);
            }
            auto begin() const -> const Type* {
                return (const Type*) src::foundation::ElasticArray_get(this->handle, 0);
            }
            auto end() -> Type* {
                return (Type*) src::foundation::ElasticArray_get(this->handle, size());
            }
            auto end() const -> const Type* {
                return (const Type*) src::foundation::ElasticArray_get(this->handle, size());
            }

            auto size() const -> usize {
                return this->handle ? src::foundation::ElasticArray_size(this->handle) : 0;
            }
            auto capacity() const -> usize {
                return this->handle ? src::foundation::ElasticArray_capacity(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::ElasticArray_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::ElasticArray* handle;
    };
}
