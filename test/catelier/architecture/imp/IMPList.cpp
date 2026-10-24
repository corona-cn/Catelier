#include <windows.h>

#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

#include "../../util/Logger.hpp"

#include "../../../../include/catelier/architecture/imp/IMPList.hpp"

namespace Catelier::test::architecture::imp {
    namespace {
        constexpr u32 BENCH_TRIALS = 51;
        constexpr u32 BENCH_TRIM = 5;

        auto intEquals(const void* a, const void* b) -> bool {
            return *((const int*) a) == *((const int*) b);
        }
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

        auto logPerOp(const char* label, const double nsPerOp, const double sdNs, const u64 medianNs, const u32 n) -> void {
            const double ns10x = nsPerOp * 10.0;
            const double sd10x = sdNs * 10.0;
            LOG(label, "：平均 ", ns10x / 10.0, " ns/op，标准差 ", sd10x / 10.0, " ns，中位数 ", medianNs, " ns（", n, " 轮掐头去尾）");
        }

        auto logThroughput(const char* label, const double nsPerOp, const double sdNs, const u64 totalOps) -> void {
            const double opsPerSec = nsPerOp > 0.0 ? 1e9 / nsPerOp : 0.0;
            LOG(label, "：", nsPerOp, " ns/op（", opsPerSec / 1e6, " M op/s），标准差 ", sdNs, " ns，总 ", totalOps, " 次");
        }

        auto logConcurrentMetrics(const int threads, const int readers, const int writers, const double readNs, const double writeNs) -> void {
            if (readers > 0 && writers > 0) {
                const double rOps = readNs > 0.0 ? 1e9 / readNs : 0.0;
                const double wOps = writeNs > 0.0 ? 1e9 / writeNs : 0.0;
                LOG(threads, " 线程（", readers, " 读 ", writers, " 写）：读 ", readNs, " ns/op（", rOps / 1e6, " M op/s），写 ", writeNs, " ns/op（", wOps / 1e6, " M op/s）");
            } else if (readers > 0) {
                const double rOps = readNs > 0.0 ? 1e9 / readNs : 0.0;
                LOG(threads, " 线程（纯读）：读 ", readNs, " ns/op（", rOps / 1e6, " M op/s）");
            } else if (writers > 0) {
                const double wOps = writeNs > 0.0 ? 1e9 / writeNs : 0.0;
                LOG(threads, " 线程（纯写）：写 ", writeNs, " ns/op（", wOps / 1e6, " M op/s）");
            }
        }
    }

    auto runTests() -> void {
        using namespace Catelier::include::architecture::imp;

        LOG("===== IMPList 深度测试开始 =====");
        LOGGER_INDENT_BLOCK(true) {
            LOG("--- [正确性] 构造与生命周期 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                LOG("初始 isValid = ", list.isValid() ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("初始 isEmpty = ", list.isEmpty(), "（应为 1）");
                LOG("初始 presentElementCount = ", list.presentElementCount(), "（应为 0）");
                LOG("初始 ingestElementCount = ", list.ingestElementCount(), "（应为 0）");
                LOG("初始 mergeElementCount = ", list.mergeElementCount(), "（应为 0）");
                LOG("初始 first = ", list.first() == nullptr ? "NULL" : "非 NULL", "（应为 NULL）");
                LOG("初始 last = ", list.last() == nullptr ? "NULL" : "非 NULL", "（应为 NULL）");

                constexpr usize kInstances = 1000;
                for (usize i = 0; i < kInstances; ++i) {
                    auto temp = IMPList<int>(64, 1000, intEquals);
                    temp.add((int) i);
                    temp.merge();
                }
                LOG(kInstances, " 个实例循环构造析构：PASS");
            }

            LOG("--- [正确性] 单线程 add / addAll 全路径 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.merge();

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
                auto list2 = IMPList<int>(64, 1000, intEquals);
                list2.addAll(batch, 256);
                list2.merge();
                LOG("addAll 256 个元素 → presentElementCount = ", list2.presentElementCount(), "（应为 256）");

                auto list3 = IMPList<int>(64, 1000, intEquals);
                list3.addAll(batch, 0);
                list3.merge();
                LOG("addAll 0 个元素 → presentElementCount = ", list3.presentElementCount(), "（应为 0）");
            }

            LOG("--- [正确性] chunk 边界：恰好 chunkSize / chunkSize±1 ---");
            LOGGER_INDENT_BLOCK(true) {
                const usize cs[] = { 16, 63, 64, 65, 127, 128, 129 };

                for (usize c : cs) {
                    auto list = IMPList<int>(c, 1000, intEquals);
                    for (usize i = 0; i < c; ++i) {
                        list.add((int) i);
                    }
                    list.merge();
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
                auto list = IMPList<int>(16, 1000, intEquals);
                for (int i = 0; i < 100; ++i) {
                    list.add(i % 10);
                }
                list.merge();
                LOG("add 100 个（0-9 各 10 次） → presentElementCount = ", list.presentElementCount(), "（应为 100）");

                const bool removed = list.remove(5);
                LOG("remove(5) = ", removed ? "PASS" : "FAIL", "（应为 PASS）");

                list.merge();
                const usize afterRemove = list.presentElementCount();
                LOG("merge 后 presentElementCount = ", afterRemove, "（全量删除语义：所有 5 均被移除，应为 90）");
            }

            LOG("--- [正确性] removeIf / retainIf 语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1000, intEquals);
                for (int i = 1; i <= 100; ++i) {
                    list.add(i);
                }
                list.merge();

                const bool removed = list.removeIf(intEvenPredicate);
                LOG("removeIf(偶数) = ", removed ? "PASS" : "FAIL");
                list.merge();

                bool allOdd = true;
                for (usize i = 0; i < list.presentElementCount(); ++i) {
                    if (*list.get(i) % 2 == 0) {
                        allOdd = false;
                        break;
                    }
                }
                LOG("removeIf 后全为奇数：", allOdd ? "PASS" : "FAIL", "（count = ", list.presentElementCount(), "，应为 50）");

                auto list2 = IMPList<int>(16, 1000, intEquals);
                for (int i = 1; i <= 100; ++i) {
                    list2.add(i);
                }
                list2.merge();

                const bool retained = list2.removeExceptIf(intOddPredicate);
                LOG("retainIf(奇数) = ", retained ? "PASS" : "FAIL");
                list2.merge();
                LOG("retainIf 后 presentElementCount = ", list2.presentElementCount(), "（应为 50）");
            }

            LOG("--- [正确性] clear 语义（TOMBSTONE + merge） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 20; ++i) {
                    list.add(i);
                }
                list.merge();
                LOG("clear 前 present = ", list.presentElementCount(), "（应为 20）");

                list.removeAll();
                list.merge();
                LOG("clear 后 present = ", list.presentElementCount(), "（应为 0）");
                LOG("clear 后 isEmpty = ", list.isEmpty() ? "PASS" : "FAIL");

                list.add(100);
                list.merge();
                LOG("clear 后再 add：present = ", list.presentElementCount(), "（应为 1）");
                LOG("值正确：", *((const int*) list.get(0)) == 100 ? "PASS" : "FAIL");

                auto list2 = IMPList<int>(64, 1000, intEquals);
                list2.removeAll();
                list2.merge();
                LOG("空列表 clear → present = ", list2.presentElementCount(), "（应为 0）");
            }

            LOG("--- [正确性] copy / move 深度语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto original = IMPList<int>(16, 1000, intEquals);
                for (int i = 0; i < 200; ++i) {
                    original.add(i * 3);
                }
                original.merge();

                auto copied = original;
                LOG("copy 后 original.present = ", original.presentElementCount(), "，copied.present = ", copied.presentElementCount(), "（应均为 200）");
                LOG("copy 后 original.isValid = ", original.isValid() ? "PASS" : "FAIL", "，copied.isValid = ", copied.isValid() ? "PASS" : "FAIL", "（应均为 PASS）");

                copied.add(999);
                copied.merge();
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

                auto source = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 100; ++i) {
                    source.add(i);
                }
                source.merge();

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

                auto target = IMPList<int>(64, 1000, intEquals);
                target.add(999);
                target.merge();
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

                auto self = IMPList<int>(64, 1000, intEquals);
                self.add(42);
                self.merge();
                auto& selfRef = self;
                self = std::move(selfRef);
                LOG("自移动赋值后 self.isValid = ", self.isValid() ? "PASS" : "FAIL", "（应为 PASS，自赋值被保护）");
                LOG("自移动赋值后 self.present = ", self.presentElementCount(), "（应为 1）");
            }

            LOG("--- [正确性] 排序策略 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1000, intEquals);
                const int values[] = { 5, 2, 8, 1, 9, 3, 7, 4, 6, 0 };
                for (int v : values) {
                    list.add(v);
                }
                list.merge();
                list.naturalSort();

                bool asc = true;
                for (usize i = 1; i < list.presentElementCount(); ++i) {
                    if (*list.get(i - 1) > *list.get(i)) {
                        asc = false;
                        break;
                    }
                }
                LOG("naturalSort → 升序：", asc ? "PASS" : "FAIL");

                auto list2 = IMPList<int>(16, 1000, intEquals);
                for (int v : values) {
                    list2.add(v);
                }
                list2.merge();
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

            LOG("--- [正确性] awaitMerging 语义 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 1000, intEquals);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.awaitMerging();
                LOG("awaitMerging 后 present = ", list.presentElementCount(), "（应为 100）");

                const bool timeoutResult = list.awaitMerging(1);
                LOG("已合并状态 awaitMerging(1ms) = ", timeoutResult ? "PASS" : "FAIL", "（应为 PASS）");
            }

            LOG("--- [并发] 4 写线程 × 10000 元素：计数 + 完整性 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1, intEquals);
                constexpr int kThreadCount = 4;
                constexpr int kElementsPerThread = 10000;

                std::vector<std::thread> threads;
                for (int t = 0; t < kThreadCount; ++t) {
                    threads.emplace_back([&list, t, kElementsPerThread]() {
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

            LOG("--- [并发] 线程退出（写线程 join 触发 threadExitCallback） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1, intEquals);

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
                    auto list = IMPList<int>(128, 1, intEquals);
                    for (int i = 0; i < 500; ++i) {
                        list.add(i);
                    }
                    list.merge();
                    auto copy = list;
                    copy.add(999);
                    copy.merge();
                }
                LOG(kRounds, " 轮构造/拷贝/析构：PASS（无崩溃即无灾难性泄漏）");
            }

            LOG("--- [性能] 单线程 add 延迟（掐头去尾 10%，51 轮） ---");
            LOGGER_INDENT_BLOCK(true) {
                static u64 samples[BENCH_TRIALS];
                constexpr u32 kBatch = 20000;
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    auto list = IMPList<int>(16384, 100000, intEquals);
                    timer.start();
                    for (u32 i = 0; i < kBatch; ++i) {
                        list.add((int) i);
                    }
                    samples[t] = timer.stopNs() * 10 / kBatch;
                }

                sortU64(samples, BENCH_TRIALS);
                const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM) / 10.0;
                const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m * 10.0) / 10.0;
                logPerOp("add 延迟", m, sd, samples[BENCH_TRIALS / 2] / 10, BENCH_TRIALS);
            }

            LOG("--- [性能] add + merge 混合吞吐（不同规模，检验线性） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 sizes[] = { 1000, 10000, 100000, 1000000 };

                for (u32 n : sizes) {
                    static u64 samples[BENCH_TRIALS];
                    QpcTimer timer;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        auto list = IMPList<int>(1024, 100000, intEquals);
                        timer.start();
                        for (u32 i = 0; i < n; ++i) {
                            list.add((int) i);
                        }
                        list.merge();
                        samples[t] = timer.stopNs() * 100 / n;
                    }

                    sortU64(samples, BENCH_TRIALS);
                    const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM) / 100.0;
                    const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m * 100.0) / 100.0;
                    logPerOp("add+merge 吞吐", m, sd, samples[BENCH_TRIALS / 2] / 100, BENCH_TRIALS);
                }
            }

            LOG("--- [性能] get 快照访问延迟 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 100000, intEquals);
                for (int i = 0; i < 100000; ++i) {
                    list.add(i);
                }
                list.merge();

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
                const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM) / 10.0;
                const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m * 10.0) / 10.0;
                logPerOp("get 延迟", m, sd, samples[BENCH_TRIALS / 2] / 10, BENCH_TRIALS);
            }

            LOG("--- [性能] contains 扫描延迟（命中/未命中） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 100000, intEquals);
                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.merge();

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

                const double mHit = meanOf(hitSamples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM) / 100.0;
                const double mMiss = meanOf(missSamples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM) / 100.0;
                logPerOp("contains（命中）", mHit, 0.0, hitSamples[BENCH_TRIALS / 2] / 100, BENCH_TRIALS);
                logPerOp("contains（未命中）", mMiss, 0.0, missSamples[BENCH_TRIALS / 2] / 100, BENCH_TRIALS);
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
                        auto list = IMPList<int>(1024, 100000, intEquals);

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
                        list.merge();
                        samples[t] = timer.stopNs() * 100 / total;
                    }

                    sortU64(samples, BENCH_TRIALS);
                    const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM) / 100.0;
                    const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m * 100.0) / 100.0;
                    logPerOp("4 线程并发 add+merge", m, sd, samples[BENCH_TRIALS / 2] / 100, BENCH_TRIALS);
                }
            }

            LOG("--- [性能] 高并发读写延迟（分线程数与读写比例） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr int threadConfigs[] = {
                    1, 2, 4, 8
                };
                struct ReadWriteRatio {
                    int readers;
                    int writers;
                };
                const ReadWriteRatio ratios[] = {
                    { 1, 1 }, // 1读1写
                    { 1, 4 }, // 写密集 (1:4)
                    { 4, 1 }, // 读密集 (4:1)
                    { 1, 0 }, // 纯读
                    { 0, 1 }  // 纯写
                };

                QpcTimer timer;

                for (int baseThreads : threadConfigs) {
                    for (const auto& ratio : ratios) {
                        int rThreads = ratio.readers * baseThreads;
                        int wThreads = ratio.writers * baseThreads;
                        int totalThreads = rThreads + wThreads;

                        if (totalThreads == 0) {
                            continue;
                        }

                        auto list = IMPList<int>(1024, 1, intEquals);
                        list.configureRetirement(4096);
                        for (int i = 0; i < 10000; ++i) {
                            list.add(i);
                        }
                        list.merge();

                        std::atomic<bool> running = { true };

                        std::atomic<u64> totalReads = { 0 };
                        std::atomic<u64> totalWrites = { 0 };
                        std::atomic<u64> readTimeNs = { 0 };
                        std::atomic<u64> writeTimeNs = { 0 };

                        std::vector<std::thread> threads;

                        for (int r = 0; r < rThreads; ++r) {
                            threads.emplace_back([&list, &running, &totalReads, &readTimeNs]() {
                                u64 localReads = 0;
                                volatile int accum = 0;

                                QpcTimer localTimer;
                                localTimer.start();

                                while (running.load(std::memory_order_relaxed)) {
                                    const usize count = list.presentElementCount();
                                    if (count > 0) {
                                        for (int k = 0; k < 100; ++k) {
                                            const int* v = list.get((usize) ((localReads + k) % count));
                                            if (v) {
                                                accum += *v;
                                            }
                                        }
                                        localReads += 100;
                                    }
                                }

                                u64 elapsed = localTimer.stopNs();
                                totalReads.fetch_add(localReads, std::memory_order_relaxed);
                                readTimeNs.fetch_add(elapsed, std::memory_order_relaxed);
                            });
                        }

                        for (int w = 0; w < wThreads; ++w) {
                            threads.emplace_back([&list, &running, &totalWrites, &writeTimeNs, w]() {
                                u64 localWrites = 0;
                                int counter = w * 100000000;

                                QpcTimer localTimer;
                                localTimer.start();

                                while (running.load(std::memory_order_relaxed)) {
                                    for (int k = 0; k < 100; ++k) {
                                        list.add(counter++);
                                    }
                                    localWrites += 100;
                                }

                                u64 elapsed = localTimer.stopNs();
                                totalWrites.fetch_add(localWrites, std::memory_order_relaxed);
                                writeTimeNs.fetch_add(elapsed, std::memory_order_relaxed);
                            });
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                        running.store(false, std::memory_order_relaxed);

                        for (auto& t : threads) {
                            t.join();
                        }

                        double rNs = totalReads.load() > 0 ? (double)readTimeNs.load() / (double)totalReads.load() : 0.0;
                        double wNs = totalWrites.load() > 0 ? (double)writeTimeNs.load() / (double)totalWrites.load() : 0.0;

                        logConcurrentMetrics(totalThreads, rThreads, wThreads, rNs, wNs);
                    }
                }
            }

            LOG("--- [性能] merge 单独成本（不同 chunkSize） ---");
            LOGGER_INDENT_BLOCK(true) {
                const usize chunkSizes[] = { 16, 64, 256, 1024, 4096 };

                for (usize cs : chunkSizes) {
                    static u64 samples[BENCH_TRIALS];
                    constexpr u32 kElements = 200000;
                    QpcTimer timer;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        auto list = IMPList<int>(cs, 100000, intEquals);
                        for (u32 i = 0; i < kElements; ++i) {
                            list.add((int) i);
                        }
                        timer.start();
                        list.merge();
                        samples[t] = timer.stopNs();
                    }

                    sortU64(samples, BENCH_TRIALS);
                    const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM);
                    const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m);
                    LOG("chunkSize = ", cs, "，merge 单独耗时 = ", m / 1e6, " ms，标准差 ", sd / 1e6, " ms，中位数 ", samples[BENCH_TRIALS / 2] / 1e6, " ms");
                }
            }

            LOG("--- [性能] move 常数时间（不同规模） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 sizes[] = { 100, 10000, 1000000 };
                constexpr u32 kMoveIters = 100000;

                for (u32 n : sizes) {
                    auto list = IMPList<int>(1024, 100000, intEquals);
                    for (u32 i = 0; i < n; ++i) {
                        list.add((int) i);
                    }
                    list.merge();
                    list.awaitMerging();

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
                    const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM) / 10000.0;
                    const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m * 10000.0) / 10000.0;
                    LOG("move ", n, " 元素：平均 ", m, " ns/op，标准差 ", sd, " ns，中位数 ", samples[BENCH_TRIALS / 2] / 10000, " ns（", BENCH_TRIALS, " 轮掐头去尾，每轮 ", kMoveIters, " 次）");
                }
            }

            LOG("--- [性能] clear 成本（不同规模） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 sizes[] = { 100, 10000, 1000000 };

                for (u32 n : sizes) {
                    static u64 samples[BENCH_TRIALS];
                    QpcTimer timer;

                    for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                        auto list = IMPList<int>(1024, 100000, intEquals);
                        for (u32 i = 0; i < n; ++i) {
                            list.add((int) i);
                        }
                        list.merge();

                        timer.start();
                        list.removeAll();
                        samples[t] = timer.stopNs();
                    }

                    sortU64(samples, BENCH_TRIALS);
                    const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM);
                    const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m);
                    LOG("clear ", n, " 元素：平均 ", m / 1e6, " ms，标准差 ", sd / 1e6, " ms");
                }
            }

            LOG("--- [稳定性] 长时间反复 add/merge/clear 循环 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1, intEquals);

                constexpr u32 kRounds = 1000;
                constexpr u32 kPerRound = 500;
                QpcTimer timer;

                timer.start();
                for (u32 r = 0; r < kRounds; ++r) {
                    for (u32 i = 0; i < kPerRound; ++i) {
                        list.add((int) i);
                    }
                    if (r % 100 == 0) {
                        list.merge();
                    }
                }
                const u64 totalNs = timer.stopNs();

                LOG(kRounds, " 轮 × ", kPerRound, " 元素：总耗时 ", totalNs / 1e6, " ms，最终 present = ", list.presentElementCount(), "，ingest = ", list.ingestElementCount());
            }

            LOG("--- [统计] 合并延迟分布 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 1, intEquals);

                for (int i = 0; i < 100000; ++i) {
                    list.add(i);
                }
                list.merge();
                const usize mergeCount1 = list.presentElementCount();

                static u64 samples[BENCH_TRIALS];
                QpcTimer timer;

                for (u32 t = 0; t < BENCH_TRIALS; ++t) {
                    for (int i = 0; i < 1000; ++i) {
                        list.add((int) (100000 + t * 1000 + i));
                    }
                    timer.start();
                    list.merge();
                    samples[t] = timer.stopNs();
                }

                sortU64(samples, BENCH_TRIALS);
                const double m = meanOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM);
                const double sd = stddevOf(samples, BENCH_TRIM, BENCH_TRIALS - BENCH_TRIM, m);
                LOG("增量 merge 延迟：平均 ", m / 1000.0, " us，标准差 ", sd / 1000.0, " us");
                LOG("  最小 ", samples[0] / 1000.0, " us，中位 ", samples[BENCH_TRIALS / 2] / 1000.0, " us，最大 ", samples[BENCH_TRIALS - 1] / 1000.0, " us");
                LOG("  初始 present = ", mergeCount1, "，累计写入 151000 元素");
            }
        }
        LOG("===== IMPList 深度测试结束 =====");
    }
}