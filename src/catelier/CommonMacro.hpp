#pragma once
#include <cstdlib>

/* === 指针管理宏 === */
/* 通用指针 */
#define PTR_FREE_AND_NULL(ptr) \
    do { \
        free(ptr); \
        ptr = nullptr; \
    } while (0)