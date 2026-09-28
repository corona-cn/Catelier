#pragma once
#include "../../../src/catelier/foundation/EdgeList.hpp"

namespace Catelier::foundation {
    template<typename Edge>
    class EdgeList {
        public:
            explicit EdgeList(const usize inNodeCount, const bool inIsDirected) {
                this->handle = src::foundation::EdgeList_construct(inNodeCount, sizeof(Edge), inIsDirected);
            }
            ~EdgeList() {
                if (this->handle) {
                    src::foundation::EdgeList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            EdgeList(const EdgeList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::EdgeList_copy(other.handle);
            }
            EdgeList(EdgeList&& other) noexcept {
                this->handle = src::foundation::EdgeList_move(other.handle);
            }
            EdgeList& operator = (const EdgeList& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::EdgeList_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::EdgeList_copy(other.handle);
                    }
                }

                return *this;
            }
            EdgeList& operator = (EdgeList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::EdgeList_destruct(this->handle);
                    }

                    this->handle = src::foundation::EdgeList_move(other.handle);
                }

                return *this;
            }

            auto addEdge(const usize fromNode, const usize toNode, const Edge& inEdge) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_addEdge(this->handle, fromNode, toNode, &inEdge);
            }

            auto removeEdge(const usize fromNode, const usize toNode) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_removeEdge(this->handle, fromNode, toNode);
            }
            auto removeAt(const usize index) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_removeAt(this->handle, index);
            }
            auto clearEdges() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_clearEdges(this->handle);
            }

            auto getEdge(const usize fromNode, const usize toNode) -> Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Edge*) src::foundation::EdgeList_getEdge(this->handle, fromNode, toNode);
            }
            auto getEdge(const usize fromNode, const usize toNode) const -> const Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Edge*) src::foundation::EdgeList_getEdge(this->handle, fromNode, toNode);
            }

            auto nodeFrom(const usize index) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::EdgeList_nodeFrom(this->handle, index);
            }
            auto nodeTo(const usize index) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::EdgeList_nodeTo(this->handle, index);
            }
            auto edgeAt(const usize index) -> Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Edge*) src::foundation::EdgeList_edgeAt(this->handle, index);
            }
            auto edgeAt(const usize index) const -> const Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Edge*) src::foundation::EdgeList_edgeAt(this->handle, index);
            }

            auto reserve(const usize newCapacity) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_reserve(this->handle, newCapacity);
            }
            auto shrinkToFit() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_shrinkToFit(this->handle);
            }

            auto nodeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::EdgeList_nodeCount(this->handle);
            }
            auto edgeSize() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::EdgeList_edgeSize(this->handle);
            }
            auto edgeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::EdgeList_edgeCount(this->handle);
            }
            auto capacity() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::EdgeList_capacity(this->handle);
            }
            auto isDirected() const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_isDirected(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::EdgeList_isEmpty(this->handle);
            }
            auto hasEdge(const usize fromNode, const usize toNode) const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::EdgeList_hasEdge(this->handle, fromNode, toNode);
            }

        private:
            src::foundation::EdgeList* handle = nullptr;
    };
}