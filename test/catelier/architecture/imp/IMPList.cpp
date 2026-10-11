#include <windows.h>

#include <thread>
#include <atomic>
#include <vector>

#include "../../util/Logger.hpp"

#include "../../../../include/catelier/architecture/imp/IMPList.hpp"

namespace Catelier::test::architecture::imp {
    namespace {
        auto intEquals(const void* a, const void* b) -> bool {
            return *((const int*) a) == *((const int*) b);
        }
        auto intEvenPredicate(const int& x) -> bool {
            return x % 2 == 0;
        }
    }

    auto runTests() -> void {
        using namespace Catelier::architecture::imp;

        LOG("===== IMPList 演示开始 =====");
        LOGGER_INDENT_BLOCK(true) {
            LOG("--- 构造与基础状态 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                LOG("构造后：isEmpty = ", list.isEmpty(), "（应为 1）");
                LOG("presentElementCount = ", list.presentElementCount(), "（应为 0）");
                LOG("ingestElementCount = ", list.ingestElementCount(), "（应为 0）");
            }

            LOG("--- 单线程 add + merge ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 5; ++i) {
                    list.add(i * 10);
                }

                LOG("add 5 个元素后：ingestElementCount = ", list.ingestElementCount(), "（应为 5）");
                LOG("merge 前 presentElementCount = ", list.presentElementCount(), "（应为 5）");

                list.merge();

                LOG("merge 后：presentElementCount = ", list.presentElementCount(), "（应为 5）");
                LOG("merge 后：ingestElementCount = ", list.ingestElementCount(), "（应为 5，段未清空）");

                bool allCorrect = true;
                for (int i = 0; i < 5; ++i) {
                    const int* v = list.get(i);
                    if (!v || *v != i * 10) {
                        allCorrect = false;
                        break;
                    }
                }
                LOG("元素内容正确：", allCorrect ? "PASS" : "FAIL");
            }

            LOG("--- 跨 chunk 写入 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1000, intEquals);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.merge();

                LOG("chunkSize = 16 写入 100 个元素：presentElementCount = ", list.presentElementCount(), "（应为 100）");

                bool allCorrect = true;
                for (int i = 0; i < 100; ++i) {
                    const int* v = list.get(i);
                    if (!v || *v != i) {
                        allCorrect = false;
                        break;
                    }
                }
                LOG("元素内容正确：", allCorrect ? "PASS" : "FAIL");
            }

            LOG("--- addAll 批量写入 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                int elements[50];
                for (int i = 0; i < 50; ++i) {
                    elements[i] = i * 3;
                }

                list.addAll(elements, 50);
                list.merge();

                LOG("addAll 50 个元素：presentElementCount = ", list.presentElementCount(), "（应为 50）");

                bool allCorrect = true;
                for (int i = 0; i < 50; ++i) {
                    const int* v = list.get(i);
                    if (!v || *v != i * 3) {
                        allCorrect = false;
                        break;
                    }
                }
                LOG("元素内容正确：", allCorrect ? "PASS" : "FAIL");
            }

            LOG("--- get / first / last ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                LOG("空列表 first = ", list.first() == nullptr ? "NULL" : "非 NULL");
                LOG("空列表 last = ", list.last() == nullptr ? "NULL" : "非 NULL");
                LOG("空列表 get(0) = ", list.get(0) == nullptr ? "NULL" : "非 NULL");

                for (int i = 1; i <= 5; ++i) {
                    list.add(i * 100);
                }
                list.merge();

                const int* first = list.first();
                const int* last = list.last();
                const int* mid = list.get(2);

                LOG("first = ", first ? *first : -1, "（应为 100）");
                LOG("last = ", last ? *last : -1, "（应为 500）");
                LOG("get(2) = ", mid ? *mid : -1, "（应为 300）");
                LOG("get(5) = ", list.get(5) == nullptr ? "NULL" : "非 NULL", "（应为 NULL）");
            }

            LOG("--- find / contains / indexOf ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 1; i <= 10; ++i) {
                    list.add(i * 5);
                }
                list.merge();

                LOG("contains(25) = ", list.contains(25) ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("contains(7) = ", !list.contains(7) ? "PASS" : "FAIL", "（应为 PASS）");
                LOG("find(30) 非空：", list.find(30) != nullptr ? "PASS" : "FAIL");
                LOG("find(99) 为 NULL：", list.find(99) == nullptr ? "PASS" : "FAIL");
                LOG("firstIndexOf(25) = ", list.firstIndexOf(25), "（应为 4）");
                LOG("lastIndexOf(50) = ", list.lastIndexOf(50), "（应为 9）");
            }

            LOG("--- 重复元素查找（隔离实例） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto dup = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 5; ++i) {
                    dup.add(7);
                }

                LOG("merge 前：presentElementCount = ", dup.presentElementCount(), "（应为 0），ingestElementCount = ", dup.ingestElementCount(), "（应为 5）");

                dup.merge();

                LOG("merge 后：presentElementCount = ", dup.presentElementCount(), "（应为 5）");
                LOG("重复元素 firstIndexOf(7) = ", dup.firstIndexOf(7), "（应为 0）");
                LOG("重复元素 lastIndexOf(7) = ", dup.lastIndexOf(7), "（应为 4）");
            }

            LOG("--- remove / removeAll ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 1; i <= 5; ++i) {
                    list.add(i);
                }
                list.merge();

                LOG("remove(3) = ", list.remove(3) ? "PASS" : "FAIL");
                LOG("remove(99) = ", !list.remove(99) ? "PASS" : "FAIL");
                list.merge();

                LOG("remove 后 presentElementCount = ", list.presentElementCount(), "（应为 4）");
                LOG("contains(3) = ", !list.contains(3) ? "PASS" : "FAIL");

                int toRemove[] = { 2, 4 };
                list.removeAll(toRemove, 2);
                list.merge();
                LOG("removeAll 后 presentElementCount = ", list.presentElementCount(), "（应为 2）");
            }

            LOG("--- removeIf / retainIf（隔离实例 + 中间诊断） ---");
            LOGGER_INDENT_BLOCK(true) {
                LOG("removeIf 场景：");
                LOGGER_INDENT_BLOCK(true) {
                    auto list = IMPList<int>(64, 1000, intEquals);
                    for (int i = 1; i <= 10; ++i) {
                        list.add(i);
                    }

                    LOG("[诊断] add 后：present=", list.presentElementCount(), " ingest=", list.ingestElementCount());

                    list.merge();
                    LOG("[诊断] 第一次 merge 后：present=", list.presentElementCount(), " ingest=", list.ingestElementCount());

                    list.removeIf(intEvenPredicate);
                    LOG("[诊断] removeIf 后：present=", list.presentElementCount(), " ingest=", list.ingestElementCount());

                    list.merge();
                    LOG("[诊断] 第二次 merge 后：present=", list.presentElementCount(), " ingest=", list.ingestElementCount());

                    LOG("removeIf(偶数) 后 presentElementCount = ", list.presentElementCount(), "（应为 5）");

                    bool noEvens = true;
                    for (usize i = 0; i < list.presentElementCount(); ++i) {
                        if (*list.get(i) % 2 == 0) {
                            noEvens = false;
                            break;
                        }
                    }
                    LOG("无偶数：", noEvens ? "PASS" : "FAIL");
                }

                LOG("retainIf 场景：");
                LOGGER_INDENT_BLOCK(true) {
                    auto list2 = IMPList<int>(64, 1000, intEquals);
                    for (int i = 1; i <= 10; ++i) {
                        list2.add(i);
                    }

                    LOG("[诊断] add 后：present=", list2.presentElementCount(), " ingest=", list2.ingestElementCount());

                    list2.merge();
                    LOG("[诊断] 第一次 merge 后：present=", list2.presentElementCount(), " ingest=", list2.ingestElementCount());

                    list2.retainIf(intEvenPredicate);
                    LOG("[诊断] retainIf 后：present=", list2.presentElementCount(), " ingest=", list2.ingestElementCount());

                    list2.merge();
                    LOG("[诊断] 第二次 merge 后：present=", list2.presentElementCount(), " ingest=", list2.ingestElementCount());

                    LOG("retainIf(偶数) 后 presentElementCount = ", list2.presentElementCount(), "（应为 5）");

                    bool allEvens = true;
                    for (usize i = 0; i < list2.presentElementCount(); ++i) {
                        if (*list2.get(i) % 2 != 0) {
                            allEvens = false;
                            break;
                        }
                    }
                    LOG("全是偶数：", allEvens ? "PASS" : "FAIL");
                }
            }

            LOG("--- clear ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 20; ++i) {
                    list.add(i);
                }
                list.merge();
                LOG("clear 前 presentElementCount = ", list.presentElementCount(), "（应为 20）");

                list.clear();
                LOG("clear 后 presentElementCount = ", list.presentElementCount(), "（应为 0）");
                LOG("clear 后 ingestElementCount = ", list.ingestElementCount(), "（应为 0）");
                LOG("clear 后 isEmpty = ", list.isEmpty() ? "PASS" : "FAIL");

                list.add(100);
                list.merge();
                LOG("clear 后再 add：presentElementCount = ", list.presentElementCount(), "（应为 1）");
                LOG("clear 后再 add：元素值 = ", *((const int*) list.get(0)), "（应为 100）");
            }

            LOG("--- copy（隔离 + 诊断） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto original = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 10; ++i) {
                    original.add(i * 7);
                }
                original.merge();

                LOG("[诊断] original.merge 后：present=", original.presentElementCount());

                auto copied = original;
                LOG("[诊断] copy 后：original.present=", original.presentElementCount(), " copied.present=", copied.presentElementCount());
                LOG("[诊断] copy 后：original.ingest=", original.ingestElementCount(), " copied.ingest=", copied.ingestElementCount());

                copied.add(999);
                copied.merge();

                LOG("copied 修改后：original.presentElementCount = ", original.presentElementCount(), "（应为 10）");
                LOG("copied.presentElementCount = ", copied.presentElementCount(), "（应为 11）");
                LOG("original 值未变：", *((const int*) original.get(0)) == 0 ? "PASS" : "FAIL");
                LOG("copied 尾部值正确：", *((const int*) copied.get(10)) == 999 ? "PASS" : "FAIL");
            }

            LOG("--- move（隔离 + 诊断） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto source = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 10; ++i) {
                    source.add(i);
                }
                source.merge();

                LOG("[诊断] source.merge 后：present=", source.presentElementCount());

                auto moved = std::move(source);
                LOG("[诊断] move 后：moved.present=", moved.presentElementCount(), " source.present=", source.presentElementCount());

                LOG("move 后：moved.presentElementCount = ", moved.presentElementCount(), "（应为 10）");
                LOG("move 后：source.presentElementCount = ", source.presentElementCount(), "（应为 0）");
                LOG("moved 第一个元素正确：", *((const int*) moved.get(0)) == 0 ? "PASS" : "FAIL");
                LOG("moved 最后一个元素正确：", *((const int*) moved.get(9)) == 9 ? "PASS" : "FAIL");
            }

            LOG("--- 排序 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                const int values[] = { 5, 2, 8, 1, 9, 3, 7, 4, 6 };
                for (int v : values) {
                    list.add(v);
                }
                list.merge();

                list.naturalSort();
                LOG("升序排序后：");
                bool ascending = true;
                for (usize i = 1; i < list.presentElementCount(); ++i) {
                    if (*list.get(i - 1) > *list.get(i)) {
                        ascending = false;
                        break;
                    }
                }
                LOG("  升序：", ascending ? "PASS" : "FAIL");

                auto list2 = IMPList<int>(64, 1000, intEquals);
                for (int v : values) {
                    list2.add(v);
                }
                list2.merge();
                list2.naturalSortDescending();

                bool descending = true;
                for (usize i = 1; i < list2.presentElementCount(); ++i) {
                    if (*list2.get(i - 1) < *list2.get(i)) {
                        descending = false;
                        break;
                    }
                }
                LOG("降序排序：", descending ? "PASS" : "FAIL");
            }

            LOG("--- 边界：连续 merge ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 5; ++i) {
                    list.add(i);
                }

                list.merge();
                LOG("第一次 merge 后：present=", list.presentElementCount(), "（应为 5）");

                list.merge();
                LOG("第二次 merge 后（无新写入）：present=", list.presentElementCount(), "（应为 5）");

                list.merge();
                LOG("第三次 merge 后：present=", list.presentElementCount(), "（应为 5）");

                bool stillCorrect = true;
                for (int i = 0; i < 5; ++i) {
                    if (*((const int*) list.get(i)) != i) {
                        stillCorrect = false;
                        break;
                    }
                }
                LOG("元素内容仍正确：", stillCorrect ? "PASS" : "FAIL");
            }

            LOG("--- 边界：空列表 merge ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                list.merge();
                LOG("空列表 merge 后：present=", list.presentElementCount(), "（应为 0）");
                LOG("空列表 merge 后：isEmpty = ", list.isEmpty() ? "PASS" : "FAIL");
            }

            LOG("--- 边界：merge 后再 add 再 merge ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1000, intEquals);
                for (int i = 0; i < 5; ++i) {
                    list.add(i);
                }
                list.merge();
                LOG("第一次 merge：present=", list.presentElementCount(), "（应为 5）");

                for (int i = 5; i < 10; ++i) {
                    list.add(i);
                }
                LOG("add 后：present=", list.presentElementCount(), "（应为 5）");
                LOG("add 后：ingest=", list.ingestElementCount(), "（应为 10）");

                list.merge();
                LOG("第二次 merge：present=", list.presentElementCount(), "（应为 10）");

                bool allCorrect = true;
                for (int i = 0; i < 10; ++i) {
                    if (*((const int*) list.get(i)) != i) {
                        allCorrect = false;
                        break;
                    }
                }
                LOG("元素内容正确：", allCorrect ? "PASS" : "FAIL");
            }

            LOG("--- 多线程并发 add ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1, intEquals);
                constexpr int kThreadCount = 4;
                constexpr int kElementsPerThread = 1000;

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

                LOG("4 线程各写 1000 个：presentElementCount = ", list.presentElementCount(), "（应为 4000）");
                LOG("总元素数正确：", list.presentElementCount() == 4000 ? "PASS" : "FAIL");
            }

            LOG("--- 多线程 add 数据完整性 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(128, 1, intEquals);
                constexpr int kThreadCount = 8;
                constexpr int kElementsPerThread = 500;

                std::vector<std::thread> threads;
                for (int t = 0; t < kThreadCount; ++t) {
                    threads.emplace_back([&list, t, kElementsPerThread]() {
                        for (int i = 0; i < kElementsPerThread; ++i) {
                            list.add(t * 1000000 + i);
                        }
                    });
                }

                for (auto& thread : threads) {
                    thread.join();
                }

                list.awaitMerging();

                std::vector<int> seen(kThreadCount, 0);
                bool dataIntact = true;
                for (usize i = 0; i < list.presentElementCount(); ++i) {
                    const int v = *list.get(i);
                    const int threadId = v / 1000000;
                    const int index = v % 1000000;
                    if (threadId < 0 || threadId >= kThreadCount || index < 0 || index >= kElementsPerThread) {
                        dataIntact = false;
                        break;
                    }
                    seen[threadId]++;
                }
                bool allSeen = true;
                for (int t = 0; t < kThreadCount; ++t) {
                    if (seen[t] != kElementsPerThread) {
                        allSeen = false;
                    }
                }
                LOG("数据完整：", dataIntact && allSeen ? "PASS" : "FAIL");
            }

            LOG("--- 并发读（多线程只读） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 1000, intEquals);
                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.merge();

                constexpr int kReaderCount = 4;
                constexpr u32 kReadsPerThread = 1000000;

                std::atomic<bool> running = { true };
                std::atomic<u64> totalReads = { 0 };

                std::vector<std::thread> readers;
                for (int r = 0; r < kReaderCount; ++r) {
                    readers.emplace_back([&list, &running, &totalReads]() {
                        u64 localReads = 0;
                        while (running.load(std::memory_order_relaxed)) {
                            for (u32 i = 0; i < 1000; ++i) {
                                const int* v = list.get(i % 10000);
                                if (v) {
                                    localReads++;
                                }
                            }
                        }
                        totalReads.fetch_add(localReads);
                    });
                }

                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);

                LARGE_INTEGER start;
                LARGE_INTEGER end;
                QueryPerformanceCounter(&start);

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                running.store(false);

                for (auto& thread : readers) {
                    thread.join();
                }

                QueryPerformanceCounter(&end);

                const u64 totalNs = (end.QuadPart - start.QuadPart) * 1000000000ULL / freq.QuadPart;
                const u64 reads = totalReads.load();
                const u64 ns10x = reads > 0 ? (totalNs * 10 / reads) : 0;
                LOG(kReaderCount, " 线程并发只读 1 秒：总读 ", reads, " 次，平均 ", ns10x / 10, ".", ns10x % 10, " ns/op");
            }

            LOG("--- 读写混合（1 写 + 4 读） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1, intEquals);
                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.merge();

                std::atomic<bool> running = { true };
                std::atomic<u64> totalWrites = { 0 };
                std::atomic<u64> totalReads = { 0 };

                std::thread writer([&list, &running, &totalWrites]() {
                    u64 localWrites = 0;
                    int counter = 0;
                    while (running.load(std::memory_order_relaxed)) {
                        list.add(counter++);
                        localWrites++;
                    }
                    totalWrites.fetch_add(localWrites);
                });

                std::vector<std::thread> readers;
                for (int r = 0; r < 4; ++r) {
                    readers.emplace_back([&list, &running, &totalReads]() {
                        u64 localReads = 0;
                        while (running.load(std::memory_order_relaxed)) {
                            const usize count = list.presentElementCount();
                            if (count > 0) {
                                const int* v = list.get(0);
                                if (v) {
                                    localReads++;
                                }
                            }
                        }
                        totalReads.fetch_add(localReads);
                    });
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                running.store(false);

                writer.join();
                for (auto& thread : readers) {
                    thread.join();
                }

                LOG("1 写 + 4 读运行 1 秒：写 ", totalWrites.load(), " 次，读 ", totalReads.load(), " 次，最终 present=", list.presentElementCount());
            }

            LOG("--- 性能：单线程 add 延迟（缓存热） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16384, 100000, intEquals);

                constexpr u32 kBatch = 10000;
                constexpr u32 kTrials = 100;
                constexpr u32 kCut = 10;
                constexpr u32 kMid = kTrials - 2 * kCut;

                static u64 samples[kTrials];

                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);

                for (u32 t = 0; t < kTrials; ++t) {
                    LARGE_INTEGER start;
                    LARGE_INTEGER end;
                    QueryPerformanceCounter(&start);
                    for (u32 i = 0; i < kBatch; ++i) {
                        list.add(i);
                    }
                    QueryPerformanceCounter(&end);
                    samples[t] = (end.QuadPart - start.QuadPart) * 1000000000ULL / freq.QuadPart;
                }

                for (u32 i = 0; i < kTrials; ++i) {
                    for (u32 j = i + 1; j < kTrials; ++j) {
                        if (samples[i] > samples[j]) {
                            const u64 tmp = samples[i];
                            samples[i] = samples[j];
                            samples[j] = tmp;
                        }
                    }
                }

                u64 sum = 0;
                for (u32 i = kCut; i < kTrials - kCut; ++i) {
                    sum += samples[i];
                }

                const u64 avgBatch = sum / kMid;
                const u64 ns10x = avgBatch * 10 / kBatch;
                LOG("add 平均：", ns10x / 10, ".", ns10x % 10, " ns/op（", kTrials, " 批 × ", kBatch, " 次，去两端各 ", kCut, " 批）");
            }

            LOG("--- 性能：add + merge 吞吐量 ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr u32 kTotalElements = 100000;
                constexpr u32 kTrials = 10;

                static u64 samples[kTrials];

                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);

                for (u32 t = 0; t < kTrials; ++t) {
                    auto list = IMPList<int>(1024, 100000, intEquals);

                    LARGE_INTEGER start;
                    LARGE_INTEGER end;
                    QueryPerformanceCounter(&start);
                    for (u32 i = 0; i < kTotalElements; ++i) {
                        list.add(i);
                    }
                    list.merge();
                    QueryPerformanceCounter(&end);

                    samples[t] = (end.QuadPart - start.QuadPart) * 1000000000ULL / freq.QuadPart;
                }

                for (u32 i = 0; i < kTrials; ++i) {
                    for (u32 j = i + 1; j < kTrials; ++j) {
                        if (samples[i] > samples[j]) {
                            const u64 tmp = samples[i];
                            samples[i] = samples[j];
                            samples[j] = tmp;
                        }
                    }
                }

                const u64 median = samples[kTrials / 2];
                const u64 ns100x = median * 100 / kTotalElements;
                LOG("add 100000 + merge 平均：", ns100x / 100, ".", (ns100x / 10) % 10, ns100x % 10, " ns/op（", kTrials, " 轮取中位数）");
            }

            LOG("--- 性能：get 延迟（快照访问） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 100000, intEquals);
                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.merge();

                constexpr u32 kIterations = 10000000;

                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);

                LARGE_INTEGER start;
                LARGE_INTEGER end;
                QueryPerformanceCounter(&start);
                int accum = 0;
                for (u32 i = 0; i < kIterations; ++i) {
                    const int* v = list.get(i % 10000);
                    accum += *v;
                }
                QueryPerformanceCounter(&end);

                if (accum == 0xDEADBEEF) {
                    LOG("impossible");
                }

                const u64 totalNs = (end.QuadPart - start.QuadPart) * 1000000000ULL / freq.QuadPart;
                const u64 ns10x = totalNs * 10 / kIterations;
                LOG("get 平均：", ns10x / 10, ".", ns10x % 10, " ns/op（", kIterations, " 次）");
            }

            LOG("--- 性能：contains 延迟（快照扫描） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 100000, intEquals);
                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
                list.merge();

                constexpr u32 kIterations = 100000;

                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);

                LARGE_INTEGER start;
                LARGE_INTEGER end;
                QueryPerformanceCounter(&start);
                u32 hits = 0;
                for (u32 i = 0; i < kIterations; ++i) {
                    if (list.contains((int) (i % 1000))) {
                        hits++;
                    }
                }
                QueryPerformanceCounter(&end);

                const u64 totalNs = (end.QuadPart - start.QuadPart) * 1000000000ULL / freq.QuadPart;
                const u64 ns100x = totalNs * 100 / kIterations;
                LOG("contains 平均：", ns100x / 100, ".", (ns100x / 10) % 10, ns100x % 10, " ns/op（", kIterations, " 次，命中 ", hits, "）");
            }

            LOG("--- 性能：多线程并发 add 吞吐量 ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr int kThreadCount = 4;
                constexpr int kElementsPerThread = 250000;
                constexpr u32 kTrials = 5;

                static u64 samples[kTrials];

                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);

                for (u32 t = 0; t < kTrials; ++t) {
                    auto list = IMPList<int>(1024, 100000, intEquals);

                    LARGE_INTEGER start;
                    LARGE_INTEGER end;
                    QueryPerformanceCounter(&start);

                    std::vector<std::thread> threads;
                    for (int th = 0; th < kThreadCount; ++th) {
                        threads.emplace_back([&list, th, kElementsPerThread]() {
                            for (int i = 0; i < kElementsPerThread; ++i) {
                                list.add(th * kElementsPerThread + i);
                            }
                        });
                    }
                    for (auto& thread : threads) {
                        thread.join();
                    }

                    list.merge();
                    QueryPerformanceCounter(&end);

                    samples[t] = (end.QuadPart - start.QuadPart) * 1000000000ULL / freq.QuadPart;
                }

                for (u32 i = 0; i < kTrials; ++i) {
                    for (u32 j = i + 1; j < kTrials; ++j) {
                        if (samples[i] > samples[j]) {
                            const u64 tmp = samples[i];
                            samples[i] = samples[j];
                            samples[j] = tmp;
                        }
                    }
                }

                const u64 median = samples[kTrials / 2];
                constexpr u64 totalElements = kThreadCount * kElementsPerThread;
                const u64 ns100x = median * 100 / totalElements;
                LOG(kThreadCount, " 线程各写 ", kElementsPerThread, " 个：", ns100x / 100, ".", (ns100x / 10) % 10, ns100x % 10, " ns/op（", kTrials, " 轮取中位数）");
            }

            LOG("--- 性能：move 的 O(1) 特性 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(1024, 100000, intEquals);
                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.merge();

                LARGE_INTEGER freq;
                QueryPerformanceFrequency(&freq);

                LARGE_INTEGER start;
                LARGE_INTEGER end;
                QueryPerformanceCounter(&start);
                auto moved = std::move(list);
                QueryPerformanceCounter(&end);

                const u64 totalNs = (end.QuadPart - start.QuadPart) * 1000000000ULL / freq.QuadPart;
                LOG("move 10000 元素列表耗时：", totalNs, " ns");
                LOG("moved.presentElementCount = ", moved.presentElementCount(), "（应为 10000）");
            }
        }
        LOG("===== IMPList 演示结束 =====");
    }
}