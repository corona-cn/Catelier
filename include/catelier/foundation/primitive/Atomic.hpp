#pragma once
#include "../../../../src/catelier/foundation/primitive/Atomic.hpp"

#include <cstring>

namespace Catelier::foundation::primitive {
    template<typename Type>
    class Atomic {
        public:
            explicit Atomic(Type value = Type{}) {
                u64 bits = 0;
                std::memcpy(&bits, &value, sizeof(Type));

                src::foundation::primitive::Atomic_init(&this->handle, bits);
            }
            ~Atomic() {
                src::foundation::primitive::Atomic_destroy(&this->handle);
            }

            Atomic(const Atomic&) = delete;
            Atomic(Atomic&&) = delete;
            Atomic& operator = (const Atomic&) = delete;
            Atomic& operator = (Atomic&&) = delete;

            auto load(const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::MEMORY_ORDER_SEQ_CST) const -> Type {
                const u64 bits = src::foundation::primitive::Atomic_load(&this->handle, order);

                Type result;
                std::memcpy(&result, &bits, sizeof(Type));

                return result;
            }
            auto store(Type value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::MEMORY_ORDER_SEQ_CST) -> void {
                u64 bits = 0;
                std::memcpy(&bits, &value, sizeof(Type));

                src::foundation::primitive::Atomic_store(&this->handle, bits, order);
            }
            auto exchange(Type value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::MEMORY_ORDER_SEQ_CST) -> Type {
                u64 bits = 0;
                std::memcpy(&bits, &value, sizeof(Type));

                const u64 oldBits = src::foundation::primitive::Atomic_exchange(&this->handle, bits, order);

                Type result;
                std::memcpy(&result, &oldBits, sizeof(Type));

                return result;
            }
            auto compareExchange(Type* expected, Type desired, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::MEMORY_ORDER_SEQ_CST) -> bool {
                u64 expectedBits = 0;
                std::memcpy(&expectedBits, expected, sizeof(Type));

                u64 desiredBits = 0;
                std::memcpy(&desiredBits, &desired, sizeof(Type));

                const bool result = src::foundation::primitive::Atomic_compareExchange(&this->handle, &expectedBits, desiredBits, order);

                std::memcpy(expected, &expectedBits, sizeof(Type));

                return result;
            }

            auto fetchAdd(Type delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::MEMORY_ORDER_SEQ_CST) -> Type {
                u64 deltaBits = 0;
                std::memcpy(&deltaBits, &delta, sizeof(Type));

                const u64 oldBits = src::foundation::primitive::Atomic_fetchAdd(&this->handle, deltaBits, order);

                Type result;
                std::memcpy(&result, &oldBits, sizeof(Type));

                return result;
            }
            auto fetchSub(Type delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::MEMORY_ORDER_SEQ_CST) -> Type {
                u64 deltaBits = 0;
                std::memcpy(&deltaBits, &delta, sizeof(Type));

                const u64 oldBits = src::foundation::primitive::Atomic_fetchSub(&this->handle, deltaBits, order);

                Type result;
                std::memcpy(&result, &oldBits, sizeof(Type));

                return result;
            }

        private:
            src::foundation::primitive::Atomic handle = {};
    };
}