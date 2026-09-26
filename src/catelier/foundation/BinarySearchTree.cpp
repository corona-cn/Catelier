#include "BinarySearchTree.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    typedef struct BinarySearchTreeNode {
        void* key;
        void* value;
        BinarySearchTreeNode* leftChild;
        BinarySearchTreeNode* rightChild;
    } BinarySearchTreeNode;

    typedef struct BinarySearchTree {
        BinarySearchTreeNode* rootNode;
        usize keySize;
        usize valueSize;
        usize size;
        i32 (*compare)(const void*, const void*);
    } BinarySearchTree;

    namespace {
        auto destructNode(const BinarySearchTree* self, BinarySearchTreeNode* subtreeRoot) -> void {
            if (!subtreeRoot) {
                return;
            }

            // 递归破坏所有子树，释放节点键值内存后，释放节点本身内存
            destructNode(self, subtreeRoot->leftChild);
            destructNode(self, subtreeRoot->rightChild);

            if (subtreeRoot->key) {
                free(subtreeRoot->key);
            }

            if (subtreeRoot->value) {
                free(subtreeRoot->value);
            }

            free(subtreeRoot);
        }
        auto copyNode(const BinarySearchTree* self, const BinarySearchTreeNode* sourceNode) -> BinarySearchTreeNode* {
            if (!sourceNode) {
                return nullptr;
            }

            // 递归复制左右子树
            auto* const newLeft = copyNode(self, sourceNode->leftChild);
            if (sourceNode->leftChild && !newLeft) {
                return nullptr;
            }

            auto* const newRight = copyNode(self, sourceNode->rightChild);
            if (sourceNode->rightChild && !newRight) {
                destructNode(self, newLeft);
                return nullptr;
            }

            auto* const newNode = (BinarySearchTreeNode*) malloc(sizeof(BinarySearchTreeNode));
            if (!newNode) {
                destructNode(self, newLeft);
                destructNode(self, newRight);
                return nullptr;
            }

            // 分配并复制键内存
            newNode->key = malloc(self->keySize);
            if (!newNode->key) {
                free(newNode);
                destructNode(self, newLeft);
                destructNode(self, newRight);
                return nullptr;
            }
            memcpy(newNode->key, sourceNode->key, self->keySize);

            // 分配并复制值内存
            newNode->value = malloc(self->valueSize);
            if (!newNode->value) {
                free(newNode->key);
                free(newNode);
                destructNode(self, newLeft);
                destructNode(self, newRight);
                return nullptr;
            }
            memcpy(newNode->value, sourceNode->value, self->valueSize);

            newNode->leftChild = newLeft;
            newNode->rightChild = newRight;

            return newNode;
        }
    }

    auto BinarySearchTree_construct(const usize inKeySize, const usize inValueSize, i32 (*compare)(const void*, const void*)) -> BinarySearchTree* {
        if (inKeySize == 0 || inValueSize == 0 || !compare) {
            return nullptr;
        }

        auto* const self = (BinarySearchTree*) malloc(sizeof(BinarySearchTree));
        if (!self) {
            return nullptr;
        }

        // 二叉搜索树按需分配，无需预分配
        self->rootNode = nullptr;
        self->keySize = inKeySize;
        self->valueSize = inValueSize;
        self->size = 0;
        self->compare = compare;

        return self;
    }
    auto BinarySearchTree_destruct(BinarySearchTree* self) -> bool {
        if (!self) {
            return false;
        }

        destructNode(self, self->rootNode);
        free(self);

        return true;
    }

    auto BinarySearchTree_copy(const BinarySearchTree* self) -> BinarySearchTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = BinarySearchTree_construct(self->keySize, self->valueSize, self->compare);
        if (!newSelf) {
            return nullptr;
        }

        // 递归复制整棵树
        newSelf->rootNode = copyNode(newSelf, self->rootNode);
        if (self->rootNode && !newSelf->rootNode) {
            BinarySearchTree_destruct(newSelf);
            return nullptr;
        }

        newSelf->size = self->size;

        return newSelf;
    }
    auto BinarySearchTree_move(BinarySearchTree* self) -> BinarySearchTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BinarySearchTree*) malloc(sizeof(BinarySearchTree));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->rootNode = self->rootNode;
        newSelf->keySize = self->keySize;
        newSelf->valueSize = self->valueSize;
        newSelf->size = self->size;
        newSelf->compare = self->compare;

        self->rootNode = nullptr;
        self->keySize = 0;
        self->valueSize = 0;
        self->size = 0;
        self->compare = nullptr;

        return newSelf;
    }

    // 根据 compare 函数决定插入新节点的位置
    // 分根节点和非根节点两种情况
    // 根节点直接分配，比较简单
    // 非根节点则需要根据 compare 函数结果分左右子节点位置，保证整体序
    auto BinarySearchTree_insert(BinarySearchTree* self, const void* inKey, const void* inValue) -> bool {
        if (!self || !inKey || !inValue) {
            return false;
        }

        // 空树情况：新节点直接作为根节点
        if (self->rootNode == nullptr) {
            auto* const newNode = (BinarySearchTreeNode*) malloc(sizeof(BinarySearchTreeNode));
            if (!newNode) {
                return false;
            }

            newNode->key = malloc(self->keySize);
            if (!newNode->key) {
                free(newNode);
                return false;
            }
            memcpy(newNode->key, inKey, self->keySize);

            newNode->value = malloc(self->valueSize);
            if (!newNode->value) {
                free(newNode->key);
                free(newNode);
                return false;
            }
            memcpy(newNode->value, inValue, self->valueSize);

            newNode->leftChild = nullptr;
            newNode->rightChild = nullptr;

            self->rootNode = newNode;

            self->size++;

            return true;
        }

        // 非空树情况：从根节点出发，逐级比较导航
        auto* currentNode = self->rootNode;
        while (true) {
            const i32 compareResult = self->compare(inKey, currentNode->key);

            // 键已存在情况，先分配新值，再释放旧值，保证新值内存分配失败时旧值不被破坏
            if (compareResult == 0) {
                void* const newValue = malloc(self->valueSize);
                if (!newValue) {
                    return false;
                }

                if (currentNode->value) {
                    free(currentNode->value);
                }

                currentNode->value = newValue;

                memcpy(currentNode->value, inValue, self->valueSize);

                return true;
            }

            // 键不存在情况，根据比较结果选择左或右子指针，比较结果小于 0 为左子节点，比较结果大于 0 为右子节点
            // 二级指针，指向子节点指针（子节点指针可空，即存在可挂节点的空位）
            auto** const childPtr = (compareResult < 0) ? &currentNode->leftChild : &currentNode->rightChild;

            // 找到空位，创建新节点挂上
            if (*childPtr == nullptr) {
                auto* const newNode = (BinarySearchTreeNode*) malloc(sizeof(BinarySearchTreeNode));
                if (!newNode) {
                    return false;
                }

                newNode->key = malloc(self->keySize);
                if (!newNode->key) {
                    free(newNode);
                    return false;
                }
                memcpy(newNode->key, inKey, self->keySize);

                newNode->value = malloc(self->valueSize);
                if (!newNode->value) {
                    free(newNode->key);
                    free(newNode);
                    return false;
                }
                memcpy(newNode->value, inValue, self->valueSize);

                newNode->leftChild = nullptr;
                newNode->rightChild = nullptr;

                *childPtr = newNode;

                self->size++;

                return true;
            }

            // 继续向下一层导航
            currentNode = *childPtr;
        }
    }
    auto BinarySearchTree_insertSlot(BinarySearchTree* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !oldValueSlotOut || !newValueSlotOut) {
            return false;
        }

        // keySlotOut 为空表示键已存在，不需要构造新键
        // oldValueSlotOut 为空表示键是新的，没有旧值需要析构
        // newValueSlotOut 必然非空（成功时），是调用者要构造的新值
        *keySlotOut = nullptr;
        *oldValueSlotOut = nullptr;
        *newValueSlotOut = nullptr;

        // 空树情况：新节点直接作为根节点
        if (self->rootNode == nullptr) {
            auto* const newNode = (BinarySearchTreeNode*) malloc(sizeof(BinarySearchTreeNode));
            if (!newNode) {
                return false;
            }

            newNode->key = malloc(self->keySize);
            if (!newNode->key) {
                free(newNode);
                return false;
            }

            newNode->value = malloc(self->valueSize);
            if (!newNode->value) {
                free(newNode->key);
                free(newNode);
                return false;
            }

            newNode->leftChild = nullptr;
            newNode->rightChild = nullptr;

            self->rootNode = newNode;
            self->size++;

            // 此时键值都是全新的，都需要调用者构造
            *keySlotOut = newNode->key;
            *newValueSlotOut = newNode->value;

            return true;
        }

        // 非空树情况：从根节点出发，逐级比较导航
        auto* currentNode = self->rootNode;
        while (true) {
            const i32 compareResult = self->compare(inKey, currentNode->key);

            // 键已存在情况，分配新值内存
            // 先分配后交接，保证内存分配失败时旧值不被破坏
            if (compareResult == 0) {
                void* const newValue = malloc(self->valueSize);
                if (!newValue) {
                    return false;
                }

                // 保存旧值指针，交给调用者析构并释放
                *oldValueSlotOut = currentNode->value;

                currentNode->value = newValue;

                // 键已存在，不需要重新构造键；调用者只需构造新值
                *newValueSlotOut = newValue;

                return true;
            }

            // 键不存在情况，根据比较结果选择左或右子指针，比较结果小于 0 为左子节点，比较结果大于 0 为右子节点
            // 二级指针，指向子节点指针（子节点指针可空，即存在可挂节点的空位）
            auto** const childPtr = (compareResult < 0) ? &currentNode->leftChild : &currentNode->rightChild;

            // 找到空位，创建新节点挂上
            if (*childPtr == nullptr) {
                auto* const newNode = (BinarySearchTreeNode*) malloc(sizeof(BinarySearchTreeNode));
                if (!newNode) {
                    return false;
                }

                newNode->key = malloc(self->keySize);
                if (!newNode->key) {
                    free(newNode);
                    return false;
                }

                newNode->value = malloc(self->valueSize);
                if (!newNode->value) {
                    free(newNode->key);
                    free(newNode);
                    return false;
                }

                newNode->leftChild = nullptr;
                newNode->rightChild = nullptr;

                *childPtr = newNode;

                self->size++;

                // 键值都是全新的，都需要调用者构造
                *keySlotOut = newNode->key;
                *newValueSlotOut = newNode->value;

                return true;
            }

            // 继续向下一层导航
            currentNode = *childPtr;
        }
    }

    auto BinarySearchTree_remove(BinarySearchTree* self, const void* inKey) -> bool {
        if (!self || !inKey) {
            return false;
        }

        // 从根节点出发，逐级比较导航，找到目标节点及其父节点
        auto* parentNode = (BinarySearchTreeNode*) nullptr;
        auto* currentNode = self->rootNode;
        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);
            if (compareResult == 0) {
                break;
            }

            parentNode = currentNode;

            currentNode = (compareResult < 0) ? currentNode->leftChild : currentNode->rightChild;
        }

        // 未找到目标节点
        if (!currentNode) {
            return false;
        }

        // 找到目标节点，分不同子节点情况讨论
        if (currentNode->leftChild == nullptr) {
            // 没有左子节点情况，直接用右子节点替代当前节点
            if (parentNode == nullptr) {
                self->rootNode = currentNode->rightChild;
            } else if (parentNode->leftChild == currentNode) {
                parentNode->leftChild = currentNode->rightChild;
            } else {
                parentNode->rightChild = currentNode->rightChild;
            }
        } else if (currentNode->rightChild == nullptr) {
            // 没有右子节点情况，直接用左子节点替代当前节点
            if (parentNode == nullptr) {
                self->rootNode = currentNode->leftChild;
            } else if (parentNode->leftChild == currentNode) {
                parentNode->leftChild = currentNode->leftChild;
            } else {
                parentNode->rightChild = currentNode->leftChild;
            }
        } else {
            // 有两个子节点情况（参考 CLRS 的 TREE-DELETE 实现，区别在于当前实现中节点没有父指针，需要显式追踪父节点）
            // 找中序后继节点，即找右子树的最左节点（二叉搜索树情况左小右大，左 → 根 → 右），并用其来替代当前节点
            auto* successorParent = currentNode;
            auto* successorNode = currentNode->rightChild;

            // 一路向左找中序后继节点
            while (successorNode->leftChild) {
                successorParent = successorNode;
                successorNode = successorNode->leftChild;
            }

            // 中序后继节点不是当前节点的直接右子节点时
            // 先把中序后继节点的右子树挂到中序后继父节点的左子节点上
            // 再把当前节点的右子树接到中序后继节点上
            // 整体即摘除中序后继节点
            if (successorParent != currentNode) {
                if (successorParent->leftChild == successorNode) {
                    successorParent->leftChild = successorNode->rightChild;
                } else {
                    successorParent->rightChild = successorNode->rightChild;
                }
                successorNode->rightChild = currentNode->rightChild;
            }

            // 用中序后继节点替代当前节点
            if (parentNode == nullptr) {
                self->rootNode = successorNode;
            } else if (parentNode->leftChild == currentNode) {
                parentNode->leftChild = successorNode;
            } else {
                parentNode->rightChild = successorNode;
            }

            // 让中序后继节点接管被删节点的左子树
            successorNode->leftChild = currentNode->leftChild;
        }

        // 释放当前节点的键值对，最后释放节点本身
        if (currentNode->key) {
            free(currentNode->key);
        }

        if (currentNode->value) {
            free(currentNode->value);
        }

        free(currentNode);

        self->size--;

        return true;
    }
    auto BinarySearchTree_removeSlot(BinarySearchTree* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !valueSlotOut) {
            return false;
        }

        *keySlotOut = nullptr;
        *valueSlotOut = nullptr;

        // 从根节点出发，逐级比较导航，找到目标节点及其父节点
        auto* parentNode = (BinarySearchTreeNode*) nullptr;
        auto* currentNode = self->rootNode;
        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);
            if (compareResult == 0) {
                break;
            }

            parentNode = currentNode;
            currentNode = (compareResult < 0) ? currentNode->leftChild : currentNode->rightChild;
        }

        // 未找到目标节点
        if (!currentNode) {
            return false;
        }

        // 保存目标节点的键值指针，交给调用者析构并释放
        *keySlotOut = currentNode->key;
        *valueSlotOut = currentNode->value;

        // 找到目标节点，分不同子节点情况讨论
        if (currentNode->leftChild == nullptr) {
            // 没有左子节点情况，直接用右子节点替代当前节点
            if (parentNode == nullptr) {
                self->rootNode = currentNode->rightChild;
            } else if (parentNode->leftChild == currentNode) {
                parentNode->leftChild = currentNode->rightChild;
            } else {
                parentNode->rightChild = currentNode->rightChild;
            }
        } else if (currentNode->rightChild == nullptr) {
            // 没有右子节点情况，直接用左子节点替代当前节点
            if (parentNode == nullptr) {
                self->rootNode = currentNode->leftChild;
            } else if (parentNode->leftChild == currentNode) {
                parentNode->leftChild = currentNode->leftChild;
            } else {
                parentNode->rightChild = currentNode->leftChild;
            }
        } else {
            // 有两个子节点情况（参考 CLRS 的 TREE-DELETE 实现，区别在于当前实现中节点没有父指针，需要显式追踪父节点）
            // 找中序后继节点，即找右子树的最左节点（二叉搜索树情况左小右大，左 → 根 → 右），并用其来替代当前节点
            auto* successorParent = currentNode;
            auto* successorNode = currentNode->rightChild;

            // 一路向左找中序后继节点
            while (successorNode->leftChild) {
                successorParent = successorNode;
                successorNode = successorNode->leftChild;
            }

            // 中序后继节点不是当前节点的直接右子节点时
            // 先把中序后继节点的右子树挂到中序后继父节点的左子节点上
            // 再把当前节点的右子树接到中序后继节点上
            // 整体即摘除中序后继节点
            if (successorParent != currentNode) {
                if (successorParent->leftChild == successorNode) {
                    successorParent->leftChild = successorNode->rightChild;
                } else {
                    successorParent->rightChild = successorNode->rightChild;
                }
                successorNode->rightChild = currentNode->rightChild;
            }

            // 用中序后继节点替代当前节点
            if (parentNode == nullptr) {
                self->rootNode = successorNode;
            } else if (parentNode->leftChild == currentNode) {
                parentNode->leftChild = successorNode;
            } else {
                parentNode->rightChild = successorNode;
            }

            // 让中序后继节点接管被删节点的左子树
            successorNode->leftChild = currentNode->leftChild;
        }

        // 释放节点本身，但不释放键值内存
        free(currentNode);

        self->size--;

        return true;
    }
    auto BinarySearchTree_clear(BinarySearchTree* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 递归释放整棵子树
        destructNode(self, self->rootNode);

        self->rootNode = nullptr;
        self->size = 0;

        return true;
    }

    auto BinarySearchTree_find(const BinarySearchTree* self, const void* inKey) -> void* {
        if (!self || !inKey) {
            return nullptr;
        }

        // 从根节点出发，逐级比较导航
        auto* currentNode = self->rootNode;
        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);

            // 比较结果命中：返回对应值的指针
            if (compareResult == 0) {
                return currentNode->value;
            }

            // 比较结果未命中：根据比较结果选择左或右子树继续导航
            currentNode = (compareResult < 0) ? currentNode->leftChild : currentNode->rightChild;
        }

        // 走到空位比较结果仍未命中，说明键不存在
        return nullptr;
    }
    auto BinarySearchTree_findNode(const BinarySearchTree* self, const void* inKey) -> BinarySearchTreeNode* {
        if (!self || !inKey) {
            return nullptr;
        }

        // 从根节点出发，逐级比较导航
        auto* currentNode = self->rootNode;
        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);

            // 比较结果命中：返回对应节点的指针
            if (compareResult == 0) {
                return currentNode;
            }

            // 比较结果未命中：根据比较结果选择左或右子树继续导航
            currentNode = (compareResult < 0) ? currentNode->leftChild : currentNode->rightChild;
        }

        // 走到空位比较结果仍未命中，说明键不存在
        return nullptr;
    }
    auto BinarySearchTree_findSuccessorNode(const BinarySearchTree* self, const void* inKey) -> BinarySearchTreeNode* {
        if (!self || !inKey) {
            return nullptr;
        }

        // 记录比 inKey 大的最小节点的候选
        // 导航过程中每次往左走时，当前节点都可能是候选
        auto* successor = (BinarySearchTreeNode*) nullptr;
        auto* currentNode = self->rootNode;

        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);

            if (compareResult < 0) {
                // inKey < currentNode：currentNode 可能是后继，记录后往左走
                successor = currentNode;
                currentNode = currentNode->leftChild;
            } else if (compareResult > 0) {
                // inKey > currentNode：currentNode 太小，往右走
                currentNode = currentNode->rightChild;
            } else {
                // inKey == currentNode：命中，后继在右子树的最左节点
                // 有右子树：后继是右子树的最小节点
                if (currentNode->rightChild) {
                    auto* node = currentNode->rightChild;
                    while (node->leftChild) {
                        node = node->leftChild;
                    }

                    return node;
                }

                // 没有右子树：后继就是导航过程中记录的最深候选
                return successor;
            }
        }

        // 走到空位仍未命中，返回导航过程中记录的候选
        // 若候选仍为空，说明 inKey 大于树中所有 key
        return successor;
    }
    auto BinarySearchTree_findPredecessorNode(const BinarySearchTree* self, const void* inKey) -> BinarySearchTreeNode* {
        if (!self || !inKey) {
            return nullptr;
        }

        // 记录比 inKey 小的最大节点的候选
        // 导航过程中每次往右走时，当前节点都可能是候选
        auto* predecessor = (BinarySearchTreeNode*) nullptr;
        auto* currentNode = self->rootNode;

        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);

            if (compareResult > 0) {
                // inKey > currentNode：currentNode 可能是前驱，记录后往右走
                predecessor = currentNode;
                currentNode = currentNode->rightChild;
            } else if (compareResult < 0) {
                // inKey < currentNode：currentNode 太大，往左走
                currentNode = currentNode->leftChild;
            } else {
                // inKey == currentNode：命中，前驱在左子树的最右节点
                // 有左子树：前驱是左子树的最大节点
                if (currentNode->leftChild) {
                    auto* node = currentNode->leftChild;
                    while (node->rightChild) {
                        node = node->rightChild;
                    }

                    return node;
                }

                // 没有左子树：前驱就是导航过程中记录的最深候选
                return predecessor;
            }
        }

        // 走到空位仍未命中，返回导航过程中记录的候选
        // 若候选仍为空，说明 inKey 小于树中所有 key
        return predecessor;
    }

    auto BinarySearchTree_rootNode(const BinarySearchTree* self) -> BinarySearchTreeNode* {
        if (!self) {
            return nullptr;
        }

        return self->rootNode;
    }
    auto BinarySearchTree_minNode(const BinarySearchTree* self) -> BinarySearchTreeNode* {
        if (!self || !self->rootNode) {
            return nullptr;
        }

        // 一路向左走到最左节点，即最小值所在节点
        auto* currentNode = self->rootNode;
        while (currentNode->leftChild) {
            currentNode = currentNode->leftChild;
        }

        return currentNode;
    }
    auto BinarySearchTree_maxNode(const BinarySearchTree* self) -> BinarySearchTreeNode* {
        if (!self || !self->rootNode) {
            return nullptr;
        }

        // 一路向右走到最右节点，即最大值所在节点
        auto* currentNode = self->rootNode;
        while (currentNode->rightChild) {
            currentNode = currentNode->rightChild;
        }

        return currentNode;
    }

    auto BinarySearchTree_keySize(const BinarySearchTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->keySize;
    }
    auto BinarySearchTree_valueSize(const BinarySearchTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->valueSize;
    }
    auto BinarySearchTree_size(const BinarySearchTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }

    auto BinarySearchTree_isEmpty(const BinarySearchTree* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }
    auto BinarySearchTree_contains(const BinarySearchTree* self, const void* inKey) -> bool {
        return BinarySearchTree_findNode(self, inKey) != nullptr;
    }

    auto BinarySearchTreeNode_key(const BinarySearchTreeNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->key;
    }
    auto BinarySearchTreeNode_value(const BinarySearchTreeNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->value;
    }
    auto BinarySearchTreeNode_left(const BinarySearchTreeNode* node) -> BinarySearchTreeNode* {
        if (!node) {
            return nullptr;
        }

        return node->leftChild;
    }
    auto BinarySearchTreeNode_right(const BinarySearchTreeNode* node) -> BinarySearchTreeNode* {
        if (!node) {
            return nullptr;
        }

        return node->rightChild;
    }
}