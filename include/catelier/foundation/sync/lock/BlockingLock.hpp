#pragma once
#include "../../../../../src/catelier/foundation/sync/lock/BlockingLock.hpp"

namespace Catelier::include::foundation::sync::lock {
    class BlockingLock {
        public:
            class Guard {
                public:
                    explicit Guard(BlockingLock& inBlockingLock) : mutex(&inBlockingLock), owns(inBlockingLock.lock()) {}
                    ~Guard() {
                        if (this->owns) {
                            this->mutex->unlock();
                        }
                    }
                    Guard(const Guard&) = delete;
                    Guard(Guard&& other) noexcept : mutex(other.mutex), owns(other.owns) {
                        other.mutex = nullptr;
                        other.owns = false;
                    }
                    Guard& operator = (const Guard&) = delete;
                    Guard& operator = (Guard&& other) noexcept {
                        if (this != &other) {
                            if (this->owns && this->mutex) {
                                this->mutex->unlock();
                            }

                            this->mutex = other.mutex;
                            this->owns = other.owns;

                            other.mutex = nullptr;
                            other.owns = false;
                        }

                        return *this;
                    }

                    auto ownsLock() const -> bool {
                        return this->owns;
                    }
                    auto unlock() -> bool {
                        if (!this->owns || !this->mutex) {
                            return false;
                        }

                        this->owns = false;

                        return this->mutex->unlock();
                    }

                private:
                    BlockingLock* mutex;
                    bool owns;
            };

            class TryGuard {
                public:
                    explicit TryGuard(BlockingLock& inBlockingLock) : mutex(&inBlockingLock), owns(inBlockingLock.tryLock()) {}
                    ~TryGuard() {
                        if (this->owns) {
                            this->mutex->unlock();
                        }
                    }
                    TryGuard(const TryGuard&) = delete;
                    TryGuard(TryGuard&& other) noexcept : mutex(other.mutex), owns(other.owns) {
                        other.mutex = nullptr;
                        other.owns = false;
                    }
                    TryGuard& operator = (const TryGuard&) = delete;
                    TryGuard& operator = (TryGuard&& other) noexcept {
                        if (this != &other) {
                            if (this->owns && this->mutex) {
                                this->mutex->unlock();
                            }

                            this->mutex = other.mutex;
                            this->owns = other.owns;

                            other.mutex = nullptr;
                            other.owns = false;
                        }

                        return *this;
                    }

                    auto ownsLock() const -> bool {
                        return this->owns;
                    }
                    auto unlock() -> bool {
                        if (!this->owns || !this->mutex) {
                            return false;
                        }

                        this->owns = false;

                        return this->mutex->unlock();
                    }

                private:
                    BlockingLock* mutex;
                    bool owns;
            };

            auto isValid() const -> bool {
                return this->handle.valid;
            }

            explicit BlockingLock() {
                src::foundation::sync::lock::BlockingLock_init(&this->handle);
            }
            ~BlockingLock() {
                if (this->handle.valid) {
                    src::foundation::sync::lock::BlockingLock_destroy(&this->handle);
                }
            }
            BlockingLock(const BlockingLock&) = delete;
            BlockingLock(BlockingLock&& other) noexcept {
                this->handle = other.handle;

                other.handle.valid = false;
            }
            BlockingLock& operator = (const BlockingLock&) = delete;
            BlockingLock& operator = (BlockingLock&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::sync::lock::BlockingLock_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto lock() -> bool {
                return src::foundation::sync::lock::BlockingLock_lock(&this->handle);
            }
            auto unlock() -> bool {
                return src::foundation::sync::lock::BlockingLock_unlock(&this->handle);
            }
            auto tryLock() -> bool {
                return src::foundation::sync::lock::BlockingLock_tryLock(&this->handle);
            }

            auto rawHandle() -> src::foundation::sync::lock::BlockingLock* {
                return &this->handle;
            }
            auto rawHandle() const -> const src::foundation::sync::lock::BlockingLock* {
                return &this->handle;
            }

        private:
            src::foundation::sync::lock::BlockingLock handle = {};
    };
}