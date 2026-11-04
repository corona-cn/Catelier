#pragma once
#include "../../../../../../src/catelier/foundation/structure/tree/disjoint/UnionFind.hpp"

namespace Catelier::include::foundation::structure::tree::disjoint {
    class UnionFind {
        public:
            explicit UnionFind(const usize inCapacity) {
                this->handle = src::foundation::structure::tree::disjoint::UnionFind_construct(inCapacity);
            }
            ~UnionFind() {
                if (this->handle) {
                    src::foundation::structure::tree::disjoint::UnionFind_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            UnionFind(const UnionFind& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::structure::tree::disjoint::UnionFind_copy(other.handle);
            }
            UnionFind(UnionFind&& other) noexcept {
                this->handle = src::foundation::structure::tree::disjoint::UnionFind_move(other.handle);
            }
            UnionFind& operator = (const UnionFind& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::structure::tree::disjoint::UnionFind_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::structure::tree::disjoint::UnionFind_copy(other.handle);
                    }
                }

                return *this;
            }
            UnionFind& operator = (UnionFind&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::structure::tree::disjoint::UnionFind_destruct(this->handle);
                    }

                    this->handle = src::foundation::structure::tree::disjoint::UnionFind_move(other.handle);
                }

                return *this;
            }

            auto find(const usize element) -> usize {
                if (!this->handle) {
                    return element;
                }

                return src::foundation::structure::tree::disjoint::UnionFind_find(this->handle, element);
            }
            auto merge(const usize elementA, const usize elementB) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::tree::disjoint::UnionFind_union(this->handle, elementA, elementB);
            }

            auto connected(const usize elementA, const usize elementB) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::tree::disjoint::UnionFind_connected(this->handle, elementA, elementB);
            }

            auto capacity() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::tree::disjoint::UnionFind_capacity(this->handle);
            }
            auto setCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::tree::disjoint::UnionFind_setCount(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::structure::tree::disjoint::UnionFind_isEmpty(this->handle);
            }

        private:
            src::foundation::structure::tree::disjoint::UnionFind* handle = nullptr;
    };
}