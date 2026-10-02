#pragma once
#include "../../../../util/Logger.hpp"

#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

#include "../../../../../include/catelier/foundation/concurrency/ThreadLocal.hpp"

namespace Catelier::test::demo::foundation::concurrency {
    typedef struct SumData {
        int total;
        int count;
    } SumData;

    inline auto testInitFunc(void* userData) -> void* {
        auto* value = (int*) malloc(sizeof(int));
        *value = 0;

        return value;
    }
    inline auto testOnThreadExit(void* value, void* userData) -> void {
        if (value) {
            free(value);
        }
    }
    inline auto testOnDestroy(void* value, void* userData) -> void {
        if (value) {
            free(value);
        }
    }

    static auto demoThreadLocal() -> void {
        using namespace Catelier::foundation::concurrency;

        LOG("===== ThreadLocal 演示开始 =====");
        LOGGER_INDENT_BLOCK(true) {
            LOG("--- 构造与基础状态 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                LOG("构造后：isValid = ", threadLocal.isValid(), "，entryCount = ", threadLocal.entryCount());
                LOG("get() 返回值 = ", threadLocal.get(), "（应该为 0）");
            }

            LOG("--- 单线程 getOrCreate ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);

                auto* value = (int*) threadLocal.getOrCreate();
                *value = 42;

                auto* value2 = (int*) threadLocal.getOrCreate();
                LOG("两次 getOrCreate 返回同一指针：", value == value2, "（应该为 1）");
                LOG("getOrCreate 后 *value = ", *value2, "（应该为 42）");
                LOG("entryCount = ", threadLocal.entryCount(), "（应该为 1）");

                auto* getValue = (int*) threadLocal.get();
                LOG("get() 返回非 null：", getValue != nullptr, "（应该为 1）");
                LOG("get() 后 *getValue = ", *getValue, "（应该为 42）");
            }

            LOG("--- get 无副作用 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);

                auto* value = (int*) threadLocal.get();
                LOG("未调用 getOrCreate 时 get() 返回值 = ", value, "（应该为 0）");
                LOG("此时 entryCount = ", threadLocal.entryCount(), "（应该为 0）");

                threadLocal.getOrCreate();
                LOG("调用 getOrCreate 后 entryCount = ", threadLocal.entryCount(), "（应该为 1）");
            }

            LOG("--- 多线程隔离 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                std::atomic<int> workerCounter = { 0 };
                std::atomic<bool> releaseWorkers = { false };

                constexpr int THREAD_COUNT = 4;
                std::vector<std::thread> threads;

                for (int i = 0; i < THREAD_COUNT; ++i) {
                    threads.emplace_back([&threadLocal, &workerCounter, &releaseWorkers, i]() {
                        auto* value = (int*) threadLocal.getOrCreate();
                        *value = (i + 1) * 100;
                        workerCounter.fetch_add(1);

                        while (!releaseWorkers.load()) {
                            std::this_thread::yield();
                        }
                    });
                }

                while (workerCounter.load() < THREAD_COUNT) {
                    std::this_thread::yield();
                }

                LOG("所有线程就绪后 workerCounter = ", workerCounter.load(), "（应该为 ", THREAD_COUNT, "）");
                LOG("此时 instance entryCount = ", threadLocal.entryCount(), "（应该为 ", THREAD_COUNT, "）");

                releaseWorkers.store(true);

                for (auto& thread : threads) {
                    thread.join();
                }

                LOG("线程退出后 instance entryCount = ", threadLocal.entryCount(), "（应该为 0）");
            }

            LOG("--- 多线程同一实例独立值 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                std::atomic<int> successCount = { 0 };

                constexpr int THREAD_COUNT = 4;
                std::vector<std::thread> threads;

                for (int i = 0; i < THREAD_COUNT; ++i) {
                    threads.emplace_back([&threadLocal, &successCount, i]() {
                        auto* value = (int*) threadLocal.getOrCreate();
                        *value = (i + 1) * 10;

                        auto* valueAgain = (int*) threadLocal.getOrCreate();
                        if (value == valueAgain && *valueAgain == (i + 1) * 10) {
                            successCount.fetch_add(1);
                        }
                    });
                }

                for (auto& thread : threads) {
                    thread.join();
                }

                LOG("每个线程 getOrCreate 两次后返回相同指针且值正确：", successCount.load(), " / ", THREAD_COUNT);
            }

            LOG("--- forEach 遍历 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                std::atomic<int> workerCounter = { 0 };
                std::atomic<bool> releaseWorkers = { false };

                constexpr int THREAD_COUNT = 3;
                std::vector<std::thread> threads;

                for (int i = 0; i < THREAD_COUNT; ++i) {
                    threads.emplace_back([&threadLocal, &workerCounter, &releaseWorkers, i]() {
                        auto* value = (int*) threadLocal.getOrCreate();
                        *value = (i + 1) * 10;
                        workerCounter.fetch_add(1);

                        while (!releaseWorkers.load()) {
                            std::this_thread::yield();
                        }
                    });
                }

                while (workerCounter.load() < THREAD_COUNT) {
                    std::this_thread::yield();
                }

                auto* mainValue = (int*) threadLocal.getOrCreate();
                *mainValue = 999;

                auto sumData = SumData { 0, 0 };
                threadLocal.forEach([](void* value, void* userData) {
                    auto* data = (SumData*) userData;
                    data->total += *((int*) value);
                    data->count++;
                }, &sumData);

                LOG("forEach 遍历 count = ", sumData.count, "（应该为 ", THREAD_COUNT + 1, "）");
                LOG("forEach 遍历 total = ", sumData.total, "（应该为 10 + 20 + 30 + 999 = 1059）");

                releaseWorkers.store(true);

                for (auto& thread : threads) {
                    thread.join();
                }
            }

            LOG("--- copy 生命周期 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto source = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto* sourceValue = (int*) source.getOrCreate();
                *sourceValue = 555;

                auto copied = source;
                LOG("copy 后：");
                LOG("source.isValid() = ", source.isValid(), "（应该为 1）");
                LOG("copied.isValid() = ", copied.isValid(), "（应该为 1）");
                LOG("source.entryCount() = ", source.entryCount(), "（应该为 1）");
                LOG("copied.entryCount() = ", copied.entryCount(), "（应该为 0）");

                auto* copiedValue = (int*) copied.get();
                LOG("copied.get() 返回 null：", copiedValue == nullptr, "（应该为 1）");

                auto* copiedCreated = (int*) copied.getOrCreate();
                LOG("copied.getOrCreate() 返回非 null：", copiedCreated != nullptr, "（应该为 1）");
                *copiedCreated = 666;

                auto* sourceAgain = (int*) source.getOrCreate();
                LOG("source 值不受影响：", *sourceAgain, "（应该为 555）");
                LOG("copied 值独立：", *copiedCreated, "（应该为 666）");
                LOG("source.entryCount() = ", source.entryCount(), "（应该为 1）");
                LOG("copied.entryCount() = ", copied.entryCount(), "（应该为 1）");
            }

            LOG("--- copy 赋值 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto source = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto* sourceValue = (int*) source.getOrCreate();
                *sourceValue = 111;

                auto target = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto* targetValue = (int*) target.getOrCreate();
                *targetValue = 222;

                target = source;
                LOG("copy 赋值后：");
                LOG("target.isValid() = ", target.isValid(), "（应该为 1）");
                LOG("target.entryCount() = ", target.entryCount(), "（应该为 0，旧的 entry 已被销毁）");
                LOG("source.entryCount() = ", source.entryCount(), "（应该为 1）");

                auto* targetCreated = (int*) target.getOrCreate();
                *targetCreated = 333;

                auto* sourceAgain = (int*) source.getOrCreate();
                LOG("source 值不受影响：", *sourceAgain, "（应该为 111）");
                LOG("target 值独立：", *targetCreated, "（应该为 333）");
            }

            LOG("--- move 生命周期 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto source = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto* sourceValue = (int*) source.getOrCreate();
                *sourceValue = 777;

                auto moved = std::move(source);
                LOG("move 后：");
                LOG("source.isValid() = ", source.isValid(), "（应该为 0）");
                LOG("moved.isValid() = ", moved.isValid(), "（应该为 1）");
                LOG("source.entryCount() = ", source.entryCount(), "（应该为 0）");
                LOG("moved.entryCount() = ", moved.entryCount(), "（应该为 1）");

                auto* movedValue = (int*) moved.get();
                LOG("moved.get() 返回非 null：", movedValue != nullptr, "（应该为 1）");
                LOG("moved 值 = ", *movedValue, "（应该为 777）");

                auto* movedAgain = (int*) moved.getOrCreate();
                LOG("moved.getOrCreate() 与 moved.get() 同指针：", movedValue == movedAgain, "（应该为 1）");
            }

            LOG("--- move 赋值 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto source = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto* sourceValue = (int*) source.getOrCreate();
                *sourceValue = 888;

                auto target = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto* targetValue = (int*) target.getOrCreate();
                *targetValue = 999;

                target = std::move(source);
                LOG("move 赋值后：");
                LOG("source.isValid() = ", source.isValid(), "（应该为 0）");
                LOG("target.isValid() = ", target.isValid(), "（应该为 1）");
                LOG("target.entryCount() = ", target.entryCount(), "（应该为 1，source 的 entry 已转移）");

                auto* targetValue2 = (int*) target.get();
                LOG("target.get() 值 = ", *targetValue2, "（应该为 888）");

                auto* targetAgain = (int*) target.getOrCreate();
                LOG("target.getOrCreate() 与 target.get() 同指针：", targetValue2 == targetAgain, "（应该为 1）");
            }

            LOG("--- 性能：缓存命中路径（连续 getOrCreate） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                threadLocal.getOrCreate();

                constexpr int ITERATIONS = 10000000;

                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < ITERATIONS; ++i) {
                    auto* value = (int*) threadLocal.getOrCreate();
                    *value += 1;
                }
                const auto end = std::chrono::steady_clock::now();

                const auto totalNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                const auto nanosecondsPerOperation = totalNanoseconds / ITERATIONS;

                LOG(ITERATIONS, " 次连续 getOrCreate：总耗时 ", totalNanoseconds, " ns，平均 ", nanosecondsPerOperation, " ns/op");
            }

            LOG("--- 性能：缓存未命中路径（两个实例交替） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocalA = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto threadLocalB = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);

                threadLocalA.getOrCreate();
                threadLocalB.getOrCreate();

                constexpr int ROUNDS = 5000000;

                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < ROUNDS; ++i) {
                    auto* valueA = (int*) threadLocalA.getOrCreate();
                    auto* valueB = (int*) threadLocalB.getOrCreate();
                    *valueA += 1;
                    *valueB += 1;
                }
                const auto end = std::chrono::steady_clock::now();

                const auto totalNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                const auto nanosecondsPerOperation = totalNanoseconds / (ROUNDS * 2);

                LOG(ROUNDS, " 轮交替 getOrCreate（共 ", ROUNDS * 2, " 次）：总耗时 ", totalNanoseconds, " ns，平均 ", nanosecondsPerOperation, " ns/op");
            }

            LOG("--- 性能：get 路径（缓存命中） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                threadLocal.getOrCreate();

                constexpr int ITERATIONS = 10000000;

                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < ITERATIONS; ++i) {
                    auto* value = (int*) threadLocal.get();
                    *value += 1;
                }
                const auto end = std::chrono::steady_clock::now();

                const auto totalNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                const auto nanosecondsPerOperation = totalNanoseconds / ITERATIONS;

                LOG(ITERATIONS, " 次连续 get：总耗时 ", totalNanoseconds, " ns，平均 ", nanosecondsPerOperation, " ns/op");
            }

            LOG("--- 性能：get 路径（缓存未命中，扫描桶） ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocalA = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                auto threadLocalB = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);

                threadLocalA.getOrCreate();
                threadLocalB.getOrCreate();

                constexpr int ROUNDS = 5000000;

                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < ROUNDS; ++i) {
                    auto* valueA = (int*) threadLocalA.get();
                    auto* valueB = (int*) threadLocalB.get();
                    *valueA += 1;
                    *valueB += 1;
                }
                const auto end = std::chrono::steady_clock::now();

                const auto totalNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                const auto nanosecondsPerOperation = totalNanoseconds / (ROUNDS * 2);

                LOG(ROUNDS, " 轮交替 get（共 ", ROUNDS * 2, " 次）：总耗时 ", totalNanoseconds, " ns，平均 ", nanosecondsPerOperation, " ns/op");
            }

            LOG("--- 性能：首次 getOrCreate（含 entry 分配） ---");
            LOGGER_INDENT_BLOCK(true) {
                constexpr int ITERATIONS = 10000;

                const auto start = std::chrono::steady_clock::now();
                for (int i = 0; i < ITERATIONS; ++i) {
                    auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                    threadLocal.getOrCreate();
                }
                const auto end = std::chrono::steady_clock::now();

                const auto totalNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                const auto nanosecondsPerOperation = totalNanoseconds / ITERATIONS;

                LOG(ITERATIONS, " 次构造 + 首次 getOrCreate + 析构：总耗时 ", totalNanoseconds, " ns，平均 ", nanosecondsPerOperation, " ns/op");
            }

            LOG("--- 性能：entryCount 查询 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);
                threadLocal.getOrCreate();

                constexpr int ITERATIONS = 10000000;

                const auto start = std::chrono::steady_clock::now();
                usize total = 0;
                for (int i = 0; i < ITERATIONS; ++i) {
                    total += threadLocal.entryCount();
                }
                const auto end = std::chrono::steady_clock::now();

                const auto totalNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                const auto nanosecondsPerOperation = totalNanoseconds / ITERATIONS;

                LOG(ITERATIONS, " 次 entryCount：总耗时 ", totalNanoseconds, " ns，平均 ", nanosecondsPerOperation, " ns/op");
                LOG("累加结果（防止优化）：", total);
            }

            LOG("--- 生命周期：线程退出清理 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto threadLocal = ThreadLocal(testInitFunc, testOnThreadExit, testOnDestroy, nullptr);

                std::thread worker([&threadLocal]() {
                    auto* value = (int*) threadLocal.getOrCreate();
                    *value = 777;
                });
                worker.join();

                LOG("worker 线程已退出，instance entryCount = ", threadLocal.entryCount(), "（应该为 0，因为 worker 退出时已清理）");
            }
        }
        LOG("===== ThreadLocal 演示结束 =====");
    }
}