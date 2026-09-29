#pragma once
#include "../../../src/catelier/foundation/HuffmanTree.hpp"

namespace Catelier::foundation {
    template<typename Type>
    struct HuffmanTree_AscendingComparator {
        auto operator()(const Type& a, const Type& b) const -> bool {
            return a < b;
        }
    };

    template<typename Key, typename Compare = HuffmanTree_AscendingComparator<Key>>
    class HuffmanTree {
        public:
            explicit HuffmanTree() {
                this->handle = src::foundation::HuffmanTree_construct(sizeof(Key), compareBridge);
            }
            ~HuffmanTree() {
                if (this->handle) {
                    src::foundation::HuffmanTree_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            HuffmanTree(const HuffmanTree& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::HuffmanTree_copy(other.handle);
            }
            HuffmanTree(HuffmanTree&& other) noexcept {
                this->handle = src::foundation::HuffmanTree_move(other.handle);
            }
            HuffmanTree& operator = (const HuffmanTree& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::HuffmanTree_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::HuffmanTree_copy(other.handle);
                    }
                }

                return *this;
            }
            HuffmanTree& operator = (HuffmanTree&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::HuffmanTree_destruct(this->handle);
                    }

                    this->handle = src::foundation::HuffmanTree_move(other.handle);
                }

                return *this;
            }

            auto build(const Key* keys, const u64* weights, const usize count) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HuffmanTree_build(this->handle, keys, weights, count);
            }

            auto encodeLength(const Key& key) const -> usize {
                if (!this->handle) {
                    return 0;
                }

                usize length = 0;
                if (findKeyPath(src::foundation::HuffmanTree_rootNode(this->handle), key, nullptr, 0, &length)) {
                    return length;
                }

                return 0;
            }
            auto encode(const Key& key, bool* outBits) const -> bool {
                if (!this->handle || !outBits) {
                    return false;
                }

                usize length = 0;
                return findKeyPath(src::foundation::HuffmanTree_rootNode(this->handle), key, outBits, 0, &length);
            }

            template<typename Function>
            auto decode(const bool* bits, const usize length, Function callback) const -> usize {
                if (!this->handle || !bits || length == 0) {
                    return 0;
                }

                auto* node = src::foundation::HuffmanTree_rootNode(this->handle);
                usize count = 0;

                for (usize i = 0; i < length; ++i) {
                    if (bits[i]) {
                        node = src::foundation::HuffmanTreeNode_right(node);
                    } else {
                        node = src::foundation::HuffmanTreeNode_left(node);
                    }

                    if (!node) {
                        return count;
                    }

                    if (src::foundation::HuffmanTreeNode_isLeaf(node)) {
                        const auto* nodeKey = (const Key*) src::foundation::HuffmanTreeNode_key(node);
                        if (nodeKey) {
                            callback(*nodeKey);
                            count++;
                        }
                        node = src::foundation::HuffmanTree_rootNode(this->handle);
                    }
                }

                return count;
            }

            auto weight() const -> u64 {
                return this->handle ? src::foundation::HuffmanTree_weight(this->handle) : 0;
            }
            auto size() const -> usize {
                return this->handle ? src::foundation::HuffmanTree_size(this->handle) : 0;
            }

            auto isEmpty() const -> bool {
                return this->handle ? src::foundation::HuffmanTree_isEmpty(this->handle) : true;
            }

        private:
            src::foundation::HuffmanTree* handle = nullptr;

            static auto compareBridge(const void* a, const void* b) -> i32 {
                if (Compare{}(*((const Key*) a), *((const Key*) b))) {
                    return -1;
                }

                if (Compare{}(*((const Key*) b), *((const Key*) a))) {
                    return 1;
                }

                return 0;
            }

            static auto findKeyPath(const src::foundation::HuffmanTreeNode* node, const Key& key, bool* outBits, const usize depth, usize* outLength) -> bool {
                if (!node) {
                    return false;
                }

                // 叶子节点：比较 key
                if (src::foundation::HuffmanTreeNode_isLeaf(node)) {
                    const auto* nodeKey = (const Key*) src::foundation::HuffmanTreeNode_key(node);
                    if (nodeKey && compareBridge(nodeKey, &key) == 0) {
                        if (outLength) {
                            *outLength = depth;
                        }
                        return true;
                    }
                    return false;
                }

                // 尝试左子树
                if (outBits) {
                    outBits[depth] = false;
                }

                if (findKeyPath(src::foundation::HuffmanTreeNode_left(node), key, outBits, depth + 1, outLength)) {
                    return true;
                }

                // 尝试右子树
                if (outBits) {
                    outBits[depth] = true;
                }

                if (findKeyPath(src::foundation::HuffmanTreeNode_right(node), key, outBits, depth + 1, outLength)) {
                    return true;
                }

                return false;
            }
    };
}