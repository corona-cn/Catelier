#include "BitArray.hpp"

#include <cstdlib>
#include <cstring>

namespace Catelier::src::foundation {
    namespace {
        constexpr usize DEFAULT_CAPACITY = 8;
        constexpr usize BYTE_BITS = 8;
    }

    typedef struct BitArray {
        u8* bytes;
        usize capacity;
        usize byteCount;
    } BitArray;

    auto BitArray_construct(const usize initialCapacity) -> BitArray* {
        usize capacity = initialCapacity;
        if (capacity < DEFAULT_CAPACITY) {
            capacity = DEFAULT_CAPACITY;
        }

        // 向上取整到 8 的倍数，并换算为字节数
        const usize byteCount = (capacity + BYTE_BITS - 1) / BYTE_BITS;

        auto* const self = (BitArray*) malloc(sizeof(BitArray));
        if (!self) {
            return nullptr;
        }

        // 分配字节数组
        self->bytes = (u8*) malloc(sizeof(u8) * byteCount);
        if (!self->bytes) {
            free(self);
            return nullptr;
        }

        // 将所有位初始化为 0
        memset(self->bytes, 0, sizeof(u8) * byteCount);

        self->capacity = capacity;
        self->byteCount = byteCount;

        return self;
    }
    auto BitArray_destruct(BitArray* self) -> bool {
        if (!self) {
            return false;
        }

        free(self->bytes);
        free(self);

        return true;
    }

    auto BitArray_copy(const BitArray* self) -> BitArray* {
        if (!self) {
            return nullptr;
        }

        // 复用构造函数分配新对象
        auto* const newSelf = BitArray_construct(self->capacity);
        if (!newSelf) {
            return nullptr;
        }

        // 直接复制字节数组内容，覆盖构造时的清零
        memcpy(newSelf->bytes, self->bytes, sizeof(u8) * self->byteCount);

        return newSelf;
    }
    auto BitArray_move(BitArray* self) -> BitArray* {
        if (!self) {
            return nullptr;
        }

        auto* const newSelf = (BitArray*) malloc(sizeof(BitArray));
        if (!newSelf) {
            return nullptr;
        }

        newSelf->bytes = self->bytes;
        newSelf->capacity = self->capacity;
        newSelf->byteCount = self->byteCount;

        self->bytes = nullptr;
        self->capacity = 0;
        self->byteCount = 0;

        return newSelf;
    }

    // 比特数组的核心操作函数
    // 核心是通过两次位操作，先锁定目标字节的目标位
    // 然后通过根据比特偏移构造的特殊的字节掩码，去与目标字节进行按位操作
    // 最后得到精确位操作后的字节
    auto BitArray_setOne(const BitArray* self, const usize index) -> bool {
        if (!self || index >= self->capacity) {
            return false;
        }

        const usize byteIndex = index >> 3;
        const usize bitOffset = index & 7;

        // 构造掩码：1 左移 bitOffset 位后，只有目标位是 1，其他位都是 0
        // 将掩码与目标字节按位或：
        //   - 掩码为 1 的位（目标位）：无论原值如何，结果强制为 1
        //   - 掩码为 0 的位（其他位）：0 与任何位或运算都保持原值不变
        const int mask = 1 << bitOffset;
        *(self->bytes + byteIndex) |= mask;

        return true;
    }
    auto BitArray_fillOne(const BitArray* self) -> bool {
        if (!self) {
            return false;
        }

        // 将字节数组整体填满为 0xFF，此时所有位（含填充位）都是 1
        // 若容量不是 8 的倍数，最后一个字节存在超出容量的填充位
        // 则需要用有效位掩码清除它们，保证填充位恒为 0
        memset(self->bytes, 0xFF, sizeof(u8) * self->byteCount);

        const usize remainder = self->capacity & 7;
        if (remainder != 0) {
            const usize lastByteIndex = self->byteCount - 1;
            const u8 validMask = (u8) ((1 << remainder) - 1);
            *(self->bytes + lastByteIndex) &= validMask;
        }

        return true;
    }
    auto BitArray_setZero(const BitArray* self, const usize index) -> bool {
        if (!self || index >= self->capacity) {
            return false;
        }

        const usize byteIndex = index >> 3;
        const usize bitOffset = index & 7;

        // 构造掩码：1 左移 bitOffset 位后，只有目标位是 1，其他位都是 0
        // 将掩码取反后与目标字节按位与：
        //   - 掩码为 0 的位（目标位）：无论原值如何，结果强制为 0
        //   - 掩码为 1 的位（其他位）：1 与任何位与运算都保持原值不变
        const int mask = 1 << bitOffset;
        *(self->bytes + byteIndex) &= ~mask;

        return true;
    }
    auto BitArray_fillZero(const BitArray* self) -> bool {
        if (!self) {
            return false;
        }

        // 将字节数组整体清零，所有位（含填充位）都变回 0
        // 填充位的目标状态本就是 0，无需额外处理
        memset(self->bytes, 0, sizeof(u8) * self->byteCount);

        return true;
    }
    auto BitArray_toggle(const BitArray* self, const usize index) -> bool {
        if (!self || index >= self->capacity) {
            return false;
        }

        const usize byteIndex = index >> 3;
        const usize bitOffset = index & 7;

        // 构造掩码：1 左移 bitOffset 位后，只有目标位是 1，其他位都是 0
        // 将掩码与目标字节按位异或：
        //   - 掩码为 1 的位（目标位）：原值翻转
        //   - 掩码为 0 的位（其他位）：0 与任何位异或运算都保持原值不变
        const int mask = 1 << bitOffset;
        *(self->bytes + byteIndex) ^= mask;

        return true;
    }
    auto BitArray_toggleAll(const BitArray* self) -> bool {
        if (!self) {
            return false;
        }

        // 逐字节按位取反，等价于翻转所有位
        // 若容量不是 8 的倍数，取反会把填充位也翻转成 1
        // 需要用有效位掩码清除它们，保证填充位恒为 0
        for (usize i = 0; i < self->byteCount; ++i) {
            *(self->bytes + i) = (u8) ~*(self->bytes + i);
        }

        const usize remainder = self->capacity & 7;
        if (remainder != 0) {
            const usize lastByteIndex = self->byteCount - 1;
            const u8 validMask = (u8) ((1 << remainder) - 1);
            *(self->bytes + lastByteIndex) &= validMask;
        }

        return true;
    }

    auto BitArray_get(const BitArray* self, const usize index, bool* outValue) -> bool {
        if (!self || !outValue || index >= self->capacity) {
            return false;
        }

        const usize byteIndex = index >> 3;
        const usize bitOffset = index & 7;

        // 构造掩码：1 左移 bitOffset 位后，只有目标位是 1，其他位都是 0
        // 将掩码与目标字节按位与：
        //   - 掩码为 1 的位（目标位）：结果为原值本身
        //   - 掩码为 0 的位（其他位）：结果恒为 0
        // 所以按位与的结果要么是 0，要么是非 0，用 != 0 归一化为 bool
        const int mask = 1 << bitOffset;
        *outValue = (*(self->bytes + byteIndex) & mask) != 0;

        return true;
    }

    auto BitArray_findFirstOne(const BitArray* self, usize* outIndex) -> bool {
        if (!self || !outIndex) {
            return false;
        }

        // 逐字节扫描，找到第一个非零字节
        // 填充位恒为 0，因此非零字节中必然存在至少一个有效位是 1
        for (usize i = 0; i < self->byteCount; ++i) {
            const u8 byte = *(self->bytes + i);
            if (byte == 0) {
                continue;
            }

            // 在非零字节内找最低位的 1
            // 不断右移直到最低位是 1，移位次数即字节内偏移
            usize bitOffset = 0;
            u8 temp = byte;
            while ((temp & 1) == 0) {
                temp >>= 1;
                ++bitOffset;
            }

            *outIndex = i * 8 + bitOffset;

            return true;
        }

        return false;
    }
    auto BitArray_findFirstZero(const BitArray* self, usize* outIndex) -> bool {
        if (!self || !outIndex) {
            return false;
        }

        // 计算最后一个字节中有效位的数量（余数）
        const usize remainder = self->capacity & 7;

        for (usize i = 0; i < self->byteCount; ++i) {
            const u8 byte = *(self->bytes + i);

            // 计算当前字节的有效位掩码：
            //   - 非最后一个字节：所有 8 位都有效，掩码是 0xFF
            //   - 最后一个字节：只有低 remainder 位有效，掩码是 (1 << remainder) - 1
            // 当容量是 8 的倍数时 remainder == 0，最后一个字节也是全有效
            u8 validMask = 0xFF;
            if (i == self->byteCount - 1 && remainder != 0) {
                validMask = (u8) ((1 << remainder) - 1);
            }

            // 只在有效位范围内查找 0
            // 取反后与有效位掩码按位与，得到"有效位中是 0 的那些位"
            // 若结果非零，说明本字节存在 0，且填充位已被掩码过滤掉
            const u8 missing = (u8) (~byte & validMask);
            if (missing == 0) {
                continue;
            }

            // 在 missing 中找到最低位的 1（即原字节中最低位的 0）
            // 不断右移直到最低位是 1，移位次数即字节内偏移
            usize bitOffset = 0;
            u8 temp = missing;
            while ((temp & 1) == 0) {
                temp >>= 1;
                ++bitOffset;
            }

            *outIndex = i * 8 + bitOffset;

            return true;
        }

        return false;
    }

    auto BitArray_bytes(const BitArray* self) -> u8* {
        if (!self) {
            return nullptr;
        }

        return self->bytes;
    }
    auto BitArray_capacity(const BitArray* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->capacity;
    }
    auto BitArray_byteCount(const BitArray* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->byteCount;
    }

    auto BitArray_countOnes(const BitArray* self) -> usize {
        if (!self) {
            return 0;
        }

        // 逐字节统计 1 的个数
        // Brian Kernighan 算法：每次 byte &= (byte - 1) 都会消掉最低位的 1
        // 循环次数恰好等于字节中 1 的个数
        // 其中填充位恒为 0，不会贡献任何 1，因此无需额外过滤
        usize count = 0;
        for (usize i = 0; i < self->byteCount; ++i) {
            u8 byte = *(self->bytes + i);
            while (byte != 0) {
                // 这里 byte - 1 会使最低为的 1 变成 0，并使最低为的 1 后面的所有位都变成 1
                // 再与原字节按位与，这样只有最低位的 1 变成了 0，其他位保持不变
                // 达到每轮循环抵消一个 1 的效果
                byte &= (u8) (byte - 1);
                ++count;
            }
        }

        return count;
    }
    auto BitArray_countZeros(const BitArray* self) -> usize {
        if (!self) {
            return 0;
        }

        return self->capacity - BitArray_countOnes(self);
    }
}