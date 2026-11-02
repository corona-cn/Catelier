#pragma once
#include <cassert>
#include <utility>

#include "../../../../src/catelier/architecture/imp/IMPList.hpp"

namespace Catelier::include::architecture::imp {
    template<typename ElementType>
    class IMPList {
        public:
            class ConstIterator {
                public:
                    explicit ConstIterator(
                        src::architecture::imp::IMPList* inListHandle = nullptr,
                        void* inSpanHandle = nullptr,
                        const ElementType* inPtr = nullptr,
                        const ElementType* inEndPtr = nullptr,
                        bool inOwnsSpanHandle = false
                    ) {
                        this->listHandle = inListHandle;
                        this->spanHandle = inSpanHandle;
                        this->ptr = inPtr;
                        this->endPtr = inEndPtr;
                        this->ownsSpanHandle = inOwnsSpanHandle;
                    }
                    ~ConstIterator() {
                        if (this->ownsSpanHandle && this->spanHandle) {
                            src::architecture::imp::IMPList_iteratorEnd(this->listHandle, this->spanHandle);
                        }
                    }
                    ConstIterator(const ConstIterator&) = delete;
                    ConstIterator& operator = (const ConstIterator&) = delete;
                    ConstIterator(ConstIterator&& other) noexcept {
                        this->listHandle = other.listHandle;
                        this->spanHandle = other.spanHandle;
                        this->ptr = other.ptr;
                        this->endPtr = other.endPtr;
                        this->ownsSpanHandle = other.ownsSpanHandle;

                        other.spanHandle = nullptr;
                        other.ownsSpanHandle = false;
                    }
                    ConstIterator& operator = (ConstIterator&& other) noexcept {
                        if (this != &other) {
                            if (this->ownsSpanHandle && this->spanHandle) {
                                src::architecture::imp::IMPList_iteratorEnd(this->listHandle, this->spanHandle);
                            }

                            this->listHandle = other.listHandle;
                            this->spanHandle = other.spanHandle;
                            this->ptr = other.ptr;
                            this->endPtr = other.endPtr;
                            this->ownsSpanHandle = other.ownsSpanHandle;

                            other.spanHandle = nullptr;
                            other.ownsSpanHandle = false;
                        }

                        return *this;
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
                        ConstIterator temp = std::move(*this);
                        ++this->ptr;
                        this->spanHandle = temp.spanHandle;
                        this->ownsSpanHandle = temp.ownsSpanHandle;
                        temp.spanHandle = nullptr;
                        temp.ownsSpanHandle = false;
                        return ConstIterator(this->listHandle, nullptr, temp.ptr, temp.endPtr, false);
                    }

                    auto operator == (const ConstIterator& other) const -> bool {
                        const bool thisIsAtEnd = (this->ptr == nullptr) || (this->ptr == this->endPtr);
                        const bool otherIsAtEnd = (other.ptr == nullptr) || (other.ptr == other.endPtr);

                        if (thisIsAtEnd || otherIsAtEnd) {
                            return thisIsAtEnd == otherIsAtEnd;
                        }

                        return this->ptr == other.ptr;
                    }
                    auto operator != (const ConstIterator& other) const -> bool {
                        return !(*this == other);
                    }

                private:
                    src::architecture::imp::IMPList* listHandle;
                    void* spanHandle;
                    const ElementType* ptr;
                    const ElementType* endPtr;
                    bool ownsSpanHandle;
            };

            auto isValid() const -> bool {
                return this->handle != nullptr;
            }

            explicit IMPList(const usize inElementChunkSize = 1024, const usize inDiscardBufferSize = 1024) {
                this->handle = src::architecture::imp::IMPList_construct(sizeof(ElementType), inElementChunkSize, inDiscardBufferSize, src::architecture::imp::IMPList_ElementSortingStrategy::UNORDERED, nullptr);
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

            auto configurePropagation(const u64 inPollingIntervalMs, const u64 inBackoffIntervalMs) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_configurePropagation(this->handle, inPollingIntervalMs, inBackoffIntervalMs);
            }
            auto configureMaintenanceMerging(const bool inIsMaintenanceMergingEnabled) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_configureMaintenanceMerging(this->handle, inIsMaintenanceMergingEnabled);
            }
            auto configurePresentElementsSpanRecycling(const bool inIsPresentElementsSpanRecyclingEnabled) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_configurePresentElementsSpanRecycling(this->handle, inIsPresentElementsSpanRecyclingEnabled);
            }
            auto configureCompaction(const bool inIsCompactionEnabled, const u32 inCompactionThresholdPercent, const u64 inCompactionIntervalMs) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_configureCompaction(this->handle, inIsCompactionEnabled, inCompactionThresholdPercent, inCompactionIntervalMs);
            }

            auto presentElementsSpan(const ElementType*& outElements, usize& outElementCount) const -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                const u8* rawElements = nullptr;
                usize rawElementCount = 0;

                if (!src::architecture::imp::IMPList_presentElementsSpan(this->handle, &rawElements, &rawElementCount)) {
                    return false;
                }

                outElements = (const ElementType*) rawElements;
                outElementCount = rawElementCount;

                return true;
            }
            auto presentElementsSpanCopy(ElementType*& outElements, usize& outElementCount) const -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                const u8* rawElements = nullptr;
                usize rawElementCount = 0;

                if (!src::architecture::imp::IMPList_presentElementsSpanCopy(this->handle, &rawElements, &rawElementCount)) {
                    return false;
                }

                outElements = (ElementType*) rawElements;
                outElementCount = rawElementCount;

                return true;
            }
            auto presentElements() const -> const ElementType* {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_presentElements(this->handle);
            }
            auto presentElementCount() const -> usize {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_presentElementCount(this->handle);
            }
            auto ingestElementCount() const -> usize {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_ingestElementCount(this->handle);
            }
            auto mergeElementCount() const -> usize {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_mergeElementCount(this->handle);
            }
            auto compactionCount() const -> usize {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_compactionCount(this->handle);
            }
            auto lastCompactionDurationMs() const -> usize {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return 0;
                }

                return src::architecture::imp::IMPList_lastCompactionDurationMs(this->handle);
            }

            auto get(const usize index) const -> const ElementType* {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_get(this->handle, index);
            }
            auto operator [] (const usize index) const -> const ElementType* {
                return this->get(index);
            }
            auto first() const -> const ElementType* {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_first(this->handle);
            }
            auto last() const -> const ElementType* {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_last(this->handle);
            }

            auto begin() const -> ConstIterator {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return ConstIterator();
                }

                void* spanHandle = nullptr;
                const u8* rawElements = nullptr;
                usize rawElementCount = 0;

                if (!src::architecture::imp::IMPList_iteratorBegin(this->handle, &spanHandle, &rawElements, &rawElementCount)) {
                    return ConstIterator();
                }

                const auto* const beginPtr = (const ElementType*) rawElements;
                const auto* const endPtr = beginPtr + rawElementCount;

                return ConstIterator(this->handle, spanHandle, beginPtr, endPtr, true);
            }
            auto end() const -> ConstIterator {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return ConstIterator();
                }

                return ConstIterator(this->handle, nullptr, nullptr, nullptr, false);
            }

            template<typename Function>
            auto forEach(Function inFunction) const -> void {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return;
                }

                void* spanHandle = nullptr;
                const u8* rawElements = nullptr;
                usize rawElementCount = 0;

                if (!src::architecture::imp::IMPList_iteratorBegin(this->handle, &spanHandle, &rawElements, &rawElementCount)) {
                    return;
                }

                const auto* const presentElements = (const ElementType*) rawElements;
                const usize presentElementCount = rawElementCount;
                for (usize i = 0; i < presentElementCount; ++i) {
                    inFunction(presentElements[i]);
                }

                if (spanHandle) {
                    src::architecture::imp::IMPList_iteratorEnd(this->handle, spanHandle);
                }
            }
            template<typename Function>
            auto forEachIndexed(Function inFunction) const -> void {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return;
                }

                void* spanHandle = nullptr;
                const u8* rawElements = nullptr;
                usize rawElementCount = 0;

                if (!src::architecture::imp::IMPList_iteratorBegin(this->handle, &spanHandle, &rawElements, &rawElementCount)) {
                    return;
                }

                const auto* const presentElements = (const ElementType*) rawElements;
                const usize presentElementCount = rawElementCount;
                for (usize i = 0; i < presentElementCount; ++i) {
                    inFunction(i, presentElements[i]);
                }

                if (spanHandle) {
                    src::architecture::imp::IMPList_iteratorEnd(this->handle, spanHandle);
                }
            }

            auto find(const ElementType& inElement) const -> const ElementType* {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return nullptr;
                }

                return (const ElementType*) src::architecture::imp::IMPList_find(this->handle, &inElement);
            }
            auto contains(const ElementType& inElement) const -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_contains(this->handle, &inElement);
            }
            auto containsAll(const ElementType* inElements, const usize inElementCount) const -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_containsAll(this->handle, inElements, inElementCount);
            }
            auto firstIndexOf(const ElementType& inElement) const -> ssize {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return -1;
                }

                return src::architecture::imp::IMPList_firstIndexOf(this->handle, &inElement);
            }
            auto lastIndexOf(const ElementType& inElement) const -> ssize {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return -1;
                }

                return src::architecture::imp::IMPList_lastIndexOf(this->handle, &inElement);
            }
            auto isEmpty() const -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return true;
                }

                return src::architecture::imp::IMPList_isEmpty(this->handle);
            }
            auto isNotEmpty() const -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_isNotEmpty(this->handle);
            }

            auto add(const ElementType& inElement) -> bool {
                assert(this->handle != nullptr);

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
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_addAll(this->handle, inElements, inElementCount);
            }

            auto remove(const ElementType& inElement) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_remove(this->handle, &inElement);
            }
            template<typename Predicate>
            auto removeIf(Predicate inPredicate) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_removeIf(this->handle, predicateBridge<Predicate>, &inPredicate);
            }
            auto removeExcept(const ElementType* inElements, const usize inElementCount) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_removeExcept(this->handle, inElements, inElementCount);
            }
            template<typename Predicate>
            auto removeExceptIf(Predicate inPredicate) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_removeExceptIf(this->handle, predicateBridge<Predicate>, &inPredicate);
            }
            auto removeAll() -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_removeAll(this->handle);
            }

            auto discard(const ElementType& inElement) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_discard(this->handle, &inElement);
            }
            auto discardAll() -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_discardAll(this->handle);
            }

            template<typename Comparer>
            auto sortWith(Comparer inComparer) -> bool {
                assert(this->handle != nullptr);

                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::ASCENDING, comparerBridge<Comparer>);
            }
            template<typename Comparer>
            auto sortWithDescending(Comparer inComparer) -> bool {
                assert(this->handle != nullptr);

                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::DESCENDING, comparerBridge<Comparer>);
            }
            template<typename Selector>
            auto sortBy(Selector inSelector) -> bool {
                assert(this->handle != nullptr);

                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::ASCENDING, selectorBridge<Selector>);
            }
            template<typename Selector>
            auto sortByDescending(Selector inSelector) -> bool {
                assert(this->handle != nullptr);

                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::DESCENDING, selectorBridge<Selector>);
            }
            auto naturalSort() -> bool {
                assert(this->handle != nullptr);

                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::ASCENDING, naturalBridge);
            }
            auto naturalSortDescending() -> bool {
                assert(this->handle != nullptr);

                return src::architecture::imp::IMPList_sort(this->handle, src::architecture::imp::IMPList_ElementSortingStrategy::DESCENDING, naturalBridge);
            }

            auto tryMerging() -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_tryMerging(this->handle);
            }
            auto awaitMerging() -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_awaitMerging(this->handle);
            }
            auto awaitMerging(const u64 inTimeoutMs) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_timedAwaitMerging(this->handle, inTimeoutMs);
            }

            auto tryCompacting() -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_tryCompacting(this->handle);
            }
            auto awaitCompacting() -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_awaitCompacting(this->handle);
            }
            auto awaitCompacting(const u64 inTimeoutMs) -> bool {
                assert(this->handle != nullptr);

                if (!this->handle) {
                    return false;
                }

                return src::architecture::imp::IMPList_timedAwaitCompacting(this->handle, inTimeoutMs);
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