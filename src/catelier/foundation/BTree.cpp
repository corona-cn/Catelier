#include "BTree.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct BTreeNode {
        u8* keyBytes;
        u8* valueBytes;
        BTreeNode** childNodes;
        usize keyCount;
        bool isLeafNode;
    } BTreeNode;

    typedef struct BTree {
        BTreeNode* rootNode;
        usize order;
        usize keySize;
        usize valueSize;
        usize size;
        i32 (*compare)(const void*, const void*);
    } BTree;

    namespace {
        auto createNode(const BTree* self, const bool isLeafNode) -> BTreeNode* {
            const usize childNodesBytes = sizeof(BTreeNode*) * self->order;
            const usize keyBytes = self->keySize * (self->order - 1);
            const usize valueBytes = self->valueSize * (self->order - 1);
            auto* const node = (BTreeNode*) malloc(sizeof(BTreeNode) + childNodesBytes + keyBytes + valueBytes);
            if (!node) {
                return nullptr;
            }

            node->childNodes = (BTreeNode**) ((u8*) node + sizeof(BTreeNode));
            node->keyBytes = (u8*) (node->childNodes + self->order);
            node->valueBytes = (u8*) (node->keyBytes + keyBytes);

            // 子节点指针数组清零
            for (usize i = 0; i < self->order; ++i) {
                *(node->childNodes + i) = nullptr;
            }

            node->keyCount = 0;
            node->isLeafNode = isLeafNode;

            return node;
        }
        auto destructNode(const BTree* self, BTreeNode* subtreeRoot) -> void {
            if (!subtreeRoot) {
                return;
            }

            // 非叶子节点先递归释放所有子节点
            if (!subtreeRoot->isLeafNode) {
                for (usize i = 0; i <= subtreeRoot->keyCount; ++i) {
                    destructNode(self, *(subtreeRoot->childNodes + i));
                }
            }

            free(subtreeRoot);
        }
        auto copyNode(const BTree* self, const BTreeNode* sourceNode) -> BTreeNode* {
            if (!sourceNode) {
                return nullptr;
            }

            auto* const newNode = createNode(self, sourceNode->isLeafNode);
            if (!newNode) {
                return nullptr;
            }

            memcpy(newNode->keyBytes, sourceNode->keyBytes, self->keySize * sourceNode->keyCount);
            memcpy(newNode->valueBytes, sourceNode->valueBytes, self->valueSize * sourceNode->keyCount);

            newNode->keyCount = sourceNode->keyCount;

            // 非叶子节点递归复制所有子节点
            if (!sourceNode->isLeafNode) {
                for (usize i = 0; i <= sourceNode->keyCount; ++i) {
                    auto* const newChild = copyNode(self, *(sourceNode->childNodes + i));
                    if (*(sourceNode->childNodes + i) && !newChild) {
                        // 逆序释放已复制的子节点
                        for (usize j = 0; j < i; ++j) {
                            destructNode(self, *(newNode->childNodes + j));
                        }

                        free(newNode);

                        return nullptr;
                    }

                    *(newNode->childNodes + i) = newChild;
                }
            }

            return newNode;
        }
    }

    namespace {
        auto findKey(const BTree* self, const BTreeNode* node, const void* inKey, usize* indexOut) -> bool {
            // 在指定节点内二分查找 key
            // 命中时 indexOut 是 key 的下标，返回 true
            // 未命中时 indexOut 是应该插入的位置，返回 false
            usize low = 0;
            usize high = node->keyCount;
            while (low < high) {
                const usize mid = low + (high - low) / 2;
                const i32 compareResult = self->compare(inKey, node->keyBytes + mid * self->keySize);
                if (compareResult == 0) {
                    *indexOut = mid;
                    return true;
                }

                if (compareResult < 0) {
                    high = mid;
                } else {
                    low = mid + 1;
                }
            }

            *indexOut = low;

            return false;
        }
        auto reserveSlot(const BTree* self, BTreeNode* node, const usize index, void** keySlotOut, void** valueSlotOut) -> void {
            // 在指定节点指定下标处腾出 key-value 空位，返回两个未初始化槽位
            // 把 [index, keyCount) 范围内的数据右移一位
            memmove(node->keyBytes + (index + 1) * self->keySize, node->keyBytes + index * self->keySize, (node->keyCount - index) * self->keySize);
            memmove(node->valueBytes + (index + 1) * self->valueSize, node->valueBytes + index * self->valueSize, (node->keyCount - index) * self->valueSize);

            *keySlotOut = node->keyBytes + index * self->keySize;
            *valueSlotOut = node->valueBytes + index * self->valueSize;

            node->keyCount++;
        }
        auto splitNode(const BTree* self, BTreeNode* parentNode, const usize index) -> bool {
            // 分裂指定父节点的第 index 个满子节点
            // 中间 key-value 上浮到父节点的 index 位置
            // 左半部分保留在原节点中，右半部分移入新建节点
            auto* const childNode = *(parentNode->childNodes + index);
            auto* const newChildNode = createNode(self, childNode->isLeafNode);
            if (!newChildNode) {
                return false;
            }

            const usize mid = (self->order - 1) / 2;
            const usize rightKeyCount = self->order - 1 - mid - 1;

            // 把 childNode 的右半部分 key / value 复制到 newChildNode
            memcpy(newChildNode->keyBytes, childNode->keyBytes + (mid + 1) * self->keySize, rightKeyCount * self->keySize);
            memcpy(newChildNode->valueBytes, childNode->valueBytes + (mid + 1) * self->valueSize, rightKeyCount * self->valueSize);
            newChildNode->keyCount = rightKeyCount;

            // 非叶节点还要把右半部分的子节点指针复制到 newChildNode
            if (!childNode->isLeafNode) {
                memcpy(newChildNode->childNodes, childNode->childNodes + (mid + 1), (rightKeyCount + 1) * sizeof(BTreeNode*));
            }

            // child 保留左半部分
            childNode->keyCount = mid;

            // parentNode 腾出 index 位置的 key / value / childNode
            memmove(parentNode->keyBytes + (index + 1) * self->keySize, parentNode->keyBytes + index * self->keySize, (parentNode->keyCount - index) * self->keySize);
            memmove(parentNode->valueBytes + (index + 1) * self->valueSize, parentNode->valueBytes + index * self->valueSize, (parentNode->keyCount - index) * self->valueSize);
            memmove(parentNode->childNodes + (index + 2), parentNode->childNodes + (index + 1), (parentNode->keyCount - index) * sizeof(BTreeNode*));

            // 把 childNode 的中间 key / value 上浮到 parentNode 的 index 位置
            memcpy(parentNode->keyBytes + index * self->keySize, childNode->keyBytes + mid * self->keySize, self->keySize);
            memcpy(parentNode->valueBytes + index * self->valueSize, childNode->valueBytes + mid * self->valueSize, self->valueSize);

            // parentNode 的 index + 1 位置指向 newChild
            *(parentNode->childNodes + index + 1) = newChildNode;

            parentNode->keyCount++;

            return true;
        }

        auto releaseSlot(const BTree* self, BTreeNode* node, const usize index) -> void {
            // 从节点中删除指定下标的 key-value
            // 把 (index, keyCount) 范围内的数据左移一位
            memmove(node->keyBytes + index * self->keySize, node->keyBytes + (index + 1) * self->keySize, (node->keyCount - index - 1) * self->keySize);
            memmove(node->valueBytes + index * self->valueSize, node->valueBytes + (index + 1) * self->valueSize, (node->keyCount - index - 1) * self->valueSize);

            node->keyCount--;
        }
        auto mergeNodes(const BTree* self, BTreeNode* parentNode, const usize index) -> void {
            // 把父节点的 index key-value 下移，与 index+1 位置的子节点合并到 index 位置的子节点
            // 合并后父节点减少一个 key 和一个子节点
            auto* leftChildNode = *(parentNode->childNodes + index);
            auto* rightChildNode = *(parentNode->childNodes + index + 1);

            // 父节点的 index key-value 追加到 leftChildNode
            memcpy(leftChildNode->keyBytes + leftChildNode->keyCount * self->keySize, parentNode->keyBytes + index * self->keySize, self->keySize);
            memcpy(leftChildNode->valueBytes + leftChildNode->keyCount * self->valueSize, parentNode->valueBytes + index * self->valueSize, self->valueSize);

            leftChildNode->keyCount++;

            // rightChildNode 的所有 key-value 追加到 leftChildNode
            memcpy(leftChildNode->keyBytes + leftChildNode->keyCount * self->keySize, rightChildNode->keyBytes, rightChildNode->keyCount * self->keySize);
            memcpy(leftChildNode->valueBytes + leftChildNode->keyCount * self->valueSize, rightChildNode->valueBytes, rightChildNode->keyCount * self->valueSize);

            // 非叶节点还要把 rightChildNode 的子节点指针追加到 leftChildNode
            if (!leftChildNode->isLeafNode) {
                memcpy(leftChildNode->childNodes + leftChildNode->keyCount, rightChildNode->childNodes, (rightChildNode->keyCount + 1) * sizeof(BTreeNode*));
            }

            leftChildNode->keyCount += rightChildNode->keyCount;

            free(rightChildNode);

            // 从父节点删除 index key-value，同时删除 index+1 位置的子节点
            releaseSlot(self, parentNode, index);

            memmove(parentNode->childNodes + index + 1, parentNode->childNodes + index + 2, (parentNode->keyCount - index) * sizeof(BTreeNode*));
        }
        auto fixChildUnderflow(const BTree* self, BTreeNode* parentNode, const usize index) -> void {
            // 修复父节点第 index 个子节点的下溢
            // 优先从左右兄弟借位，都不行则合并
            const usize minKeyCount = (self->order - 2) / 2;
            auto* childNode = *(parentNode->childNodes + index);

            // 尝试从左兄弟借位
            if (index > 0) {
                auto* leftSiblingNode = *(parentNode->childNodes + index - 1);
                if (leftSiblingNode->keyCount > minKeyCount) {
                    // childNode 的 key-value 右移一位
                    memmove(childNode->keyBytes + self->keySize, childNode->keyBytes, childNode->keyCount * self->keySize);
                    memmove(childNode->valueBytes + self->valueSize, childNode->valueBytes, childNode->keyCount * self->valueSize);

                    // childNode 的子节点指针右移一位，左兄弟的最后一个子节点移到最左
                    if (!childNode->isLeafNode) {
                        memmove(childNode->childNodes + 1, childNode->childNodes, (childNode->keyCount + 1) * sizeof(BTreeNode*));
                        *(childNode->childNodes) = *(leftSiblingNode->childNodes + leftSiblingNode->keyCount);
                    }

                    // 父节点的 index-1 key-value 下移到 childNode 的最左
                    memcpy(childNode->keyBytes, parentNode->keyBytes + (index - 1) * self->keySize, self->keySize);
                    memcpy(childNode->valueBytes, parentNode->valueBytes + (index - 1) * self->valueSize, self->valueSize);

                    // 左兄弟的最后一个 key-value 上移到父节点的 index-1
                    memcpy(parentNode->keyBytes + (index - 1) * self->keySize, leftSiblingNode->keyBytes + (leftSiblingNode->keyCount - 1) * self->keySize, self->keySize);
                    memcpy(parentNode->valueBytes + (index - 1) * self->valueSize, leftSiblingNode->valueBytes + (leftSiblingNode->keyCount - 1) * self->valueSize, self->valueSize);

                    childNode->keyCount++;
                    leftSiblingNode->keyCount--;

                    return;
                }
            }

            // 尝试从右兄弟借位
            if (index < parentNode->keyCount) {
                auto* rightSiblingNode = *(parentNode->childNodes + index + 1);
                if (rightSiblingNode->keyCount > minKeyCount) {
                    // 父节点的 index key-value 追加到 childNode 末尾
                    memcpy(childNode->keyBytes + childNode->keyCount * self->keySize, parentNode->keyBytes + index * self->keySize, self->keySize);
                    memcpy(childNode->valueBytes + childNode->keyCount * self->valueSize, parentNode->valueBytes + index * self->valueSize, self->valueSize);

                    // 右兄弟的第一个 key-value 上移到父节点的 index
                    memcpy(parentNode->keyBytes + index * self->keySize, rightSiblingNode->keyBytes, self->keySize);
                    memcpy(parentNode->valueBytes + index * self->valueSize, rightSiblingNode->valueBytes, self->valueSize);

                    // 右兄弟的第一个子节点移到 childNode 末尾
                    if (!childNode->isLeafNode) {
                        *(childNode->childNodes + childNode->keyCount + 1) = *(rightSiblingNode->childNodes);
                        memmove(rightSiblingNode->childNodes, rightSiblingNode->childNodes + 1, rightSiblingNode->keyCount * sizeof(BTreeNode*));
                    }

                    // 右兄弟的 key-value 左移一位
                    memmove(rightSiblingNode->keyBytes, rightSiblingNode->keyBytes + self->keySize, (rightSiblingNode->keyCount - 1) * self->keySize);
                    memmove(rightSiblingNode->valueBytes, rightSiblingNode->valueBytes + self->valueSize, (rightSiblingNode->keyCount - 1) * self->valueSize);

                    childNode->keyCount++;
                    rightSiblingNode->keyCount--;

                    return;
                }
            }

            // 无法借位，合并
            if (index > 0) {
                // 把 childNode 合并到左兄弟
                mergeNodes(self, parentNode, index - 1);
            } else {
                // 把右兄弟合并到 childNode
                mergeNodes(self, parentNode, index);
            }
        }
        auto removeFromSubtree(const BTree* self, BTreeNode* node, const void* inKey, const usize minKeyCount) -> bool {
            usize index;
            const bool found = findKey(self, node, inKey, &index);

            if (found) {
                if (node->isLeafNode) {
                    // 叶子命中，直接删除
                    releaseSlot(self, node, index);
                    return true;
                }

                // 内部节点命中
                auto* leftChildNode = *(node->childNodes + index);
                auto* rightChildNode = *(node->childNodes + index + 1);

                if (leftChildNode->keyCount > minKeyCount) {
                    // 用前驱替换：找左子树的最右节点，即中序前驱
                    auto* predNode = leftChildNode;
                    while (!predNode->isLeafNode) {
                        predNode = *(predNode->childNodes + predNode->keyCount);
                    }

                    // 把前驱的键值复制到当前节点
                    memcpy(node->keyBytes + index * self->keySize, predNode->keyBytes + (predNode->keyCount - 1) * self->keySize, self->keySize);
                    memcpy(node->valueBytes + index * self->valueSize, predNode->valueBytes + (predNode->keyCount - 1) * self->valueSize, self->valueSize);

                    // 递归删除前驱：前驱已被复制到 node 中，从子树中删除原始前驱
                    // 递归删除的键现在是 node 中的键，它恰好等于前驱
                    return removeFromSubtree(self, leftChildNode, node->keyBytes + index * self->keySize, minKeyCount);
                }

                if (rightChildNode->keyCount > minKeyCount) {
                    // 用后继替换：找右子树的最左节点，即中序后继
                    auto* succNode = rightChildNode;
                    while (!succNode->isLeafNode) {
                        succNode = *(succNode->childNodes);
                    }

                    // 把后继的键值复制到当前节点
                    memcpy(node->keyBytes + index * self->keySize, succNode->keyBytes, self->keySize);
                    memcpy(node->valueBytes + index * self->valueSize, succNode->valueBytes, self->valueSize);

                    // 递归删除后继
                    return removeFromSubtree(self, rightChildNode, node->keyBytes + index * self->keySize, minKeyCount);
                }

                // 两个子节点都没有富余，合并
                mergeNodes(self, node, index);

                // 合并后目标键落入 leftChildNode
                return removeFromSubtree(self, leftChildNode, inKey, minKeyCount);
            }

            if (node->isLeafNode) {
                return false;
            }

            // 在下降之前，确保子节点有富余
            auto* childNode = *(node->childNodes + index);
            if (childNode->keyCount == minKeyCount) {
                fixChildUnderflow(self, node, index);

                // 合并可能改变 node 的结构，需要重新查找下降位置
                findKey(self, node, inKey, &index);
                childNode = *(node->childNodes + index);
            }

            return removeFromSubtree(self, childNode, inKey, minKeyCount);
        }
    }

    auto BTree_construct(const usize inOrder, const usize inKeySize, const usize inValueSize, i32 (*compare)(const void*, const void*)) -> BTree* {
        if (inOrder < 3 || inKeySize == 0 || inValueSize == 0 || !compare) {
            return nullptr;
        }

        auto* const self = (BTree*) malloc(sizeof(BTree));
        if (!self) {
            return nullptr;
        }

        self->rootNode = nullptr;
        self->order = inOrder;
        self->keySize = inKeySize;
        self->valueSize = inValueSize;
        self->size = 0;
        self->compare = compare;

        return self;
    }
    auto BTree_destruct(BTree* self) -> bool {
        if (!self) {
            return false;
        }

        destructNode(self, self->rootNode);

        free(self);

        return true;
    }

    auto BTree_copy(const BTree* self) -> BTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = BTree_construct(self->order, self->keySize, self->valueSize, self->compare);
        if (!newSelf) {
            return nullptr;
        }

        newSelf->rootNode = copyNode(newSelf, self->rootNode);
        if (self->rootNode && !newSelf->rootNode) {
            BTree_destruct(newSelf);
            return nullptr;
        }

        newSelf->size = self->size;

        return newSelf;
    }
    auto BTree_move(BTree* self) -> BTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BTree*) malloc(sizeof(BTree));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->rootNode = self->rootNode;
        newSelf->order = self->order;
        newSelf->keySize = self->keySize;
        newSelf->valueSize = self->valueSize;
        newSelf->size = self->size;
        newSelf->compare = self->compare;

        self->rootNode = nullptr;
        self->order = 0;
        self->keySize = 0;
        self->valueSize = 0;
        self->size = 0;
        self->compare = nullptr;

        return newSelf;
    }

    auto BTree_insert(BTree* self, const void* inKey, const void* inValue) -> bool {
        if (!self || !inKey || !inValue) {
            return false;
        }

        // 空树：创建根节点，直接插入
        if (!self->rootNode) {
            self->rootNode = createNode(self, true);
            if (!self->rootNode) {
                return false;
            }

            void* keySlot = nullptr;
            void* valueSlot = nullptr;

            // 在指定节点指定下标处腾出键值空位，并绑定给 keySlot 和 valueSlot
            reserveSlot(self, self->rootNode, 0, &keySlot, &valueSlot);

            memcpy(keySlot, inKey, self->keySize);
            memcpy(valueSlot, inValue, self->valueSize);

            self->size++;

            return true;
        }

        // 根满时先分裂根，树高加 1
        if (self->rootNode->keyCount == self->order - 1) {
            auto* const newRoot = createNode(self, false);
            if (!newRoot) {
                return false;
            }

            *(newRoot->childNodes) = self->rootNode;

            if (!splitNode(self, newRoot, 0)) {
                free(newRoot);
                return false;
            }

            self->rootNode = newRoot;
        }

        // 向下导航，遇到满子节点先预分裂
        auto* node = self->rootNode;
        while (!node->isLeafNode) {
            usize index;
            if (findKey(self, node, inKey, &index)) {
                // 命中当前节点的 key，替换 value
                memcpy(node->valueBytes + index * self->valueSize, inValue, self->valueSize);
                return true;
            }

            auto* childNode = *(node->childNodes + index);
            if (childNode->keyCount == self->order - 1) {
                if (!splitNode(self, node, index)) {
                    return false;
                }

                // 分裂后 childNode 的中间 key 上浮到 node 的 index 位置
                // 重新比较 inKey 与上浮的 key，决定去左半还是右半
                const i32 compareResult = self->compare(inKey, node->keyBytes + index * self->keySize);
                if (compareResult == 0) {
                    memcpy(node->valueBytes + index * self->valueSize, inValue, self->valueSize);
                    return true;
                }

                if (compareResult > 0) {
                    index++;
                }

                childNode = *(node->childNodes + index);
            }

            node = childNode;
        }

        // 到达叶子，此时叶子必定不满
        usize index;
        if (findKey(self, node, inKey, &index)) {
            // 命中叶子中的 key，替换 value
            memcpy(node->valueBytes + index * self->valueSize, inValue, self->valueSize);
            return true;
        }

        // 新键，在指定节点指定下标处腾出键值空位，并绑定给 keySlot 和 valueSlot
        void* keySlot = nullptr;
        void* valueSlot = nullptr;

        reserveSlot(self, node, index, &keySlot, &valueSlot);

        memcpy(keySlot, inKey, self->keySize);
        memcpy(valueSlot, inValue, self->valueSize);

        self->size++;

        return true;
    }
    auto BTree_insertSlot(BTree* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !oldValueSlotOut || !newValueSlotOut) {
            return false;
        }

        // keySlotOut 为空表示键已存在，不需要构造新键
        // oldValueSlotOut 为空表示键是新的，没有旧值需要析构
        // newValueSlotOut 必然非空（成功时），是调用者要构造的新值
        *keySlotOut = nullptr;
        *oldValueSlotOut = nullptr;
        *newValueSlotOut = nullptr;

        // 空树：创建根节点，直接预留槽位
        if (!self->rootNode) {
            self->rootNode = createNode(self, true);
            if (!self->rootNode) {
                return false;
            }

            reserveSlot(self, self->rootNode, 0, keySlotOut, newValueSlotOut);

            self->size++;

            return true;
        }

        // 根满时先分裂根，树高加 1
        if (self->rootNode->keyCount == self->order - 1) {
            auto* const newRoot = createNode(self, false);
            if (!newRoot) {
                return false;
            }

            *(newRoot->childNodes) = self->rootNode;

            if (!splitNode(self, newRoot, 0)) {
                free(newRoot);
                return false;
            }

            self->rootNode = newRoot;
        }

        // 向下导航，遇到满子节点先预分裂
        auto* node = self->rootNode;
        while (!node->isLeafNode) {
            usize index;
            if (findKey(self, node, inKey, &index)) {
                // 命中当前节点的 key，返回旧值与新值槽位（同一位置）
                *oldValueSlotOut = node->valueBytes + index * self->valueSize;
                *newValueSlotOut = node->valueBytes + index * self->valueSize;
                return true;
            }

            auto* childNode = *(node->childNodes + index);
            if (childNode->keyCount == self->order - 1) {
                if (!splitNode(self, node, index)) {
                    return false;
                }

                // 分裂后 childNode 的中间 key 上浮到 node 的 index 位置
                const i32 compareResult = self->compare(inKey, node->keyBytes + index * self->keySize);
                if (compareResult == 0) {
                    *oldValueSlotOut = node->valueBytes + index * self->valueSize;
                    *newValueSlotOut = node->valueBytes + index * self->valueSize;
                    return true;
                }

                if (compareResult > 0) {
                    index++;
                }

                childNode = *(node->childNodes + index);
            }

            node = childNode;
        }

        // 到达叶子，此时叶子必定不满
        usize index;
        if (findKey(self, node, inKey, &index)) {
            // 命中叶子中的 key，返回旧值与新值槽位（同一位置）
            *oldValueSlotOut = node->valueBytes + index * self->valueSize;
            *newValueSlotOut = node->valueBytes + index * self->valueSize;
            return true;
        }

        // 新键，腾出空间并返回槽位
        reserveSlot(self, node, index, keySlotOut, newValueSlotOut);

        self->size++;

        return true;
    }

    auto BTree_remove(BTree* self, const void* inKey) -> bool {
        if (!self || !inKey || !self->rootNode) {
            return false;
        }

        const usize minKeyCount = (self->order - 1) / 2;

        if (!removeFromSubtree(self, self->rootNode, inKey, minKeyCount)) {
            return false;
        }

        self->size--;

        // 根变空时降低树高或清空
        if (self->rootNode->keyCount == 0) {
            if (self->rootNode->isLeafNode) {
                free(self->rootNode);
                self->rootNode = nullptr;
            } else {
                auto* oldRoot = self->rootNode;
                self->rootNode = *(oldRoot->childNodes);
                free(oldRoot);
            }
        }

        return true;
    }
    auto BTree_removeSlot(BTree* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !valueSlotOut || !self->rootNode) {
            return false;
        }

        *keySlotOut = nullptr;
        *valueSlotOut = nullptr;

        // 先在树中查找目标 key
        auto* node = self->rootNode;
        while (true) {
            usize index;
            if (findKey(self, node, inKey, &index)) {
                // 找到目标，把 key-value 复制到新分配内存
                void* keyCopy = malloc(self->keySize);
                if (!keyCopy) {
                    return false;
                }
                memcpy(keyCopy, node->keyBytes + index * self->keySize, self->keySize);

                void* valueCopy = malloc(self->valueSize);
                if (!valueCopy) {
                    free(keyCopy);
                    return false;
                }
                memcpy(valueCopy, node->valueBytes + index * self->valueSize, self->valueSize);

                // 从树中删除目标 key
                if (!BTree_remove(self, inKey)) {
                    free(keyCopy);
                    free(valueCopy);
                    return false;
                }

                *keySlotOut = keyCopy;
                *valueSlotOut = valueCopy;

                return true;
            }

            if (node->isLeafNode) {
                return false;
            }

            node = *(node->childNodes + index);
        }
    }
    auto BTree_clear(BTree* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        destructNode(self, self->rootNode);

        self->rootNode = nullptr;
        self->size = 0;

        return true;
    }

    auto BTree_find(const BTree* self, const void* inKey) -> void* {
        if (!self || !inKey || !self->rootNode) {
            return nullptr;
        }

        auto* node = self->rootNode;
        while (true) {
            usize index;
            if (findKey(self, node, inKey, &index)) {
                return node->valueBytes + index * self->valueSize;
            }

            if (node->isLeafNode) {
                return nullptr;
            }

            node = *(node->childNodes + index);
        }
    }
    auto BTree_findSlot(const BTree* self, const void* inKey, BTreeNode** nodeOut, usize* indexOut) -> bool {
        if (!self || !inKey || !nodeOut || !indexOut || !self->rootNode) {
            return false;
        }

        auto* node = self->rootNode;
        while (true) {
            usize index;
            if (findKey(self, node, inKey, &index)) {
                *nodeOut = node;
                *indexOut = index;
                return true;
            }

            if (node->isLeafNode) {
                return false;
            }

            node = *(node->childNodes + index);
        }
    }
    auto BTree_findSuccessorSlot(const BTree* self, const void* inKey, BTreeNode** nodeOut, usize* indexOut) -> bool {
        if (!self || !inKey || !nodeOut || !indexOut || !self->rootNode) {
            return false;
        }

        // 记录比 inKey 大的最小槽位候选
        // 下降过程中每次未命中且 index < keyCount时，当前节点的 index 槽位都可能是候选
        auto* successorNode = (BTreeNode*) nullptr;
        usize successorIndex = 0;

        auto* node = self->rootNode;
        while (true) {
            usize index;
            const bool found = findKey(self, node, inKey, &index);

            if (found) {
                // 命中当前节点的 key
                if (!node->isLeafNode) {
                    // 后继是右子树的最小槽位：一路向左到叶子，取第 0 个 key
                    auto* successor = *(node->childNodes + index + 1);
                    while (!successor->isLeafNode) {
                        successor = *(successor->childNodes);
                    }

                    *nodeOut = successor;
                    *indexOut = 0;
                    return true;
                }

                // 叶子命中：后继是同一节点的下一槽位，或是最深候选
                if (index + 1 < node->keyCount) {
                    *nodeOut = node;
                    *indexOut = index + 1;
                    return true;
                }

                if (successorNode) {
                    *nodeOut = successorNode;
                    *indexOut = successorIndex;
                    return true;
                }

                return false;
            }

            // 未命中：index < keyCount 时，当前节点的 index 槽位是候选
            if (index < node->keyCount) {
                successorNode = node;
                successorIndex = index;
            }

            if (node->isLeafNode) {
                if (successorNode) {
                    *nodeOut = successorNode;
                    *indexOut = successorIndex;
                    return true;
                }

                return false;
            }

            node = *(node->childNodes + index);
        }
    }
    auto BTree_findPredecessorSlot(const BTree* self, const void* inKey, BTreeNode** nodeOut, usize* indexOut) -> bool {
        if (!self || !inKey || !nodeOut || !indexOut || !self->rootNode) {
            return false;
        }

        // 记录比 inKey 小的最大槽位候选
        // 下降过程中每次未命中且 index > 0时，当前节点的 index-1 槽位都可能是候选
        auto* predecessorNode = (BTreeNode*) nullptr;
        usize predecessorIndex = 0;

        auto* node = self->rootNode;
        while (true) {
            usize index;
            const bool found = findKey(self, node, inKey, &index);

            if (found) {
                // 命中当前节点的 key
                if (!node->isLeafNode) {
                    // 前驱是左子树的最大槽位：一路向右到叶子，取最后一个 key
                    auto* predecessor = *(node->childNodes + index);
                    while (!predecessor->isLeafNode) {
                        predecessor = *(predecessor->childNodes + predecessor->keyCount);
                    }

                    *nodeOut = predecessor;
                    *indexOut = predecessor->keyCount - 1;
                    return true;
                }

                // 叶子命中：前驱是同一节点的上一槽位，或是最深候选
                if (index > 0) {
                    *nodeOut = node;
                    *indexOut = index - 1;
                    return true;
                }

                if (predecessorNode) {
                    *nodeOut = predecessorNode;
                    *indexOut = predecessorIndex;
                    return true;
                }

                return false;
            }

            // 未命中：index > 0 时，当前节点的 index-1 槽位是候选
            if (index > 0) {
                predecessorNode = node;
                predecessorIndex = index - 1;
            }

            if (node->isLeafNode) {
                if (predecessorNode) {
                    *nodeOut = predecessorNode;
                    *indexOut = predecessorIndex;
                    return true;
                }

                return false;
            }

            node = *(node->childNodes + index);
        }
    }

    auto BTree_rootNode(const BTree* self) -> BTreeNode* {
        if (!self) {
            return nullptr;
        }

        return self->rootNode;
    }
    auto BTree_minSlot(const BTree* self, BTreeNode** nodeOut, usize* indexOut) -> bool {
        if (!self || !nodeOut || !indexOut || !self->rootNode) {
            return false;
        }

        // 一路向左走到最左叶子，最小值是该叶子的第 0 个 key
        auto* node = self->rootNode;
        while (!node->isLeafNode) {
            node = *(node->childNodes);
        }

        *nodeOut = node;
        *indexOut = 0;

        return true;
    }
    auto BTree_maxSlot(const BTree* self, BTreeNode** nodeOut, usize* indexOut) -> bool {
        if (!self || !nodeOut || !indexOut || !self->rootNode) {
            return false;
        }

        // 一路向右走到最右叶子，最大值是该叶子的最后一个 key
        auto* node = self->rootNode;
        while (!node->isLeafNode) {
            node = *(node->childNodes + node->keyCount);
        }

        *nodeOut = node;
        *indexOut = node->keyCount - 1;

        return true;
    }

    auto BTree_order(const BTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->order;
    }
    auto BTree_keySize(const BTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->keySize;
    }
    auto BTree_valueSize(const BTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->valueSize;
    }
    auto BTree_size(const BTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }

    auto BTree_isEmpty(const BTree* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }
    auto BTree_contains(const BTree* self, const void* inKey) -> bool {
        if (!self || !inKey || !self->rootNode) {
            return false;
        }

        auto* node = self->rootNode;
        while (true) {
            usize index;
            if (findKey(self, node, inKey, &index)) {
                return true;
            }

            if (node->isLeafNode) {
                return false;
            }

            node = *(node->childNodes + index);
        }
    }

    auto BTreeNode_key(const BTreeNode* node, const usize index, const usize inKeySize) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->keyBytes + index * inKeySize;
    }
    auto BTreeNode_value(const BTreeNode* node, const usize index, const usize inValueSize) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->valueBytes + index * inValueSize;
    }
    auto BTreeNode_child(const BTreeNode* node, const usize index) -> BTreeNode* {
        if (!node) {
            return nullptr;
        }

        return *(node->childNodes + index);
    }
    auto BTreeNode_keyCount(const BTreeNode* node) -> usize {
        if (!node) {
            return 0;
        }

        return node->keyCount;
    }
    auto BTreeNode_isLeaf(const BTreeNode* node) -> bool {
        if (!node) {
            return true;
        }

        return node->isLeafNode;
    }
}