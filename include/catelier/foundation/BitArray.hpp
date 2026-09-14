#pragma once
#include "../../../src/catelier/foundation/BitArray.hpp"

namespace Catelier::foundation {
    class BitArray {
        public:
            explicit BitArray(const usize initialCapacity = 8) {
                this->handle = src::foundation::BitArray_construct(initialCapacity);
            }
            ~BitArray() {
                if (this->handle) {
                    src::foundation::BitArray_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            BitArray(const BitArray& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::BitArray_copy(other.handle);
            }
            BitArray(BitArray&& other) noexcept {
                this->handle = src::foundation::BitArray_move(other.handle);
            }
            BitArray& operator = (const BitArray& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::BitArray_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::BitArray_copy(other.handle);
                    }
                }

                return *this;
            }
            BitArray& operator = (BitArray&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::BitArray_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    this->handle = src::foundation::BitArray_move(other.handle);
                }

                return *this;
            }

            auto setOne(const usize index) -> bool {
                return src::foundation::BitArray_setOne(this->handle, index);
            }
            auto fillOne() -> bool {
                return src::foundation::BitArray_fillOne(this->handle);
            }
            auto setZero(const usize index) -> bool {
                return src::foundation::BitArray_setZero(this->handle, index);
            }
            auto fillZero() -> bool {
                return src::foundation::BitArray_fillZero(this->handle);
            }
            auto toggle(const usize index) -> bool {
                return src::foundation::BitArray_toggle(this->handle, index);
            }
            auto toggleAll() -> bool {
                return src::foundation::BitArray_toggleAll(this->handle);
            }

            auto get(const usize index, bool* outValue) const -> bool {
                return src::foundation::BitArray_get(this->handle, index, outValue);
            }

            auto findFirstOne(usize* outIndex) const -> bool {
                return src::foundation::BitArray_findFirstOne(this->handle, outIndex);
            }
            auto findFirstZero(usize* outIndex) const -> bool {
                return src::foundation::BitArray_findFirstZero(this->handle, outIndex);
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::BitArray_capacity(this->handle) : 0;
            }
            auto byteCount() const -> usize {
                return this->handle ? src::foundation::BitArray_byteCount(this->handle) : 0;
            }

            auto countOnes() const -> usize {
                return this->handle ? src::foundation::BitArray_countOnes(this->handle) : 0;
            }
            auto countZeros() const -> usize {
                return this->handle ? src::foundation::BitArray_countZeros(this->handle) : 0;
            }

        private:
            src::foundation::BitArray* handle = nullptr;
    };
}