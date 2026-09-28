#pragma once
#include "../../../src/catelier/foundation/HeapString.hpp"

namespace Catelier::foundation {
    class HeapString {
        public:
            explicit HeapString() {
                this->handle = src::foundation::HeapString_construct();
            }
            explicit HeapString(const u8* inChars, const usize inLength) {
                this->handle = src::foundation::HeapString_constructFrom(inChars, inLength);
            }
            explicit HeapString(const char* inChars) {
                usize length = 0;
                while (*(inChars + length) != '\0') {
                    ++length;
                }

                this->handle = src::foundation::HeapString_constructFrom((const u8*) inChars, length);
            }
            ~HeapString() {
                if (this->handle) {
                    src::foundation::HeapString_destruct(this->handle);

                    this->handle = nullptr;
                }
            }

            HeapString(const HeapString& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::foundation::HeapString_copy(other.handle);
            }
            HeapString(HeapString&& other) noexcept {
                this->handle = src::foundation::HeapString_move(other.handle);
            }
            HeapString& operator = (const HeapString& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::HeapString_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::foundation::HeapString_copy(other.handle);
                    }
                }

                return *this;
            }
            HeapString& operator = (HeapString&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::foundation::HeapString_destruct(this->handle);
                    }

                    this->handle = src::foundation::HeapString_move(other.handle);
                }

                return *this;
            }

            auto append(const HeapString& other) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_append(this->handle, other.handle);
            }
            auto appendChars(const u8* inChars, const usize inLength) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_appendChars(this->handle, inChars, inLength);
            }
            auto appendChar(const u8 inChar) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_appendChar(this->handle, inChar);
            }
            auto insert(const usize index, const HeapString& other) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_insert(this->handle, index, other.handle);
            }

            auto remove(const usize start, const usize length) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_remove(this->handle, start, length);
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_clear(this->handle);
            }

            auto findBF(const u8* inPattern, const usize patternLength) const -> ssize {
                if (!this->handle) {
                    return -1;
                }

                return src::foundation::HeapString_findBF(this->handle, inPattern, patternLength);
            }
            auto findKMP(const u8* inPattern, const usize patternLength) const -> ssize {
                if (!this->handle) {
                    return -1;
                }

                return src::foundation::HeapString_findKMP(this->handle, inPattern, patternLength);
            }
            auto find(const u8* inPattern, const usize patternLength) const -> ssize {
                if (!this->handle) {
                    return -1;
                }

                return src::foundation::HeapString_find(this->handle, inPattern, patternLength);
            }
            auto find(const HeapString& pattern) const -> ssize {
                return find(src::foundation::HeapString_chars(pattern.handle), src::foundation::HeapString_length(pattern.handle));
            }
            auto replace(const u8* inOldPattern, const usize oldLength, const u8* inNewPattern, const usize newLength) -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::HeapString_replace(this->handle, inOldPattern, oldLength, inNewPattern, newLength);
            }
            auto replace(const HeapString& oldPattern, const HeapString& newPattern) -> usize {
                return replace(
                    src::foundation::HeapString_chars(oldPattern.handle),
                    src::foundation::HeapString_length(oldPattern.handle),
                    src::foundation::HeapString_chars(newPattern.handle),
                    src::foundation::HeapString_length(newPattern.handle)
                );
            }

            auto substring(const usize start, const usize length) const -> HeapString {
                HeapString result;
                if (!this->handle) {
                    return result;
                }

                auto* const subHandle = src::foundation::HeapString_substring(this->handle, start, length);
                if (subHandle) {
                    result.handle = subHandle;
                }

                return result;
            }

            auto reserve(const usize newCapacity) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_reserve(this->handle, newCapacity);
            }
            auto shrinkToFit() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_shrinkToFit(this->handle);
            }

            auto chars() -> u8* {
                if (!this->handle) {
                    return nullptr;
                }

                return src::foundation::HeapString_chars(this->handle);
            }
            auto chars() const -> const u8* {
                if (!this->handle) {
                    return nullptr;
                }

                return src::foundation::HeapString_chars(this->handle);
            }
            auto charAt(const usize index) const -> u8 {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::HeapString_charAt(this->handle, index);
            }
            auto setCharAt(const usize index, const u8 inChar) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::foundation::HeapString_setCharAt(this->handle, index, inChar);
            }

            auto compare(const HeapString& other) const -> i32 {
                if (!this->handle || !other.handle) {
                    return 0;
                }

                return src::foundation::HeapString_compare(this->handle, other.handle);
            }
            auto equals(const HeapString& other) const -> bool {
                if (!this->handle || !other.handle) {
                    return false;
                }

                return src::foundation::HeapString_equals(this->handle, other.handle);
            }

            auto length() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::HeapString_length(this->handle);
            }
            auto capacity() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::foundation::HeapString_capacity(this->handle);
            }

            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::foundation::HeapString_isEmpty(this->handle);
            }

            auto operator == (const HeapString& other) const -> bool {
                return this->equals(other);
            }
            auto operator != (const HeapString& other) const -> bool {
                return !this->equals(other);
            }

        private:
            src::foundation::HeapString* handle = nullptr;
    };
}