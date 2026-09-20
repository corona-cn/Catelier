#include "AVLTree.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    namespace {
        constexpr i32 EMPTY_NODE_HEIGHT = -1;
    }

    typedef struct AVLTreeNode {
        void* key;
        void* value;
        AVLTreeNode* leftChild;
        AVLTreeNode* rightChild;
        i32 height;
    } AVLTreeNode;

    typedef struct AVLTree {
        AVLTreeNode* rootNode;
        usize keySize;
        usize valueSize;
        usize size;
        i32 (*compare)(const void*, const void*);
    } AVLTree;

    namespace {
        auto destructSubtree(AVLTreeNode* node) -> void {
            if (!node) {
                return;
            }

            destructSubtree(node->leftChild);
            destructSubtree(node->rightChild);

            if (node->key) {
                free(node->key);
            }

            if (node->value) {
                free(node->value);
            }

            free(node);
        }
        auto copySubtree(const AVLTree* self, const AVLTreeNode* node) -> AVLTreeNode* {
            if (!node) {
                return nullptr;
            }

            // 递归复制左右子树
            auto* const newLeft = copySubtree(self, node->leftChild);
            if (node->leftChild && !newLeft) {
                return nullptr;
            }

            auto* const newRight = copySubtree(self, node->rightChild);
            if (node->rightChild && !newRight) {
                destructSubtree(newLeft);
                return nullptr;
            }

            auto* const newNode = (AVLTreeNode*) malloc(sizeof(AVLTreeNode));
            if (!newNode) {
                destructSubtree(newLeft);
                destructSubtree(newRight);
                return nullptr;
            }

            // 分配并复制键内存
            newNode->key = malloc(self->keySize);
            if (!newNode->key) {
                free(newNode);
                destructSubtree(newLeft);
                destructSubtree(newRight);
                return nullptr;
            }
            memcpy(newNode->key, node->key, self->keySize);

            // 分配并复制值内存
            newNode->value = malloc(self->valueSize);
            if (!newNode->value) {
                free(newNode->key);
                free(newNode);
                destructSubtree(newLeft);
                destructSubtree(newRight);
                return nullptr;
            }
            memcpy(newNode->value, node->value, self->valueSize);

            newNode->leftChild = newLeft;
            newNode->rightChild = newRight;

            // 复制高度字段，保持与原树一致
            newNode->height = node->height;

            return newNode;
        }

        auto createEmptyNode(const AVLTree* self) -> AVLTreeNode* {
            auto* const newNode = (AVLTreeNode*) malloc(sizeof(AVLTreeNode));
            if (!newNode) {
                return nullptr;
            }

            newNode->key = malloc(self->keySize);
            if (!newNode->key) {
                free(newNode);
                return nullptr;
            }

            newNode->value = malloc(self->valueSize);
            if (!newNode->value) {
                free(newNode->key);
                free(newNode);
                return nullptr;
            }

            newNode->leftChild = nullptr;
            newNode->rightChild = nullptr;
            newNode->height = 0;

            return newNode;
        }
        auto createFilledNode(const AVLTree* self, const void* inKey, const void* inValue) -> AVLTreeNode* {
            auto* const newNode = (AVLTreeNode*) malloc(sizeof(AVLTreeNode));
            if (!newNode) {
                return nullptr;
            }

            newNode->key = malloc(self->keySize);
            if (!newNode->key) {
                free(newNode);
                return nullptr;
            }
            memcpy(newNode->key, inKey, self->keySize);

            newNode->value = malloc(self->valueSize);
            if (!newNode->value) {
                free(newNode->key);
                free(newNode);
                return nullptr;
            }
            memcpy(newNode->value, inValue, self->valueSize);

            newNode->leftChild = nullptr;
            newNode->rightChild = nullptr;
            newNode->height = 0;

            return newNode;
        }
    }

    namespace {
        auto heightOf(const AVLTreeNode* node) -> i32 {
            if (!node) {
                return EMPTY_NODE_HEIGHT;
            }

            return node->height;
        }
        auto updateHeight(AVLTreeNode* node) -> void {
            if (!node) {
                return;
            }

            const i32 leftHeight = heightOf(node->leftChild);
            const i32 rightHeight = heightOf(node->rightChild);

            node->height = 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
        }

        auto balanceFactorOf(const AVLTreeNode* node) -> i32 {
            if (!node) {
                return 0;
            }

            // 计算节点平衡因子：左子树高度 - 右子树高度
            // 平衡节点的平衡因子 ∈ {-1, 0, 1}
            // 不平衡节点的平衡因子为 ±2
            return heightOf(node->leftChild) - heightOf(node->rightChild);
        }

        auto rotateLeft(AVLTreeNode* node) -> AVLTreeNode* {
            auto* const newRootNode = node->rightChild;

            node->rightChild = newRootNode->leftChild;

            newRootNode->leftChild = node;

            // 先更新节点的高度，再更新新根节点的高度
            updateHeight(node);
            updateHeight(newRootNode);

            return newRootNode;
        }
        auto rotateRight(AVLTreeNode* node) -> AVLTreeNode* {
            auto* const newRootNode = node->leftChild;

            node->leftChild = newRootNode->rightChild;

            newRootNode->rightChild = node;

            // 先更新节点的高度，再更新新根节点的高度
            updateHeight(node);
            updateHeight(newRootNode);

            return newRootNode;
        }

        auto rebalance(AVLTreeNode* node) -> AVLTreeNode* {
            // 更新节点高度
            updateHeight(node);

            // 计算平衡因子
            const i32 factor = balanceFactorOf(node);

            // 左子树过高
            if (factor > 1) {
                // LL 情况，右旋
                if (balanceFactorOf(node->leftChild) >= 0) {
                    return rotateRight(node);
                }

                // LR 情况，先左旋左子转为 LL，再右旋当前节点
                node->leftChild = rotateLeft(node->leftChild);

                return rotateRight(node);
            }

            // 右子树过高
            if (factor < -1) {
                // RR 情况，左旋
                if (balanceFactorOf(node->rightChild) <= 0) {
                    return rotateLeft(node);
                }

                // RL 情况，先右旋右子转为 RR，再左旋当前节点
                node->rightChild = rotateRight(node->rightChild);

                return rotateLeft(node);
            }

            return node;
        }
    }

    auto AVLTree_construct(const usize inKeySize, const usize inValueSize, i32 (*compare)(const void*, const void*)) -> AVLTree* {
        if (inKeySize == 0 || inValueSize == 0 || !compare) {
            return nullptr;
        }

        auto* const self = (AVLTree*) malloc(sizeof(AVLTree));
        if (!self) {
            return nullptr;
        }

        // 平衡二叉搜索树按需分配，无需预分配
        self->rootNode = nullptr;
        self->keySize = inKeySize;
        self->valueSize = inValueSize;
        self->size = 0;
        self->compare = compare;

        return self;
    }
    auto AVLTree_destruct(AVLTree* self) -> bool {
        if (!self) {
            return false;
        }

        destructSubtree(self->rootNode);

        free(self);

        return true;
    }

    auto AVLTree_copy(const AVLTree* self) -> AVLTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = AVLTree_construct(self->keySize, self->valueSize, self->compare);
        if (!newSelf) {
            return nullptr;
        }

        // 递归复制整棵树，保持原有树形结构与高度字段
        newSelf->rootNode = copySubtree(newSelf, self->rootNode);
        if (self->rootNode && !newSelf->rootNode) {
            AVLTree_destruct(newSelf);
            return nullptr;
        }

        newSelf->size = self->size;

        return newSelf;
    }
    auto AVLTree_move(AVLTree* self) -> AVLTree* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (AVLTree*) malloc(sizeof(AVLTree));
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

    auto AVLTree_insert(AVLTree* self, const void* inKey, const void* inValue) -> bool {
        if (!self || !inKey || !inValue) {
            return false;
        }

        // 向下导航时记录路径节点，供回溯使用
        AVLTreeNode* pathStack[128];
        usize stackSize = 0;

        // 二级指针直接定位新节点要挂的位置，旋转时也能就地更新父节点的子指针
        auto** nodePtr = &self->rootNode;
        while (*nodePtr) {
            auto* const node = *nodePtr;
            pathStack[stackSize++] = node;

            const i32 compareResult = self->compare(inKey, node->key);

            if (compareResult < 0) {
                nodePtr = &node->leftChild;
            } else if (compareResult > 0) {
                nodePtr = &node->rightChild;
            } else {
                // 键已存在，先分配新值再释放旧值
                // 保证新值内存分配失败时旧值不被破坏
                void* const newValue = malloc(self->valueSize);
                if (!newValue) {
                    return false;
                }

                if (node->value) {
                    free(node->value);
                }

                node->value = newValue;
                memcpy(node->value, inValue, self->valueSize);

                // 结构不变，无需回溯
                return true;
            }
        }

        // 创建新节点并挂到空位
        auto* const newNode = createFilledNode(self, inKey, inValue);
        if (!newNode) {
            return false;
        }

        *nodePtr = newNode;
        self->size++;

        // 向上回溯：从下往上更新高度并检查平衡
        while (stackSize > 0) {
            auto* const node = pathStack[--stackSize];
            auto* const newRootNode = rebalance(node);

            if (stackSize > 0) {
                // 把旋转后的新根挂回父节点
                auto* const parent = pathStack[stackSize - 1];
                if (parent->leftChild == node) {
                    parent->leftChild = newRootNode;
                } else {
                    parent->rightChild = newRootNode;
                }
            } else {
                // 到达根，更新根指针
                self->rootNode = newRootNode;
            }
        }

        return true;
    }
    auto AVLTree_insertSlot(AVLTree* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !oldValueSlotOut || !newValueSlotOut) {
            return false;
        }

        // keySlotOut 为空表示键已存在，不需要构造新键
        // oldValueSlotOut 为空表示键是新的，没有旧值需要析构
        // newValueSlotOut 必然非空（成功时），是调用者要构造的新值
        *keySlotOut = nullptr;
        *oldValueSlotOut = nullptr;
        *newValueSlotOut = nullptr;

        // 向下导航时记录路径节点，供回溯使用
        AVLTreeNode* pathStack[128];
        usize stackSize = 0;

        // 二级指针直接定位新节点要挂的位置，旋转时也能就地更新父节点的子指针
        auto** nodePtr = &self->rootNode;
        while (*nodePtr) {
            auto* const node = *nodePtr;
            pathStack[stackSize++] = node;

            const i32 compareResult = self->compare(inKey, node->key);

            if (compareResult < 0) {
                nodePtr = &node->leftChild;
            } else if (compareResult > 0) {
                nodePtr = &node->rightChild;
            } else {
                // 键已存在，分配新值内存，先分配后交接
                void* const newValue = malloc(self->valueSize);
                if (!newValue) {
                    return false;
                }

                // 保存旧值指针，交给调用者析构并释放
                *oldValueSlotOut = node->value;
                node->value = newValue;

                // 键已存在，不需要重新构造键；调用者只需构造新值
                *newValueSlotOut = newValue;

                // 结构不变，无需回溯
                return true;
            }
        }

        // 创建新节点并挂到空位
        auto* const newNode = createEmptyNode(self);
        if (!newNode) {
            return false;
        }

        *nodePtr = newNode;
        self->size++;

        // 键值都是全新的，都需要调用者构造
        *keySlotOut = newNode->key;
        *newValueSlotOut = newNode->value;

        // 向上回溯：从下往上更新高度并检查平衡
        while (stackSize > 0) {
            auto* const node = pathStack[--stackSize];
            auto* const newRootNode = rebalance(node);

            if (stackSize > 0) {
                // 把旋转后的新根挂回父节点
                auto* const parent = pathStack[stackSize - 1];
                if (parent->leftChild == node) {
                    parent->leftChild = newRootNode;
                } else {
                    parent->rightChild = newRootNode;
                }
            } else {
                // 到达根，更新根指针
                self->rootNode = newRootNode;
            }
        }

        return true;
    }

    auto AVLTree_remove(AVLTree* self, const void* inKey) -> bool {
        if (!self || !inKey) {
            return false;
        }

        // 向下导航时记录路径节点，供回溯使用
        AVLTreeNode* pathStack[128];
        usize stackSize = 0;

        // 二级指针直接定位目标节点
        auto** nodePtr = &self->rootNode;
        while (*nodePtr) {
            auto* const node = *nodePtr;
            pathStack[stackSize++] = node;

            const i32 compareResult = self->compare(inKey, node->key);

            if (compareResult < 0) {
                nodePtr = &node->leftChild;
            } else if (compareResult > 0) {
                nodePtr = &node->rightChild;
            } else {
                break;
            }
        }

        // 未找到目标节点
        if (!*nodePtr) {
            return false;
        }

        auto* const targetNode = *nodePtr;
        const usize targetIndex = stackSize - 1;

        if (targetNode->leftChild == nullptr) {
            // 没有左子节点情况
            // 直接用右子节点替代当前节点
            *nodePtr = targetNode->rightChild;

            // 释放当前节点的键值对，最后释放节点本身
            if (targetNode->key) {
                free(targetNode->key);
            }

            if (targetNode->value) {
                free(targetNode->value);
            }

            free(targetNode);

            self->size--;

            // 从路径栈中弹出被删节点，回溯从父节点开始
            stackSize--;
        } else if (targetNode->rightChild == nullptr) {
            // 没有右子节点情况
            // 直接用左子节点替代当前节点
            *nodePtr = targetNode->leftChild;

            if (targetNode->key) {
                free(targetNode->key);
            }
            if (targetNode->value) {
                free(targetNode->value);
            }
            free(targetNode);

            self->size--;

            stackSize--;
        } else {
            // 有两个子节点情况
            // 用中序后继节点替代当前节点
            // 找中序后继节点，即找右子树的最左节点，同时把中间路径压入路径栈
            auto* successorParent = targetNode;
            auto* successorNode = targetNode->rightChild;
            while (successorNode->leftChild) {
                pathStack[stackSize++] = successorNode;
                successorParent = successorNode;
                successorNode = successorNode->leftChild;
            }

            // 中序后继节点不是当前节点的直接右子节点时
            // 先把中序后继节点的右子树挂到中序后继节点父节点的左子节点上
            // 再把当前节点的右子树接到中序后继节点上
            if (successorParent != targetNode) {
                successorParent->leftChild = successorNode->rightChild;
                successorNode->rightChild = targetNode->rightChild;
            }

            // 让中序后继节点接管当前节点的左子树
            successorNode->leftChild = targetNode->leftChild;

            // 用中序后继节点替换当前节点
            *nodePtr = successorNode;

            // 路径栈中当前节点的位置替换为中序后继节点，供回溯时使用
            pathStack[targetIndex] = successorNode;

            // 释放当前节点的键值对，最后释放节点本身
            if (targetNode->key) {
                free(targetNode->key);
            }

            if (targetNode->value) {
                free(targetNode->value);
            }

            free(targetNode);

            self->size--;
        }

        // 向上回溯：从下往上更新高度并检查平衡
        // 删除可能触发多次旋转，必须一直回溯到根
        while (stackSize > 0) {
            auto* const node = pathStack[--stackSize];
            auto* const newRootNode = rebalance(node);

            if (stackSize > 0) {
                // 把旋转后的新根挂回父节点
                auto* const parent = pathStack[stackSize - 1];
                if (parent->leftChild == node) {
                    parent->leftChild = newRootNode;
                } else {
                    parent->rightChild = newRootNode;
                }
            } else {
                // 到达根，更新根指针
                self->rootNode = newRootNode;
            }
        }

        return true;
    }
    auto AVLTree_removeSlot(AVLTree* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !valueSlotOut) {
            return false;
        }

        *keySlotOut = nullptr;
        *valueSlotOut = nullptr;

        // 向下导航时记录路径节点，供回溯使用
        AVLTreeNode* pathStack[128];
        usize stackSize = 0;

        auto** nodePtr = &self->rootNode;
        while (*nodePtr) {
            auto* const node = *nodePtr;
            pathStack[stackSize++] = node;

            const i32 compareResult = self->compare(inKey, node->key);

            if (compareResult < 0) {
                nodePtr = &node->leftChild;
            } else if (compareResult > 0) {
                nodePtr = &node->rightChild;
            } else {
                break;
            }
        }

        // 未找到目标节点
        if (!*nodePtr) {
            return false;
        }

        auto* const targetNode = *nodePtr;
        const usize targetIndex = stackSize - 1;

        // 保存目标节点的键值指针，交给调用者析构并释放
        *keySlotOut = targetNode->key;
        *valueSlotOut = targetNode->value;

        if (targetNode->leftChild == nullptr) {
            // 没有左子节点情况
            *nodePtr = targetNode->rightChild;

            // 释放节点本身，但不释放键值内存
            free(targetNode);

            self->size--;

            stackSize--;
        } else if (targetNode->rightChild == nullptr) {
            // 没有右子节点情况
            *nodePtr = targetNode->leftChild;

            free(targetNode);

            self->size--;

            stackSize--;
        } else {
            // 有两个子节点情况
            // 找中序后继节点，即找右子树的最左节点，同时把中间路径压入路径栈
            auto* successorParent = targetNode;
            auto* successorNode = targetNode->rightChild;
            while (successorNode->leftChild) {
                pathStack[stackSize++] = successorNode;
                successorParent = successorNode;
                successorNode = successorNode->leftChild;
            }

            // 中序后继节点不是当前节点的直接右子节点时
            // 先把中序后继节点的右子树挂到中序后继节点父节点的左子节点上
            // 再把当前节点的右子树接到中序后继节点上
            if (successorParent != targetNode) {
                successorParent->leftChild = successorNode->rightChild;
                successorNode->rightChild = targetNode->rightChild;
            }

            // 让中序后继节点接管当前节点的左子树
            successorNode->leftChild = targetNode->leftChild;

            // 用中序后继节点替换当前节点
            *nodePtr = successorNode;

            // 路径栈中当前节点的位置替换为中序后继节点
            pathStack[targetIndex] = successorNode;

            // 释放节点本身，但不释放键值内存
            free(targetNode);

            self->size--;
        }

        // 向上回溯：从下往上更新高度并检查平衡
        while (stackSize > 0) {
            auto* const node = pathStack[--stackSize];
            auto* const newRootNode = rebalance(node);

            if (stackSize > 0) {
                auto* const parent = pathStack[stackSize - 1];
                if (parent->leftChild == node) {
                    parent->leftChild = newRootNode;
                } else {
                    parent->rightChild = newRootNode;
                }
            } else {
                self->rootNode = newRootNode;
            }
        }

        return true;
    }
    auto AVLTree_clear(AVLTree* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        destructSubtree(self->rootNode);

        self->rootNode = nullptr;
        self->size = 0;

        return true;
    }

    auto AVLTree_find(const AVLTree* self, const void* inKey) -> void* {
        if (!self || !inKey) {
            return nullptr;
        }

        auto* currentNode = self->rootNode;
        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);
            if (compareResult == 0) {
                return currentNode->value;
            }

            currentNode = (compareResult < 0) ? currentNode->leftChild : currentNode->rightChild;
        }

        return nullptr;
    }
    auto AVLTree_findNode(const AVLTree* self, const void* inKey) -> AVLTreeNode* {
        if (!self || !inKey) {
            return nullptr;
        }

        auto* currentNode = self->rootNode;
        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);
            if (compareResult == 0) {
                return currentNode;
            }

            currentNode = (compareResult < 0) ? currentNode->leftChild : currentNode->rightChild;
        }

        return nullptr;
    }
    auto AVLTree_findSuccessorNode(const AVLTree* self, const void* inKey) -> AVLTreeNode* {
        if (!self || !inKey) {
            return nullptr;
        }

        auto* successor = (AVLTreeNode*) nullptr;
        auto* currentNode = self->rootNode;
        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);
            if (compareResult < 0) {
                successor = currentNode;
                currentNode = currentNode->leftChild;
            } else if (compareResult > 0) {
                currentNode = currentNode->rightChild;
            } else {
                if (currentNode->rightChild) {
                    auto* node = currentNode->rightChild;
                    while (node->leftChild) {
                        node = node->leftChild;
                    }

                    return node;
                }

                return successor;
            }
        }

        return successor;
    }
    auto AVLTree_findPredecessorNode(const AVLTree* self, const void* inKey) -> AVLTreeNode* {
        if (!self || !inKey) {
            return nullptr;
        }

        auto* predecessor = (AVLTreeNode*) nullptr;
        auto* currentNode = self->rootNode;

        while (currentNode) {
            const i32 compareResult = self->compare(inKey, currentNode->key);
            if (compareResult > 0) {
                predecessor = currentNode;
                currentNode = currentNode->rightChild;
            } else if (compareResult < 0) {
                currentNode = currentNode->leftChild;
            } else {
                if (currentNode->leftChild) {
                    auto* node = currentNode->leftChild;
                    while (node->rightChild) {
                        node = node->rightChild;
                    }

                    return node;
                }

                return predecessor;
            }
        }

        return predecessor;
    }

    auto AVLTree_rootNode(const AVLTree* self) -> AVLTreeNode* {
        if (!self) {
            return nullptr;
        }

        return self->rootNode;
    }
    auto AVLTree_minNode(const AVLTree* self) -> AVLTreeNode* {
        if (!self || !self->rootNode) {
            return nullptr;
        }

        auto* currentNode = self->rootNode;
        while (currentNode->leftChild) {
            currentNode = currentNode->leftChild;
        }

        return currentNode;
    }
    auto AVLTree_maxNode(const AVLTree* self) -> AVLTreeNode* {
        if (!self || !self->rootNode) {
            return nullptr;
        }

        auto* currentNode = self->rootNode;
        while (currentNode->rightChild) {
            currentNode = currentNode->rightChild;
        }

        return currentNode;
    }

    auto AVLTree_height(const AVLTree* self) -> i32 {
        if (!self || !self->rootNode) {
            return 0;
        }

        // 内部高度：空节点 -1，叶子 0
        // 对外高度：空树 0，叶子 1
        return self->rootNode->height + 1;
    }
    auto AVLTree_keySize(const AVLTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->keySize;
    }
    auto AVLTree_valueSize(const AVLTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->valueSize;
    }
    auto AVLTree_size(const AVLTree* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }

    auto AVLTree_isEmpty(const AVLTree* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }
    auto AVLTree_contains(const AVLTree* self, const void* inKey) -> bool {
        return AVLTree_findNode(self, inKey) != nullptr;
    }

    auto AVLTreeNode_key(const AVLTreeNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->key;
    }
    auto AVLTreeNode_value(const AVLTreeNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->value;
    }
    auto AVLTreeNode_left(const AVLTreeNode* node) -> AVLTreeNode* {
        if (!node) {
            return nullptr;
        }

        return node->leftChild;
    }
    auto AVLTreeNode_right(const AVLTreeNode* node) -> AVLTreeNode* {
        if (!node) {
            return nullptr;
        }

        return node->rightChild;
    }
    auto AVLTreeNode_height(const AVLTreeNode* node) -> i32 {
        if (!node) {
            return 0;
        }

        // 内部高度：空节点 -1，叶子 0
        // 对外高度：空树 0，叶子 1
        return node->height + 1;
    }
}