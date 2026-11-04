#pragma once
#include "../../../../../src/catelier/foundation/structure/graph/EdgeList.hpp"

namespace Catelier::include::foundation::structure::graph {
    template<typename Edge>
    class EdgeList {
        public:
            explicit EdgeList(const usize inNodeCount, const bool inIsDirected) {
                this->handle = src::foundation::structure::graph::EdgeList_construct(inNodeCount, sizeof(Edge), inIsDirected);
            }
            ~EdgeList() {
                if (this->handle) {
                    src::foundation::structure::graph::EdgeList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }
            EdgeList(const EdgeList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::structure::graph::EdgeList_copy(other.handle);
            }
            EdgeList(EdgeList&& other) noexcept {
                this->handle = src::foundation::structure::graph::EdgeList_move(other.handle);
            }
            EdgeList& operator = (const EdgeList& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::structure::graph::EdgeList_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::structure::graph::EdgeList_copy(other.handle);
                    }
                }

                return *this;
            }
            EdgeList& operator = (EdgeList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::structure::graph::EdgeList_destruct(this->handle);
                    }

                    this->handle = src::foundation::structure::graph::EdgeList_move(other.handle);
                }

                return *this;
            }

            auto addEdge(const usize fromNode, const usize toNode, const Edge& inEdge) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_addEdge(this->handle, fromNode, toNode, &inEdge);
            }

            auto removeEdge(const usize fromNode, const usize toNode) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_removeEdge(this->handle, fromNode, toNode);
            }
            auto removeAt(const usize index) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_removeAt(this->handle, index);
            }
            auto clearEdges() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_clearEdges(this->handle);
            }

            auto getEdge(const usize fromNode, const usize toNode) -> Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Edge*) src::foundation::structure::graph::EdgeList_getEdge(this->handle, fromNode, toNode);
            }
            auto getEdge(const usize fromNode, const usize toNode) const -> const Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Edge*) src::foundation::structure::graph::EdgeList_getEdge(this->handle, fromNode, toNode);
            }

            auto nodeFrom(const usize index) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::graph::EdgeList_nodeFrom(this->handle, index);
            }
            auto nodeTo(const usize index) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::graph::EdgeList_nodeTo(this->handle, index);
            }
            auto edgeAt(const usize index) -> Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Edge*) src::foundation::structure::graph::EdgeList_edgeAt(this->handle, index);
            }
            auto edgeAt(const usize index) const -> const Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Edge*) src::foundation::structure::graph::EdgeList_edgeAt(this->handle, index);
            }

            auto reserve(const usize newCapacity) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_reserve(this->handle, newCapacity);
            }
            auto shrinkToFit() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_shrinkToFit(this->handle);
            }

            auto nodeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::graph::EdgeList_nodeCount(this->handle);
            }
            auto edgeSize() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::graph::EdgeList_edgeSize(this->handle);
            }
            auto edgeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::graph::EdgeList_edgeCount(this->handle);
            }
            auto capacity() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::structure::graph::EdgeList_capacity(this->handle);
            }
            auto isDirected() const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_isDirected(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::structure::graph::EdgeList_isEmpty(this->handle);
            }
            auto hasEdge(const usize fromNode, const usize toNode) const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::structure::graph::EdgeList_hasEdge(this->handle, fromNode, toNode);
            }

        private:
            src::foundation::structure::graph::EdgeList* handle = nullptr;
    };
}