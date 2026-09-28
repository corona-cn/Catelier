#include "HuffmanTree.hpp"

#include <cstdlib>
#include <cstring>

#include "BinaryHeap.hpp"

namespace Catelier::src::foundation {
    typedef struct HuffmanTreeNode {
        void* key;
        u64 weight;
        HuffmanTreeNode* leftChild;
        HuffmanTreeNode* rightChild;
    } HuffmanTreeNode;

    typedef struct HuffmanTree {
        HuffmanTreeNode* rootNode;
        usize keySize;
        usize size;
        i32 (*compare)(const void*, const void*);
    } HuffmanTree;

    namespace {
        auto destructNode(const HuffmanTree* self, HuffmanTreeNode* subtreeRoot) -> void {
            if (!subtreeRoot) {
                return;
            }

            destructNode(self, subtreeRoot->leftChild);
            destructNode(self, subtreeRoot->rightChild);

            if (subtreeRoot->key) {
                free(subtreeRoot->key);
            }

            free(subtreeRoot);
        }
        auto copyNode(const HuffmanTree* self, const HuffmanTreeNode* sourceNode) -> HuffmanTreeNode* {
            if (!sourceNode) {
                return nullptr;
            }

            auto* const newNode = (HuffmanTreeNode*) malloc(sizeof(HuffmanTreeNode));
            if (!newNode) {
                return nullptr;
            }

            // 复制键内存，内部节点没有键
            if (sourceNode->key) {
                newNode->key = malloc(self->keySize);
                if (!newNode->key) {
                    free(newNode);
                    return nullptr;
                }
                memcpy(newNode->key, sourceNode->key, self->keySize);
            } else {
                newNode->key = nullptr;
            }

            newNode->weight = sourceNode->weight;

            // 递归复制左右子树
            auto* const newLeft = copyNode(self, sourceNode->leftChild);
            if (sourceNode->leftChild && !newLeft) {
                if (newNode->key) {
                    free(newNode->key);
                }
                free(newNode);
                return nullptr;
            }

            auto* const newRight = copyNode(self, sourceNode->rightChild);
            if (sourceNode->rightChild && !newRight) {
                destructNode(self, newLeft);
                if (newNode->key) {
                    free(newNode->key);
                }
                free(newNode);
                return nullptr;
            }

            newNode->leftChild = newLeft;
            newNode->rightChild = newRight;

            return newNode;
        }
    }

    namespace {
        auto weightCompare(const void* a, const void* b) -> bool {
            auto* const nodeA = *(const HuffmanTreeNode**) a;
            auto* const nodeB = *(const HuffmanTreeNode**) b;

            return nodeA->weight < nodeB->weight;
        }
        auto destructHeapNodes(const BinaryHeap* heap) -> void {
            const usize size = BinaryHeap_size(heap);
            const usize elementSize = BinaryHeap_elementSize(heap);
            u8* const elements = BinaryHeap_elements(heap);

            for (usize i = 0; i < size; ++i) {
                auto* const node = *(HuffmanTreeNode**) (elements + i * elementSize);
                destructNode(nullptr, node);
            }
        }
    }

    auto HuffmanTree_construct(const usize inKeySize, i32 (*compare)(const void*, const void*)) -> HuffmanTree* {
        if (inKeySize == 0 || !compare) {
            return nullptr;
        }

        auto* const self = (HuffmanTree*) malloc(sizeof(HuffmanTree));
        if (!self) {
            return nullptr;
        }

        self->rootNode = nullptr;
        self->keySize = inKeySize;
        self->size = 0;
        self->compare = compare;

        return self;
    }
    auto HuffmanTree_destruct(HuffmanTree* self) -> bool {
        if (!self) {
            return false;
        }

        destructNode(self, self->rootNode);

        free(self);

        return true;
    }

    auto HuffmanTree_copy(const HuffmanTree* self) -> HuffmanTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = HuffmanTree_construct(self->keySize, self->compare);
        if (!newSelf) {
            return nullptr;
        }

        // 递归复制整棵树
        newSelf->rootNode = copyNode(newSelf, self->rootNode);
        if (self->rootNode && !newSelf->rootNode) {
            HuffmanTree_destruct(newSelf);
            return nullptr;
        }

        newSelf->size = self->size;

        return newSelf;
    }
    auto HuffmanTree_move(HuffmanTree* self) -> HuffmanTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (HuffmanTree*) malloc(sizeof(HuffmanTree));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->rootNode = self->rootNode;
        newSelf->keySize = self->keySize;
        newSelf->size = self->size;
        newSelf->compare = self->compare;

        self->rootNode = nullptr;
        self->keySize = 0;
        self->size = 0;
        self->compare = nullptr;

        return newSelf;
    }

    auto HuffmanTree_build(HuffmanTree* self, const void* inKeys, const u64* inWeights, const usize count) -> bool {
        if (!self || !inKeys || !inWeights || count == 0) {
            return false;
        }

        // 清空已有树
        if (self->rootNode) {
            destructNode(self, self->rootNode);
            self->rootNode = nullptr;
            self->size = 0;
        }

        auto* const heap = BinaryHeap_construct(count, sizeof(HuffmanTreeNode*), weightCompare);
        if (!heap) {
            return false;
        }

        // 把所有叶子节点压入堆
        for (usize i = 0; i < count; ++i) {
            auto* const newNode = (HuffmanTreeNode*) malloc(sizeof(HuffmanTreeNode));
            if (!newNode) {
                destructHeapNodes(heap);
                BinaryHeap_destruct(heap);
                return false;
            }

            newNode->key = malloc(self->keySize);
            if (!newNode->key) {
                free(newNode);
                destructHeapNodes(heap);
                BinaryHeap_destruct(heap);
                return false;
            }
            memcpy(newNode->key, (const u8*) inKeys + i * self->keySize, self->keySize);

            newNode->weight = *(inWeights + i);
            newNode->leftChild = nullptr;
            newNode->rightChild = nullptr;

            if (!BinaryHeap_push(heap, &newNode)) {
                destructNode(self, newNode);
                destructHeapNodes(heap);
                BinaryHeap_destruct(heap);
                return false;
            }
        }

        // 贪心合并：每次取权值最小的两棵树，合并成一棵新树放回堆中
        while (BinaryHeap_size(heap) > 1) {
            // 取出权值最小的节点
            auto** const slot1 = (HuffmanTreeNode**) BinaryHeap_popSlot(heap);
            auto* const node1 = *slot1;

            // 取出权值次小的节点
            auto** const slot2 = (HuffmanTreeNode**) BinaryHeap_popSlot(heap);
            auto* const node2 = *slot2;

            // 创建新树：左子为最小，右子为次小，权值为两者之和
            auto* const parentNode = (HuffmanTreeNode*) malloc(sizeof(HuffmanTreeNode));
            if (!parentNode) {
                destructNode(self, node1);
                destructNode(self, node2);
                destructHeapNodes(heap);
                BinaryHeap_destruct(heap);
                return false;
            }

            parentNode->key = nullptr;
            parentNode->weight = node1->weight + node2->weight;
            parentNode->leftChild = node1;
            parentNode->rightChild = node2;

            // 新树放回堆中
            if (!BinaryHeap_push(heap, &parentNode)) {
                destructNode(self, parentNode);
                destructHeapNodes(heap);
                BinaryHeap_destruct(heap);
                return false;
            }
        }

        // 堆中剩下的唯一节点就是哈夫曼树的根
        auto** const rootSlot = (HuffmanTreeNode**) BinaryHeap_popSlot(heap);
        self->rootNode = *rootSlot;
        self->size = count;

        // 释放堆
        BinaryHeap_destruct(heap);

        return true;
    }

    auto HuffmanTree_rootNode(const HuffmanTree* self) -> HuffmanTreeNode* {
        if (!self) {
            return nullptr;
        }

        return self->rootNode;
    }
    auto HuffmanTree_weight(const HuffmanTree* self) -> u64 {
        if (!self || !self->rootNode) {
            return 0;
        }

        return self->rootNode->weight;
    }
    auto HuffmanTree_size(const HuffmanTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }

    auto HuffmanTree_isEmpty(const HuffmanTree* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }

    auto HuffmanTreeNode_key(const HuffmanTreeNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->key;
    }
    auto HuffmanTreeNode_weight(const HuffmanTreeNode* node) -> u64 {
        if (!node) {
            return 0;
        }

        return node->weight;
    }
    auto HuffmanTreeNode_left(const HuffmanTreeNode* node) -> HuffmanTreeNode* {
        if (!node) {
            return nullptr;
        }

        return node->leftChild;
    }
    auto HuffmanTreeNode_right(const HuffmanTreeNode* node) -> HuffmanTreeNode* {
        if (!node) {
            return nullptr;
        }

        return node->rightChild;
    }
    auto HuffmanTreeNode_isLeaf(const HuffmanTreeNode* node) -> bool {
        if (!node) {
            return true;
        }

        return node->leftChild == nullptr && node->rightChild == nullptr;
    }
}