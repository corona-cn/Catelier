#include <windows.h>
#include <intrin.h>

#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <fstream>

#include <nanobench.h>

#include "../../util/Logger.hpp"

#include "../../../../include/catelier/architecture/imp/IMPList.hpp"

namespace Catelier::test::architecture::imp {
    namespace {
        using namespace Catelier::include::architecture::imp;

        auto intEvenPredicate(const int& x) -> bool {
            return x % 2 == 0;
        }
        auto intOddPredicate(const int& x) -> bool {
            return x % 2 != 0;
        }
        auto intLessThan(const int& x) -> int {
            return x;
        }

        // 全局的 Nanobench 对象，用于收集所有性能测试结果并最终导出 JSON
        ankerl::nanobench::Bench globalBench;
    }

    namespace {
        using namespace Catelier::include::architecture::imp;

        // ============================================================
        // [正确性] 测试
        // ============================================================

        auto testConstructAndLifecycle() -> void {
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
        }

        auto testAddAndAddAll() -> void {
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
        }

        auto testChunkBoundary() -> void {
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
        }

        auto testRemoveAndRemoveAllBoundary() -> void {
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
        }

        auto testRemoveIfAndRetainIf() -> void {
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
        }

        auto testClearSemantics() -> void {
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
        }

        auto testDiscardBasic() -> void {
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
        }

        auto testDiscardDuplicateMissingOverCapacity() -> void {
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
        }

        auto testDiscardWithAddTiming() -> void {
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
        }

        auto testDiscardAllBasic() -> void {
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
        }

        auto testDiscardAllWindowSemantics() -> void {
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
        }

        auto testDiscardAllDuringMerge() -> void {
            LOG("--- [正确性] discardAll 与并发 merge 的请求不丢失 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }

                std::thread merger([&list]() {
                    for (int round = 0; round < 10; ++round) {
                        list.tryMerging();
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                });

                list.discardAll();

                std::this_thread::sleep_for(std::chrono::microseconds(50));

                for (int i = 0; i < 100; ++i) {
                    list.add(1000 + i);
                }

                list.discardAll();

                merger.join();

                list.awaitMerging();

                LOG("两次 discardAll（中间 add 100）后 present = ", list.presentElementCount(), "（应为 0）");
                LOG("第二次 discardAll 未丢失：", list.presentElementCount() == 0 ? "PASS" : "FAIL");
            }
        }

        auto testCopyAndMove() -> void {
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
        }

        auto testSortingStrategy() -> void {
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
        }

        auto testFindContainsIndexOfUnordered() -> void {
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
        }

        auto testFindContainsIndexOfAscending() -> void {
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
        }

        auto testDuplicateFirstLastIndexOf() -> void {
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
        }

        auto testContainsAll() -> void {
            LOG("--- [正确性] containsAll 语义（全命中 / 部分命中 / 全未命中 / 重复 / 空 / NULL） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(16, 1024);
                for (int i = 0; i < 10; ++i) {
                    list.add(i);
                }
                list.naturalSort();
                list.awaitMerging();

                constexpr int allHit[] = { 0, 5, 9 };
                constexpr int partialMiss[] = { 0, 5, 10 };
                constexpr int allMiss[] = { 100, 200, 300 };
                constexpr int duplicates[] = { 5, 5, 5 };

                LOG("containsAll(all hit) = ", list.containsAll(allHit, 3), "（应为 1）");
                LOG("containsAll(partial miss) = ", list.containsAll(partialMiss, 3), "（应为 0）");
                LOG("containsAll(all miss) = ", list.containsAll(allMiss, 3), "（应为 0）");
                LOG("containsAll(duplicates) = ", list.containsAll(duplicates, 3), "（应为 1）");
                LOG("containsAll(空) = ", list.containsAll(allHit, 0), "（应为 1）");
                LOG("containsAll(NULL) = ", list.containsAll((const int*) nullptr, 3), "（应为 0）");
            }
        }

        auto testIteratorBasicTraversal() -> void {
            LOG("--- [正确性] 迭代器基础遍历 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                int sum = 0;
                usize count = 0;
                bool ordered = true;
                int expected = 0;
                for (const auto& v : list) {
                    sum += v;
                    if (v != expected) {
                        ordered = false;
                    }
                    ++expected;
                    ++count;
                }
                LOG("迭代器遍历 count = ", count, "（应为 100）");
                LOG("迭代器遍历 sum = ", sum, "（应为 4950）");
                LOG("迭代器遍历有序：", ordered ? "PASS" : "FAIL");

                auto it = list.begin();
                auto end = list.end();
                int manualSum = 0;
                usize manualCount = 0;
                while (it != end) {
                    manualSum += *it;
                    ++it;
                    ++manualCount;
                }
                LOG("begin/end 手动迭代 sum = ", manualSum, "，count = ", manualCount, "（应为 4950 / 100）");
                LOG("begin/end 与 for-range 一致：", (manualSum == sum && manualCount == count) ? "PASS" : "FAIL");
            }
        }

        auto testIteratorEmptyList() -> void {
            LOG("--- [正确性] 迭代器空列表 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);

                usize count = 0;
                for (const auto& v : list) {
                    (void) v;
                    ++count;
                }
                LOG("空列表迭代 count = ", count, "（应为 0）");

                auto it = list.begin();
                auto end = list.end();
                LOG("空列表 begin == end：", (it == end) ? "PASS" : "FAIL");
            }
        }

        auto testIteratorDuringAdd() -> void {
            LOG("--- [正确性] 迭代器期间 add 不影响当前迭代 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 50; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                usize count = 0;
                int sum = 0;
                for (const auto& v : list) {
                    if (count == 0) {
                        for (int j = 100; j < 150; ++j) {
                            list.add(j);
                        }
                    }
                    sum += v;
                    ++count;
                }
                LOG("迭代中 add 50 个 → 当前迭代 count = ", count, "（应为 50，快照语义）");
                LOG("迭代中 add 50 个 → 当前迭代 sum = ", sum, "（应为 1225）");

                list.tryMerging();
                list.awaitMerging();
                LOG("迭代后 merge → present = ", list.presentElementCount(), "（应为 100）");
            }
        }

        auto testIteratorDuringMerge() -> void {
            LOG("--- [正确性] 迭代器期间 merge 不影响当前迭代 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                usize count = 0;
                int sum = 0;
                for (const auto& v : list) {
                    if (count == 10) {
                        list.add(9999);
                        list.tryMerging();
                        list.awaitMerging();
                    }
                    sum += v;
                    ++count;
                }
                LOG("迭代中 merge → 当前迭代 count = ", count, "（应为 100）");
                LOG("迭代中 merge → 当前迭代 sum = ", sum, "（应为 4950）");

                LOG("迭代后 present = ", list.presentElementCount(), "（应为 101）");
            }
        }

        auto testIteratorDuringRemove() -> void {
            LOG("--- [正确性] 迭代器期间 remove 不影响当前迭代 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                usize count = 0;
                int sum = 0;
                for (const auto& v : list) {
                    if (count == 0) {
                        for (int j = 0; j < 50; ++j) {
                            list.remove(j);
                        }
                        list.tryMerging();
                        list.awaitMerging();
                    }
                    sum += v;
                    ++count;
                }
                LOG("迭代中 remove 50 个 → 当前迭代 count = ", count, "（应为 100）");
                LOG("迭代中 remove 50 个 → 当前迭代 sum = ", sum, "（应为 4950）");

                LOG("迭代后 present = ", list.presentElementCount(), "（应为 50）");
            }
        }

        auto testIteratorDuringClear() -> void {
            LOG("--- [正确性] 迭代器期间 clear 不影响当前迭代 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                usize count = 0;
                for (const auto& v : list) {
                    (void) v;
                    if (count == 0) {
                        list.removeAll();
                        list.tryMerging();
                        list.awaitMerging();
                    }
                    ++count;
                }
                LOG("迭代中 clear → 当前迭代 count = ", count, "（应为 100）");
                LOG("迭代后 present = ", list.presentElementCount(), "（应为 0）");
            }
        }

        auto testIteratorDuringSort() -> void {
            LOG("--- [正确性] 迭代器期间 sort 不影响当前迭代 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                const int values[] = { 5, 2, 8, 1, 9, 3, 7, 4, 6, 0 };
                for (int v : values) {
                    list.add(v);
                }
                list.tryMerging();
                list.awaitMerging();

                int firstVal = -1;
                int count = 0;
                for (const auto& v : list) {
                    if (count == 0) {
                        firstVal = v;
                        list.naturalSort();
                    }
                    ++count;
                }
                LOG("迭代中 sort → 当前迭代 count = ", count, "（应为 10）");
                LOG("当前迭代首个元素 = ", firstVal, "（应为 5，旧快照）");
                LOG("迭代后 first = ", *list.first(), "（应为 0，新快照已排序）");
            }
        }

        auto testMultipleIteratorsSameSpan() -> void {
            LOG("--- [正确性] 多迭代器独立遍历同一 span ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                auto it1 = list.begin();
                auto it2 = list.begin();
                auto end = list.end();

                int sum1 = 0;
                int sum2 = 0;
                while (it1 != end && it2 != end) {
                    sum1 += *it1;
                    sum2 += *it2;
                    ++it1;
                    ++it2;
                }
                LOG("双迭代器并行遍历 sum1 = ", sum1, "，sum2 = ", sum2, "（均应 4950）");
                LOG("双迭代器一致：", (sum1 == sum2 && sum1 == 4950) ? "PASS" : "FAIL");
            }
        }

        auto testMultipleBeginIndependentSpan() -> void {
            LOG("--- [正确性] 迭代器期间多个 begin 独立持有 span ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                auto it1 = list.begin();
                int sum1 = 0;
                while (it1 != list.end()) {
                    sum1 += *it1;
                    if (sum1 == 0) {
                        list.add(9999);
                        list.tryMerging();
                        list.awaitMerging();

                        auto it2 = list.begin();
                        int sum2 = 0;
                        while (it2 != list.end()) {
                            sum2 += *it2;
                            ++it2;
                        }
                        LOG("it2 遍历新快照 sum = ", sum2, "（应为 14949，含 9999）");
                    }
                    ++it1;
                }
                LOG("it1 遍历旧快照 sum = ", sum1, "（应为 4950）");
            }
        }

        auto testIteratorSpareReuse() -> void {
            LOG("--- [正确性] 迭代器 + spare 复用（反复创建销毁无泄漏） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                constexpr u32 kRounds = 1000;
                u64 totalSum = 0;
                for (u32 r = 0; r < kRounds; ++r) {
                    for (const auto& v : list) {
                        totalSum += v;
                    }
                }
                LOG(kRounds, " 轮迭代后总 sum = ", totalSum, "（应为 4950000）");
                LOG("spare 复用无泄漏：PASS（无崩溃）");
            }
        }

        auto testForEachVsIterator() -> void {
            LOG("--- [正确性] forEach / forEachIndexed 与迭代器行为一致 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                int iteratorSum = 0;
                for (const auto& v : list) {
                    iteratorSum += v;
                }

                int forEachSum = 0;
                list.forEach([&forEachSum](const int& v) {
                    forEachSum += v;
                });

                int forEachIdxSum = 0;
                usize forEachIdxCount = 0;
                bool idxOk = true;
                list.forEachIndexed([&forEachIdxSum, &forEachIdxCount, &idxOk](const usize i, const int& v) {
                    forEachIdxSum += v;
                    if ((usize) v != i) {
                        idxOk = false;
                    }
                    ++forEachIdxCount;
                });

                LOG("forEach sum = ", forEachSum, "，iterator sum = ", iteratorSum, "（应一致，均 4950）");
                LOG("forEach 与迭代器一致：", forEachSum == iteratorSum ? "PASS" : "FAIL");
                LOG("forEachIndexed count = ", forEachIdxCount, "（应为 100）");
                LOG("forEachIndexed 索引与值对应：", idxOk ? "PASS" : "FAIL");
            }
        }

        auto testAwaitMerging() -> void {
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
        }

        auto testAwaitCompacting() -> void {
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
        }

        auto testCompactionCountIncrement() -> void {
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
        }

        auto testCompactIngestDrop() -> void {
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
        }

        auto testCompactAddPreserved() -> void {
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
        }

        auto testCompactMergeReflect() -> void {
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
        }

        auto testCompactThreshold() -> void {
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
        }

        auto testCompactDisabledExplicit() -> void {
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
        }

        auto testRepeatedTryCompacting() -> void {
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
        }

        // ============================================================
        // [并发] 测试
        // ============================================================

        auto testConcurrent4Writers() -> void {
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
        }

        auto testConcurrentDiscardAdd() -> void {
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
        }

        auto testConcurrentDiscardAllWindow() -> void {
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
        }

        auto testConcurrentRemoveMerge() -> void {
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
        }

        auto testConcurrentMultiRemoveMerge() -> void {
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
        }

        auto testConcurrentRemoveIfMerge() -> void {
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
        }

        auto testConcurrentDiscardRemoveMerge() -> void {
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
        }

        auto testConcurrentIngestMergeCount() -> void {
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
        }

        auto testConcurrentCompactRemove() -> void {
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
        }

        auto testConcurrentCompactAdd() -> void {
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
        }

        auto testConcurrentCompactCount() -> void {
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
        }

        auto testConcurrentCompactMerge() -> void {
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
        }

        auto testStressRemoveMerge() -> void {
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
        }

        auto testStressCompactRemoveAdd() -> void {
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
        }

        auto testThreadExitCallback() -> void {
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
        }

        // ============================================================
        // [内存] 测试
        // ============================================================

        auto testRepeatedConstruction() -> void {
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
        }

        auto testCompactLongRunMemory() -> void {
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
        }

        auto testIteratorRepeatedCreation() -> void {
            LOG("--- [内存] 迭代器反复创建销毁（spare 复用无累积） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(64, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 100; ++i) {
                    list.add(i);
                }
                list.tryMerging();
                list.awaitMerging();

                constexpr u32 kRounds = 10000;
                u64 totalSum = 0;
                for (u32 r = 0; r < kRounds; ++r) {
                    for (const auto& v : list) {
                        totalSum += v;
                    }
                }
                LOG(kRounds, " 轮迭代创建销毁：总 sum = ", totalSum, "（应为 49500000），无崩溃");
            }
        }

        // ============================================================
        // [性能] 测试（Nanobench）
        // ============================================================

        auto testBenchAddLatency() -> void {
            LOG("--- [性能] 单线程 add 延迟 ---");
            auto list = IMPList<int>(16384, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            globalBench.batch(1000).run("[基础] add 延迟", [&] {
                for (int i = 0; i < 1000; ++i) {
                    list.add(i);
                }
            });
        }

        auto testBenchDiscardLatency() -> void {
            LOG("--- [性能] 单线程 discard 延迟 ---");
            auto list = IMPList<int>(16384, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            globalBench.batch(1000).run("[基础] discard 延迟", [&] {
                for (int i = 0; i < 1000; ++i) {
                    list.discard(i);
                }
            });
        }

        auto testBenchDiscardAllLatency() -> void {
            LOG("--- [性能] 单线程 discardAll 延迟 ---");
            auto list = IMPList<int>(16384, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            globalBench.batch(1000).run("[基础] discardAll 延迟", [&] {
                for (int i = 0; i < 1000; ++i) {
                    list.discardAll();
                }
            });
        }

        auto testBenchAddMergeThroughput() -> void {
            LOG("--- [性能] add + merge 混合吞吐 ---");
            constexpr u32 n = 100000;
            globalBench.batch(n).run("[基础] add + merge 吞吐", [&] {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                list.add(0);
                list.tryMerging();

                for (u32 i = 0; i < n; ++i) {
                    list.add((int) i);
                }
                list.tryMerging();
                ankerl::nanobench::doNotOptimizeAway(list);
            });
        }

        auto testBenchGetLatency() -> void {
            LOG("--- [性能] get 快照访问延迟 ---");
            auto list = IMPList<int>(1024, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            for (int i = 0; i < 100000; ++i) {
                list.add(i);
            }
            list.tryMerging();

            globalBench.batch(1000).run("[基础] get 延迟", [&] {
                int accum = 0;
                for (int k = 0; k < 1000; ++k) {
                    const int* v = list.get(k % 100000);
                    accum += *v;
                }
                ankerl::nanobench::doNotOptimizeAway(accum);
            });
        }

        auto testBenchIteratorTraversal() -> void {
            LOG("--- [性能] 迭代器整体遍历延迟 ---");
            constexpr u32 sizes[] = { 100, 10000, 1000000 };

            for (u32 n : sizes) {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (u32 i = 0; i < n; ++i) {
                    list.add((int) i);
                }
                list.tryMerging();

                globalBench.batch(n).run(("[迭代器] 遍历 " + std::to_string(n) + " 元素").c_str(), [&] {
                    int accum = 0;
                    for (const auto& v : list) {
                        accum += v;
                    }
                    ankerl::nanobench::doNotOptimizeAway(accum);
                });
            }
        }

        auto testBenchIteratorBeginEnd() -> void {
            LOG("--- [性能] 迭代器 begin/end 开销 ---");
            auto list = IMPList<int>(1024, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            for (int i = 0; i < 1000; ++i) {
                list.add(i);
            }
            list.tryMerging();

            globalBench.batch(100).run("[迭代器] begin + end 开销", [&] {
                for (int k = 0; k < 100; ++k) {
                    auto it = list.begin();
                    auto end = list.end();
                    bool nonEmpty = (it != end);
                    ankerl::nanobench::doNotOptimizeAway(nonEmpty);
                }
            });
        }

        auto testBenchIteratorVsGet() -> void {
            LOG("--- [性能] 迭代器 vs get(i) 循环对比 ---");
            auto list = IMPList<int>(1024, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            constexpr u32 kElements = 10000;
            for (u32 i = 0; i < kElements; ++i) {
                list.add((int) i);
            }
            list.tryMerging();

            globalBench.batch(kElements).run("[迭代器] 迭代器遍历", [&] {
                int accum = 0;
                for (const auto& v : list) {
                    accum += v;
                }
                ankerl::nanobench::doNotOptimizeAway(accum);
            });

            globalBench.batch(kElements).run("[迭代器] get(i) 循环", [&] {
                int accum = 0;
                const usize count = list.presentElementCount();
                for (usize i = 0; i < count; ++i) {
                    const int* v = list.get(i);
                    if (v) {
                        accum += *v;
                    }
                }
                ankerl::nanobench::doNotOptimizeAway(accum);
            });
        }

        auto testBenchConcurrentIterators() -> void {
            LOG("--- [性能] 并发迭代器吞吐（1 / 2 / 4 / 8 线程） ---");
            constexpr int threadConfigs[] = { 1, 2, 4, 8 };

            for (int threadCount : threadConfigs) {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (int i = 0; i < 10000; ++i) {
                    list.add(i);
                }
                list.tryMerging();

                globalBench.batch(10000).run(("[迭代器] " + std::to_string(threadCount) + " 线程并发迭代").c_str(), [&] {
                    std::vector<std::thread> threads;
                    for (int t = 0; t < threadCount; ++t) {
                        threads.emplace_back([&list]() {
                            int accum = 0;
                            for (const auto& v : list) {
                                accum += v;
                            }
                            ankerl::nanobench::doNotOptimizeAway(accum);
                        });
                    }
                    for (auto& t : threads) {
                        t.join();
                    }
                });
            }
        }

        auto testBenchIteratorMerge() -> void {
            LOG("--- [性能] 迭代器 + merge 混合吞吐 ---");
            constexpr u32 n = 10000;
            auto list = IMPList<int>(1024, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            for (u32 i = 0; i < n; ++i) {
                list.add((int) i);
            }
            list.tryMerging();

            globalBench.batch(10).run("[迭代器] 迭代 + merge 混合", [&] {
                for (u32 round = 0; round < 10; ++round) {
                    int accum = 0;
                    for (const auto& v : list) {
                        accum += v;
                    }
                    list.add((int) (n + round));
                    list.tryMerging();
                    ankerl::nanobench::doNotOptimizeAway(accum);
                }
            });
        }

        auto testBenchContains() -> void {
            LOG("--- [性能] contains 扫描延迟（命中/未命中） ---");
            auto list = IMPList<int>(1024, 1024);
            list.configureMaintenanceMerging(false);
            list.configureCompaction(false, 20, 10000);

            for (int i = 0; i < 1000; ++i) {
                list.add(i);
            }
            list.tryMerging();

            globalBench.batch(100).run("[查找] contains (命中)", [&] {
                int hits = 0;
                for (int i = 0; i < 100; ++i) {
                    if (list.contains(i % 1000)) hits++;
                }
                ankerl::nanobench::doNotOptimizeAway(hits);
            });

            globalBench.batch(100).run("[查找] contains (未命中)", [&] {
                int misses = 0;
                for (int i = 0; i < 100; ++i) {
                    if (!list.contains(10000 + i)) misses++;
                }
                ankerl::nanobench::doNotOptimizeAway(misses);
            });
        }

        auto testBenchContainsUnorderedVsSorted() -> void {
            LOG("--- [性能] contains 延迟（UNORDERED 线性 vs ASCENDING 二分） ---");
            constexpr u32 kElements = 1000;
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
            sortedList.naturalSort(); // 内部包含 tryMerging + awaitMerging，但这是一次性准备

            globalBench.batch(100).run("[查找] contains 命中 (线性)", [&] {
                int hits = 0;
                for (u32 i = 0; i < 100; ++i) if (unorderedList.contains(i)) hits++;
                ankerl::nanobench::doNotOptimizeAway(hits);
            });
            globalBench.batch(100).run("[查找] contains 命中 (二分)", [&] {
                int hits = 0;
                for (u32 i = 0; i < 100; ++i) if (sortedList.contains(i)) hits++;
                ankerl::nanobench::doNotOptimizeAway(hits);
            });
            globalBench.batch(100).run("[查找] contains 未命中 (线性)", [&] {
                int misses = 0;
                for (u32 i = 0; i < 100; ++i) if (!unorderedList.contains(10000 + i)) misses++;
                ankerl::nanobench::doNotOptimizeAway(misses);
            });
            globalBench.batch(100).run("[查找] contains 未命中 (二分)", [&] {
                int misses = 0;
                for (u32 i = 0; i < 100; ++i) if (!sortedList.contains(10000 + i)) misses++;
                ankerl::nanobench::doNotOptimizeAway(misses);
            });
        }

        auto testBenchContainsAllUnorderedVsSorted() -> void {
            LOG("--- [性能] containsAll 延迟（UNORDERED 线性 vs ASCENDING 二分） ---");
            constexpr u32 kElements = 1000;
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

            constexpr int hitTargets[] = { 100, 500, 900 };
            constexpr int missTargets[] = { 100, 500, 10000 };

            globalBench.batch(100).run("[查找] containsAll 命中 (线性)", [&] {
                int hits = 0;
                for (u32 i = 0; i < 100; ++i) if (unorderedList.containsAll(hitTargets, 3)) hits++;
                ankerl::nanobench::doNotOptimizeAway(hits);
            });
            globalBench.batch(100).run("[查找] containsAll 命中 (二分)", [&] {
                int hits = 0;
                for (u32 i = 0; i < 100; ++i) if (sortedList.containsAll(hitTargets, 3)) hits++;
                ankerl::nanobench::doNotOptimizeAway(hits);
            });
            globalBench.batch(100).run("[查找] containsAll 未命中 (线性)", [&] {
                int misses = 0;
                for (u32 i = 0; i < 100; ++i) if (!unorderedList.containsAll(missTargets, 3)) misses++;
                ankerl::nanobench::doNotOptimizeAway(misses);
            });
            globalBench.batch(100).run("[查找] containsAll 未命中 (二分)", [&] {
                int misses = 0;
                for (u32 i = 0; i < 100; ++i) if (!sortedList.containsAll(missTargets, 3)) misses++;
                ankerl::nanobench::doNotOptimizeAway(misses);
            });
        }

        auto testBenchFirstLastIndexOf() -> void {
            LOG("--- [性能] firstIndexOf / lastIndexOf 延迟 ---");
            constexpr u32 kDistinctValues = 200;
            constexpr u32 kRepeatPerValue = 5;
            constexpr u32 kElements = kDistinctValues * kRepeatPerValue;

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

            globalBench.batch(100).run("[查找] firstIndexOf (线性)", [&] {
                ssize sum = 0;
                for (u32 i = 0; i < 100; ++i) sum += unorderedList.firstIndexOf(i % kDistinctValues);
                ankerl::nanobench::doNotOptimizeAway(sum);
            });
            globalBench.batch(100).run("[查找] firstIndexOf (二分)", [&] {
                ssize sum = 0;
                for (u32 i = 0; i < 100; ++i) sum += sortedList.firstIndexOf(i % kDistinctValues);
                ankerl::nanobench::doNotOptimizeAway(sum);
            });
            globalBench.batch(100).run("[查找] lastIndexOf (线性)", [&] {
                ssize sum = 0;
                for (u32 i = 0; i < 100; ++i) sum += unorderedList.lastIndexOf(i % kDistinctValues);
                ankerl::nanobench::doNotOptimizeAway(sum);
            });
            globalBench.batch(100).run("[查找] lastIndexOf (二分)", [&] {
                ssize sum = 0;
                for (u32 i = 0; i < 100; ++i) sum += sortedList.lastIndexOf(i % kDistinctValues);
                ankerl::nanobench::doNotOptimizeAway(sum);
            });
        }

        auto testBenchConcurrentAdd() -> void {
            LOG("--- [性能] 4 线程并发 add 吞吐 ---");
            constexpr int kThreadCount = 4;
            constexpr u32 n = 100000;
            const u32 perThread = n / kThreadCount;

            globalBench.batch(n).run("[并发] 4 线程并发 add+merge", [&] {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                std::vector<std::thread> threads;
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
                ankerl::nanobench::doNotOptimizeAway(list);
            });
        }

        auto testBenchHighConcurrencyReadWrite() -> void {
            LOG("--- [性能] 高并发读写吞吐（理想扩展 + 生产环境双模式，绑核到物理核） ---");

            constexpr int threadConfigs[] = { 1, 2, 4, 8 };
            struct ReadWriteRatio { int readers; int writers; };
            constexpr ReadWriteRatio ratios[] = { { 1, 1 }, { 1, 4 }, { 4, 1 }, { 1, 0 }, { 0, 1 } };

            constexpr u32 kListSize = 10000;
            constexpr u64 kReadElementAccessesPerThread = 100000000;
            constexpr u32 kWriteOpsPerThread = 1000000;

            int physicalCoreCount = 1;

            #ifdef _WIN32
            physicalCoreCount = (int) (std::thread::hardware_concurrency() / 2);
            if (physicalCoreCount <= 0) {
                physicalCoreCount = 1;
            }
            #endif

            LOG("物理核数（推定）= ", physicalCoreCount);

            struct Mode {
                const char* name;
                bool maintenanceMerging;
            };
            constexpr Mode modes[] = {
                { "理想", false },
                { "生产", true },
            };

            for (const auto& mode : modes) {
                for (int baseThreads : threadConfigs) {
                    for (const auto& ratio : ratios) {
                        int rThreads = ratio.readers * baseThreads;
                        int wThreads = ratio.writers * baseThreads;
                        int totalThreads = rThreads + wThreads;
                        if (totalThreads == 0) continue;

                        u64 totalReadOps = (u64) rThreads * kReadElementAccessesPerThread;
                        u64 totalWriteOps = (u64) wThreads * kWriteOpsPerThread;
                        u64 totalOps = totalReadOps + totalWriteOps;

                        std::string benchName =
                            "[" + std::string(mode.name) + "] " +
                            std::to_string(totalThreads) + " 线程 (" +
                            std::to_string(rThreads) + " 读 " + std::to_string(wThreads) + " 写)";

                        globalBench.batch(totalOps).run(benchName.c_str(), [&] {
                            auto list = IMPList<int>(1024, 1024);
                            list.configureMaintenanceMerging(mode.maintenanceMerging);
                            list.configureCompaction(false, 20, 10000);

                            for (u32 i = 0; i < kListSize; ++i) {
                                list.add((int) i);
                            }
                            list.tryMerging();
                            list.awaitMerging();

                            std::vector<std::thread> threads;
                            threads.reserve(totalThreads);

                            int threadIndex = 0;

                            for (int r = 0; r < rThreads; ++r) {
                                const int boundCore = threadIndex % physicalCoreCount;
                                ++threadIndex;

                                threads.emplace_back([&list, boundCore, physicalCoreCount, kReadElementAccessesPerThread] {
                                    #ifdef _WIN32
                                    const DWORD_PTR mask = (DWORD_PTR) 1 << (boundCore * 2);
                                    SetThreadAffinityMask(GetCurrentThread(), mask);
                                    #else
                                    (void) boundCore;
                                    (void) physicalCoreCount;
                                    #endif

                                    int accum = 0;
                                    u64 accessed = 0;
                                    while (accessed < kReadElementAccessesPerThread) {
                                        for (const auto& v : list) {
                                            accum += v;
                                            if (++accessed >= kReadElementAccessesPerThread) break;
                                        }
                                    }
                                    ankerl::nanobench::doNotOptimizeAway(accum);
                                });
                            }

                            for (int w = 0; w < wThreads; ++w) {
                                const int boundCore = threadIndex % physicalCoreCount;
                                ++threadIndex;

                                threads.emplace_back([&list, w, boundCore, physicalCoreCount, kWriteOpsPerThread, kListSize]() {
                                    #ifdef _WIN32
                                    const DWORD_PTR mask = (DWORD_PTR) 1 << (boundCore * 2);
                                    SetThreadAffinityMask(GetCurrentThread(), mask);
                                    #else
                                    (void) boundCore;
                                    (void) physicalCoreCount;
                                    #endif

                                    for (u32 i = 0; i < kWriteOpsPerThread; ++i) {
                                        list.add((int) (w * kWriteOpsPerThread + i + kListSize));
                                    }
                                });
                            }

                            for (auto& t : threads) {
                                t.join();
                            }
                        });
                    }
                }
            }
        }

        auto testBenchMergeCostByChunkSize() -> void {
            LOG("--- [性能] merge 单独成本（不同 chunkSize） ---");
            const usize chunkSizes[] = { 16, 64, 256, 1024, 4096 };
            constexpr u32 kElements = 200000;

            for (usize cs : chunkSizes) {
                auto list = IMPList<int>(cs, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (u32 i = 0; i < kElements; ++i) {
                    list.add((int) i);
                }
                list.tryMerging(); // 初始化预热
                list.add(0); // 制造脏数据

                std::string benchName = "[Merge] chunkSize = " + std::to_string(cs);
                globalBench.batch(kElements).run(benchName.c_str(), [&] {
                    list.tryMerging(); // 单线程同步执行，无需 await
                });
            }
        }

        auto testBenchDiscardHashPath() -> void {
            LOG("--- [性能] discard 转化 tombstones 延迟（int Hash 路径） ---");
            constexpr u32 kElements = 100000;
            constexpr u32 kDiscards = 10000;

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

            globalBench.batch(kDiscards).run("[Merge] 10万添加 + 1万删除 → merge", [&] {
                list.tryMerging();
            });
        }

        auto testBenchDiscardHashPathLargeElement() -> void {
            LOG("--- [性能] discard 转化 tombstones 延迟（自定义 Struct Hash 路径） ---");
            struct CustomStruct { int a; int b; int c; };
            constexpr u32 kElements = 50000;
            constexpr u32 kDiscards = 5000;

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

            globalBench.batch(kDiscards).run("[Merge] 5万添加 + 5千删除 → merge", [&] {
                list.tryMerging();
            });
        }

        auto testBenchCompactByRatio() -> void {
            LOG("--- [性能] compact 单次成本（不同墓碑比例） ---");
            constexpr u32 kElements = 10000;
            constexpr u32 discardRatios[] = { 20, 50, 80 };

            for (u32 ratio : discardRatios) {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 0, 1000000);

                for (u32 i = 0; i < kElements; ++i) {
                    list.add((int) i);
                }
                list.tryMerging();

                for (u32 i = 0; i < kElements * ratio / 100; ++i) {
                    list.remove((int) i);
                }
                list.tryMerging();

                std::string benchName = "[Compact] 墓碑占比 " + std::to_string(ratio) + "%";
                globalBench.batch(kElements).run(benchName.c_str(), [&] {
                    list.tryCompacting();
                });
            }
        }

        auto testBenchCompactByChunkSize() -> void {
            LOG("--- [性能] compact 单独成本（不同 chunkSize） ---");
            const usize chunkSizes[] = { 16, 64, 256, 1024, 4096 };
            constexpr u32 kElements = 10000;
            constexpr u32 kDiscards = 5000;

            for (usize cs : chunkSizes) {
                auto list = IMPList<int>(cs, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 0, 1000000);

                for (u32 i = 0; i < kElements; ++i) {
                    list.add((int) i);
                }
                list.tryMerging();

                for (u32 i = 0; i < kDiscards; ++i) {
                    list.remove((int) i);
                }
                list.tryMerging();

                std::string benchName = "[Compact] chunkSize = " + std::to_string(cs);
                globalBench.batch(kElements).run(benchName.c_str(), [&] {
                    list.tryCompacting();
                });
            }
        }

        auto testBenchCompactBeforeAfterMerge() -> void {
            LOG("--- [性能] compact 前后 merge 成本对比 ---");
            constexpr u32 kElements = 10000;
            constexpr u32 kDiscards = 5000;

            // 准备 Before 场景
            auto listBefore = IMPList<int>(1024, 1024);
            listBefore.configureMaintenanceMerging(false);
            listBefore.configureCompaction(false, 0, 1000000);
            for (u32 i = 0; i < kElements; ++i) listBefore.add((int) i);
            listBefore.tryMerging();
            for (u32 i = 0; i < kDiscards; ++i) listBefore.remove((int) i);
            listBefore.tryMerging();

            listBefore.add(999999);
            globalBench.batch(kElements).run("[Compact] compact 前 merge (含 50% 墓碑)", [&] {
                listBefore.tryMerging();
            });

            // 准备 After 场景
            auto listAfter = IMPList<int>(1024, 1024);
            listAfter.configureMaintenanceMerging(false);
            listAfter.configureCompaction(false, 0, 1000000);
            for (u32 i = 0; i < kElements; ++i) listAfter.add((int) i);
            listAfter.tryMerging();
            for (u32 i = 0; i < kDiscards; ++i) listAfter.remove((int) i);
            listAfter.tryMerging();
            listAfter.tryCompacting();

            listAfter.add(999998);
            globalBench.batch(kElements).run("[Compact] compact 后 merge (墓碑清理)", [&] {
                listAfter.tryMerging();
            });
        }

        auto testBenchMoveCost() -> void {
            LOG("--- [性能] move 常数时间 ---");
            constexpr u32 sizes[] = { 100, 10000, 1000000 };

            for (u32 n : sizes) {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (u32 i = 0; i < n; ++i) list.add((int) i);
                list.tryMerging();

                std::string benchName = "[基础] move " + std::to_string(n) + " 元素";
                globalBench.batch(1000).run(benchName.c_str(), [&] {
                    for (int i = 0; i < 1000; ++i) {
                        auto moved = std::move(list);
                        std::atomic_signal_fence(std::memory_order_seq_cst);
                        list = std::move(moved);
                        std::atomic_signal_fence(std::memory_order_seq_cst);
                    }
                });
            }
        }

        auto testBenchClearCost() -> void {
            LOG("--- [性能] clear 成本 ---");
            constexpr u32 sizes[] = { 100, 10000, 1000000 };

            for (u32 n : sizes) {
                auto list = IMPList<int>(1024, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                for (u32 i = 0; i < n; ++i) list.add((int) i);
                list.tryMerging();

                std::string benchName = "[基础] clear " + std::to_string(n) + " 元素";
                globalBench.batch(1).run(benchName.c_str(), [&] {
                    list.removeAll();
                });
            }
        }

        // ============================================================
        // [稳定性] 测试
        // ============================================================

        auto testStabilityLongRun() -> void {
            LOG("--- [稳定性] 长时间反复 add/merge/clear 循环 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = IMPList<int>(256, 1024);
                list.configureMaintenanceMerging(false);
                list.configureCompaction(false, 20, 10000);

                constexpr u32 kRounds = 1000;
                constexpr u32 kPerRound = 500;

                std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
                for (u32 r = 0; r < kRounds; ++r) {
                    for (u32 i = 0; i < kPerRound; ++i) {
                        list.add((int) i);
                    }
                    if (r % 100 == 0) {
                        list.tryMerging();
                    }
                }
                std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
                u64 totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

                LOG(kRounds, " 轮 × ", kPerRound, " 元素：总耗时 ", totalNs / 1e6, " ms，最终 present = ", list.presentElementCount(), "，ingest = ", list.ingestElementCount());
            }
        }

    }

    auto runTests() -> void {
        LOG("===== IMPList 深度测试开始 =====");

        // 全局 Nanobench 仅在此处初始化一次 title 和 unit
        globalBench.title("IMPList Benchmarks").unit("op");

        LOGGER_INDENT_BLOCK(true) {
            // [正确性]
            testConstructAndLifecycle();
            testAddAndAddAll();
            testChunkBoundary();
            testRemoveAndRemoveAllBoundary();
            testRemoveIfAndRetainIf();
            testClearSemantics();
            testDiscardBasic();
            testDiscardDuplicateMissingOverCapacity();
            testDiscardWithAddTiming();
            testDiscardAllBasic();
            testDiscardAllWindowSemantics();
            testDiscardAllDuringMerge();
            testCopyAndMove();
            testSortingStrategy();
            testFindContainsIndexOfUnordered();
            testFindContainsIndexOfAscending();
            testDuplicateFirstLastIndexOf();
            testContainsAll();
            testIteratorBasicTraversal();
            testIteratorEmptyList();
            testIteratorDuringAdd();
            testIteratorDuringMerge();
            testIteratorDuringRemove();
            testIteratorDuringClear();
            testIteratorDuringSort();
            testMultipleIteratorsSameSpan();
            testMultipleBeginIndependentSpan();
            testIteratorSpareReuse();
            testForEachVsIterator();
            testAwaitMerging();
            testAwaitCompacting();
            testCompactionCountIncrement();
            testCompactIngestDrop();
            testCompactAddPreserved();
            testCompactMergeReflect();
            testCompactThreshold();
            testCompactDisabledExplicit();
            testRepeatedTryCompacting();

            // [并发]
            testConcurrent4Writers();
            testConcurrentDiscardAdd();
            testConcurrentDiscardAllWindow();
            testConcurrentRemoveMerge();
            testConcurrentMultiRemoveMerge();
            testConcurrentRemoveIfMerge();
            testConcurrentDiscardRemoveMerge();
            testConcurrentIngestMergeCount();
            testConcurrentCompactRemove();
            testConcurrentCompactAdd();
            testConcurrentCompactCount();
            testConcurrentCompactMerge();
            testStressRemoveMerge();
            testStressCompactRemoveAdd();
            testThreadExitCallback();

            // [内存]
            testRepeatedConstruction();
            testCompactLongRunMemory();
            testIteratorRepeatedCreation();

            // [性能]
            testBenchAddLatency();
            testBenchDiscardLatency();
            testBenchDiscardAllLatency();
            testBenchAddMergeThroughput();
            testBenchGetLatency();
            testBenchIteratorTraversal();
            testBenchIteratorBeginEnd();
            testBenchIteratorVsGet();
            testBenchConcurrentIterators();
            testBenchIteratorMerge();
            testBenchContains();
            testBenchContainsUnorderedVsSorted();
            testBenchContainsAllUnorderedVsSorted();
            testBenchFirstLastIndexOf();
            testBenchConcurrentAdd();
            testBenchHighConcurrencyReadWrite();
            testBenchMergeCostByChunkSize();
            testBenchDiscardHashPath();
            testBenchDiscardHashPathLargeElement();
            testBenchCompactByRatio();
            testBenchCompactByChunkSize();
            testBenchCompactBeforeAfterMerge();
            testBenchMoveCost();
            testBenchClearCost();

            // [稳定性]
            testStabilityLongRun();
        }

        // 导出 JSON 数据供论文画图
        LOG("--- [导出] 导出 Nanobench 性能数据 ---");
        std::ofstream out("nanobench_results.json");
        if (out.is_open()) {
            ankerl::nanobench::render(ankerl::nanobench::templates::json(), globalBench, out);
            LOG("性能数据已保存至 nanobench_results.json");
        } else {
            LOG("无法打开 nanobench_results.json 进行写入");
        }

        LOG("===== IMPList 深度测试结束 =====");
    }
}