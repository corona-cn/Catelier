#pragma once
#include "../../../../src/catelier/foundation/primitive/SpinMutex.hpp"

namespace Catelier::include::foundation::primitive {
    class SpinMutex {
        public:
            class Guard {
                public:
                    explicit Guard(SpinMutex& inSpinMutex) : spinMutex(&inSpinMutex), owns(inSpinMutex.lock()) {}
                    ~Guard() {
                        if (this->owns) {
                            this->spinMutex->unlock();
                        }
                    }
                    Guard(const Guard&) = delete;
                    Guard(Guard&& other) noexcept : spinMutex(other.spinMutex), owns(other.owns) {
                        other.spinMutex = nullptr;
                        other.owns = false;
                    }
                    Guard& operator = (const Guard&) = delete;
                    Guard& operator = (Guard&& other) noexcept {
                        if (this != &other) {
                            if (this->owns && this->spinMutex) {
                                this->spinMutex->unlock();
                            }

                            this->spinMutex = other.spinMutex;
                            this->owns = other.owns;

                            other.spinMutex = nullptr;
                            other.owns = false;
                        }

                        return *this;
                    }

                    auto ownsLock() const -> bool {
                        return this->owns;
                    }
                    auto unlock() -> bool {
                        if (!this->owns || !this->spinMutex) {
                            return false;
                        }

                        this->owns = false;

                        return this->spinMutex->unlock();
                    }

                private:
                    SpinMutex* spinMutex;
                    bool owns;
            };

            class TryGuard {
                public:
                    explicit TryGuard(SpinMutex& inSpinMutex) : spinMutex(&inSpinMutex), owns(inSpinMutex.tryLock()) {}
                    ~TryGuard() {
                        if (this->owns) {
                            this->spinMutex->unlock();
                        }
                    }
                    TryGuard(const TryGuard&) = delete;
                    TryGuard(TryGuard&& other) noexcept : spinMutex(other.spinMutex), owns(other.owns) {
                        other.spinMutex = nullptr;
                        other.owns = false;
                    }
                    TryGuard& operator = (const TryGuard&) = delete;
                    TryGuard& operator = (TryGuard&& other) noexcept {
                        if (this != &other) {
                            if (this->owns && this->spinMutex) {
                                this->spinMutex->unlock();
                            }

                            this->spinMutex = other.spinMutex;
                            this->owns = other.owns;

                            other.spinMutex = nullptr;
                            other.owns = false;
                        }

                        return *this;
                    }

                    auto ownsLock() const -> bool {
                        return this->owns;
                    }
                    auto unlock() -> bool {
                        if (!this->owns || !this->spinMutex) {
                            return false;
                        }

                        this->owns = false;

                        return this->spinMutex->unlock();
                    }

                private:
                    SpinMutex* spinMutex;
                    bool owns;
            };

            auto isValid() const -> bool {
                return this->handle.valid;
            }

            explicit SpinMutex() {
                src::foundation::primitive::SpinMutex_init(&this->handle);
            }
            ~SpinMutex() {
                if (this->handle.valid) {
                    src::foundation::primitive::SpinMutex_destroy(&this->handle);
                }
            }
            SpinMutex(const SpinMutex&) = delete;
            SpinMutex(SpinMutex&& other) noexcept {
                this->handle = other.handle;

                other.handle.valid = false;
            }
            SpinMutex& operator = (const SpinMutex&) = delete;
            SpinMutex& operator = (SpinMutex&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::primitive::SpinMutex_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto lock() -> bool {
                return src::foundation::primitive::SpinMutex_lock(&this->handle);
            }
            auto unlock() -> bool {
                return src::foundation::primitive::SpinMutex_unlock(&this->handle);
            }
            auto tryLock() -> bool {
                return src::foundation::primitive::SpinMutex_tryLock(&this->handle);
            }

            auto rawHandle() -> src::foundation::primitive::SpinMutex* {
                return &this->handle;
            }
            auto rawHandle() const -> const src::foundation::primitive::SpinMutex* {
                return &this->handle;
            }

        private:
            src::foundation::primitive::SpinMutex handle = {};
    };
}