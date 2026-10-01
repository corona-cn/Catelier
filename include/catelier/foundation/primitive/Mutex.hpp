#pragma once
#include "../../../../src/catelier/foundation/primitive/Mutex.hpp"

namespace Catelier::foundation::primitive {
    class Mutex {
        public:
            class Guard {
                public:
                    explicit Guard(Mutex& inMutex) : mutex(&inMutex), owns(inMutex.lock()) {}
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
                    Mutex* mutex;
                    bool owns;
            };

            class TryGuard {
                public:
                    explicit TryGuard(Mutex& inMutex) : mutex(&inMutex), owns(inMutex.tryLock()) {}
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
                    Mutex* mutex;
                    bool owns;
            };

            explicit Mutex() {
                src::foundation::primitive::Mutex_init(&this->handle);
            }
            ~Mutex() {
                if (this->handle.valid) {
                    src::foundation::primitive::Mutex_destroy(&this->handle);
                }
            }

            Mutex(const Mutex&) = delete;
            Mutex(Mutex&& other) noexcept {
                this->handle = other.handle;

                other.handle.valid = false;
            }
            Mutex& operator = (const Mutex&) = delete;
            Mutex& operator = (Mutex&& other) noexcept {
                if (this != &other) {
                    if (this->handle.valid) {
                        src::foundation::primitive::Mutex_destroy(&this->handle);
                    }

                    this->handle = other.handle;

                    other.handle.valid = false;
                }

                return *this;
            }

            auto lock() -> bool {
                return src::foundation::primitive::Mutex_lock(&this->handle);
            }
            auto unlock() -> bool {
                return src::foundation::primitive::Mutex_unlock(&this->handle);
            }
            auto tryLock() -> bool {
                return src::foundation::primitive::Mutex_tryLock(&this->handle);
            }

            auto isValid() const -> bool {
                return this->handle.valid;
            }

        private:
            src::foundation::primitive::Mutex handle = {};
    };
}