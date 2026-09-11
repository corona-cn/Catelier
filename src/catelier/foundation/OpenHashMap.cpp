#include "OpenHashMap.hpp"

#include <cstdlib>
#include <cstring>

#include "ElasticArray.hpp"
#include "../CommonMacro.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
        constexpr float DEFAULT_LOAD_FACTOR = 0.75f;
    }

    namespace {
        enum NodeState {
            EMPTY = 0,
            OCCUPIED = 1,
        };

        typedef struct Node {
            void* key;
            void* value;
            NodeState state;
        } Node;
    }

    typedef struct OpenHashMap {
        ElasticArray* nodes;
        usize size;
        usize capacity;
        usize keySize;
        usize valueSize;
        u64 (*hash)(const void*);
        bool (*keyEquals)(const void*, const void*);
        float loadFactor;
    } OpenHashMap;

    auto OpenHashMap_construct(const usize initialCapacity, u64 (*hash)(const void* key), bool (*keyEquals)(const void* a, const void* b)) -> OpenHashMap* {
        if (!hash || !keyEquals) {
            return nullptr;
        }

        // 确保容量至少为 DEFAULT_CAPACITY，并调整为 2 的幂
        usize capacity = 1;
        while (capacity < initialCapacity || capacity < DEFAULT_CAPACITY) {
            capacity <<= 1;
        }

        auto* const self = (OpenHashMap*) malloc(sizeof(OpenHashMap));
        if (!self) {
            return nullptr;
        }

        // 构造节点集
        self->nodes = ElasticArray_construct(capacity);
        if (!self->nodes) {
            free(self);
            return nullptr;
        }

        // 将节点集填充为 nullptr
        for (usize i = 0; i < capacity; ++i) {
            if (!ElasticArray_push(self->nodes, nullptr, sizeof(void*))) {
                ElasticArray_destruct(self->nodes);

                free(self);

                return nullptr;
            }
        }

        self->size = 0;
        self->capacity = capacity;
        self->keySize = 0;
        self->valueSize = 0;
        self->hash = hash;
        self->keyEquals = keyEquals;
        self->loadFactor = DEFAULT_LOAD_FACTOR;

        return self;
    }
    auto OpenHashMap_destruct(OpenHashMap* self) -> bool {
        if (!self) {
            return false;
        }

        // 遍历所有节点，释放所有节点的键值，最后释放所有节点
        for (usize i = 0; i < self->capacity; ++i) {
            auto* node = (Node*) ElasticArray_get(self->nodes, i);
            if (node) {
                if (node->key) {
                    free(node->key);
                }

                if (node->value) {
                    free(node->value);
                }

                free(node);
            }
        }

        ElasticArray_destruct(self->nodes);

        free(self);

        return true;
    }

    auto OpenHashMap_copy(const OpenHashMap* self) -> OpenHashMap* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = OpenHashMap_construct(self->capacity, self->hash, self->keyEquals);
        if (!newSelf) {
            return nullptr;
        }

        newSelf->keySize = self->keySize;
        newSelf->valueSize = self->valueSize;

        // 遍历所有节点，复制 OCCUPIED 节点
        for (usize i = 0; i < self->capacity; ++i) {
            const auto* const node = (Node*) ElasticArray_get(self->nodes, i);
            if (!node || node->state != OCCUPIED) {
                continue;
            }

            // 分配并复制键内存
            void* const key = malloc(self->keySize);
            if (!key) {
                OpenHashMap_destruct(newSelf);

                return nullptr;
            }
            memcpy(key, node->key, self->keySize);

            // 分配并复制值内存
            void* const value = malloc(self->valueSize);
            if (!value) {
                free(key);

                OpenHashMap_destruct(newSelf);

                return nullptr;
            }
            memcpy(value, node->value, self->valueSize);

            // 尝试插入新的键值对，失败则释放键值对，并销毁新自身
            if (!OpenHashMap_insert(newSelf, key, self->keySize, value, self->valueSize)) {
                free(key);
                free(value);

                OpenHashMap_destruct(newSelf);

                return nullptr;
            }
        }

        return newSelf;
    }
    auto OpenHashMap_move(OpenHashMap* self) -> OpenHashMap* {
        if (!self) {
            return nullptr;
        }

        auto* newSelf = (OpenHashMap*) malloc(sizeof(OpenHashMap));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->nodes = self->nodes;
        newSelf->size = self->size;
        newSelf->capacity = self->capacity;
        newSelf->keySize = self->keySize;
        newSelf->valueSize = self->valueSize;
        newSelf->hash = self->hash;
        newSelf->keyEquals = self->keyEquals;
        newSelf->loadFactor = self->loadFactor;

        self->nodes = nullptr;
        self->size = 0;
        self->capacity = 0;
        self->keySize = 0;
        self->valueSize = 0;
        self->hash = nullptr;
        self->keyEquals = nullptr;
        self->loadFactor = DEFAULT_LOAD_FACTOR;

        return newSelf;
    }

    auto OpenHashMap_insert(OpenHashMap* self, const void* inKey, const usize inKeySize, const void* inValue, const usize inValueSize) -> bool {
        if (!self || !inKey || inKeySize == 0 || !inValue || inValueSize == 0) {
            return false;
        }

        // 如果插入后负载因子超过阈值，先扩容
        if ((self->size + 1) > (usize) (self->capacity * self->loadFactor)) {
            const usize newCapacity = self->capacity * 2;
            if (!OpenHashMap_reserve(self, newCapacity)) {
                return false;
            }
        }

        // 计算哈希值并映射到数组索引范围
        const usize hash = self->hash(inKey);
        usize index = hash & (self->capacity - 1);

        // 线性探测，开放寻址，直到找到空位或匹配键
        while (true) {
            auto* const node = (Node*) ElasticArray_get(self->nodes, index);

            // 如果当前槽位是 nullptr，表示从未使用过，直接分配新节点存入
            if (node == nullptr) {
                auto* const newNode = (Node*) malloc(sizeof(Node));
                if (!newNode) {
                    return false;
                }

                // 分配并复制键内存
                newNode->key = malloc(inKeySize);
                if (!newNode->key) {
                    free(newNode);
                    return false;
                }
                memcpy(newNode->key, inKey, inKeySize);

                // 分配并复制值内存
                newNode->value = malloc(inValueSize);
                if (!newNode->value) {
                    free(newNode->key);
                    free(newNode);
                    return false;
                }
                memcpy(newNode->value, inValue, inValueSize);

                // 记录节点状态为被占用
                newNode->state = OCCUPIED;

                // 向节点集写入该节点
                ElasticArray_set(self->nodes, index, newNode);

                // 更新状态
                self->size++;
                self->keySize = inKeySize;
                self->valueSize = inValueSize;

                return true;
            }

            // 如果当前槽位节点是 EMPTY 状态，可以复用这个节点
            if (node->state == EMPTY) {
                // 释放旧键内存并置空
                if (node->key) {
                    PTR_FREE_AND_NULL(node->key);
                }

                // 释放旧值内存并置空
                if (node->value) {
                    PTR_FREE_AND_NULL(node->value);
                }

                // 分配并复制新键内存
                node->key = malloc(inKeySize);
                if (!node->key) {
                    return false;
                }
                memcpy(node->key, inKey, inKeySize);

                // 分配并复制新值内存
                node->value = malloc(inValueSize);
                if (!node->value) {
                    free(node->key);
                    node->key = nullptr;
                    return false;
                }
                memcpy(node->value, inValue, inValueSize);

                // 记录节点状态为被占用
                node->state = OCCUPIED;

                // 此时节点键值对已经改变，不需要通过节点集 setter 重复写入

                // 更新状态
                self->size++;
                self->keySize = inKeySize;
                self->valueSize = inValueSize;

                return true;
            }

            // 如果键已存在，覆盖值并返回
            if (self->keyEquals(node->key, inKey)) {
                // 释放旧值内存
                if (node->value) {
                    free(node->value);
                }

                // 分配并复制新值内存
                node->value = malloc(inValueSize);
                if (!node->value) {
                    return false;
                }
                memcpy(node->value, inValue, inValueSize);

                // 如果新值更大，更新 valueSize
                if (inValueSize > self->valueSize) {
                    self->valueSize = inValueSize;
                }

                return true;
            }

            // 本轮线性探测没找到匹配的键，继续往后探测
            index = (index + 1) & (self->capacity - 1);
        }
    }

    auto OpenHashMap_vacate(OpenHashMap* self, const void* inKey) -> bool {
        if (!self || !inKey) {
            return false;
        }

        // 计算哈希值并映射到数组索引范围
        const usize hash = self->hash(inKey);
        usize index = hash & (self->capacity - 1);

        // 线性探测，开放寻址，查找目标节点
        while (true) {
            auto* const node = (Node*) ElasticArray_get(self->nodes, index);

            // 遇到 nullptr，说明从未使用过，无需腾空
            if (node == nullptr) {
                return false;
            }

            // 遇到 EMPTY，说明探测链已结束，无需腾空
            if (node->state == EMPTY) {
                return false;
            }

            // 找到匹配的键
            if (node->state == OCCUPIED && self->keyEquals(node->key, inKey)) {
                // 释放键内存并置空
                if (node->key) {
                    PTR_FREE_AND_NULL(node->key);
                }

                // 释放值内存并置空
                if (node->value) {
                    PTR_FREE_AND_NULL(node->value);
                }

                // 记录节点状态为空
                node->state = EMPTY;

                // 更新状态
                self->size--;

                return true;
            }

            // 本轮线性探测没找到匹配的键，继续往后探测
            index = (index + 1) & (self->capacity - 1);
        }
    }
    auto OpenHashMap_vacateAll(OpenHashMap* self) -> bool {
        if (!self) {
            return false;
        }

        // 遍历所有槽位，腾空所有节点
        for (usize i = 0; i < self->capacity; ++i) {
            auto* const node = (Node*) ElasticArray_get(self->nodes, i);

            // 只处理被占用的节点
            if (node && node->state == OCCUPIED) {
                // 释放键内存并置空
                if (node->key) {
                    PTR_FREE_AND_NULL(node->key);
                }

                // 释放值内存并置空
                if (node->value) {
                    PTR_FREE_AND_NULL(node->value);
                }

                // 记录节点状态为空
                node->state = EMPTY;
            }
        }

        // 更新状态
        self->size = 0;
        self->keySize = 0;
        self->valueSize = 0;

        return true;
    }
    auto OpenHashMap_erase(OpenHashMap* self, const void* inKey) -> bool {
        if (!self || !inKey) {
            return false;
        }

        // 计算哈希值并映射到数组索引范围
        const usize hash = self->hash(inKey);
        usize index = hash & (self->capacity - 1);

        // 线性探测，开放寻址，查找目标节点
        while (true) {
            auto* const node = (Node*) ElasticArray_get(self->nodes, index);

            // 遇到 nullptr，说明从未使用过，无需擦除
            if (node == nullptr) {
                return false;
            }

            // 遇到 EMPTY，说明探测链已结束，无需擦除
            if (node->state == EMPTY) {
                return false;
            }

            // 找到匹配的键，找到目标节点，进行擦除
            if (node->state == OCCUPIED && self->keyEquals(node->key, inKey)) {
                // 释放节点键值对，同时释放节点
                free(node->key);
                free(node->value);
                free(node);

                // 将当前节点槽位元素设置为 nullptr，即擦除当前节点
                ElasticArray_set(self->nodes, index, nullptr);

                // 节点被擦除，节点集大小递减
                self->size--;

                // 继续整理后续节点，修复探测链
                usize nextIndex = (index + 1) & (self->capacity - 1);
                while (true) {
                    auto* const nextNode = (Node*) ElasticArray_get(self->nodes, nextIndex);
                    if (nextNode == nullptr || nextNode->state == EMPTY) {
                        break;
                    }

                    // 计算该节点原本应该放在哪个位置
                    const usize originalIndex = self->hash(nextNode->key) & (self->capacity - 1);

                    // 如果原始位置在当前删除位置之前或等于删除位置，
                    // 说明它被挤到了后面，需要把它移回前面
                    if (originalIndex <= index) {
                        // 将当前索引位元素设为下一节点
                        ElasticArray_set(self->nodes, index, nextNode);

                        // 将下一索引位元素设为 nullptr
                        ElasticArray_set(self->nodes, nextIndex, nullptr);

                        // 索引更新
                        index = nextIndex;
                    }

                    // 继续探测
                    nextIndex = (nextIndex + 1) & (self->capacity - 1);
                }

                return true;
            }

            // 继续探测
            index = (index + 1) & (self->capacity - 1);
        }
    }
    auto OpenHashMap_eraseAll(OpenHashMap* self) -> bool {
        if (!self) {
            return false;
        }

        // 遍历所有槽位，擦除所有节点
        for (usize i = 0; i < self->capacity; ++i) {
            auto* const node = (Node*) ElasticArray_get(self->nodes, i);
            if (node) {
                // 释放键内存并置空
                if (node->key) {
                    PTR_FREE_AND_NULL(node->key);
                }

                // 释放值内存并置空
                if (node->value) {
                    PTR_FREE_AND_NULL(node->value);
                }

                // 释放节点内存
                free(node);
            }

            // 将当前槽位置为 nullptr
            ElasticArray_set(self->nodes, i, nullptr);
        }

        // 更新状态
        self->size = 0;
        self->keySize = 0;
        self->valueSize = 0;

        return true;
    }

    auto OpenHashMap_find(const OpenHashMap* self, const void* inKey, void** valueOut) -> bool {
        if (!self || !inKey || !valueOut) {
            return false;
        }

        // 计算哈希值并映射到数组索引范围
        const usize hash = self->hash(inKey);
        usize index = hash & (self->capacity - 1);

        // 线性探测，开放寻址，查找目标节点
        while (true) {
            const auto* const node = (const Node*) ElasticArray_get(self->nodes, index);

            // 遇到 nullptr，说明从未使用过，可认为不存在
            if (node == nullptr) {
                return false;
            }

            // 遇到 EMPTY，说明探测链已结束，可认为不存在
            if (node->state == EMPTY) {
                return false;
            }

            // 找到匹配的键
            if (node->state == OCCUPIED && self->keyEquals(node->key, inKey)) {
                *valueOut = node->value;
                return true;
            }

            // 键不匹配，继续探测下一个位置
            index = (index + 1) & (self->capacity - 1);
        }
    }

    auto OpenHashMap_reserve(OpenHashMap* self, const usize newCapacity) -> bool {
        if (!self || newCapacity <= self->capacity) {
            return false;
        }

        // 确保新容量至少为 4，并调整为 2 的幂
        usize capacity = 1;
        while (capacity < newCapacity || capacity < DEFAULT_CAPACITY) {
            capacity <<= 1;
        }

        // 创建新的节点集
        auto* const newNodes = ElasticArray_construct(capacity);
        if (!newNodes) {
            return false;
        }

        // 填充节点集为 nullptr
        for (usize i = 0; i < capacity; ++i) {
            if (!ElasticArray_push(newNodes, nullptr, sizeof(void*))) {
                ElasticArray_destruct(newNodes);
                return false;
            }
        }

        // 保存旧的节点集和容量
        auto* const oldNodes = self->nodes;
        const usize oldCapacity = self->capacity;

        // 临时切换为新数组，临时重置节点集大小
        self->nodes = newNodes;
        self->capacity = capacity;
        self->size = 0;

        // 遍历旧数组，将所有被占用的节点迁移到新数组
        for (usize i = 0; i < oldCapacity; ++i) {
            auto* const oldNode = (Node*) ElasticArray_get(oldNodes, i);
            if (oldNode && oldNode->state == OCCUPIED) {
                // 计算哈希值并映射到数组索引范围
                const usize hash = self->hash(oldNode->key);
                usize index = hash & (self->capacity - 1);

                // 线性探测，开放寻址，寻找空位
                while (true) {
                    auto* const node = (Node*) ElasticArray_get(self->nodes, index);

                    if (node == nullptr) {
                        ElasticArray_set(self->nodes, index, oldNode);
                        self->size++;
                        break;
                    }

                    if (node->state == EMPTY) {
                        node->key = oldNode->key;
                        node->value = oldNode->value;
                        node->state = OCCUPIED;
                        self->size++;
                        break;
                    }

                    // 继续探测
                    index = (index + 1) & (self->capacity - 1);
                }
            }
        }

        // 销毁旧数组，其中旧节点集已经迁移
        ElasticArray_destruct(oldNodes);

        return true;
    }

    auto OpenHashMap_size(const OpenHashMap* self) -> usize {
        return self ? self->size : 0;
    }
    auto OpenHashMap_keySize(const OpenHashMap* self) -> usize {
        return self ? self->keySize : 0;
    }
    auto OpenHashMap_valueSize(const OpenHashMap* self) -> usize {
        return self ? self->valueSize : 0;
    }
    auto OpenHashMap_capacity(const OpenHashMap* self) -> usize {
        return self ? self->capacity : 0;
    }
    auto OpenHashMap_isEmpty(const OpenHashMap* self) -> bool {
        return self ? self->size == 0 : true;
    }
}
