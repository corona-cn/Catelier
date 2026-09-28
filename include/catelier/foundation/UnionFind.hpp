#pragma once
#include "../../../src/catelier/foundation/UnionFind.hpp"

namespace Catelier::foundation {
    class UnionFind {
        public:
            explicit UnionFind(const usize inCapacity) {
                this->handle = src::foundation::UnionFind_construct(inCapacity);
            }
            ~UnionFind() {
                if (this->handle) {
                    src::foundation::UnionFind_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            UnionFind(const UnionFind& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::UnionFind_copy(other.handle);
            }
            UnionFind(UnionFind&& other) noexcept {
                this->handle = src::foundation::UnionFind_move(other.handle);
            }
            UnionFind& operator = (const UnionFind& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::UnionFind_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::UnionFind_copy(other.handle);
                    }
                }

                return *this;
            }
            UnionFind& operator = (UnionFind&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::UnionFind_destruct(this->handle);
                    }

                    this->handle = src::foundation::UnionFind_move(other.handle);
                }

                return *this;
            }

            auto find(const usize element) -> usize {
                if (!this->handle) {
                    return element;
                }

                return src::foundation::UnionFind_find(this->handle, element);
            }
            auto merge(const usize elementA, const usize elementB) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::UnionFind_union(this->handle, elementA, elementB);
            }

            auto connected(const usize elementA, const usize elementB) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::UnionFind_connected(this->handle, elementA, elementB);
            }

            auto capacity() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::UnionFind_capacity(this->handle);
            }
            auto setCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::UnionFind_setCount(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::UnionFind_isEmpty(this->handle);
            }

        private:
            src::foundation::UnionFind* handle = nullptr;
    };
}