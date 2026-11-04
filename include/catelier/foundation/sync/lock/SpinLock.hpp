#pragma once
#include "../../../../../src/catelier/foundation/sync/lock/SpinLock.hpp"

namespace Catelier::include::foundation::sync::lock {
    class SpinLock {
        public:
            class Guard {
                public:
                    explicit Guard(SpinLock& inSpinLock) : spinBlockingLock(&inSpinLock), owns(inSpinLock.lock()) {}
                    ~Guard() {
                        if (this->owns) {
                            this->spinBlockingLock->unlock();
                        }
                    }
                    Guard(const Guard&) = delete;
                    Guard(Guard&& other) noexcept : spinBlockingLock(other.spinBlockingLock), owns(other.owns) {
                        other.spinBlockingLock = nullptr;
                        other.owns = false;
                    }
                    Guard& operator = (const Guard&) = delete;
                    Guard& operator = (Guard&& other) noexcept {
                        if (this != &other) {
                            if (this->owns && this->spinBlockingLock) {
                                this->spinBlockingLock->unlock();
                            }

                            this->spinBlockingLock = other.spinBlockingLock;
                            this->owns = other.owns;

                            other.spinBlockingLock = nullptr;
                            other.owns = false;
                        }

                        return *this;
                    }

                    auto ownsLock() const -> bool {
                        return this->owns;
                    }
                    auto unlock() -> bool {
                        if (!this->owns || !this->spinBlockingLock) {
                            return false;
                        }

                        this->owns = false;

                        return this->spinBlockingLock->unlock();
                    }

                private:
                    SpinLock* spinBlockingLock;
                    bool owns;
            };

            class TryGuard {
                public:
                    explicit TryGuard(SpinLock& inSpinLock) : spinBlockingLock(&inSpinLock), owns(inSpinLock.tryLock()) {}
                    ~TryGuard() {
                        if (this->owns) {
                            this->spinBlockingLock->unlock();
                        }
                    }
                    TryGuard(const TryGuard&) = delete;
                    TryGuard(TryGuard&& other) noexcept : spinBlockingLock(other.spinBlockingLock), owns(other.owns) {
                        other.spinBlockingLock = nullptr;
                        other.owns = false;
                    }
                    TryGuard& operator = (const TryGuard&) = delete;
                    TryGuard& operator = (TryGuard&& other) noexcept {
                        if (this != &other) {
                            if (this->owns && this->spinBlockingLock) {
                                this->spinBlockingLock->unlock();
                            }

                            this->spinBlockingLock = other.spinBlockingLock;
                            this->owns = other.owns;

                            other.spinBlockingLock = nullptr;
                            other.owns = false;
                        }

                        return *this;
                    }

                    auto ownsLock() const -> bool {
                        return this->owns;
                    }
                    auto unlock() -> bool {
                        if (!this->owns || !this->spinBlockingLock) {
                            return false;
                        }

                        this->owns = false;

                        return this->spinBlockingLock->unlock();
                    }

                private:
                    SpinLock* spinBlockingLock;
                    bool owns;
            };

            auto isValid() const -> bool {
                return this->handle.valid;
            }

            explicit SpinLock() {
                src::foundation::sync::lock::SpinLock_init(&this->handle);
            }
            ~SpinLock() {
                if (this->handle.valid) {
                    src::foundation::sync::lock::SpinLock_destroy(&this->handle);
                }
            }
            SpinLock(const SpinLock&) = delete;
            SpinLock(SpinLock&& other) noexcept {
                this->handle = other.handle;

                other.handle.valid = false;
            }
            SpinLock& operator = (const SpinLock&) = delete;
            SpinLock& operator = (SpinLock&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::sync::lock::SpinLock_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto lock() -> bool {
                return src::foundation::sync::lock::SpinLock_lock(&this->handle);
            }
            auto unlock() -> bool {
                return src::foundation::sync::lock::SpinLock_unlock(&this->handle);
            }
            auto tryLock() -> bool {
                return src::foundation::sync::lock::SpinLock_tryLock(&this->handle);
            }

            auto rawHandle() -> src::foundation::sync::lock::SpinLock* {
                return &this->handle;
            }
            auto rawHandle() const -> const src::foundation::sync::lock::SpinLock* {
                return &this->handle;
            }

        private:
            src::foundation::sync::lock::SpinLock handle = {};
    };
}