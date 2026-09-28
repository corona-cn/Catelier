#pragma once
#include "../../../src/catelier/foundation/AdjacencyList.hpp"

namespace Catelier::foundation {
    template<typename Edge>
    class AdjacencyList {
        public:
            explicit AdjacencyList(const usize inNodeCount, const bool inIsDirected) {
                this->handle = src::foundation::AdjacencyList_construct(inNodeCount, sizeof(Edge), inIsDirected);
            }
            ~AdjacencyList() {
                if (this->handle) {
                    src::foundation::AdjacencyList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            AdjacencyList(const AdjacencyList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::AdjacencyList_copy(other.handle);
            }
            AdjacencyList(AdjacencyList&& other) noexcept {
                this->handle = src::foundation::AdjacencyList_move(other.handle);
            }
            AdjacencyList& operator = (const AdjacencyList& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::AdjacencyList_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::AdjacencyList_copy(other.handle);
                    }
                }

                return *this;
            }
            AdjacencyList& operator = (AdjacencyList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::AdjacencyList_destruct(this->handle);
                    }

                    this->handle = src::foundation::AdjacencyList_move(other.handle);
                }

                return *this;
            }

            auto addEdge(const usize fromNode, const usize toNode, const Edge& inEdge) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyList_addEdge(this->handle, fromNode, toNode, &inEdge);
            }

            auto removeEdge(const usize fromNode, const usize toNode) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyList_removeEdge(this->handle, fromNode, toNode);
            }
            auto clearEdges() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyList_clearEdges(this->handle);
            }

            auto getEdge(const usize fromNode, const usize toNode) -> Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Edge*) src::foundation::AdjacencyList_getEdge(this->handle, fromNode, toNode);
            }
            auto getEdge(const usize fromNode, const usize toNode) const -> const Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Edge*) src::foundation::AdjacencyList_getEdge(this->handle, fromNode, toNode);
            }

            auto neighborCount(const usize node) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyList_neighborCount(this->handle, node);
            }
            auto neighborNodeAt(const usize node, const usize index) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyList_neighborNodeAt(this->handle, node, index);
            }
            auto neighborEdgeAt(const usize node, const usize index) -> Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Edge*) src::foundation::AdjacencyList_neighborEdgeAt(this->handle, node, index);
            }
            auto neighborEdgeAt(const usize node, const usize index) const -> const Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Edge*) src::foundation::AdjacencyList_neighborEdgeAt(this->handle, node, index);
            }

            auto nodeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyList_nodeCount(this->handle);
            }
            auto edgeSize() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyList_edgeSize(this->handle);
            }
            auto edgeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyList_edgeCount(this->handle);
            }
            auto isDirected() const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyList_isDirected(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::AdjacencyList_isEmpty(this->handle);
            }
            auto hasEdge(const usize fromNode, const usize toNode) const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyList_hasEdge(this->handle, fromNode, toNode);
            }
            auto degree(const usize node) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyList_degree(this->handle, node);
            }

        private:
            src::foundation::AdjacencyList* handle = nullptr;
    };
}