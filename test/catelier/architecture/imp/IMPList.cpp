#include <windows.h>
#include <intrin.h>

#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

#include "../../util/Logger.hpp"

#include "../../../../include/catelier/architecture/imp/IMPList.hpp"

namespace Catelier::test::architecture::imp {
    namespace {
        using namespace Catelier::include::architecture::imp;

        constexpr u32 BENCH_TRIALS = 101;
        constexpr u64 kWarmupNs = 200000000ULL;
        constexpr u64 kMeasureNs = 800000000ULL;

        auto intEvenPredicate(const int& x) -> bool {
            return x % 2 == 0;
        }
        auto intOddPredicate(const int& x) -> bool {
            return x % 2 != 0;
        }
        auto intLessThan(const int& x) -> int {
            return x;
        }

        auto sortU64(u64* data, const u32 count) -> void {
            for (u32 i = 0; i < count; ++i) {
                for (u32 j = i + 1; j < count; ++j) {
                    if (data[i] > data[j]) {
                        const u64 tmp = data[i];
                        data[i] = data[j];
                        data[j] = tmp;
                    }
                }
            }
        }
        auto meanOf(const u64* data, const u32 start, const u32 end) -> double {
            if (end <= start) {
                return 0.0;
            }

            u64 sum = 0;
            for (u32 i = start; i < end; ++i) {
                sum += data[i];
            }

            return (double) sum / (double) (end - start);
        }
        auto stddevOf(const u64* data, const u32 start, const u32 end, const double mean) -> double {
            if (end <= start) {
                return 0.0;
            }

            double accum = 0.0;
            for (u32 i = start; i < end; ++i) {
                const double d = (double) data[i] - mean;
                accum += d * d;
            }

            return std::sqrt(accum / (double) (end - start));
        }

        auto computePercentile(const u64* sortedData, const u32 count, const double percentile) -> u64 {
            if (count == 0) {
                return 0;
            }
            if (count == 1) {
                return sortedData[0];
            }

            const double rank = percentile * (double) (count - 1);
            const u32 lowerIndex = (u32) rank;
            const u32 upperIndex = (lowerIndex + 1 >= count) ? (count - 1) : (lowerIndex + 1);
            const double weight = rank - (double) lowerIndex;

            return (u64) ((double) sortedData[lowerIndex] * (1.0 - weight) + (double) sortedData[upperIndex] * weight);
        }

        auto filterOutliers(
            const u64* rawData,
            const u32 rawCount,
            u64* outFilteredData,
            u32* outFilteredCount,
            const double inLowerIqrMultiplier,
            const double inUpperIqrMultiplier
        ) -> void {
            if (!rawData || !outFilteredData || !outFilteredCount || rawCount == 0) {
                if (outFilteredCount) {
                    *outFilteredCount = 0;
                }
                return;
            }

            static u64 sortedCopy[4096];
            const u32 copyCount = rawCount > 4096 ? 4096 : rawCount;

            for (u32 i = 0; i < copyCount; ++i) {
                sortedCopy[i] = rawData[i];
            }
            sortU64(sortedCopy, copyCount);

            const u64 q1 = computePercentile(sortedCopy, copyCount, 0.25);
            const u64 q3 = computePercentile(sortedCopy, copyCount, 0.75);
            const double iqr = (double) q3 - (double) q1;

            double lowerBound;
            double upperBound;

            if (iqr == 0.0) {
                const u64 median = sortedCopy[copyCount / 2];
                lowerBound = (double) median * 0.5;
                upperBound = (double) median * 1.5 + 1.0;
            } else {
                lowerBound = (double) q1 - inLowerIqrMultiplier * iqr;
                upperBound = (double) q3 + inUpperIqrMultiplier * iqr;

                if (lowerBound < 0.0) {
                    lowerBound = 0.0;
                }
            }

            u32 filteredCount = 0;
            for (u32 i = 0; i < rawCount; ++i) {
                const double v = (double) rawData[i];
                if (v >= lowerBound && v <= upperBound) {
                    outFilteredData[filteredCount] = rawData[i];
                    filteredCount++;
                }
            }

            *outFilteredCount = filteredCount;
        }

        auto computeMeanSdIqr(
            const u64* samples,
            const u32 n,
            double* outMean,
            double* outSd,
            u32* outKept
        ) -> void {
            static u64 filteredSamples[4096];
            u32 kept = 0;
            filterOutliers(samples, n, filteredSamples, &kept, 1.5, 2.0);

            if (kept == 0) {
                *outMean = meanOf(samples, 0, n);
                *outSd = stddevOf(samples, 0, n, *outMean);
                *outKept = n;
            } else {
                *outMean = meanOf(filteredSamples, 0, kept);
                *outSd = stddevOf(filteredSamples, 0, kept, *outMean);
                *outKept = kept;
            }
        }

        auto meanIqr(const u64* samples, const u32 n) -> double {
            double mean = 0.0;
            double sd = 0.0;
            u32 kept = 0;
            computeMeanSdIqr(samples, n, &mean, &sd, &kept);
            return mean;
        }

        auto computeTrialMeanWithIqr(
            const u64* rawSubSamples,
            const u32 subCount,
            const double inLowerIqrMultiplier,
            const double inUpperIqrMultiplier
        ) -> u64 {
            if (!rawSubSamples || subCount == 0) {
                return 0;
            }

            static u64 sorted[4096];
            const u32 n = subCount > 4096 ? 4096 : subCount;

            for (u32 i = 0; i < n; ++i) {
                sorted[i] = rawSubSamples[i];
            }
            sortU64(sorted, n);

            const u64 q1 = computePercentile(sorted, n, 0.25);
            const u64 q3 = computePercentile(sorted, n, 0.75);
            const double iqr = (double) q3 - (double) q1;

            // 子批间几乎无差异（正常稳态），直接返回中位数
            if (iqr == 0.0) {
                return sorted[n / 2];
            }

            double lowerBound = (double) q1 - inLowerIqrMultiplier * iqr;
            const double upperBound = (double) q3 + inUpperIqrMultiplier * iqr;

            if (lowerBound < 0.0) {
                lowerBound = 0.0;
            }

            u64 sum = 0;
            u32 keptCount = 0;
            for (u32 i = 0; i < n; ++i) {
                const double v = (double) sorted[i];
                if (v >= lowerBound && v <= upperBound) {
                    sum += sorted[i];
                    keptCount++;
                }
            }

            if (keptCount == 0) {
                return sorted[n / 2];
            }

            return sum / keptCount;
        }

        class QpcTimer {
            public:
                explicit QpcTimer() {
                    LARGE_INTEGER freq;
                    QueryPerformanceFrequency(&freq);
                    this->invFreqNs = 1000000000.0 / (double) freq.QuadPart;
                }

                auto start() -> void {
                    QueryPerformanceCounter(&this->begin);
                }

                auto stopNs() -> u64 {
                    LARGE_INTEGER end;
                    QueryPerformanceCounter(&end);
                    return (u64) ((double) (end.QuadPart - this->begin.QuadPart) * this->invFreqNs);
                }

            private:
                LARGE_INTEGER begin = {};
                double invFreqNs = 1.0;
        };

        class TscTimer {
            public:
                explicit TscTimer() {
                    LARGE_INTEGER freq;
                    LARGE_INTEGER t1;
                    LARGE_INTEGER t2;
                    QueryPerformanceFrequency(&freq);

                    const double qpcInvFreqNs = 1000000000.0 / (double) freq.QuadPart;

                    const u64 tscStart = __rdtsc();
                    QueryPerformanceCounter(&t1);

                    Sleep(100);

                    const u64 tscEnd = __rdtsc();
                    QueryPerformanceCounter(&t2);

                    const double elapsedNs = (double) (t2.QuadPart - t1.QuadPart) * qpcInvFreqNs;
                    const double elapsedTicks = (double) (tscEnd - tscStart);

                    this->invTicksPerNs = elapsedNs / elapsedTicks;
                }

                auto start() -> void {
                    _mm_lfence();
                    this->begin = __rdtsc();
                }

                auto stopTicks() -> u64 {
                    const u64 end = __rdtsc();
                    _mm_lfence();
                    return end - this->begin;
                }

                auto ticksToScaledNs(const u64 ticks, const double scale) const -> double {
                    return (double) ticks * this->invTicksPerNs * scale;
                }

            private:
                u64 begin = 0;
                double invTicksPerNs = 1.0;
        };

        auto logPerOp(const char* label, const u64* samples, const u32 n, const double scale) -> void {
            double meanRaw = 0.0;
            double sdRaw = 0.0;
            u32 kept = 0;
            computeMeanSdIqr(samples, n, &meanRaw, &sdRaw, &kept);

            const double meanNs = meanRaw / scale;
            const double sdNs = sdRaw / scale;
            const double minNs = (double) samples[0] / scale;
            const double p50Ns = (double) computePercentile(samples, n, 0.50) / scale;
            const double p90Ns = (double) computePercentile(samples, n, 0.90) / scale;
            const double p99Ns = (double) computePercentile(samples, n, 0.99) / scale;
            const double maxNs = (double) samples[n - 1] / scale;

            LOG(label, "：平均 ", meanNs, " ns/op，标准差 ", sdNs, " ns，最小 ", minNs, " ns，P50 ", p50Ns, " ns，P90 ", p90Ns, " ns，P99 ", p99Ns, " ns，最大 ", maxNs, " ns（", n, " 轮，", kept, " 个样本通过 IQR）");
        }

        auto logThroughput(const char* label, const double nsPerOp, const double sdNs, const u64 totalOps) -> void {
            const double opsPerSec = nsPerOp > 0.0 ? 1e9 / nsPerOp : 0.0;
            LOG(label, "：", nsPerOp, " ns/op（", opsPerSec / 1e6, " M op/s），标准差 ", sdNs, " ns，总 ", totalOps, " 次");
        }

        auto logConcurrentMetrics(const int threads, const int readers, const int writers, const u64 totalROps, const u64 totalRElapsedNs, const u64 totalWOps, const u64 totalWElapsedNs) -> void {
            double rOpsPerSec = 0.0;
            double wOpsPerSec = 0.0;

            if (readers > 0 && totalRElapsedNs > 0) {
                rOpsPerSec = (double) totalROps / ((double) totalRElapsedNs / 1e9);
            }
            if (writers > 0 && totalWElapsedNs > 0) {
                wOpsPerSec = (double) totalWOps / ((double) totalWElapsedNs / 1e9);
            }

            const double rNs = rOpsPerSec > 0.0 ? 1e9 / rOpsPerSec : 0.0;
            const double wNs = wOpsPerSec > 0.0 ? 1e9 / wOpsPerSec : 0.0;

            if (readers > 0 && writers > 0) {
                LOG(threads, " 线程（", readers, " 读 ", writers, " 写）：读 ", rOpsPerSec / 1e6, " M op/s（", rNs, " ns/op），写 ", wOpsPerSec / 1e6, " M op/s（", wNs, " ns/op）");
            } else if (readers > 0) {
                LOG(threads, " 线程（纯读）：读 ", rOpsPerSec / 1e6, " M op/s（", rNs, " ns/op）");
            } else if (writers > 0) {
                LOG(threads, " 线程（纯写）：写 ", wOpsPerSec / 1e6, " M op/s（", wNs, " ns/op）");
            }
        }

        struct ConcurrentTaskResult {
            u64 ops;
            u64 elapsedNs;
        };

        auto runConcurrentTask(IMPList<int>& list, const bool isReader, const u64 threadId, const u64 runNs, const u64 warmupNs) -> ConcurrentTaskResult {
            QpcTimer timer;
            u64 localOps = 0;
            volatile int accum = 0;
            int counter = (int) (threadId * 100000000);

            timer.start();
            while (true) {
                if (isReader) {
                    const usize count = list.presentElementCount();
                    if (count > 0) {
                        for (int k = 0; k < 100; ++k) {
                            const int* v = list.get((usize) ((localOps + k) % count));
                            if (v) {
                                accum += *v;
                            }
                        }
                        localOps += 100;
                    }
                } else {
                    for (int k = 0; k < 100; ++k) {
                        list.add(counter++);
                    }
                    localOps += 100;
                }

                if (localOps > 0 && (localOps & 4095) == 0) {
                    if (timer.stopNs() >= warmupNs) {
                        break;
                    }
                }
            }

            localOps = 0;
            timer.start();
            while (true) {
                if (isReader) {
                    const usize count = list.presentElementCount();
                    if (count > 0) {
                        for (int k = 0; k < 100; ++k) {
                            const int* v = list.get((usize) ((localOps + k) % count));
                            if (v) {
                                accum += *v;
                            }
                        }
                        localOps += 100;
                    }
                } else {
                    for (int k = 0; k < 100; ++k) {
                        list.add(counter++);
                    }
                    localOps += 100;
                }

                if (localOps > 0 && (localOps & 4095) == 0) {
                    if (timer.stopNs() >= runNs) {
                        break;
                    }
                }
            }

            u64 elapsedNs = timer.stopNs();

            return { localOps, elapsedNs };
        }
    }

    auto runTests() -> void {
        using namespace Catelier::include::architecture::imp;

        LOG("===== IMPList 深度测试开始 =====");
        LOGGER_INDENT_BLOCK(true) {
            LOG("--- [正确性] 构造与生命周期 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                LOG("初始 isValid = ", list.isValid() ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("初始 isEmpty = ", list.isEmpty(), "（应为 1）");
                LOG("初始 presentElementCount = ", list.presentElementCount(), "（应为 0）");
                LOG("初始 ingestElementCount = ", list.ingestElementCount(), "（应为 0）");
                LOG("初始 mergeElementCount = ", list.mergeElementCount(), "（应为 0）");
                LOG("初始 compactionCount = ", list.compactionCount(), "（应为 0）");
                LOG("初始 lastCompactionDurationMs = ", list.lastCompactionDurationMs(), "（应为 0）");
                LOG("初始 first = ", list.first() == nullptr ? "NULL" : "非 NULL", "（应为 NULL）");
                LOG("初始 last = ", list.last() == nullptr ? "NULL" : "非 NULL", "（应为 NULL）");

                constexpr usize kInstances = 1000;
                for (usize i = 0; i < kInstances; ++i) {
                    auto temp = IMPList<int>(64, 1024);
                    temp.add((int) i);
                    temp.tryMerging();
                }
                LOG(kInstances, " 个实例循环构造析构：PASS");
            }

            LOG("--- [正确性] 单线程 add / addAll 全路径 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                bool contentOk = true;
                for (int i = 0; i < 1000; ++i) {
                    const int* v = list.get((usize) i);
                    if (!v || *v != i) {
                        contentOk = false;
                        break;
                    }
                }
                LOG("add 1000 个元素 → 内容正确：", contentOk ? "PASS" : "FAIL");

                int batch[256];
                for (int i = 0; i < 256; ++i) {
                    batch[i] = i + 1000;
                }
                auto list2 = IMPList<int>(64, 1024);
                list2.addAll(batch, 256);
                list2.tryMerging();
                list2.awaitMerging();
                LOG("addAll 256 个元素 → presentElementCount = ", list2.presentElementCount(), "（应为 256）");

                auto list3 = IMPList<int>(64, 1024);
                list3.addAll(batch, 0);
                list3.tryMerging();
                list3.awaitMerging();
                LOG("addAll 0 个元素 → presentElementCount = ", list3.presentElementCount(), "（应为 0）");
            }

            LOG("--- [正确性] chunk 边界：恰好 chunkSize / chunkSize±1 ---");
            LOGGER_INDENT_BLOCK(true) {
                const usize cs[] = { 16, 63, 64, 65, 127, 128, 129 };

                for (usize c : cs) {
                    auto list = IMPList<int>(c, 1024);
                    for (usize i = 0; i < c; ++i) {
                        list.add((int) i);
                    }
                    list.tryMerging();
                    list.awaitMerging();
                    bool ok = list.presentElementCount() == c;
                    for (usize i = 0; ok && i < c; ++i) {
                        const int* v = list.get(i);
                        if (!v || *v != (int) i) {
                            ok = false;
                        }
                    }
                    LOG("chunkSize = ", c, " 写入 ", c, " 个 → ", ok ? "PASS" : "FAIL");
                }
            }

            LOG("--- [正确性] remove / removeAll 边界 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i % 10);
                }
                list.tryMerging();
                list.awaitMerging();
                LOG("add 100 个（0-9 各 10 次） → presentElementCount = ", list.presentElementCount(), "（应为 100）");

                const bool removed = list.remove(5);
                LOG("remove(5) = ", removed ? "PASS" : "FAIL", "（应为 PASS）");

                list.tryMerging();
                list.awaitMerging();
                const usize afterRemove = list.presentElementCount();
                LOG("merge 后 presentElementCount = ", afterRemove, "（全量删除语义：所有 5 均被移除，应为 90）");
            }

            LOG("--- [正确性] removeIf / retainIf 语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);
                for (int i = 1; i <= 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                const bool removed = list.removeIf(intEvenPredicate);
                LOG("removeIf(偶数) = ", removed ? "PASS" : "FAIL");
                list.tryMerging();
                list.awaitMerging();

                bool allOdd = true;
                for (usize i = 0; i < list.presentElementCount(); ++i) {
                    if (*list.get(i) % 2 == 0) {
                        allOdd = false;
                        break;
                    }
                }
                LOG("removeIf 后全为奇数：", allOdd ? "PASS" : "FAIL", "（count = ", list.presentElementCount(), "，应为 50）");

                auto list2 = IMPList<int>(16, 1024);
                for (int i = 1; i <= 100; ++i) {
                    list2.add(i);
                }
                list2.tryMerging();
                list2.awaitMerging();

                const bool retained = list2.removeExceptIf(intOddPredicate);
                LOG("retainIf(奇数) = ", retained ? "PASS" : "FAIL");
                list2.tryMerging();
                list2.awaitMerging();
                LOG("retainIf 后 presentElementCount = ", list2.presentElementCount(), "（应为 50）");
            }

            LOG("--- [正确性] clear 语义（TOMBSTONE + merge） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 20; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                LOG("clear 前 present = ", list.presentElementCount(), "（应为 20）");

                list.removeAll();
                list.tryMerging();
                list.awaitMerging();
                LOG("clear 后 present = ", list.presentElementCount(), "（应为 0）");
                LOG("clear 后 isEmpty = ", list.isEmpty() ? "PASS" : "FAIL");

                list.add(100);
                list.tryMerging();
                list.awaitMerging();
                LOG("clear 后再 add：present = ", list.presentElementCount(), "（应为 1）");
                LOG("值正确：", *((const int*) list.get(0)) == 100 ? "PASS" : "FAIL");

                auto list2 = IMPList<int>(64, 1024);
                list2.removeAll();
                list2.tryMerging();
                list2.awaitMerging();
                LOG("空列表 clear → present = ", list2.presentElementCount(), "（应为 0）");
            }

            LOG("--- [正确性] discard 基础语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                LOG("discard 前 present = ", list.presentElementCount(), "（应为 100）");

                const bool discarded = list.discard(50);
                LOG("discard(50) = ", discarded ? "PASS" : "FAIL", "（应为 PASS）");

                LOG("merge 前 present = ", list.presentElementCount(), "（可能为 100 或 99，取决于后台是否已合并）");

                list.tryMerging();
                list.awaitMerging();
                LOG("merge 后 present = ", list.presentElementCount(), "（应为 99）");
                LOG("contains(50) = ", list.contains(50) ? "FAIL" : "PASS", "（应为 PASS，已被删除）");
            }

            LOG("--- [正确性] discard 重复、不存在与超容量 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 16);
                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                list.discard(5);
                list.discard(5);
                list.discard(5);
                list.discard(1000);

                for (int i = 100; i < 500; ++i) {
                    list.discard(i);
                }

                list.tryMerging();
                list.awaitMerging();
                LOG("discard 重复/不存在/超容量后 present = ", list.presentElementCount(), "（应为 599）");
                LOG("contains(5) = ", list.contains(5) ? "FAIL" : "PASS");
                LOG("contains(250) = ", list.contains(250) ? "FAIL" : "PASS");
                LOG("contains(999) = ", list.contains(999) ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] discard 与 add 时序交互 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);

                list.add(1);
                list.add(2);
                list.discard(3);

                list.tryMerging();
                list.awaitMerging();
                LOG("先 add 后 discard 不存在的元素，present = ", list.presentElementCount(), "（应为 2）");

                list.add(3);
                list.discard(1);
                list.tryMerging();
                list.awaitMerging();
                LOG("追加 add(3) 和 discard(1) 后 present = ", list.presentElementCount(), "（应为 2）");
                LOG("contains(1) = ", list.contains(1) ? "FAIL" : "PASS");
                LOG("contains(3) = ", list.contains(3) ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] discardAll 基础语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                LOG("discardAll 前 present = ", list.presentElementCount(), "（应为 100）");

                list.discardAll();
                list.awaitMerging();
                LOG("discardAll 后 present = ", list.presentElementCount(), "（应为 0）");
                LOG("discardAll 后 isEmpty = ", list.isEmpty() ? "PASS" : "FAIL");

                auto list2 = IMPList<int>(64, 1024);
                list2.discardAll();
                list2.awaitMerging();
                LOG("空列表 discardAll → present = ", list2.presentElementCount(), "（应为 0）");

                list.discardAll();
                list.discardAll();
                list.awaitMerging();
                LOG("重复 discardAll 幂等 → present = ", list.presentElementCount(), "（应为 0）");

                list.add(999);
                list.awaitMerging();
                LOG("discardAll 后 add：present = ", list.presentElementCount(), "（应为 1）");
                LOG("值正确：", *((const int*) list.get(0)) == 999 ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] discardAll 窗口语义（窗口内 add 丢弃） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 50; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                LOG("discardAll 前 present = ", list.presentElementCount(), "（应为 50）");

                list.discardAll();
                for (int i = 0; i < 20; ++i) {
                    list.add(100 + i);
                }

                list.tryMerging();
                list.awaitMerging();
                LOG("窗口内 add 20 个 → present = ", list.presentElementCount(), "（窗口语义：应为 0）");
                LOG("窗口内 add 全部丢弃：", list.presentElementCount() == 0 ? "PASS" : "FAIL");

                list.configureMaintenanceMerging(true);
                list.configureCompaction(true, 20, 10000);
                list.add(200);
                list.awaitMerging();
                LOG("窗口结束后 add：present = ", list.presentElementCount(), "（应为 1）");
                LOG("窗口结束后 add 正常保留：", list.presentElementCount() == 1 && *((const int*) list.get(0)) == 200 ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] copy / move 深度语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto original = IMPList<int>(16, 1024);
                for (int i = 0; i < 200; ++i) {
                    original.add(i * 3);
                }
                original.tryMerging();
                original.awaitMerging();

                auto copied = original;
                LOG("copy 后 original.present = ", original.presentElementCount(), "，copied.present = ", copied.presentElementCount(), "（应均为 200）");
                LOG("copy 后 original.isValid = ", original.isValid() ? "PASS" : "FAIL", "，copied.isValid = ", copied.isValid() ? "PASS" : "FAIL", "（应均为 PASS）");

                copied.add(999);
                copied.tryMerging();
                copied.awaitMerging();
                LOG("copied 加 1 个 → original.present = ", original.presentElementCount(), "（应为 200）");
                LOG("copied.present = ", copied.presentElementCount(), "（应为 201）");

                bool origIntact = true;
                for (int i = 0; i < 200; ++i) {
                    if (*original.get((usize) i) != i * 3) {
                        origIntact = false;
                        break;
                    }
                }
                LOG("original 内容未变：", origIntact ? "PASS" : "FAIL");

                auto source = IMPList<int>(64, 1024);
                for (int i = 0; i < 100; ++i) {
                    source.add(i);
                }
                source.tryMerging();
                source.awaitMerging();

                LOG("move 构造前 source.isValid = ", source.isValid() ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("move 构造前 source.present = ", source.presentElementCount(), "（应为 100）");

                auto moved = std::move(source);
                LOG("move 构造后 moved.isValid = ", moved.isValid() ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("move 构造后 moved.present = ", moved.presentElementCount(), "（应为 100）");
                LOG("move 构造后 source.isValid = ", !source.isValid() ? "PASS" : "FAIL", "（应为 PASS，源已失效）");

                bool movedContentOk = true;
                for (int i = 0; i < 100; ++i) {
                    if (*moved.get((usize) i) != i) {
                        movedContentOk = false;
                        break;
                    }
                }
                LOG("moved 内容完整：", movedContentOk ? "PASS" : "FAIL");

                auto target = IMPList<int>(64, 1024);
                target.add(999);
                target.tryMerging();
                target.awaitMerging();
                LOG("move 赋值前 target.isValid = ", target.isValid() ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("move 赋值前 target.present = ", target.presentElementCount(), "（应为 1）");

                target = std::move(moved);
                LOG("move 赋值后 target.isValid = ", target.isValid() ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("move 赋值后 target.present = ", target.presentElementCount(), "（应为 100）");
                LOG("move 赋值后 moved.isValid = ", !moved.isValid() ? "PASS" : "FAIL", "（应为 PASS，源已失效）");

                bool targetContentOk = true;
                for (int i = 0; i < 100; ++i) {
                    if (*target.get((usize) i) != i) {
                        targetContentOk = false;
                        break;
                    }
                }
                LOG("target 内容完整（覆盖了旧内容）：", targetContentOk ? "PASS" : "FAIL");

                auto self = IMPList<int>(64, 1024);
                self.add(42);
                self.tryMerging();
                self.awaitMerging();
                auto& selfRef = self;
                self = std::move(selfRef);
                LOG("自移动赋值后 self.isValid = ", self.isValid() ? "PASS" : "FAIL", "（应为 PASS，自赋值被保护）");
                LOG("自移动赋值后 self.present = ", self.presentElementCount(), "（应为 1）");
            }

            LOG("--- [正确性] 排序策略 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);
                const int values[] = { 5, 2, 8, 1, 9, 3, 7, 4, 6, 0 };
                for (int v : values) {
                    list.add(v);
                }
                list.awaitMerging();
                list.naturalSort();

                bool asc = true;
                for (usize i = 1; i < list.presentElementCount(); ++i) {
                    if (*list.get(i - 1) > *list.get(i)) {
                        asc = false;
                        break;
                    }
                }
                LOG("naturalSort → 升序：", asc ? "PASS" : "FAIL");

                auto list2 = IMPList<int>(16, 1024);
                for (int v : values) {
                    list2.add(v);
                }
                list2.awaitMerging();
                list2.naturalSortDescending();

                bool desc = true;
                for (usize i = 1; i < list2.presentElementCount(); ++i) {
                    if (*list2.get(i - 1) < *list2.get(i)) {
                        desc = false;
                        break;
                    }
                }
                LOG("naturalSortDescending → 降序：", desc ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] find / contains / indexOf 查找（UNORDERED 线性路径） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i * 2);
                }
                list.tryMerging();
                list.awaitMerging();

                const int* const found = list.find(50);
                LOG("find(50) = ", found ? "命中" : "未命中", "（应为命中）");
                LOG("  *found = ", found ? *found : -1, "（应为 50）");

                LOG("find(51) = ", list.find(51) ? "命中" : "未命中", "（应为未命中）");
                LOG("find(0) = ", list.find(0) ? "命中" : "未命中", "（首元素，应为命中）");
                LOG("find(198) = ", list.find(198) ? "命中" : "未命中", "（尾元素，应为命中）");
                LOG("find(200) = ", list.find(200) ? "命中" : "未命中", "（越界，应为未命中）");

                LOG("contains(50) = ", list.contains(50), "（应为 1）");
                LOG("contains(51) = ", list.contains(51), "（应为 0）");

                LOG("firstIndexOf(50) = ", list.firstIndexOf(50), "（应为 25）");
                LOG("lastIndexOf(50) = ", list.lastIndexOf(50), "（应为 25）");
                LOG("firstIndexOf(1) = ", list.firstIndexOf(1), "（应为 -1）");
                LOG("lastIndexOf(1) = ", list.lastIndexOf(1), "（应为 -1）");
            }

            LOG("--- [正确性] find / contains / indexOf 查找（ASCENDING 二分路径，与线性路径行为一致） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i * 2);
                }
                list.naturalSort();
                list.awaitMerging();

                const int* const found = list.find(50);
                LOG("find(50) = ", found ? "命中" : "未命中", "（应为命中）");
                LOG("  *found = ", found ? *found : -1, "（应为 50）");

                LOG("find(51) = ", list.find(51) ? "命中" : "未命中", "（应为未命中）");
                LOG("find(0) = ", list.find(0) ? "命中" : "未命中", "（首元素，应为命中）");
                LOG("find(198) = ", list.find(198) ? "命中" : "未命中", "（尾元素，应为命中）");
                LOG("find(200) = ", list.find(200) ? "命中" : "未命中", "（越界，应为未命中）");
                LOG("find(-2) = ", list.find(-2) ? "命中" : "未命中", "（越界，应为未命中）");

                LOG("contains(50) = ", list.contains(50), "（应为 1）");
                LOG("contains(51) = ", list.contains(51), "（应为 0）");

                LOG("firstIndexOf(50) = ", list.firstIndexOf(50), "（应为 25）");
                LOG("lastIndexOf(50) = ", list.lastIndexOf(50), "（应为 25）");
                LOG("firstIndexOf(1) = ", list.firstIndexOf(1), "（应为 -1）");
                LOG("lastIndexOf(1) = ", list.lastIndexOf(1), "（应为 -1）");
            }

            LOG("--- [正确性] 重复元素下 firstIndexOf / lastIndexOf（二分等价区间） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);
                for (int i = 0; i < 10; ++i) {
                    list.add(i / 3);
                }
                list.naturalSort();
                list.awaitMerging();

                LOG("排序后 present = ", list.presentElementCount(), "（应为 10）");
                LOG("firstIndexOf(0) = ", list.firstIndexOf(0), "（应为 0）");
                LOG("lastIndexOf(0) = ", list.lastIndexOf(0), "（应为 2）");
                LOG("firstIndexOf(1) = ", list.firstIndexOf(1), "（应为 3）");
                LOG("lastIndexOf(1) = ", list.lastIndexOf(1), "（应为 5）");
                LOG("firstIndexOf(2) = ", list.firstIndexOf(2), "（应为 6）");
                LOG("lastIndexOf(2) = ", list.lastIndexOf(2), "（应为 8）");
                LOG("firstIndexOf(3) = ", list.firstIndexOf(3), "（应为 9）");
                LOG("lastIndexOf(3) = ", list.lastIndexOf(3), "（应为 9）");
                LOG("contains(4) = ", list.contains(4), "（应为 0）");
            }

            LOG("--- [正确性] containsAll 语义（全命中 / 部分命中 / 全未命中 / 重复 / 空 / NULL） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);
                for (int i = 0; i < 10; ++i) {
                    list.add(i);
                }
                list.naturalSort();
                list.awaitMerging();

                const int allHit[] = { 0, 5, 9 };
                const int partialMiss[] = { 0, 5, 10 };
                const int allMiss[] = { 100, 200, 300 };
                const int duplicates[] = { 5, 5, 5 };

                LOG("containsAll(all hit) = ", list.containsAll(allHit, 3), "（应为 1）");
                LOG("containsAll(partial miss) = ", list.containsAll(partialMiss, 3), "（应为 0）");
                LOG("containsAll(all miss) = ", list.containsAll(allMiss, 3), "（应为 0）");
                LOG("containsAll(duplicates) = ", list.containsAll(duplicates, 3), "（应为 1）");
                LOG("containsAll(空) = ", list.containsAll(allHit, 0), "（应为 1）");
                LOG("containsAll(NULL) = ", list.containsAll((const int*) nullptr, 3), "（应为 0）");
            }

            LOG("--- [正确性] awaitMerging 语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.awaitMerging();
                LOG("awaitMerging 后 present = ", list.presentElementCount(), "（应为 100）");

                const bool timeoutResult = list.awaitMerging(1);
                LOG("已合并状态 awaitMerging(1ms) = ", timeoutResult ? "PASS" : "FAIL", "（应为 PASS）");
            }

            LOG("--- [正确性] awaitCompacting 语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.awaitMerging();

                const bool timeoutResult = list.awaitCompacting(1);
                LOG("已空闲状态 awaitCompacting(1ms) = ", timeoutResult ? "PASS" : "FAIL", "（应为 PASS）");
            }

            LOG("--- [正确性] compactionCount 递增 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                LOG("初始 compactionCount = ", list.compactionCount(), "（应为 0）");

                for (int i = 0; i < 5000; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                LOG("remove 后 present = ", list.presentElementCount(), "（应为 5000）");
                LOG("remove 后 compactionCount = ", list.compactionCount(), "（仍应为 0，未触发 compact）");

                list.tryCompacting();

                LOG("compact 后 compactionCount = ", list.compactionCount(), "（应为 1）");
                LOG("compact 后 present = ", list.presentElementCount(), "（仍应为 5000，compact 不改变可见元素）");
            }

            LOG("--- [正确性] compact 后 ingestElementCount 下降（内存收敛验证） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                for (int i = 0; i < 8000; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                const usize before = list.ingestElementCount();
                LOG("compact 前 ingestElementCount = ", before, "（应接近 10000，含墓碑）");

                list.tryCompacting();

                const usize after = list.ingestElementCount();
                LOG("compact 后 ingestElementCount = ", after, "（应接近 2000，墓碑已清理）");
                LOG("内存收敛：", after < before ? "PASS" : "FAIL");
                LOG("present 不变：", list.presentElementCount() == 2000 ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] compact 后 add 正常保留 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 5000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                for (int i = 0; i < 4000; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                list.tryCompacting();

                const usize beforeAdd = list.presentElementCount();
                for (int i = 10000; i < 10500; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                LOG("compact 后 add 500 个 → present = ", list.presentElementCount(), "（应为 ", beforeAdd + 500, "）");
                LOG("add 保留正常：", list.presentElementCount() == beforeAdd + 500 ? "PASS" : "FAIL");

                bool newElementsOk = true;
                for (int i = 10000; i < 10500; ++i) {
                    if (!list.contains(i)) {
                        newElementsOk = false;
                        break;
                    }
                }
                LOG("新增元素全部保留：", newElementsOk ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] compact 后 merge 反映清理结果 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 5000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                for (int i = 0; i < 4500; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                list.tryCompacting();

                list.tryMerging();
                list.awaitMerging();

                bool contentOk = true;
                for (int i = 4500; i < 5000; ++i) {
                    if (!list.contains(i)) {
                        contentOk = false;
                        break;
                    }
                }
                LOG("compact + merge 后剩余元素完整：", contentOk ? "PASS" : "FAIL", "（present = ", list.presentElementCount(), "，应为 500）");
            }

            LOG("--- [正确性] compact 阈值控制（不同 threshold 影响重建范围） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto aggressive = IMPList<int>(64, 1024);
                aggressive.configureMaintenanceMerging(false);
                aggressive.configureCompaction(true, 0, 10000);

                for (int i = 0; i < 5000; ++i) {
                    aggressive.add(i);
                }
                aggressive.tryMerging();
                aggressive.awaitMerging();
                for (int i = 0; i < 1000; ++i) {
                    aggressive.remove(i * 5);
                }
                aggressive.tryMerging();
                aggressive.awaitMerging();
                const usize aggressiveBefore = aggressive.ingestElementCount();
                aggressive.tryCompacting();
                const usize aggressiveAfter = aggressive.ingestElementCount();

                LOG("threshold=0：ingest ", aggressiveBefore, " → ", aggressiveAfter, "（应下降，任何有墓碑的 chunk 都重建）");
                LOG("threshold=0 触发：", aggressiveAfter < aggressiveBefore ? "PASS" : "FAIL");

                auto conservative = IMPList<int>(64, 1024);
                conservative.configureMaintenanceMerging(false);
                conservative.configureCompaction(true, 99, 10000);

                for (int i = 0; i < 5000; ++i) {
                    conservative.add(i);
                }
                conservative.tryMerging();
                conservative.awaitMerging();
                for (int i = 0; i < 1000; ++i) {
                    conservative.remove(i * 5);
                }
                conservative.tryMerging();
                conservative.awaitMerging();
                const usize conservativeBefore = conservative.ingestElementCount();
                conservative.tryCompacting();
                const usize conservativeAfter = conservative.ingestElementCount();

                LOG("threshold=99：ingest ", conservativeBefore, " → ", conservativeAfter, "（应基本不变，每 chunk 墓碑占比仅 ~20%，未达 99%）");
                LOG("threshold=99 保守：", conservativeAfter == conservativeBefore ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] compact 关闭时显式 tryCompacting 仍生效 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 5000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                for (int i = 0; i < 4000; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                const usize before = list.ingestElementCount();
                list.tryCompacting();
                const usize after = list.ingestElementCount();

                LOG("关闭自动 + 显式 tryCompacting：ingest ", before, " → ", after, "（应下降）");
                LOG("显式调用仍生效：", after < before ? "PASS" : "FAIL");
            }

            LOG("--- [正确性] 重复 tryCompacting 幂等 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                for (int i = 0; i < 500; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                list.tryCompacting();
                const usize countAfterFirst = list.compactionCount();
                const usize ingestAfterFirst = list.ingestElementCount();

                list.tryCompacting();
                list.tryCompacting();

                LOG("3 次 tryCompacting 后 compactionCount = ", list.compactionCount(), "（应为 ", countAfterFirst + 2, "）");
                LOG("ingestElementCount 稳定 = ", list.ingestElementCount(), "（应为 ", ingestAfterFirst, "，无变化）");
                LOG("present 不变 = ", list.presentElementCount(), "（应为 500）");
            }

            LOG("--- [并发] 4 写线程 × 10000 元素：计数 + 完整性 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1024);
                constexpr int kThreadCount = 4;
                constexpr int kElementsPerThread = 10000;

                std::vector<std::thread> threads;
                for (int t = 0; t < kThreadCount; ++t) {
                    threads.emplace_back([&list, t, kElementsPerThread] {
                        for (int i = 0; i < kElementsPerThread; ++i) {
                            list.add(t * kElementsPerThread + i);
                        }
                    });
                }
                for (auto& thread : threads) {
                    thread.join();
                }

                list.awaitMerging();

                constexpr usize expected = (usize) kThreadCount * (usize) kElementsPerThread;
                LOG("present = ", list.presentElementCount(), "（应为 ", expected, "）");

                std::vector<int> seen((usize) kThreadCount, 0);
                bool dataIntact = true;
                for (usize i = 0; i < list.presentElementCount(); ++i) {
                    const int v = *list.get(i);
                    const int tid = v / kElementsPerThread;
                    const int idx = v % kElementsPerThread;
                    if (tid < 0 || tid >= kThreadCount || idx < 0 || idx >= kElementsPerThread) {
                        dataIntact = false;
                        break;
                    }
                    seen[(usize) tid]++;
                }
                bool allSeen = true;
                for (int t = 0; t < kThreadCount; ++t) {
                    if (seen[(usize) t] != kElementsPerThread) {
                        allSeen = false;
                    }
                }
                LOG("数据完整性：", (dataIntact && allSeen) ? "PASS" : "FAIL");
            }

            LOG("--- [并发] 并发 discard 与 add ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                std::vector<std::thread> threads;
                for (int t = 0; t < 4; ++t) {
                    threads.emplace_back([&list, t]() {
                        for (int i = 0; i < 2500; ++i) {
                            list.discard(t * 2500 + i);
                        }
                    });
                }
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();
                LOG("并发 discard 10000 元素后 present = ", list.presentElementCount(), "（应为 0）");
            }

            LOG("--- [并发] discardAll 与并发 add 窗口语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                LOG("discardAll 前 present = ", list.presentElementCount(), "（应为 10000）");

                list.discardAll();

                std::vector<std::thread> threads;
                for (int t = 0; t < 4; ++t) {
                    threads.emplace_back([&list, t]() {
                        for (int i = 0; i < 2500; ++i) {
                            list.add(t * 2500 + i);
                        }
                    });
                }
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();
                LOG("4 线程并发 add 10000 元素 → present = ", list.presentElementCount(), "（窗口语义：应为 0）");
                LOG("并发 add 全部丢弃：", list.presentElementCount() == 0 ? "PASS" : "FAIL");

                list.configureMaintenanceMerging(true);
                list.configureCompaction(true, 20, 10000);
            }

            LOG("--- [并发] remove 与 merge 并发正确性 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();
                LOG("初始 present = ", list.presentElementCount(), "（应为 10000）");

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    for (int i = 0; i < 2500; ++i) {
                        list.remove(i);
                    }
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 50; ++round) {
                        list.tryMerging();
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();

                bool noRemoved = true;
                for (int i = 0; i < 2500; ++i) {
                    if (list.contains(i)) {
                        noRemoved = false;
                        break;
                    }
                }
                LOG("remove 的元素最终全部消失：", noRemoved ? "PASS" : "FAIL", "（present = ", list.presentElementCount(), "）");

                bool restIntact = true;
                for (int i = 2500; i < 10000; ++i) {
                    if (!list.contains(i)) {
                        restIntact = false;
                        break;
                    }
                }
                LOG("未 remove 的元素保留：", restIntact ? "PASS" : "FAIL");
            }

            LOG("--- [并发] 多线程并发 remove 不同范围 + merge ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 20000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                constexpr int kThreadCount = 4;
                constexpr int kPerThread = 5000;

                std::vector<std::thread> threads;
                for (int t = 0; t < kThreadCount; ++t) {
                    threads.emplace_back([&list, t, kPerThread] {
                        for (int i = 0; i < kPerThread; ++i) {
                            list.remove(t * kPerThread + i);
                        }
                    });
                }
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();
                LOG("4 线程并发 remove 20000 元素后 present = ", list.presentElementCount(), "（应为 0）");
                LOG("isEmpty = ", list.isEmpty() ? "PASS" : "FAIL");
            }

            LOG("--- [并发] removeIf 与 merge 并发 + 语义一致性 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    list.removeIf(intEvenPredicate);
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 50; ++round) {
                        list.tryMerging();
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();

                bool allOdd = true;
                for (usize i = 0; i < list.presentElementCount(); ++i) {
                    if (*list.get(i) % 2 == 0) {
                        allOdd = false;
                        break;
                    }
                }
                LOG("removeIf 偶数 + merge 并发后全为奇数：", allOdd ? "PASS" : "FAIL", "（present = ", list.presentElementCount(), "，应为 5000）");
            }

            LOG("--- [并发] discard + remove + merge 三方并发（discardBuffer 去锁验证） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    for (int i = 0; i < 2500; ++i) {
                        list.discard(i);
                    }
                });
                threads.emplace_back([&list]() {
                    for (int i = 5000; i < 7500; ++i) {
                        list.remove(i);
                    }
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 50; ++round) {
                        list.tryMerging();
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();

                bool removedAll = true;
                for (int i = 0; i < 2500; ++i) {
                    if (list.contains(i)) {
                        removedAll = false;
                        break;
                    }
                }
                for (int i = 5000; i < 7500; ++i) {
                    if (list.contains(i)) {
                        removedAll = false;
                        break;
                    }
                }
                LOG("discard+remove+merge 并发后目标元素消失：", removedAll ? "PASS" : "FAIL", "（present = ", list.presentElementCount(), "，应为 5000）");
            }

            LOG("--- [并发] ingest / mergeElementCount 与 merge 并发（无锁遍历安全性） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    for (int i = 0; i < 5000; ++i) {
                        list.add(10000 + i);
                    }
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 100; ++round) {
                        volatile usize i = list.ingestElementCount();
                        volatile usize m = list.mergeElementCount();
                        (void) i;
                        (void) m;
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();
                LOG("ingest/mergeElementCount 与 add 并发无崩溃：PASS（present = ", list.presentElementCount(), "，应为 15000）");
            }

            LOG("--- [并发] compact 与 remove 并发正确性 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                for (int i = 0; i < 5000; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                LOG("compact 前 present = ", list.presentElementCount(), "（应为 5000）");

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    for (int i = 5000; i < 7500; ++i) {
                        list.remove(i);
                    }
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 10; ++round) {
                        list.tryCompacting();
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();

                bool removedAll = true;
                for (int i = 0; i < 7500; ++i) {
                    if (list.contains(i)) {
                        removedAll = false;
                        break;
                    }
                }
                LOG("compact + remove 并发后 0~7499 全消失：", removedAll ? "PASS" : "FAIL", "（present = ", list.presentElementCount(), "，应为 2500）");

                bool intact = true;
                for (int i = 7500; i < 10000; ++i) {
                    if (!list.contains(i)) {
                        intact = false;
                        break;
                    }
                }
                LOG("未 remove 的元素保留：", intact ? "PASS" : "FAIL");
            }

            LOG("--- [并发] compact 与 add 并发（add 不阻塞） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 5000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                for (int i = 0; i < 4000; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    for (int i = 10000; i < 15000; ++i) {
                        list.add(i);
                    }
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 10; ++round) {
                        list.tryCompacting();
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();

                bool newElementsOk = true;
                for (int i = 10000; i < 15000; ++i) {
                    if (!list.contains(i)) {
                        newElementsOk = false;
                        break;
                    }
                }
                LOG("compact 与 add 并发：新 add 元素全部保留：", newElementsOk ? "PASS" : "FAIL", "（present = ", list.presentElementCount(), "）");
            }

            LOG("--- [并发] compact 与 count 并发（无锁遍历安全性） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                for (int i = 0; i < 5000; ++i) {
                    list.remove(i);
                }
                list.tryMerging();
                list.awaitMerging();

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 10; ++round) {
                        list.tryCompacting();
                    }
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 200; ++round) {
                        volatile usize i = list.ingestElementCount();
                        volatile usize m = list.mergeElementCount();
                        (void) i;
                        (void) m;
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                LOG("compact 与 count 并发无崩溃：PASS（present = ", list.presentElementCount(), "，应为 5000）");
            }

            LOG("--- [并发] compact 与 merge 并发（tryCompacting 排队） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(true, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                for (int i = 0; i < 8000; ++i) {
                    list.remove(i);
                }

                std::vector<std::thread> threads;
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 20; ++round) {
                        list.tryMerging();
                    }
                });
                threads.emplace_back([&list]() {
                    for (int round = 0; round < 20; ++round) {
                        list.tryCompacting();
                    }
                });
                for (auto& t : threads) {
                    t.join();
                }

                list.tryMerging();
                list.awaitMerging();
                list.tryCompacting();
                list.tryMerging();
                list.awaitMerging();

                LOG("compact + merge 并发后 present = ", list.presentElementCount(), "（应为 2000）");
                LOG("compact + merge 并发后 compactionCount = ", list.compactionCount(), "（应 >= 1）");
            }

            LOG("--- [并发] 高压 remove + merge 反复压力测试 ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kRounds = 100;
                bool ok = true;
                for (u32 r = 0; r < kRounds; ++r) {
                    auto list = IMPList<int>(64, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    for (int i = 0; i < 2000; ++i) {
                        list.add(i);
                    }
                    list.tryMerging();

                    std::vector<std::thread> threads;
                    threads.emplace_back([&list]() {
                        for (int i = 0; i < 500; ++i) {
                            list.remove(i);
                        }
                    });
                    threads.emplace_back([&list]() {
                        for (int i = 500; i < 1000; ++i) {
                            list.remove(i);
                        }
                    });
                    threads.emplace_back([&list]() {
                        for (int i = 0; i < 10; ++i) {
                            list.tryMerging();
                        }
                    });
                    for (auto& t : threads) {
                        t.join();
                    }

                    list.tryMerging();
                    list.awaitMerging();

                    bool intact = true;
                    for (int i = 0; i < 1000; ++i) {
                        if (list.contains(i)) {
                            intact = false;
                            break;
                        }
                    }
                    if (intact) {
                        for (int i = 1000; i < 2000; ++i) {
                            if (!list.contains(i)) {
                                intact = false;
                                break;
                            }
                        }
                    }
                    if (!intact) {
                        ok = false;
                        break;
                    }
                }
                LOG(kRounds, " 轮 remove + merge 并发压力：", ok ? "PASS" : "FAIL");
            }

            LOG("--- [并发] 高压 compact + remove + add 反复压力测试 ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kRounds = 100;
                bool ok = true;
                for (u32 r = 0; r < kRounds; ++r) {
                    auto list = IMPList<int>(64, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(true, 20, 10000);

                    for (int i = 0; i < 5000; ++i) {
                        list.add(i);
                    }
                    list.tryMerging();
                    list.awaitMerging();

                    std::vector<std::thread> threads;
                    threads.emplace_back([&list]() {
                        for (int i = 0; i < 2000; ++i) {
                            list.remove(i);
                        }
                    });
                    threads.emplace_back([&list]() {
                        for (int i = 10000; i < 12000; ++i) {
                            list.add(i);
                        }
                    });
                    threads.emplace_back([&list]() {
                        for (int round = 0; round < 5; ++round) {
                            list.tryCompacting();
                        }
                    });
                    for (auto& t : threads) {
                        t.join();
                    }

                    list.tryMerging();
                    list.awaitMerging();
                    list.tryCompacting();
                    list.tryMerging();
                    list.awaitMerging();

                    bool intact = true;
                    for (int i = 0; i < 2000; ++i) {
                        if (list.contains(i)) {
                            intact = false;
                            break;
                        }
                    }
                    if (intact) {
                        for (int i = 2000; i < 5000; ++i) {
                            if (!list.contains(i)) {
                                intact = false;
                                break;
                            }
                        }
                    }
                    if (intact) {
                        for (int i = 10000; i < 12000; ++i) {
                            if (!list.contains(i)) {
                                intact = false;
                                break;
                            }
                        }
                    }
                    if (!intact) {
                        ok = false;
                        break;
                    }
                }
                LOG(kRounds, " 轮 compact + remove + add 并发压力：", ok ? "PASS" : "FAIL");
            }

            LOG("--- [并发] 线程退出（写线程 join 触发 threadExitCallback） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureCompaction(false, 20, 10000);

                for (int round = 0; round < 100; ++round) {
                    std::thread t([&list, round]() {
                        for (int i = 0; i < 50; ++i) {
                            list.add(round * 50 + i);
                        }
                    });
                    t.join();
                }

                list.awaitMerging();
                const usize finalCount = list.presentElementCount();
                LOG("100 轮线程退出后 present = ", finalCount, "（应 >= 5000，含未合并元素）");
                LOG("总元素数一致：", finalCount + list.ingestElementCount() >= 5000 ? "PASS" : "FAIL");
            }

            LOG("--- [内存] 反复构造析构稳定性 ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kRounds = 200;
                for (u32 r = 0; r < kRounds; ++r) {
                    auto list = IMPList<int>(128, 1024);
                    list.configureCompaction(false, 20, 10000);
                    for (int i = 0; i < 500; ++i) {
                        list.add(i);
                    }
                    list.tryMerging();
                    auto copy = list;
                    copy.add(999);
                    copy.tryMerging();
                }
                LOG(kRounds, " 轮构造/拷贝/析构：PASS（无崩溃即无灾难性泄漏）");
            }

            LOG("--- [内存] compact 长期运行内存收敛稳定性（200 轮 add + remove + compact） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                constexpr u32 kRounds = 200;
                usize maxIngest = 0;
                usize lastIngest = 0;

                for (u32 r = 0; r < kRounds; ++r) {
                    const int base = (int) (r * 1000);
                    for (int i = 0; i < 1000; ++i) {
                        list.add(base + i);
                    }
                    for (int i = 0; i < 900; ++i) {
                        list.remove(base + i);
                    }
                    list.tryMerging();

                    if ((r + 1) % 20 == 0) {
                        list.tryCompacting();
                    }

                    if ((r + 1) % 10 == 0) {
                        const usize ingest = list.ingestElementCount();
                        if (ingest > maxIngest) {
                            maxIngest = ingest;
                        }
                        lastIngest = ingest;
                    }
                }

                list.tryCompacting();
                list.tryMerging();
                list.awaitMerging();

                LOG("200 轮后 present = ", list.presentElementCount(), "（应为 20000，每轮净增 100）");
                LOG("ingest 峰值 = ", maxIngest, "，最终 = ", lastIngest, "（应远小于累积写入 200000）");
                LOG("内存收敛有效：", lastIngest < maxIngest ? "PASS" : "FAIL");
            }

            LOG("--- [性能] 单线程 add 延迟（完整 CAS，跳过合并数据处理，带预热，trial 内子批 IQR 剔除 + 全局 IQR，TSC 计时） ---");
            LOGGER_INDENT_BLOCK(true) {
                static u64 samples[BENCH_TRIALS];
                constexpr u32 kBatch = 20000;
                constexpr u32 kSubBatchSize = 100;
                constexpr u32 kSubBatchCount = kBatch / kSubBatchSize;

                static u64 subSamples[kSubBatchCount];
                TscTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    auto list = IMPList<int>(16384, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    list.add(0);

                    u32 writeIndex = 0;
                    for (u32 s = 0; s < kSubBatchCount; ++s) {
                        timer.start();
                        for (u32 i = 0; i < kSubBatchSize; ++i) {
                            list.add((int) (writeIndex++));
                        }
                        subSamples[s] = timer.stopTicks();
                    }

                    const u64 ticksMean = computeTrialMeanWithIqr(subSamples, kSubBatchCount, 1.5, 2.0);
                    samples[t] = (u64) (timer.ticksToScaledNs(ticksMean, 10.0 / (double) kSubBatchSize) + 0.5);
                }

                sortU64(samples, BENCH_TRIALS);
                logPerOp("add 延迟", samples, BENCH_TRIALS, 10.0);
            }

            LOG("--- [性能] 单线程 discard 延迟（完整 CAS，跳过合并数据处理，带预热，trial 内子批 IQR 剔除 + 全局 IQR，TSC 计时） ---");
            LOGGER_INDENT_BLOCK(true) {
                static u64 samples[BENCH_TRIALS];
                constexpr u32 kBatch = 20000;
                constexpr u32 kSubBatchSize = 100;
                constexpr u32 kSubBatchCount = kBatch / kSubBatchSize;

                static u64 subSamples[kSubBatchCount];
                TscTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    auto list = IMPList<int>(16384, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    list.discard(0);

                    u32 writeIndex = 0;
                    for (u32 s = 0; s < kSubBatchCount; ++s) {
                        timer.start();
                        for (u32 i = 0; i < kSubBatchSize; ++i) {
                            list.discard((int) (writeIndex++));
                        }
                        subSamples[s] = timer.stopTicks();
                    }

                    const u64 ticksMean = computeTrialMeanWithIqr(subSamples, kSubBatchCount, 1.5, 2.0);
                    samples[t] = (u64) (timer.ticksToScaledNs(ticksMean, 10.0 / (double) kSubBatchSize) + 0.5);
                }

                sortU64(samples, BENCH_TRIALS);
                logPerOp("discard 延迟", samples, BENCH_TRIALS, 10.0);
            }

            LOG("--- [性能] 单线程 discardAll 调用延迟（O(1) 标志设置，不含合并，带预热，trial 内子批 IQR 剔除 + 全局 IQR，TSC 计时） ---");
            LOGGER_INDENT_BLOCK(true) {
                static u64 samples[BENCH_TRIALS];
                constexpr u32 kBatch = 20000;
                constexpr u32 kSubBatchSize = 100;
                constexpr u32 kSubBatchCount = kBatch / kSubBatchSize;

                static u64 subSamples[kSubBatchCount];
                TscTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    auto list = IMPList<int>(16384, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    list.discardAll();

                    for (u32 s = 0; s < kSubBatchCount; ++s) {
                        timer.start();
                        for (u32 i = 0; i < kSubBatchSize; ++i) {
                            list.discardAll();
                        }
                        subSamples[s] = timer.stopTicks();
                    }

                    const u64 ticksMean = computeTrialMeanWithIqr(subSamples, kSubBatchCount, 1.5, 2.0);
                    samples[t] = (u64) (timer.ticksToScaledNs(ticksMean, 10.0 / (double) kSubBatchSize) + 0.5);
                }

                sortU64(samples, BENCH_TRIALS);
                logPerOp("discardAll 调用延迟", samples, BENCH_TRIALS, 10.0);
            }

            LOG("--- [性能] add + merge 混合吞吐（不同规模，检验线性） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 sizes[] = { 1000, 10000, 100000, 1000000 };

                for (u32 n : sizes) {
                    static u64 samples[BENCH_TRIALS];
                    QpcTimer timer;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        auto list = IMPList<int>(1024, 1024);
                        list.configureMaintenanceMerging(false);
                        list.configureCompaction(false, 20, 10000);

                        list.add(0);
                        list.tryMerging();

                        timer.start();
                        for (u32 i = 0; i < n; ++i) {
                            list.add((int) i);
                        }
                        list.tryMerging();
                        samples[t] = timer.stopNs() * 100 / n;
                    }

                    sortU64(samples, BENCH_TRIALS);
                    logPerOp("add+merge 吞吐", samples, BENCH_TRIALS, 100.0);
                }
            }

            LOG("--- [性能] get 快照访问延迟 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 100000; ++i) {
                    list.add(i);
                }
                list.tryMerging();

                static u64 samples[BENCH_TRIALS];
                constexpr u32 kIters = 20000000;
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    timer.start();
                    int accum = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        const int* v = list.get((usize) (i % 100000));
                        accum += *v;
                    }
                    samples[t] = timer.stopNs() * 10 / kIters;
                    if (accum == 0xDEADBEEF) {
                        LOG("impossible");
                    }
                }

                sortU64(samples, BENCH_TRIALS);
                logPerOp("get 延迟", samples, BENCH_TRIALS, 10.0);
            }

            LOG("--- [性能] contains 扫描延迟（命中/未命中） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.tryMerging();

                static u64 hitSamples[BENCH_TRIALS];
                static u64 missSamples[BENCH_TRIALS];
                constexpr u32 kIters = 100000;
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    timer.start();
                    u32 hits = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (list.contains((int) (i % 1000))) {
                            hits++;
                        }
                    }
                    hitSamples[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    u32 misses = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (!list.contains((int) (10000 + i))) {
                            misses++;
                        }
                    }
                    missSamples[t] = timer.stopNs() * 100 / kIters;

                    if (hits + misses == 0xDEADBEEF) {
                        LOG("impossible");
                    }
                }

                sortU64(hitSamples, BENCH_TRIALS);
                sortU64(missSamples, BENCH_TRIALS);

                logPerOp("contains（命中）", hitSamples, BENCH_TRIALS, 100.0);
                logPerOp("contains（未命中）", missSamples, BENCH_TRIALS, 100.0);
            }

            LOG("--- [性能] contains 延迟（UNORDERED 线性 vs ASCENDING 二分，1000 元素） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kElements = 1000;
                constexpr u32 kIters = 100000;

                auto unorderedList = IMPList<int>(1024, 1024);
                auto sortedList = IMPList<int>(1024, 1024);
                unorderedList.configureMaintenanceMerging(false);
                unorderedList.configureCompaction(false, 20, 10000);
                sortedList.configureMaintenanceMerging(false);
                sortedList.configureCompaction(false, 20, 10000);

                for (u32 i = 0; i < kElements; ++i) {
                    unorderedList.add((int) i);
                    sortedList.add((int) i);
                }
                unorderedList.tryMerging();
                sortedList.naturalSort();

                static u64 hitUnordered[BENCH_TRIALS];
                static u64 hitSorted[BENCH_TRIALS];
                static u64 missUnordered[BENCH_TRIALS];
                static u64 missSorted[BENCH_TRIALS];
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    timer.start();
                    u32 hitsU = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (unorderedList.contains((int) (i % kElements))) {
                            hitsU++;
                        }
                    }
                    hitUnordered[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    u32 hitsS = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (sortedList.contains((int) (i % kElements))) {
                            hitsS++;
                        }
                    }
                    hitSorted[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    u32 missesU = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (!unorderedList.contains((int) (10000 + i))) {
                            missesU++;
                        }
                    }
                    missUnordered[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    u32 missesS = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (!sortedList.contains((int) (10000 + i))) {
                            missesS++;
                        }
                    }
                    missSorted[t] = timer.stopNs() * 100 / kIters;

                    if (hitsU + hitsS + missesU + missesS == 0xDEADBEEF) {
                        LOG("impossible");
                    }
                }

                sortU64(hitUnordered, BENCH_TRIALS);
                sortU64(hitSorted, BENCH_TRIALS);
                sortU64(missUnordered, BENCH_TRIALS);
                sortU64(missSorted, BENCH_TRIALS);

                const double mHU = meanIqr(hitUnordered, BENCH_TRIALS) / 100.0;
                const double mHS = meanIqr(hitSorted, BENCH_TRIALS) / 100.0;
                const double mMU = meanIqr(missUnordered, BENCH_TRIALS) / 100.0;
                const double mMS = meanIqr(missSorted, BENCH_TRIALS) / 100.0;

                logPerOp("contains 命中（UNORDERED 线性）", hitUnordered, BENCH_TRIALS, 100.0);
                logPerOp("contains 命中（ASCENDING 二分）", hitSorted, BENCH_TRIALS, 100.0);
                logPerOp("contains 未命中（UNORDERED 线性）", missUnordered, BENCH_TRIALS, 100.0);
                logPerOp("contains 未命中（ASCENDING 二分）", missSorted, BENCH_TRIALS, 100.0);

                LOG("加速比（命中）= ", mHS > 0.0 ? mHU / mHS : 0.0, "x，加速比（未命中）= ", mMS > 0.0 ? mMU / mMS : 0.0, "x");
            }

            LOG("--- [性能] containsAll 延迟（UNORDERED 线性 vs ASCENDING 二分，1000 元素 × 3 targets） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kElements = 1000;
                constexpr u32 kIters = 10000;

                auto unorderedList = IMPList<int>(1024, 1024);
                auto sortedList = IMPList<int>(1024, 1024);
                unorderedList.configureMaintenanceMerging(false);
                unorderedList.configureCompaction(false, 20, 10000);
                sortedList.configureMaintenanceMerging(false);
                sortedList.configureCompaction(false, 20, 10000);

                for (u32 i = 0; i < kElements; ++i) {
                    unorderedList.add((int) i);
                    sortedList.add((int) i);
                }
                unorderedList.tryMerging();
                sortedList.naturalSort();

                const int hitTargets[] = { 100, 500, 900 };
                const int missTargets[] = { 100, 500, 10000 };

                static u64 hitUnordered[BENCH_TRIALS];
                static u64 hitSorted[BENCH_TRIALS];
                static u64 missUnordered[BENCH_TRIALS];
                static u64 missSorted[BENCH_TRIALS];
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    timer.start();
                    u32 hitsU = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (unorderedList.containsAll(hitTargets, 3)) {
                            hitsU++;
                        }
                    }
                    hitUnordered[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    u32 hitsS = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (sortedList.containsAll(hitTargets, 3)) {
                            hitsS++;
                        }
                    }
                    hitSorted[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    u32 missesU = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (!unorderedList.containsAll(missTargets, 3)) {
                            missesU++;
                        }
                    }
                    missUnordered[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    u32 missesS = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        if (!sortedList.containsAll(missTargets, 3)) {
                            missesS++;
                        }
                    }
                    missSorted[t] = timer.stopNs() * 100 / kIters;

                    if (hitsU + hitsS + missesU + missesS == 0xDEADBEEF) {
                        LOG("impossible");
                    }
                }

                sortU64(hitUnordered, BENCH_TRIALS);
                sortU64(hitSorted, BENCH_TRIALS);
                sortU64(missUnordered, BENCH_TRIALS);
                sortU64(missSorted, BENCH_TRIALS);

                const double mHU = meanIqr(hitUnordered, BENCH_TRIALS) / 100.0;
                const double mHS = meanIqr(hitSorted, BENCH_TRIALS) / 100.0;
                const double mMU = meanIqr(missUnordered, BENCH_TRIALS) / 100.0;
                const double mMS = meanIqr(missSorted, BENCH_TRIALS) / 100.0;

                logPerOp("containsAll 命中（UNORDERED 线性）", hitUnordered, BENCH_TRIALS, 100.0);
                logPerOp("containsAll 命中（ASCENDING 二分）", hitSorted, BENCH_TRIALS, 100.0);
                logPerOp("containsAll 未命中（UNORDERED 线性）", missUnordered, BENCH_TRIALS, 100.0);
                logPerOp("containsAll 未命中（ASCENDING 二分）", missSorted, BENCH_TRIALS, 100.0);

                LOG("加速比（命中）= ", mHS > 0.0 ? mHU / mHS : 0.0, "x，加速比（未命中）= ", mMS > 0.0 ? mMU / mMS : 0.0, "x");
            }

            LOG("--- [性能] firstIndexOf / lastIndexOf 延迟（重复元素，UNORDERED 线性 vs ASCENDING 二分） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kDistinctValues = 200;
                constexpr u32 kRepeatPerValue = 5;
                constexpr u32 kElements = kDistinctValues * kRepeatPerValue;
                constexpr u32 kIters = 20000;

                auto unorderedList = IMPList<int>(1024, 1024);
                auto sortedList = IMPList<int>(1024, 1024);
                unorderedList.configureMaintenanceMerging(false);
                unorderedList.configureCompaction(false, 20, 10000);
                sortedList.configureMaintenanceMerging(false);
                sortedList.configureCompaction(false, 20, 10000);

                for (u32 i = 0; i < kElements; ++i) {
                    const int v = (int) (i / kRepeatPerValue);
                    unorderedList.add(v);
                    sortedList.add(v);
                }
                unorderedList.tryMerging();
                sortedList.naturalSort();

                static u64 firstUnordered[BENCH_TRIALS];
                static u64 firstSorted[BENCH_TRIALS];
                static u64 lastUnordered[BENCH_TRIALS];
                static u64 lastSorted[BENCH_TRIALS];
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    timer.start();
                    ssize firstSumU = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        firstSumU += unorderedList.firstIndexOf((int) (i % kDistinctValues));
                    }
                    firstUnordered[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    ssize firstSumS = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        firstSumS += sortedList.firstIndexOf((int) (i % kDistinctValues));
                    }
                    firstSorted[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    ssize lastSumU = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        lastSumU += unorderedList.lastIndexOf((int) (i % kDistinctValues));
                    }
                    lastUnordered[t] = timer.stopNs() * 100 / kIters;

                    timer.start();
                    ssize lastSumS = 0;
                    for (u32 i = 0; i < kIters; ++i) {
                        lastSumS += sortedList.lastIndexOf((int) (i % kDistinctValues));
                    }
                    lastSorted[t] = timer.stopNs() * 100 / kIters;

                    if (firstSumU + firstSumS + lastSumU + lastSumS == 0xDEADBEEF) {
                        LOG("impossible");
                    }
                }

                sortU64(firstUnordered, BENCH_TRIALS);
                sortU64(firstSorted, BENCH_TRIALS);
                sortU64(lastUnordered, BENCH_TRIALS);
                sortU64(lastSorted, BENCH_TRIALS);

                const double mFU = meanIqr(firstUnordered, BENCH_TRIALS) / 100.0;
                const double mFS = meanIqr(firstSorted, BENCH_TRIALS) / 100.0;
                const double mLU = meanIqr(lastUnordered, BENCH_TRIALS) / 100.0;
                const double mLS = meanIqr(lastSorted, BENCH_TRIALS) / 100.0;

                logPerOp("firstIndexOf（UNORDERED 线性）", firstUnordered, BENCH_TRIALS, 100.0);
                logPerOp("firstIndexOf（ASCENDING 二分）", firstSorted, BENCH_TRIALS, 100.0);
                logPerOp("lastIndexOf（UNORDERED 线性）", lastUnordered, BENCH_TRIALS, 100.0);
                logPerOp("lastIndexOf（ASCENDING 二分）", lastSorted, BENCH_TRIALS, 100.0);

                LOG("加速比（firstIndexOf）= ", mFS > 0.0 ? mFU / mFS : 0.0, "x，加速比（lastIndexOf）= ", mLS > 0.0 ? mLU / mLS : 0.0, "x");
            }

            LOG("--- [性能] 4 线程并发 add 吞吐（不同规模，检验可扩展性） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 sizes[] = { 10000, 100000, 1000000 };

                for (u32 total : sizes) {
                    static u64 samples[BENCH_TRIALS];
                    constexpr int kThreadCount = 4;
                    const u32 perThread = total / kThreadCount;
                    QpcTimer timer;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        auto list = IMPList<int>(1024, 1024);
                        list.configureMaintenanceMerging(false);
                        list.configureCompaction(false, 20, 10000);

                        std::vector<std::thread> threads;
                        timer.start();
                        for (int th = 0; th < kThreadCount; ++th) {
                            threads.emplace_back([&list, th, perThread]() {
                                for (u32 i = 0; i < perThread; ++i) {
                                    list.add((int) (th * perThread + i));
                                }
                            });
                        }
                        for (auto& thread : threads) {
                            thread.join();
                        }
                        list.tryMerging();
                        samples[t] = timer.stopNs() * 100 / total;
                    }

                    sortU64(samples, BENCH_TRIALS);
                    logPerOp("4 线程并发 add+merge", samples, BENCH_TRIALS, 100.0);
                }
            }

            LOG("--- [性能] 高并发读写吞吐（分线程数与读写比例，维护线程自动合并，真实并发场景） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr int threadConfigs[] = {
                    1, 2, 4, 8
                };
                struct ReadWriteRatio {
                    int readers;
                    int writers;
                };
                const ReadWriteRatio ratios[] = {
                    { 1, 1 },
                    { 1, 4 },
                    { 4, 1 },
                    { 1, 0 },
                    { 0, 1 }
                };

                for (int baseThreads : threadConfigs) {
                    for (const auto& ratio : ratios) {
                        int rThreads = ratio.readers * baseThreads;
                        int wThreads = ratio.writers * baseThreads;
                        int totalThreads = rThreads + wThreads;

                        if (totalThreads == 0) {
                            continue;
                        }

                        auto list = IMPList<int>(1024, 1024);
                        list.configureRetirement(4096);
                        list.configureCompaction(false, 20, 10000);
                        for (int i = 0; i < 10000; ++i) {
                            list.add(i);
                        }
                        list.tryMerging();
                        list.awaitMerging();

                        std::vector<std::thread> threads;
                        threads.reserve(totalThreads);
                        std::vector<ConcurrentTaskResult> results(totalThreads);

                        for (int r = 0; r < rThreads; ++r) {
                            threads.emplace_back([&list, &results, r]() {
                                results[r] = runConcurrentTask(list, true, r, kMeasureNs, kWarmupNs);
                            });
                        }

                        for (int w = 0; w < wThreads; ++w) {
                            threads.emplace_back([&list, &results, rThreads, w]() {
                                results[rThreads + w] = runConcurrentTask(list, false, w, kMeasureNs, kWarmupNs);
                            });
                        }

                        for (auto& t : threads) {
                            t.join();
                        }

                        u64 totalR = 0;
                        u64 totalRElapsedNs = 0;
                        u64 totalW = 0;
                        u64 totalWElapsedNs = 0;

                        for (int r = 0; r < rThreads; ++r) {
                            totalR += results[r].ops;
                            if (results[r].elapsedNs > totalRElapsedNs) {
                                totalRElapsedNs = results[r].elapsedNs;
                            }
                        }
                        for (int w = 0; w < wThreads; ++w) {
                            totalW += results[rThreads + w].ops;
                            if (results[rThreads + w].elapsedNs > totalWElapsedNs) {
                                totalWElapsedNs = results[rThreads + w].elapsedNs;
                            }
                        }

                        logConcurrentMetrics(totalThreads, rThreads, wThreads, totalR, totalRElapsedNs, totalW, totalWElapsedNs);
                    }
                }
            }

            LOG("--- [性能] merge 单独成本（不同 chunkSize，带 Span 复用，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                const usize chunkSizes[] = { 16, 64, 256, 1024, 4096 };

                for (usize cs : chunkSizes) {
                    static u64 samples[BENCH_TRIALS];
                    constexpr u32 kElements = 200000;
                    QpcTimer timer;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        auto list = IMPList<int>(cs, 1024);
                        list.configureMaintenanceMerging(false);
                        list.configureCompaction(false, 20, 10000);

                        for (u32 i = 0; i < kElements; ++i) {
                            list.add((int) i);
                        }

                        list.tryMerging();
                        list.add(0);

                        timer.start();
                        list.tryMerging();
                        samples[t] = timer.stopNs();
                    }

                    sortU64(samples, BENCH_TRIALS);

                    double mean = 0.0;
                    double sd = 0.0;
                    u32 kept = 0;
                    computeMeanSdIqr(samples, BENCH_TRIALS, &mean, &sd, &kept);

                    LOG("chunkSize = ", cs, "，merge 单独耗时 = 平均 ", mean / 1e6, " ms，标准差 ", sd / 1e6, " ms，最小 ", samples[0] / 1e6, " ms，P50 ", computePercentile(samples, BENCH_TRIALS, 0.50) / 1e6, " ms，P90 ", computePercentile(samples, BENCH_TRIALS, 0.90) / 1e6, " ms，P99 ", computePercentile(samples, BENCH_TRIALS, 0.99) / 1e6, " ms，最大 ", samples[BENCH_TRIALS - 1] / 1e6, " ms（", kept, " 个样本通过 IQR）");
                }
            }

            LOG("--- [性能] discard 转化 tombstones 延迟（elementSize = 4，FNV Hash 路径，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kElements = 100000;
                constexpr u32 kDiscards = 10000;
                static u64 samples[BENCH_TRIALS];
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    auto list = IMPList<int>(1024, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    for (u32 i = 0; i < kElements; ++i) {
                        list.add((int) i);
                    }

                    list.tryMerging();

                    for (u32 i = 0; i < kDiscards; ++i) {
                        list.discard((int) i);
                    }

                    timer.start();
                    list.tryMerging();
                    samples[t] = timer.stopNs();
                }

                sortU64(samples, BENCH_TRIALS);

                double mean = 0.0;
                double sd = 0.0;
                u32 kept = 0;
                computeMeanSdIqr(samples, BENCH_TRIALS, &mean, &sd, &kept);

                LOG("10万添加 + 1万删除 → 增量 merge 耗时 = 平均 ", mean / 1000.0, " us，标准差 ", sd / 1000.0, " us，最小 ", samples[0] / 1000.0, " us，P50 ", computePercentile(samples, BENCH_TRIALS, 0.50) / 1000.0, " us，P90 ", computePercentile(samples, BENCH_TRIALS, 0.90) / 1000.0, " us，P99 ", computePercentile(samples, BENCH_TRIALS, 0.99) / 1000.0, " us，最大 ", samples[BENCH_TRIALS - 1] / 1000.0, " us（", kept, " 个样本通过 IQR）");
            }

            LOG("--- [性能] discard 转化 tombstones 延迟（elementSize = 12，FNV Hash 路径，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                struct CustomStruct {
                    int a;
                    int b;
                    int c;
                };

                constexpr u32 kElements = 50000;
                constexpr u32 kDiscards = 5000;
                static u64 samples[BENCH_TRIALS];
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    auto list = IMPList<CustomStruct>(1024, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    for (u32 i = 0; i < kElements; ++i) {
                        list.add({ (int) i, (int) i, (int) i });
                    }

                    list.tryMerging();

                    for (u32 i = 0; i < kDiscards; ++i) {
                        list.discard({ (int) i, (int) i, (int) i });
                    }

                    timer.start();
                    list.tryMerging();
                    samples[t] = timer.stopNs();
                }

                sortU64(samples, BENCH_TRIALS);

                double mean = 0.0;
                double sd = 0.0;
                u32 kept = 0;
                computeMeanSdIqr(samples, BENCH_TRIALS, &mean, &sd, &kept);

                LOG("5万添加 + 5千删除 → 增量 merge 耗时 = 平均 ", mean / 1000.0, " us，标准差 ", sd / 1000.0, " us，最小 ", samples[0] / 1000.0, " us，P50 ", computePercentile(samples, BENCH_TRIALS, 0.50) / 1000.0, " us，P90 ", computePercentile(samples, BENCH_TRIALS, 0.90) / 1000.0, " us，P99 ", computePercentile(samples, BENCH_TRIALS, 0.99) / 1000.0, " us，最大 ", samples[BENCH_TRIALS - 1] / 1000.0, " us（", kept, " 个样本通过 IQR）");
            }

            LOG("--- [性能] compact 单次成本（不同墓碑比例，elementSize = 4，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kElements = 10000;
                constexpr u32 kTrials = 11;
                const u32 discardRatios[] = { 20, 50, 80 };

                for (u32 ratio : discardRatios) {
                    static u64 samples[kTrials];
                    QpcTimer timer;

                    const u32 discards = kElements * ratio / 100;

                    for (u32 t = 0; t < kTrials; ++t) {
                        auto list = IMPList<int>(1024, 1024);
                        list.configureMaintenanceMerging(false);
                        list.configureCompaction(false, 0, 1000000);

                        for (u32 i = 0; i < kElements; ++i) {
                            list.add((int) i);
                        }
                        list.tryMerging();
                        list.awaitMerging();

                        for (u32 i = 0; i < discards; ++i) {
                            list.remove((int) i);
                        }
                        list.tryMerging();
                        list.awaitMerging();

                        timer.start();
                        list.tryCompacting();
                        samples[t] = timer.stopNs();
                    }

                    sortU64(samples, kTrials);

                    double mean = 0.0;
                    double sd = 0.0;
                    u32 kept = 0;
                    computeMeanSdIqr(samples, kTrials, &mean, &sd, &kept);

                    LOG("墓碑占比 ", ratio, "%：compact 耗时 = 平均 ", mean / 1000.0, " us，标准差 ", sd / 1000.0, " us，最小 ", samples[0] / 1000.0, " us，P50 ", computePercentile(samples, kTrials, 0.50) / 1000.0, " us，P90 ", computePercentile(samples, kTrials, 0.90) / 1000.0, " us，P99 ", computePercentile(samples, kTrials, 0.99) / 1000.0, " us，最大 ", samples[kTrials - 1] / 1000.0, " us（", kept, " 个样本通过 IQR）");
                }
            }

            LOG("--- [性能] compact 单独成本（不同 chunkSize，墓碑占比 50%，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                const usize chunkSizes[] = { 16, 64, 256, 1024, 4096 };
                constexpr u32 kElements = 10000;
                constexpr u32 kDiscards = 5000;
                constexpr u32 kTrials = 11;

                for (usize cs : chunkSizes) {
                    static u64 samples[kTrials];
                    QpcTimer timer;

                    for (u32 t = 0; t < kTrials; ++t) {
                        auto list = IMPList<int>(cs, 1024);
                        list.configureMaintenanceMerging(false);
                        list.configureCompaction(false, 0, 1000000);

                        for (u32 i = 0; i < kElements; ++i) {
                            list.add((int) i);
                        }
                        list.tryMerging();
                        list.awaitMerging();

                        for (u32 i = 0; i < kDiscards; ++i) {
                            list.remove((int) i);
                        }
                        list.tryMerging();
                        list.awaitMerging();

                        timer.start();
                        list.tryCompacting();
                        samples[t] = timer.stopNs();
                    }

                    sortU64(samples, kTrials);

                    double mean = 0.0;
                    double sd = 0.0;
                    u32 kept = 0;
                    computeMeanSdIqr(samples, kTrials, &mean, &sd, &kept);

                    LOG("chunkSize = ", cs, "，compact 耗时 = 平均 ", mean / 1e6, " ms，标准差 ", sd / 1e6, " ms，最小 ", samples[0] / 1e6, " ms，P50 ", computePercentile(samples, kTrials, 0.50) / 1e6, " ms，P90 ", computePercentile(samples, kTrials, 0.90) / 1e6, " ms，P99 ", computePercentile(samples, kTrials, 0.99) / 1e6, " ms，最大 ", samples[kTrials - 1] / 1e6, " ms（", kept, " 个样本通过 IQR）");
                }
            }

            LOG("--- [性能] compact 前后 merge 成本对比（验证墓碑清理收益） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kElements = 10000;
                constexpr u32 kDiscards = 5000;
                constexpr u32 kTrials = 11;
                static u64 beforeSamples[kTrials];
                static u64 afterSamples[kTrials];
                QpcTimer timer;

                for (u32 t = 0; t < kTrials; ++t) {
                    auto list = IMPList<int>(1024, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 0, 1000000);

                    for (u32 i = 0; i < kElements; ++i) {
                        list.add((int) i);
                    }
                    list.tryMerging();
                    list.awaitMerging();

                    for (u32 i = 0; i < kDiscards; ++i) {
                        list.remove((int) i);
                    }
                    list.tryMerging();
                    list.awaitMerging();

                    list.add(999999);

                    timer.start();
                    list.tryMerging();
                    beforeSamples[t] = timer.stopNs();

                    list.tryCompacting();
                    list.tryMerging();
                    list.awaitMerging();

                    list.add(999998);

                    timer.start();
                    list.tryMerging();
                    afterSamples[t] = timer.stopNs();
                }

                sortU64(beforeSamples, kTrials);
                sortU64(afterSamples, kTrials);

                double beforeMean = 0.0;
                double beforeSd = 0.0;
                u32 beforeKept = 0;
                computeMeanSdIqr(beforeSamples, kTrials, &beforeMean, &beforeSd, &beforeKept);

                double afterMean = 0.0;
                double afterSd = 0.0;
                u32 afterKept = 0;
                computeMeanSdIqr(afterSamples, kTrials, &afterMean, &afterSd, &afterKept);

                LOG("compact 前 merge（含 50% 墓碑）：平均 ", beforeMean / 1000.0, " us，标准差 ", beforeSd / 1000.0, " us，P50 ", computePercentile(beforeSamples, kTrials, 0.50) / 1000.0, " us（", beforeKept, " 个样本通过 IQR）");
                LOG("compact 后 merge（墓碑清理）：平均 ", afterMean / 1000.0, " us，标准差 ", afterSd / 1000.0, " us，P50 ", computePercentile(afterSamples, kTrials, 0.50) / 1000.0, " us（", afterKept, " 个样本通过 IQR）");
                LOG("merge 加速比 = ", afterMean > 0.0 ? beforeMean / afterMean : 0.0, "x");
            }

            LOG("--- [性能] move 常数时间（不同规模，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 sizes[] = { 100, 10000, 1000000 };
                constexpr u32 kMoveIters = 100000;

                for (u32 n : sizes) {
                    auto list = IMPList<int>(1024, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    for (u32 i = 0; i < n; ++i) {
                        list.add((int) i);
                    }
                    list.tryMerging();

                    static u64 samples[BENCH_TRIALS];
                    QpcTimer timer;

                    bool intact = true;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        timer.start();
                        for (u32 i = 0; i < kMoveIters; ++i) {
                            auto moved = std::move(list);
                            std::atomic_signal_fence(std::memory_order_seq_cst);
                            list = std::move(moved);
                            std::atomic_signal_fence(std::memory_order_seq_cst);
                        }
                        samples[t] = timer.stopNs() * 10000 / kMoveIters;

                        if (!list.isValid() || list.presentElementCount() != n) {
                            intact = false;
                        }
                    }

                    if (!intact) {
                        LOG("impossible");
                    }

                    sortU64(samples, BENCH_TRIALS);

                    double mean = 0.0;
                    double sd = 0.0;
                    u32 kept = 0;
                    computeMeanSdIqr(samples, BENCH_TRIALS, &mean, &sd, &kept);

                    LOG("move ", n, " 元素：平均 ", mean / 10000.0, " ns/op，标准差 ", sd / 10000.0, " ns，最小 ", samples[0] / 10000.0, " ns，P50 ", computePercentile(samples, BENCH_TRIALS, 0.50) / 10000.0, " ns，P90 ", computePercentile(samples, BENCH_TRIALS, 0.90) / 10000.0, " ns，P99 ", computePercentile(samples, BENCH_TRIALS, 0.99) / 10000.0, " ns，最大 ", samples[BENCH_TRIALS - 1] / 10000.0, " ns（", kept, " 个样本通过 IQR，每轮 ", kMoveIters, " 次）");
                }
            }

            LOG("--- [性能] clear 成本（不同规模，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 sizes[] = { 100, 10000, 1000000 };

                for (u32 n : sizes) {
                    static u64 samples[BENCH_TRIALS];
                    QpcTimer timer;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        auto list = IMPList<int>(1024, 1024);
                        list.configureMaintenanceMerging(false);
                        list.configureCompaction(false, 20, 10000);

                        for (u32 i = 0; i < n; ++i) {
                            list.add((int) i);
                        }
                        list.tryMerging();

                        timer.start();
                        list.removeAll();
                        samples[t] = timer.stopNs();
                    }

                    sortU64(samples, BENCH_TRIALS);

                    double mean = 0.0;
                    double sd = 0.0;
                    u32 kept = 0;
                    computeMeanSdIqr(samples, BENCH_TRIALS, &mean, &sd, &kept);

                    LOG("clear ", n, " 元素：平均 ", mean / 1e6, " ms，标准差 ", sd / 1e6, " ms，最小 ", samples[0] / 1e6, " ms，P50 ", computePercentile(samples, BENCH_TRIALS, 0.50) / 1e6, " ms，P90 ", computePercentile(samples, BENCH_TRIALS, 0.90) / 1e6, " ms，P99 ", computePercentile(samples, BENCH_TRIALS, 0.99) / 1e6, " ms，最大 ", samples[BENCH_TRIALS - 1] / 1e6, " ms（", kept, " 个样本通过 IQR）");
                }
            }

            LOG("--- [稳定性] 长时间反复 add/merge/clear 循环 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                constexpr u32 kRounds = 1000;
                constexpr u32 kPerRound = 500;
                QpcTimer timer;

                timer.start();
                for (u32 r = 0; r < kRounds; ++r) {
                    for (u32 i = 0; i < kPerRound; ++i) {
                        list.add((int) i);
                    }
                    if (r % 100 == 0) {
                        list.tryMerging();
                    }
                }
                const u64 totalNs = timer.stopNs();

                LOG(kRounds, " 轮 × ", kPerRound, " 元素：总耗时 ", totalNs / 1e6, " ms，最终 present = ", list.presentElementCount(), "，ingest = ", list.ingestElementCount());
            }

            LOG("--- [统计] 合并延迟分布（每次独立重建，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                static u64 samples[BENCH_TRIALS];
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    auto list = IMPList<int>(1024, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 20, 10000);

                    for (int i = 0; i < 100000; ++i) {
                        list.add(i);
                    }
                    list.tryMerging();

                    for (int i = 0; i < 1000; ++i) {
                        list.add(100000 + i);
                    }

                    timer.start();
                    list.tryMerging();
                    samples[t] = timer.stopNs();
                }

                sortU64(samples, BENCH_TRIALS);

                double mean = 0.0;
                double sd = 0.0;
                u32 kept = 0;
                computeMeanSdIqr(samples, BENCH_TRIALS, &mean, &sd, &kept);

                LOG("增量 merge 延迟：平均 ", mean / 1000.0, " us，标准差 ", sd / 1000.0, " us（", kept, " 个样本通过 IQR）");
                LOG("  最小 ", samples[0] / 1000.0, " us，P50 ", computePercentile(samples, BENCH_TRIALS, 0.50) / 1000.0, " us，P90 ", computePercentile(samples, BENCH_TRIALS, 0.90) / 1000.0, " us，P99 ", computePercentile(samples, BENCH_TRIALS, 0.99) / 1000.0, " us，最大 ", samples[BENCH_TRIALS - 1] / 1000.0, " us");
                LOG("  每次 trial 独立重建：100000 基准元素 + 1000 增量元素，工作量恒定");
            }

            LOG("--- [统计] 增量 compact 延迟分布（每次独立重建，IQR 剔除） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kElements = 10000;
                constexpr u32 kDiscards = 5000;
                constexpr u32 kTrials = 11;
                static u64 samples[kTrials];
                QpcTimer timer;

                for (u32 t = 0; t < kTrials; ++t) {
                    auto list = IMPList<int>(1024, 1024);
                    list.configureMaintenanceMerging(false);
                    list.configureCompaction(false, 0, 1000000);

                    for (u32 i = 0; i < kElements; ++i) {
                        list.add((int) i);
                    }
                    list.tryMerging();
                    list.awaitMerging();

                    for (u32 i = 0; i < kDiscards; ++i) {
                        list.remove((int) i);
                    }
                    list.tryMerging();
                    list.awaitMerging();

                    timer.start();
                    list.tryCompacting();
                    samples[t] = timer.stopNs();
                }

                sortU64(samples, kTrials);

                double mean = 0.0;
                double sd = 0.0;
                u32 kept = 0;
                computeMeanSdIqr(samples, kTrials, &mean, &sd, &kept);

                LOG("增量 compact 延迟（1 万基准 + 5 千墓碑）：平均 ", mean / 1000.0, " us，标准差 ", sd / 1000.0, " us（", kept, " 个样本通过 IQR）");
                LOG("  最小 ", samples[0] / 1000.0, " us，P50 ", computePercentile(samples, kTrials, 0.50) / 1000.0, " us，P90 ", computePercentile(samples, kTrials, 0.90) / 1000.0, " us，P99 ", computePercentile(samples, kTrials, 0.99) / 1000.0, " us，最大 ", samples[kTrials - 1] / 1000.0, " us");
            }
        }
        LOG("===== IMPList 深度测试结束 =====");
    }
}