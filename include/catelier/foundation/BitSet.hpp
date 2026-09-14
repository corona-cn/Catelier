#pragma once
#include "../../../src/catelier/foundation/BitSet.hpp"

namespace Catelier::foundation {
    class BitSet {
        public:
            explicit BitSet(const usize initialCapacity = 8) {
                this->handle = src::foundation::BitSet_construct(initialCapacity);
            }
            ~BitSet() {
                if (this->handle) {
                    src::foundation::BitSet_destruct(this->handle);
                    this->handle = nullptr;
                }
            }

            BitSet(const BitSet& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::BitSet_copy(other.handle);
            }
            BitSet(BitSet&& other) noexcept {
                this->handle = src::foundation::BitSet_move(other.handle);
            }
            BitSet& operator = (const BitSet& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::BitSet_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::BitSet_copy(other.handle);
                    }
                }

                return *this;
            }
            BitSet& operator = (BitSet&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::BitSet_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    this->handle = src::foundation::BitSet_move(other.handle);
                }

                return *this;
            }

            auto insert(const usize value) -> bool {
                return src::foundation::BitSet_insert(this->handle, value);
            }
            auto insertAll() -> bool {
                return src::foundation::BitSet_insertAll(this->handle);
            }

            auto toggle(const usize value) -> bool {
                return src::foundation::BitSet_toggle(this->handle, value);
            }
            auto toggleAll() -> bool {
                return src::foundation::BitSet_toggleAll(this->handle);
            }

            auto remove(const usize value) -> bool {
                return src::foundation::BitSet_remove(this->handle, value);
            }
            auto removeAll() -> bool {
                return src::foundation::BitSet_removeAll(this->handle);
            }
            auto clear() -> bool {
                return src::foundation::BitSet_clear(this->handle);
            }

            auto findFirst(usize* outValue) const -> bool {
                return src::foundation::BitSet_findFirst(this->handle, outValue);
            }
            auto findNext(const usize fromValue, usize* outValue) const -> bool {
                return src::foundation::BitSet_findNext(this->handle, fromValue, outValue);
            }

            auto unionWith(const BitSet& other) -> bool {
                return src::foundation::BitSet_unionWith(this->handle, other.handle);
            }
            auto intersectWith(const BitSet& other) -> bool {
                return src::foundation::BitSet_intersectWith(this->handle, other.handle);
            }
            auto differenceWith(const BitSet& other) -> bool {
                return src::foundation::BitSet_differenceWith(this->handle, other.handle);
            }
            auto symmetricDifferenceWith(const BitSet& other) -> bool {
                return src::foundation::BitSet_symmetricDifferenceWith(this->handle, other.handle);
            }

            auto capacity() const -> usize {
                return this->handle ? src::foundation::BitSet_capacity(this->handle) : 0;
            }

            auto contains(const usize value) const -> bool {
                return src::foundation::BitSet_contains(this->handle, value);
            }
            auto count() const -> usize {
                return this->handle ? src::foundation::BitSet_count(this->handle) : 0;
            }
            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::BitSet_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::BitSet* handle = nullptr;
    };
}