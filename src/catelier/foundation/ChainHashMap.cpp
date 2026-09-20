#include "ChainHashMap.hpp"

#include <cstdlib>
#include <cstring>

#include "ArrayList.hpp"

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 4;
        constexpr float DEFAULT_LOAD_FACTOR = 1.0f;
    }

    typedef struct ChainHashMapNode {
        void* key;
        void* value;
        ChainHashMapNode* nextNode;
    } ChainHashMapNode;

    typedef struct ChainHashMap {
        ArrayList* buckets;
        usize capacity;
        usize size;
        usize keySize;
        usize valueSize;
        u64 (*hash)(const void*);
        bool (*keyEquals)(const void*, const void*);
        float loadFactor;
    } ChainHashMap;

    auto ChainHashMap_construct(const usize initialCapacity, const usize inKeySize, const usize inValueSize, u64 (*hash)(const void*), bool (*keyEquals)(const void*, const void*)) -> ChainHashMap* {
        if (inKeySize == 0 || inValueSize == 0 || !hash || !keyEquals) {
            return nullptr;
        }

        // 确保容量至少为 DEFAULT_CAPACITY，并调整为 2 的幂
        usize capacity = 1;
        while (capacity < initialCapacity || capacity < DEFAULT_CAPACITY) {
            capacity <<= 1;
        }

        auto* const self = (ChainHashMap*) malloc(sizeof(ChainHashMap));
        if (!self) {
            return nullptr;
        }

        // 构造桶数组，元素是指向链表头节点的指针
        self->buckets = ArrayList_construct(capacity, sizeof(void*));
        if (!self->buckets) {
            free(self);
            return nullptr;
        }

        // 将桶数组填充为 nullptr，即所有桶都是空链表
        for (usize i = 0; i < capacity; ++i) {
            if (!ArrayList_push(self->buckets, nullptr)) {
                ArrayList_destruct(self->buckets);

                free(self);

                return nullptr;
            }
        }

        self->size = 0;
        self->capacity = capacity;
        self->keySize = inKeySize;
        self->valueSize = inValueSize;
        self->hash = hash;
        self->keyEquals = keyEquals;
        self->loadFactor = DEFAULT_LOAD_FACTOR;

        return self;
    }
    auto ChainHashMap_destruct(ChainHashMap* self) -> bool {
        if (!self) {
            return false;
        }

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 遍历所有桶，释放每个桶的链表节点
        for (usize i = 0; i < self->capacity; ++i) {
            auto* currentNode = *(buckets + i);
            while (currentNode) {
                auto* const nextNode = currentNode->nextNode;

                // 释放节点键值对，最后释放节点
                if (currentNode->key) {
                    free(currentNode->key);
                }

                if (currentNode->value) {
                    free(currentNode->value);
                }

                free(currentNode);

                currentNode = nextNode;
            }
        }

        ArrayList_destruct(self->buckets);

        free(self);

        return true;
    }

    auto ChainHashMap_copy(const ChainHashMap* self) -> ChainHashMap* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = ChainHashMap_construct(self->capacity, self->keySize, self->valueSize, self->hash, self->keyEquals);
        if (!newSelf) {
            return nullptr;
        }

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 遍历所有桶，复制每个桶的链表节点
        for (usize i = 0; i < self->capacity; ++i) {
            const auto* currentNode = *(buckets + i);
            while (currentNode) {
                // 分配并复制键内存
                void* const key = malloc(self->keySize);
                if (!key) {
                    ChainHashMap_destruct(newSelf);
                    return nullptr;
                }
                memcpy(key, currentNode->key, self->keySize);

                // 分配并复制值内存
                void* const value = malloc(self->valueSize);
                if (!value) {
                    free(key);
                    ChainHashMap_destruct(newSelf);
                    return nullptr;
                }
                memcpy(value, currentNode->value, self->valueSize);

                // 尝试插入新的键值对，失败则释放键值对，并销毁新自身
                if (!ChainHashMap_insert(newSelf, key, value)) {
                    free(key);
                    free(value);
                    ChainHashMap_destruct(newSelf);
                    return nullptr;
                }

                // 继续下一轮拷贝
                currentNode = currentNode->nextNode;
            }
        }

        return newSelf;
    }
    auto ChainHashMap_move(ChainHashMap* self) -> ChainHashMap* {
        if (!self) {
            return nullptr;
        }

        auto* newSelf = (ChainHashMap*) malloc(sizeof(ChainHashMap));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->buckets = self->buckets;
        newSelf->size = self->size;
        newSelf->capacity = self->capacity;
        newSelf->keySize = self->keySize;
        newSelf->valueSize = self->valueSize;
        newSelf->hash = self->hash;
        newSelf->keyEquals = self->keyEquals;
        newSelf->loadFactor = self->loadFactor;

        self->buckets = nullptr;
        self->size = 0;
        self->capacity = 0;
        self->keySize = 0;
        self->valueSize = 0;
        self->hash = nullptr;
        self->keyEquals = nullptr;
        self->loadFactor = DEFAULT_LOAD_FACTOR;

        return newSelf;
    }

    auto ChainHashMap_insert(ChainHashMap* self, const void* inKey, const void* inValue) -> bool {
        if (!self || !inKey || !inValue) {
            return false;
        }

        // 如果插入后负载因子超过阈值，则先扩容
        if ((self->size + 1) > ((usize) (self->capacity * self->loadFactor))) {
            const usize newCapacity = self->capacity * 2;
            if (!ChainHashMap_reserve(self, newCapacity)) {
                return false;
            }
        }

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 计算哈希值并映射到桶索引范围
        const u64 hash = self->hash(inKey);
        const usize index = hash & (self->capacity - 1);

        // 取出该桶的链表头节点
        auto* currentNode = *(buckets + index);

        // 遍历链表，查找键是否已存在
        while (currentNode) {
            if (self->keyEquals(currentNode->key, inKey)) {
                // 键已存在，释放旧值内存
                if (currentNode->value) {
                    free(currentNode->value);
                }

                // 分配并复制新值内存
                currentNode->value = malloc(self->valueSize);
                if (!currentNode->value) {
                    return false;
                }
                memcpy(currentNode->value, inValue, self->valueSize);

                return true;
            }

            // 继续下一轮查找匹配的键
            currentNode = currentNode->nextNode;
        }

        // 键不存在，创建新节点并头插到链表
        auto* const newNode = (ChainHashMapNode*) malloc(sizeof(ChainHashMapNode));
        if (!newNode) {
            return false;
        }

        // 分配并复制键内存
        newNode->key = malloc(self->keySize);
        if (!newNode->key) {
            free(newNode);
            return false;
        }
        memcpy(newNode->key, inKey, self->keySize);

        // 分配并复制值内存
        newNode->value = malloc(self->valueSize);
        if (!newNode->value) {
            free(newNode->key);
            free(newNode);
            return false;
        }
        memcpy(newNode->value, inValue, self->valueSize);

        // 将新节点指向原头节点
        newNode->nextNode = *(buckets + index);

        // 更新桶头节点为新节点
        *(buckets + index) = newNode;

        // 更新状态
        self->size++;

        return true;
    }
    auto ChainHashMap_insertSlot(ChainHashMap* self, const void* inKey, void** keySlotOut, void** oldValueSlotOut, void** newValueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !oldValueSlotOut || !newValueSlotOut) {
            return false;
        }

        // keySlotOut 为 null 表示 key 已存在，不需要构造新 key
        // oldValueSlotOut 为 null 表示 key 是新的，没有旧 value 需要析构
        // newValueSlotOut 必然非 null（成功时），是调用者要构造的新 value
        *keySlotOut = nullptr;
        *oldValueSlotOut = nullptr;
        *newValueSlotOut = nullptr;

        // 如果插入后负载因子超过阈值，则先扩容
        if ((self->size + 1) > ((usize) (self->capacity * self->loadFactor))) {
            const usize newCapacity = self->capacity * 2;
            if (!ChainHashMap_reserve(self, newCapacity)) {
                return false;
            }
        }

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 计算哈希值并映射到桶索引范围
        const u64 hash = self->hash(inKey);
        const usize index = hash & (self->capacity - 1);

        // 取出该桶的链表头节点
        auto* currentNode = *(buckets + index);

        // 遍历链表，查找键是否已存在
        while (currentNode) {
            if (self->keyEquals(currentNode->key, inKey)) {
                // key 已存在，保存旧 value 指针，交给调用者析构 + 释放
                *oldValueSlotOut = currentNode->value;

                // 分配新的 value 内存
                void* const newValue = malloc(self->valueSize);
                if (!newValue) {
                    return false;
                }

                currentNode->value = newValue;

                // key 已存在，不需要重新构造 key；调用者只需构造新 value
                *newValueSlotOut = newValue;

                return true;
            }

            // 继续下一轮查找匹配的键
            currentNode = currentNode->nextNode;
        }

        // key 不存在，创建新节点并头插到链表
        auto* const newNode = (ChainHashMapNode*) malloc(sizeof(ChainHashMapNode));
        if (!newNode) {
            return false;
        }

        // 分配 key 和 value 的未初始化内存，交给调用者做 placement new
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

        // 将新节点指向原头节点
        newNode->nextNode = *(buckets + index);

        // 更新桶头节点为新节点
        *(buckets + index) = newNode;

        // 更新状态
        self->size++;

        // key 和 value 都是全新的，都需要调用者构造
        *keySlotOut = newNode->key;
        *newValueSlotOut = newNode->value;

        return true;
    }

    auto ChainHashMap_remove(ChainHashMap* self, const void* inKey) -> bool {
        if (!self || !inKey || self->size == 0) {
            return false;
        }

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 计算哈希值并映射到桶索引范围
        const u64 hash = self->hash(inKey);
        const usize index = hash & (self->capacity - 1);

        // 取出该桶的链表头节点
        auto* currentNode = *(buckets + index);
        if (currentNode == nullptr) {
            return false;
        }

        // 目标节点在链表头部
        if (self->keyEquals(currentNode->key, inKey)) {
            // 更新桶头节点为下一节点
            *(buckets + index) = currentNode->nextNode;

            // 释放头节点键
            if (currentNode->key) {
                free(currentNode->key);
            }

            // 释放头节点值
            if (currentNode->value) {
                free(currentNode->value);
            }

            // 释放头节点
            free(currentNode);

            self->size--;

            return true;
        }

        // 目标节点在链表中间，则头节点必定非空且不匹配
        // 从头节点的下一节点开始遍历，上一节点则是以头节点开始
        auto* prevNode = currentNode;
        currentNode = currentNode->nextNode;
        while (currentNode) {
            // 找到匹配节点
            if (self->keyEquals(currentNode->key, inKey)) {
                // 前驱跳过当前节点
                prevNode->nextNode = currentNode->nextNode;

                // 释放当前节点键
                if (currentNode->key) {
                    free(currentNode->key);
                }

                // 释放当前节点值
                if (currentNode->value) {
                    free(currentNode->value);
                }

                // 释放当前节点
                free(currentNode);

                self->size--;

                return true;
            }

            // 继续下一轮查找匹配
            prevNode = currentNode;
            currentNode = currentNode->nextNode;
        }

        return false;
    }
    auto ChainHashMap_removeSlot(ChainHashMap* self, const void* inKey, void** keySlotOut, void** valueSlotOut) -> bool {
        if (!self || !inKey || !keySlotOut || !valueSlotOut || self->size == 0) {
            return false;
        }

        *keySlotOut = nullptr;
        *valueSlotOut = nullptr;

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 计算哈希值并映射到桶索引范围
        const u64 hash = self->hash(inKey);
        const usize index = hash & (self->capacity - 1);

        // 取出该桶的链表头节点
        auto* currentNode = *(buckets + index);
        if (currentNode == nullptr) {
            return false;
        }

        // 目标节点在链表头部
        if (self->keyEquals(currentNode->key, inKey)) {
            // 更新桶头节点为下一节点
            *(buckets + index) = currentNode->nextNode;

            // 保存 key 和 value 指针，交给调用者析构 + 释放
            *keySlotOut = currentNode->key;
            *valueSlotOut = currentNode->value;

            // 释放节点结构体本身，但不释放 key/value 内存
            free(currentNode);

            self->size--;

            return true;
        }

        // 目标节点在链表中间，从头节点的下一节点开始遍历
        auto* prevNode = currentNode;
        currentNode = currentNode->nextNode;
        while (currentNode) {
            // 找到匹配节点
            if (self->keyEquals(currentNode->key, inKey)) {
                // 前驱跳过当前节点
                prevNode->nextNode = currentNode->nextNode;

                // 保存 key 和 value 指针，交给调用者析构 + 释放
                *keySlotOut = currentNode->key;
                *valueSlotOut = currentNode->value;

                // 释放节点结构体本身，但不释放 key/value 内存
                free(currentNode);

                self->size--;

                return true;
            }

            // 继续下一轮查找匹配
            prevNode = currentNode;
            currentNode = currentNode->nextNode;
        }

        return false;
    }
    auto ChainHashMap_clear(ChainHashMap* self) -> bool {
        if (!self || self->size == 0) {
            return false;
        }

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 遍历所有桶，释放每个桶的链表节点
        for (usize i = 0; i < self->capacity; ++i) {
            auto* currentNode = *(buckets + i);
            while (currentNode) {
                auto* const nextNode = currentNode->nextNode;

                // 释放当前节点键
                if (currentNode->key) {
                    free(currentNode->key);
                }

                // 释放当前节点值
                if (currentNode->value) {
                    free(currentNode->value);
                }

                // 释放当前节点
                free(currentNode);

                currentNode = nextNode;
            }

            // 将当前桶置为空桶空链表
            *(buckets + i) = nullptr;
        }

        // 更新状态
        self->size = 0;

        return true;
    }

    auto ChainHashMap_find(const ChainHashMap* self, const void* inKey, void** valueOut) -> bool {
        if (!self || !inKey || !valueOut || self->size == 0) {
            return false;
        }

        // 取出连续桶指针数组
        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        // 计算哈希值并映射到桶索引范围
        const u64 hash = self->hash(inKey);
        const usize index = hash & (self->capacity - 1);

        // 取出该桶的链表头节点
        const auto* currentNode = *(buckets + index);

        // 遍历链表查找匹配的键
        while (currentNode) {
            if (self->keyEquals(currentNode->key, inKey)) {
                *valueOut = currentNode->value;
                return true;
            }

            // 继续下一轮查找匹配的键
            currentNode = currentNode->nextNode;
        }

        return false;
    }

    auto ChainHashMap_headAt(const ChainHashMap* self, const usize index) -> ChainHashMapNode* {
        if (!self || index >= self->capacity) {
            return nullptr;
        }

        auto** const buckets = (ChainHashMapNode**) ArrayList_elements(self->buckets);

        return *(buckets + index);
    }

    auto ChainHashMap_reserve(ChainHashMap* self, const usize newCapacity) -> bool {
        if (!self || newCapacity <= self->capacity) {
            return false;
        }

        // 确保新容量至少为 DEFAULT_CAPACITY，并调整为 2 的幂
        usize capacity = 1;
        while (capacity < newCapacity || capacity < DEFAULT_CAPACITY) {
            capacity <<= 1;
        }

        // 创建新的桶集
        auto* const newBuckets = ArrayList_construct(capacity, sizeof(void*));
        if (!newBuckets) {
            return false;
        }

        // 将桶数组填充为 nullptr，即所有桶都是空桶空链表
        for (usize i = 0; i < capacity; ++i) {
            if (!ArrayList_push(newBuckets, nullptr)) {
                ArrayList_destruct(newBuckets);
                return false;
            }
        }

        // 保存旧的桶集
        auto* const oldBuckets = self->buckets;

        // 临时切换为新桶集，临时重置元素数量
        self->buckets = newBuckets;
        self->capacity = capacity;
        self->size = 0;

        // 取出新旧桶集内部的元素数组
        auto** const oldElements = (ChainHashMapNode**) ArrayList_elements(oldBuckets);
        auto** const newElements = (ChainHashMapNode**) ArrayList_elements(newBuckets);

        // 遍历旧元素数组，将所有节点迁移到新元素数组
        for (usize i = 0; i < ArrayList_size(oldBuckets); ++i) {
            auto* currentNode = *(oldElements + i);

            // 遍历当前桶的链表
            while (currentNode) {
                auto* const nextNode = currentNode->nextNode;

                // 计算哈希值并映射到新桶索引范围
                const u64 hash = self->hash(currentNode->key);
                const usize index = hash & (self->capacity - 1);

                // 将新节点指向新桶的原头节点
                currentNode->nextNode = *(newElements + index);

                // 更新新桶头节点为当前节点
                *(newElements + index) = currentNode;

                self->size++;

                // 继续下一轮迁移
                currentNode = nextNode;
            }
        }

        // 销毁旧桶集，其中旧节点已经迁移
        ArrayList_destruct(oldBuckets);

        return true;
    }

    auto ChainHashMap_capacity(const ChainHashMap* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->capacity;
    }
    auto ChainHashMap_size(const ChainHashMap* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->size;
    }
    auto ChainHashMap_keySize(const ChainHashMap* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->keySize;
    }
    auto ChainHashMap_valueSize(const ChainHashMap* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->valueSize;
    }

    auto ChainHashMap_isEmpty(const ChainHashMap* self) -> bool {
        if (!self) {
            return true;
        }

        return self->size == 0;
    }

    auto ChainHashMapNode_key(const ChainHashMapNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->key;
    }
    auto ChainHashMapNode_value(const ChainHashMapNode* node) -> void* {
        if (!node) {
            return nullptr;
        }

        return node->value;
    }
    auto ChainHashMapNode_next(const ChainHashMapNode* node) -> ChainHashMapNode* {
        if (!node) {
            return nullptr;
        }

        return node->nextNode;
    }
}