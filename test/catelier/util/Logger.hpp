#pragma once
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cctype>

#include "../../../src/catelier/CommonPrimitives.hpp"

namespace Catelier::test::util {
    inline int& indentLevel() {
        static int level = 0;
        return level;
    }
    inline void indentPush() {
        ++indentLevel();
    }
    inline void indentPop() {
        if (indentLevel() > 0) {
            --indentLevel();
        }
    }

    inline const char* getRelativeFile(const char* file) {
        static char cacheKeys[64][256];
        static char cacheValues[64][256];
        static int cacheCount = 0;

        for (int i = 0; i < cacheCount; ++i) {
            if (strcmp(cacheKeys[i], file) == 0) {
                return cacheValues[i];
            }
        }

        char fileStr[256];
        char rootStr[256];

        strncpy(fileStr, file, sizeof(fileStr) - 1);
        fileStr[sizeof(fileStr) - 1] = '\0';
        strncpy(rootStr, PROJECT_SOURCE_DIR, sizeof(rootStr) - 1);
        rootStr[sizeof(rootStr) - 1] = '\0';

        for (int i = 0; fileStr[i] != '\0'; ++i) {
            if (fileStr[i] == '\\') {
                fileStr[i] = '/';
            }
        }
        for (int i = 0; rootStr[i] != '\0'; ++i) {
            if (rootStr[i] == '\\') {
                rootStr[i] = '/';
            }
        }

        const usize fileLen = strlen(fileStr);
        const usize rootLen = strlen(rootStr);

        char fileLower[256];
        char rootLower[256];
        for (usize i = 0; i <= fileLen; ++i) {
            fileLower[i] = (char) tolower((uchar) fileStr[i]);
        }
        for (usize i = 0; i <= rootLen; ++i) {
            rootLower[i] = (char) tolower((uchar) rootStr[i]);
        }

        const char* relative;
        const char* pos = strstr(fileLower, rootLower);
        if (pos != nullptr) {
            relative = fileStr + rootLen;
            while (*relative == '/' || *relative == '\\') {
                ++relative;
            }
        } else {
            relative = fileStr;
        }

        if (cacheCount < 64) {
            strncpy(cacheKeys[cacheCount], file, sizeof(cacheKeys[cacheCount]) - 1);
            cacheKeys[cacheCount][sizeof(cacheKeys[cacheCount]) - 1] = '\0';
            strncpy(cacheValues[cacheCount], relative, sizeof(cacheValues[cacheCount]) - 1);
            cacheValues[cacheCount][sizeof(cacheValues[cacheCount]) - 1] = '\0';
            ++cacheCount;
            return cacheValues[cacheCount - 1];
        }

        static char overflow[256];
        strncpy(overflow, relative, sizeof(overflow) - 1);
        overflow[sizeof(overflow) - 1] = '\0';
        return overflow;
    }

    template<typename... Args>
    auto logPrefixed(const char* file, const int line, const bool indented, const Args&... args) -> void {
        char buf[8];
        snprintf(buf, sizeof(buf), "%05d", line);

        std::cout << "[" << getRelativeFile(file) << ":" << buf << "] ";

        if (indented) {
            const int level = indentLevel();
            for (int i = 0; i < level; ++i) {
                std::cout << "  |";
            }
        }

        (std::cout << ... << args);
        std::cout << std::endl;
    }
}

#define LOG(...) \
    Catelier::test::util::logPrefixed(__FILE__, __LINE__, true, __VA_ARGS__)

#define PURE_LOG(...) \
    Catelier::test::util::logPrefixed(__FILE__, __LINE__, false, __VA_ARGS__)

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define LOGGER_INDENT_BLOCK(enabled) \
    for (int CONCAT(_indent_, __LINE__) = \
        (Catelier::test::util::indentPush(), \
        (enabled) ? 0 : (LOG("── 折叠块 ──"), 0)); \
        CONCAT(_indent_, __LINE__) < 1; \
        (Catelier::test::util::indentPop(), ++CONCAT(_indent_, __LINE__))) \
        if (enabled)