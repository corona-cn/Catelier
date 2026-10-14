#pragma once
#include "../../../../src/catelier/foundation/primitive/Atomic.hpp"

namespace Catelier::include::foundation::primitive {
    class Atomic8 {
        public:
            explicit Atomic8(const u8 value = 0) {
                this->storage = value;
            }
            Atomic8(const Atomic8&) = delete;
            Atomic8(Atomic8&&) = delete;
            Atomic8& operator = (const Atomic8&) = delete;
            Atomic8& operator = (Atomic8&&) = delete;

            auto load(const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) const -> u8 {
                return src::foundation::primitive::Atomic8_load(&this->storage, order);
            }
            auto store(const u8 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> void {
                src::foundation::primitive::Atomic8_store(&this->storage, value, order);
            }
            auto exchange(const u8 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u8 {
                return src::foundation::primitive::Atomic8_exchange(&this->storage, value, order);
            }
            auto compareExchange(u8* expected, const u8 desired, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> bool {
                return src::foundation::primitive::Atomic8_compareExchange(&this->storage, expected, desired, order);
            }
            auto fetchAdd(const u8 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u8 {
                return src::foundation::primitive::Atomic8_fetchAdd(&this->storage, delta, order);
            }
            auto fetchSub(const u8 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u8 {
                return src::foundation::primitive::Atomic8_fetchSub(&this->storage, delta, order);
            }

        private:
            u8 storage;
    };

    class Atomic16 {
        public:
            explicit Atomic16(const u16 value = 0) {
                this->storage = value;
            }
            Atomic16(const Atomic16&) = delete;
            Atomic16(Atomic16&&) = delete;
            Atomic16& operator = (const Atomic16&) = delete;
            Atomic16& operator = (Atomic16&&) = delete;

            auto load(const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) const -> u16 {
                return src::foundation::primitive::Atomic16_load(&this->storage, order);
            }
            auto store(const u16 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> void {
                src::foundation::primitive::Atomic16_store(&this->storage, value, order);
            }
            auto exchange(const u16 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u16 {
                return src::foundation::primitive::Atomic16_exchange(&this->storage, value, order);
            }
            auto compareExchange(u16* expected, const u16 desired, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> bool {
                return src::foundation::primitive::Atomic16_compareExchange(&this->storage, expected, desired, order);
            }
            auto fetchAdd(const u16 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u16 {
                return src::foundation::primitive::Atomic16_fetchAdd(&this->storage, delta, order);
            }
            auto fetchSub(const u16 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u16 {
                return src::foundation::primitive::Atomic16_fetchSub(&this->storage, delta, order);
            }

        private:
            u16 storage;
    };

    class Atomic32 {
        public:
            explicit Atomic32(const u32 value = 0) {
                this->storage = value;
            }
            Atomic32(const Atomic32&) = delete;
            Atomic32(Atomic32&&) = delete;
            Atomic32& operator = (const Atomic32&) = delete;
            Atomic32& operator = (Atomic32&&) = delete;

            auto load(const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) const -> u32 {
                return src::foundation::primitive::Atomic32_load(&this->storage, order);
            }
            auto store(const u32 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> void {
                src::foundation::primitive::Atomic32_store(&this->storage, value, order);
            }
            auto exchange(const u32 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u32 {
                return src::foundation::primitive::Atomic32_exchange(&this->storage, value, order);
            }
            auto compareExchange(u32* expected, const u32 desired, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> bool {
                return src::foundation::primitive::Atomic32_compareExchange(&this->storage, expected, desired, order);
            }
            auto fetchAdd(const u32 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u32 {
                return src::foundation::primitive::Atomic32_fetchAdd(&this->storage, delta, order);
            }
            auto fetchSub(const u32 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u32 {
                return src::foundation::primitive::Atomic32_fetchSub(&this->storage, delta, order);
            }

        private:
            u32 storage;
    };

    class Atomic64 {
        public:
            explicit Atomic64(const u64 value = 0) {
                this->storage = value;
            }
            Atomic64(const Atomic64&) = delete;
            Atomic64(Atomic64&&) = delete;
            Atomic64& operator = (const Atomic64&) = delete;
            Atomic64& operator = (Atomic64&&) = delete;

            auto load(const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) const -> u64 {
                return src::foundation::primitive::Atomic64_load(&this->storage, order);
            }
            auto store(const u64 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> void {
                src::foundation::primitive::Atomic64_store(&this->storage, value, order);
            }
            auto exchange(const u64 value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u64 {
                return src::foundation::primitive::Atomic64_exchange(&this->storage, value, order);
            }
            auto compareExchange(u64* expected, const u64 desired, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> bool {
                return src::foundation::primitive::Atomic64_compareExchange(&this->storage, expected, desired, order);
            }
            auto fetchAdd(const u64 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u64 {
                return src::foundation::primitive::Atomic64_fetchAdd(&this->storage, delta, order);
            }
            auto fetchSub(const u64 delta, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> u64 {
                return src::foundation::primitive::Atomic64_fetchSub(&this->storage, delta, order);
            }

        private:
            u64 storage;
    };

    class AtomicBool {
        public:
            explicit AtomicBool(const bool value = false) {
                this->storage = value;
            }
            AtomicBool(const AtomicBool&) = delete;
            AtomicBool(AtomicBool&&) = delete;
            AtomicBool& operator = (const AtomicBool&) = delete;
            AtomicBool& operator = (AtomicBool&&) = delete;

            auto load(const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) const -> bool {
                return src::foundation::primitive::AtomicBool_load(&this->storage, order);
            }
            auto store(const bool value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> void {
                src::foundation::primitive::AtomicBool_store(&this->storage, value, order);
            }
            auto exchange(const bool value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> bool {
                return src::foundation::primitive::AtomicBool_exchange(&this->storage, value, order);
            }
            auto compareExchange(bool* expected, const bool desired, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> bool {
                return src::foundation::primitive::AtomicBool_compareExchange(&this->storage, expected, desired, order);
            }

        private:
            bool storage;
    };

    template<typename Type>
    class AtomicPtr {
        public:
            explicit AtomicPtr(Type* value = nullptr) {
                this->storage = value;
            }
            AtomicPtr(const AtomicPtr&) = delete;
            AtomicPtr(AtomicPtr&&) = delete;
            AtomicPtr& operator = (const AtomicPtr&) = delete;
            AtomicPtr& operator = (AtomicPtr&&) = delete;

            auto load(const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) const -> Type* {
                return (Type*) src::foundation::primitive::AtomicPtr_load((void* const*) &this->storage, order);
            }
            auto store(Type* value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> void {
                src::foundation::primitive::AtomicPtr_store((void**) &this->storage, (void*) value, order);
            }
            auto exchange(Type* value, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> Type* {
                return (Type*) src::foundation::primitive::AtomicPtr_exchange((void**) &this->storage, (void*) value, order);
            }
            auto compareExchange(Type** expected, Type* desired, const src::foundation::primitive::MemoryOrder order = src::foundation::primitive::SEQUENTIAL_CONSISTENT) -> bool {
                return src::foundation::primitive::AtomicPtr_compareExchange((void**) &this->storage, (void**) expected, (void*) desired, order);
            }

        private:
            Type* storage;
    };
}