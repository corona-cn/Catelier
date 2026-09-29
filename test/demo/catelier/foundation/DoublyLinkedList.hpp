#pragma once
#include "../../../util/Logger.hpp"

#include <utility>

#include "../../../../include/catelier/foundation/DoublyLinkedList.hpp"

namespace Catelier::test::demo::foundation {
    static auto demoDoublyLinkedList() -> void {
        using namespace Catelier::foundation;

        LOG("===== DoublyLinkedList 演示开始 =====");
        LOGGER_INDENT_BLOCK(true) {
            LOG("--- 构造与基础状态 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                LOG("构造后：size = ", list.size(), "，isEmpty = ", list.isEmpty());

                list.pushTail(10);
                list.pushTail(20);
                list.pushTail(30);
                LOG("pushTail 三个元素后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");
            }

            LOG("--- pushHead / pushTail ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                list.pushTail(20);
                list.pushTail(30);
                LOG("pushTail(20) / pushTail(30) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), "]");

                list.pushHead(10);
                LOG("pushHead(10) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");

                list.pushHead(5);
                LOG("pushHead(5) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), "]");
            }

            LOG("--- popHead / popTail ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.pushTail(i * 10);
                }
                LOG("初始 list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), ", ", *list.get(4), "]");

                list.popHead();
                LOG("popHead() 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), "]");

                list.popTail();
                LOG("popTail() 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");
            }

            LOG("--- insertAt / removeAt ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                list.pushTail(10);
                list.pushTail(20);
                list.pushTail(30);
                LOG("初始 list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");

                list.insertAt(1, 15);
                LOG("insertAt(1, 15) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), "]");

                list.insertAt(0, 5);
                LOG("insertAt(0, 5) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), ", ", *list.get(4), "]");

                list.insertAt(list.size(), 99);
                LOG("insertAt(size, 99) 后（等价 pushTail）：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), ", ", *list.get(4), ", ", *list.get(5), "]");

                list.removeAt(0);
                LOG("removeAt(0) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), ", ", *list.get(4), "]");

                list.removeAt(list.size() - 1);
                LOG("removeAt(size - 1) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), "]");
            }

            LOG("--- removeIf ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                list.pushTail(1);
                list.pushTail(2);
                list.pushTail(3);
                list.pushTail(2);
                list.pushTail(4);
                list.pushTail(2);
                LOG("初始 list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), ", ", *list.get(4), ", ", *list.get(5), "]");

                list.removeIf(2);
                LOG("removeIf(2) 后：size = ", list.size());
                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");

                list.removeIf(100);
                LOG("removeIf(100)（不存在的值）后：size = ", list.size());
            }

            LOG("--- get / set / head / tail ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                list.pushTail(1);
                list.pushTail(2);
                list.pushTail(3);

                LOG("初始 list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");

                list.set(1, 200);
                LOG("set(1, 200) 后：list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");

                LOG("get(0) = ", *list.get(0));
                LOG("get(2) = ", *list.get(2));
                LOG("head() = ", *list.head());
                LOG("tail() = ", *list.tail());
            }

            LOG("--- contains ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                list.pushTail(10);
                list.pushTail(20);
                list.pushTail(30);

                LOG("list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), "]");
                LOG("contains(20) = ", list.contains(20), "（应该是 1）");
                LOG("contains(99) = ", list.contains(99), "（应该是 0）");
            }

            LOG("--- 迭代器正向遍历 ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.pushTail(i);
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
                auto list = DoublyLinkedList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.pushTail(i);
                }

                LOG("反向遍历：");
                LOGGER_INDENT_BLOCK(true) {
                    for (auto it = list.reverseBegin(); it != list.reverseEnd(); ++it) {
                        LOG(*it);
                    }
                }
            }

            LOG("--- reverse ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.pushTail(i);
                }
                LOG("反转前 list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), ", ", *list.get(4), "]");

                list.reverse();
                LOG("reverse() 后：size = ", list.size());
                LOG("反转后 list = [", *list.get(0), ", ", *list.get(1), ", ", *list.get(2), ", ", *list.get(3), ", ", *list.get(4), "]");
                LOG("head() = ", *list.head(), "，tail() = ", *list.tail());
            }

            LOG("--- clear ---");
            LOGGER_INDENT_BLOCK(true) {
                auto list = DoublyLinkedList<int>();
                for (int i = 1; i <= 5; ++i) {
                    list.pushTail(i);
                }
                LOG("清空前：size = ", list.size());

                list.clear();
                LOG("clear() 后：size = ", list.size(), "，isEmpty = ", list.isEmpty());
            }

            LOG("--- copy ---");
            LOGGER_INDENT_BLOCK(true) {
                auto original = DoublyLinkedList<int>();
                for (int i = 1; i <= 5; ++i) {
                    original.pushTail(i * 100);
                }
                LOG("original = [", *original.get(0), ", ", *original.get(1), ", ", *original.get(2), ", ", *original.get(3), ", ", *original.get(4), "]");

                auto copied = original;
                LOG("copied = [", *copied.get(0), ", ", *copied.get(1), ", ", *copied.get(2), ", ", *copied.get(3), ", ", *copied.get(4), "]");

                copied.set(0, 999);
                LOG("修改 copied[0] = 999 后：");
                LOG("original[0] = ", *original.get(0), "（应该还是 100）");
                LOG("copied[0]   = ", *copied.get(0), "（应该是 999）");
            }

            LOG("--- move ---");
            LOGGER_INDENT_BLOCK(true) {
                auto source = DoublyLinkedList<int>();
                for (int i = 1; i <= 5; ++i) {
                    source.pushTail(i * 10);
                }
                LOG("source = [", *source.get(0), ", ", *source.get(1), ", ", *source.get(2), ", ", *source.get(3), ", ", *source.get(4), "]");

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

                auto points = DoublyLinkedList<Point>();
                points.pushTail(Point(1, 2));
                points.pushTail(Point(3, 4));
                points.pushTail(Point(5, 6));

                LOG("points.size() = ", points.size());

                LOG("正向遍历所有 Point：");
                LOGGER_INDENT_BLOCK(true) {
                    for (const auto& p : points) {
                        LOG("(", p.x, ", ", p.y, ")");
                    }
                }

                LOG("反向遍历所有 Point：");
                LOGGER_INDENT_BLOCK(true) {
                    for (auto it = points.reverseBegin(); it != points.reverseEnd(); ++it) {
                        LOG("(", it->x, ", ", it->y, ")");
                    }
                }
            }
        }
        LOG("===== DoublyLinkedList 演示结束 =====");
    }
}