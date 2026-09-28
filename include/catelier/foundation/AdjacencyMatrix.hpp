#pragma once
#include "../../../src/catelier/foundation/AdjacencyMatrix.hpp"

namespace Catelier::foundation {
    template<typename Edge>
    class AdjacencyMatrix {
        public:
            explicit AdjacencyMatrix(const usize inNodeCount, const bool inIsDirected, const Edge& inNoEdge) {
                this->handle = src::foundation::AdjacencyMatrix_construct(inNodeCount, sizeof(Edge), inIsDirected, &inNoEdge);
            }
            ~AdjacencyMatrix() {
                if (this->handle) {
                    src::foundation::AdjacencyMatrix_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            AdjacencyMatrix(const AdjacencyMatrix& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::AdjacencyMatrix_copy(other.handle);
            }
            AdjacencyMatrix(AdjacencyMatrix&& other) noexcept {
                this->handle = src::foundation::AdjacencyMatrix_move(other.handle);
            }
            AdjacencyMatrix& operator = (const AdjacencyMatrix& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::AdjacencyMatrix_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::AdjacencyMatrix_copy(other.handle);
                    }
                }

                return *this;
            }
            AdjacencyMatrix& operator = (AdjacencyMatrix&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::AdjacencyMatrix_destruct(this->handle);
                    }

                    this->handle = src::foundation::AdjacencyMatrix_move(other.handle);
                }

                return *this;
            }

            auto setEdge(const usize fromNode, const usize toNode, const Edge& inEdge) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyMatrix_setEdge(this->handle, fromNode, toNode, &inEdge);
            }

            auto removeEdge(const usize fromNode, const usize toNode) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyMatrix_removeEdge(this->handle, fromNode, toNode);
            }
            auto clearEdges() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyMatrix_clearEdges(this->handle);
            }

            auto getEdge(const usize fromNode, const usize toNode) -> Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (Edge*) src::foundation::AdjacencyMatrix_getEdge(this->handle, fromNode, toNode);
            }
            auto getEdge(const usize fromNode, const usize toNode) const -> const Edge* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const Edge*) src::foundation::AdjacencyMatrix_getEdge(this->handle, fromNode, toNode);
            }

            auto nodeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyMatrix_nodeCount(this->handle);
            }
            auto edgeSize() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyMatrix_edgeSize(this->handle);
            }
            auto edgeCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyMatrix_edgeCount(this->handle);
            }
            auto isDirected() const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyMatrix_isDirected(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::AdjacencyMatrix_isEmpty(this->handle);
            }
            auto hasEdge(const usize fromNode, const usize toNode) const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::AdjacencyMatrix_hasEdge(this->handle, fromNode, toNode);
            }
            auto degree(const usize node) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::AdjacencyMatrix_degree(this->handle, node);
            }

        private:
            src::foundation::AdjacencyMatrix* handle = nullptr;
    };
}