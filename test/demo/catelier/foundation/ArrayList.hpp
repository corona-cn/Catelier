#pragma once
#include "../../../util/Logger.hpp"

#include <utility>

#include "../../../../include/catelier/foundation/ArrayList.hpp"

namespace Catelier::test::demo::foundation {
    static auto demoArrayList() -> void {
        using namespace Catelier::foundation;

        LOG("===== ArrayList 演示开始 =====");
        LOGGER_INDENT_BLOCK(true) {
            LOG("--- 构造与基础状态 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                LOG("构造后：size = ", list.size(), "，capacity = ", list.capacity(), "，isEmpty = ", list.isEmpty());

                list.push(10);
                list.push(20);
                list.push(30);
                LOG("push 三个元素后：size = ", list.size(), "，capacity = ", list.capacity());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], "]");
            }

            LOG("--- push / pop ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.push(i * 10);
                }
                LOG("push 1..5 后：size = ", list.size(), "，capacity = ", list.capacity());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], ", ", list[3], ", ", list[4], "]");

                list.pop();
                list.pop();
                LOG("pop 两次后：size = ", list.size(), "，capacity = ", list.capacity());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], "]");
            }

            LOG("--- insertAt / removeAt ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                list.push(10);
                list.push(20);
                list.push(30);
                LOG("初始 list = [", list[0], ", ", list[1], ", ", list[2], "]");

                list.insertAt(1, 15);
                LOG("insertAt(1, 15) 后：size = ", list.size());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], ", ", list[3], "]");

                list.insertAt(0, 5);
                LOG("insertAt(0, 5) 后：size = ", list.size());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], ", ", list[3], ", ", list[4], "]");

                list.insertAt(list.size(), 99);
                LOG("insertAt(size, 99) 后（等价 push）：size = ", list.size());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], ", ", list[3], ", ", list[4], ", ", list[5], "]");

                list.removeAt(0);
                LOG("removeAt(0) 后：size = ", list.size());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], ", ", list[3], ", ", list[4], "]");

                list.removeAt(list.size() - 1);
                LOG("removeAt(size - 1) 后：size = ", list.size());
                LOG("list = [", list[0], ", ", list[1], ", ", list[2], ", ", list[3], "]");
            }

            LOG("--- get / set ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                list.push(1);
                list.push(2);
                list.push(3);

                LOG("初始 list = [", list[0], ", ", list[1], ", ", list[2], "]");

                list.set(1, 200);
                LOG("set(1, 200) 后：list = [", list[0], ", ", list[1], ", ", list[2], "]");

                LOG("get(0) = ", list.get(0));
                LOG("get(2) = ", list.get(2));
            }

            LOG("--- 迭代器正向遍历 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.push(i);
                }

                LOG("正向遍历：");
                LOGGER_INDENT_BLOCK(true) {
                    for (auto it = list.begin(); it != list.end(); ++it) {
                        LOG(*it);
                    }
                }

                LOG("范围 for：");
                LOGGER_INDENT_BLOCK(true) {
                    for (const auto& value : list) {
                        LOG(value);
                    }
                }
            }

            LOG("--- 迭代器反向遍历 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.push(i);
                }

                LOG("反向遍历：");
                LOGGER_INDENT_BLOCK(true) {
                    for (auto it = list.end() - 1; it >= list.begin(); --it) {
                        LOG(*it);
                    }
                }
            }

            LOG("--- 迭代器随机访问 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.push(i * 10);
                }

                auto it = list.begin();
                LOG("it = begin()，*it = ", *it);

                it += 2;
                LOG("it += 2 后，*it = ", *it);

                it -= 1;
                LOG("it -= 1 后，*it = ", *it);

                LOG("it[1] = ", it[1]);
                LOG("it[2] = ", it[2]);

                const auto dist = list.end() - list.begin();
                LOG("list.end() - list.begin() = ", dist);
            }

            LOG("--- reserve / shrinkToFit ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                LOG("初始：size = ", list.size(), "，capacity = ", list.capacity());

                list.reserve(100);
                LOG("reserve(100) 后：size = ", list.size(), "，capacity = ", list.capacity());

                for (int i = 0; i < 10; ++i) {
                    list.push(i);
                }
                LOG("push 10 个元素后：size = ", list.size(), "，capacity = ", list.capacity());

                list.shrinkToFit();
                LOG("shrinkToFit() 后：size = ", list.size(), "，capacity = ", list.capacity());
            }

            LOG("--- clear ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = ArrayList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.push(i);
                }
                LOG("清空前：size = ", list.size(), "，capacity = ", list.capacity());

                list.clear();
                LOG("clear() 后：size = ", list.size(), "，capacity = ", list.capacity(), "，isEmpty = ", list.isEmpty());
            }

            LOG("--- copy ---");
            LOGGER_INDENT_BLOCK(true) {
                auto original = ArrayList<int>();
                for (int i = 1; i <= 5; ++i) {
                    original.push(i * 100);
                }
                LOG("original = [", original[0], ", ", original[1], ", ", original[2], ", ", original[3], ", ", original[4], "]");

                auto copied = original;
                LOG("copied = [", copied[0], ", ", copied[1], ", ", copied[2], ", ", copied[3], ", ", copied[4], "]");

                copied.set(0, 999);
                LOG("修改 copied[0] = 999 后：");
                LOG("original[0] = ", original[0], "（应该还是 100）");
                LOG("copied[0]   = ", copied[0], "（应该是 999）");
            }

            LOG("--- move ---");
            LOGGER_INDENT_BLOCK(true) {
                auto source = ArrayList<int>();
                for (int i = 1; i <= 5; ++i) {
                    source.push(i * 10);
                }
                LOG("source = [", source[0], ", ", source[1], ", ", source[2], ", ", source[3], ", ", source[4], "]");

                auto moved = std::move(source);
                LOG("move 后：");
                LOG("moved.size()  = ", moved.size());
                LOG("source.size() = ", source.size(), "（move 后应该为 0）");
            }

            LOG("--- 自定义类型 ---");
            LOGGER_INDENT_BLOCK(true) {
                struct Point {
                    int x;
                    int y;

                    explicit Point(const int x = 0, const int y = 0) : x(x), y(y) {}
                };

                auto points = ArrayList<Point>();
                points.push(Point(1, 2));
                points.push(Point(3, 4));
                points.push(Point(5, 6));

                LOG("points.size() = ", points.size());

                LOG("遍历所有 Point：");
                LOGGER_INDENT_BLOCK(true) {
                    for (const auto& p : points) {
                        LOG("(", p.x, ", ", p.y, ")");
                    }
                }
            }
        }
        LOG("===== ArrayList 演示结束 =====");
    }
}