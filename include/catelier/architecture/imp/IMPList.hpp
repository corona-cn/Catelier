#pragma once
#include "../../../../src/catelier/architecture/imp/IMPList.hpp"

namespace Catelier::architecture::imp {
    template<typename ElementType>
    class IMPList {
        public:
            class ConstIterator {
                public:
                    explicit ConstIterator(const ElementType* inPtr = nullptr) {
                        this->ptr = inPtr;
                    }

                    auto operator * () const -> const ElementType& {
                        return *this->ptr;
                    }
                    auto operator -> () const -> const ElementType* {
                        return this->ptr;
                    }

                    auto operator ++ () -> ConstIterator& {
                        ++this->ptr;
                        return *this;
                    }
                    auto operator ++ (int) -> ConstIterator {
                        ConstIterator temp = *this;
                        ++this->ptr;
                        return temp;
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        return this->ptr == other.ptr;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return this->ptr != other.ptr;
                    }

                private:
                    const ElementType* ptr;
            };

            explicit IMPList(const usize inChunkSize = 1024, const u64 inPropagationDurationMs = 1, bool (*inElementEquals)(const void*, const void*) = nullptr) {
                this->handle = src::architecture::imp::IMPList_construct(sizeof(ElementType), inChunkSize, inPropagationDurationMs, src::architecture::imp::UNORDERED, nullptr, inElementEquals);
            }
            ~IMPList() {
                if (this->handle) {
                    src::architecture::imp::IMPList_destruct(this->handle);

                    this->handle = nullptr;
                }
            }
            IMPList(const IMPList& other) {
                if (!other.handle) {
                    this->handle = nullptr;
                    return;
                }

                this->handle = src::architecture::imp::IMPList_copy(other.handle);
            }
            IMPList(IMPList&& other) noexcept {
                this->handle = other.handle;

                other.handle = nullptr;
            }
            IMPList& operator = (const IMPList& other) {
                if (this != &other) {
                    if (this->handle) {
                        src::architecture::imp::IMPList_destruct(this->handle);
                        this->handle = nullptr;
                    }

                    if (other.handle) {
                        this->handle = src::architecture::imp::IMPList_copy(other.handle);
                    }
                }

                return *this;
            }
            IMPList& operator = (IMPList&& other) noexcept {
                if (this != &other) {
                    if (this->handle) {
                        src::architecture::imp::IMPList_destruct(this->handle);
                    }

                    this->handle = other.handle;

                    other.handle = nullptr;
                }

                return *this;
            }

            auto presentElements() const -> const ElementType* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_presentElements(this->handle);
            }
            auto presentElementsCopy(ElementType*& outElements, usize& outElementCount) const -> bool {
                if (!this->handle) {
                    return false;
                }

                u8* rawElements = nullptr;
                usize rawElementCount = 0;

                if (!src::architecture::imp::IMPList_presentElementsCopy(this->handle, &rawElements, &rawElementCount)) {
                    return false;
                }

                outElements = (ElementType*) rawElements;
                outElementCount = rawElementCount;

                return true;
            }
            auto presentElementCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_presentElementCount(this->handle);
            }
            auto ingestElementCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_ingestElementCount(this->handle);
            }
            auto mergeElementCount() const -> usize {
                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_mergeElementCount(this->handle);
            }

            auto get(const usize index) const -> const ElementType* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_get(this->handle, index);
            }
            auto operator [] (const usize index) const -> const ElementType* {
                return this->get(index);
            }
            auto first() const -> const ElementType* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_first(this->handle);
            }
            auto last() const -> const ElementType* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_last(this->handle);
            }

            auto add(const ElementType& inElement) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_add(this->handle, &inElement);
            }
            auto operator += (const ElementType& inElement) -> IMPList& {
                this->add(inElement);

                return *this;
            }
            auto addAll(const ElementType* inElements, const usize inElementCount) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_addAll(this->handle, inElements, inElementCount);
            }

            auto remove(const ElementType& inElement) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_remove(this->handle, &inElement);
            }
            auto removeAll(const ElementType* inElements, const usize inElementCount) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_removeAll(this->handle, inElements, inElementCount);
            }
            template<typename Predicate>
            auto removeIf(Predicate inPredicate) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_removeIf(this->handle, predicateBridge<Predicate>, &inPredicate);
            }
            auto retain(const ElementType* inElements, const usize inElementCount) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_retain(this->handle, inElements, inElementCount);
            }
            template<typename Predicate>
            auto retainIf(Predicate inPredicate) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_retainIf(this->handle, predicateBridge<Predicate>, &inPredicate);
            }
            auto clear() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_clear(this->handle);
            }

            auto begin() const -> ConstIterator {
                return ConstIterator(this->presentElements());
            }
            auto end() const -> ConstIterator {
                if (!this->handle) {
                    return ConstIterator(nullptr);
                }

                return ConstIterator(this->presentElements() + this->presentElementCount());
            }

            template<typename Function>
            auto forEach(Function function) const -> void {
                if (!this->handle) {
                    return;
                }

                const auto* const presentElements = (const ElementType*) src::architecture::imp::IMPList_presentElements(this->handle);
                const usize presentElementCount = src::architecture::imp::IMPList_presentElementCount(this->handle);
                for (usize i = 0; i < presentElementCount; ++i) {
                    function(presentElements[i]);
                }
            }
            template<typename Function>
            auto forEachIndexed(Function function) const -> void {
                if (!this->handle) {
                    return;
                }

                const auto* const presentElements = (const ElementType*) src::architecture::imp::IMPList_presentElements(this->handle);
                const usize presentElementCount = src::architecture::imp::IMPList_presentElementCount(this->handle);
                for (usize i = 0; i < presentElementCount; ++i) {
                    function(i, presentElements[i]);
                }
            }

            auto find(const ElementType& inElement) const -> const ElementType* {
                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_find(this->handle, &inElement);
            }
            auto contains(const ElementType& inElement) const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_contains(this->handle, &inElement);
            }
            auto containsAll(const ElementType* inElements, const usize inElementCount) const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_containsAll(this->handle, inElements, inElementCount);
            }
            auto firstIndexOf(const ElementType& inElement) const -> ssize {
                if (!this->handle) {
                    return -1;
                }

                return src::architecture::imp::IMPList_firstIndexOf(this->handle, &inElement);
            }
            auto lastIndexOf(const ElementType& inElement) const -> ssize {
                if (!this->handle) {
                    return -1;
                }

                return src::architecture::imp::IMPList_lastIndexOf(this->handle, &inElement);
            }
            auto isEmpty() const -> bool {
                if (!this->handle) {
                    return true;
                }

                return src::architecture::imp::IMPList_isEmpty(this->handle);
            }
            auto isNotEmpty() const -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_isNotEmpty(this->handle);
            }

            template<typename Comparer>
            auto sortWith(Comparer inComparer) -> bool {
                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::ASCENDING, comparerBridge<Comparer>);
            }
            template<typename Comparer>
            auto sortWithDescending(Comparer inComparer) -> bool {
                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::DESCENDING, comparerBridge<Comparer>);
            }
            template<typename Selector>
            auto sortBy(Selector inSelector) -> bool {
                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::ASCENDING, selectorBridge<Selector>);
            }
            template<typename Selector>
            auto sortByDescending(Selector inSelector) -> bool {
                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::DESCENDING, selectorBridge<Selector>);
            }
            auto naturalSort() -> bool {
                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::ASCENDING, naturalBridge);
            }
            auto naturalSortDescending() -> bool {
                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::DESCENDING, naturalBridge);
            }

            auto merge() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_merge(this->handle);
            }
            auto awaitMerging() -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_awaitMerging(this->handle);
            }
            auto awaitMerging(const u64 inTimeoutMs) -> bool {
                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_timedAwaitMerging(this->handle, inTimeoutMs);
            }

        private:
            src::architecture::imp::IMPList* handle = nullptr;

            template<typename Predicate>
            static auto predicateBridge(const void* inElement, void* inPredicateContext) -> bool {
                auto* const predicate = (Predicate*) inPredicateContext;
                return (*predicate)(*((const ElementType*) inElement));
            }

            template<typename Comparer>
            static auto comparerBridge(const void* inA, const void* inB) -> i32 {
                const Comparer comparer{};

                const auto& a = *((const ElementType*) inA);
                const auto& b = *((const ElementType*) inB);

                if (comparer(a, b)) {
                    return -1;
                }

                if (comparer(b, a)) {
                    return 1;
                }

                return 0;
            }
            template<typename Selector>
            static auto selectorBridge(const void* inA, const void* inB) -> i32 {
                const Selector selector{};

                const auto& a = *((const ElementType*) inA);
                const auto& b = *((const ElementType*) inB);

                const auto keyA = selector(a);
                const auto keyB = selector(b);

                if (keyA < keyB) {
                    return -1;
                }

                if (keyB < keyA) {
                    return 1;
                }

                return 0;
            }
            static auto naturalBridge(const void* inA, const void* inB) -> i32 {
                const auto& a = *((const ElementType*) inA);
                const auto& b = *((const ElementType*) inB);

                if (a < b) {
                    return -1;
                }

                if (b < a) {
                    return 1;
                }

                return 0;
            }
    };
}