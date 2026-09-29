#pragma once
#include <algorithm>
#include <iostream>
#include <map>

namespace Catelier::test::util {
    inline std::string formatLineNumber(const int line) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%05d", line);
        return std::string(buf);
    }

    template <typename T, typename... Args>
    auto log(const T& first, const Args&... rest) -> void {
        std::cout << first;
        (std::cout << ... << rest);
        std::cout << std::endl;
    }

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
    inline std::string getIndent() {
        std::string result;

        const int level = indentLevel();
        result.reserve(level * 3);
        for (int i = 0; i < level; ++i) {
            result += "  |";
        }

        return result;
    }

    inline const char* getRelativeFile(const char* file) {
        static std::map<std::string, std::string> cache;

        const std::string fileKey(file);
        const auto it = cache.find(fileKey);
        if (it != cache.end()) {
            return it->second.c_str();
        }

        std::string fileStr(file);
        std::string rootStr(PROJECT_SOURCE_DIR);

        for (char& c : fileStr) {
            if (c == '\\') {
                c = '/';
            }
        }

        for (char& c : rootStr) {
            if (c == '\\') {
                c = '/';
            }
        }

        std::string fileLower = fileStr;
        std::string rootLower = rootStr;
        std::transform(fileLower.begin(), fileLower.end(), fileLower.begin(), tolower);
        std::transform(rootLower.begin(), rootLower.end(), rootLower.begin(), tolower);

        std::string relative;
        const size_t pos = fileLower.find(rootLower);
        if (pos != std::string::npos) {
            relative = fileStr.substr(rootStr.length());
            while (!relative.empty() && (relative.front() == '/' || relative.front() == '\\')) {
                relative.erase(relative.begin());
            }
        } else {
            relative = fileStr;
        }

        const auto [newIt, inserted] = cache.emplace(fileKey, std::move(relative));
        return newIt->second.c_str();
    }
}

#define CONCAT_IMPL(a, b) \
    a##b

#define CONCAT(a, b) \
    CONCAT_IMPL(a, b)

#define LOGGER_INDENT_BLOCK(enabled) \
    for (int CONCAT(_indent_, __LINE__) = \
        (Catelier::test::util::indentPush(), \
        (enabled) ? 0 : (LOG("── 折叠块 ──"), 0)); \
        CONCAT(_indent_, __LINE__) < 1; \
        (Catelier::test::util::indentPop(), ++CONCAT(_indent_, __LINE__))) \
        if (enabled)

#define LOG(...) \
    Catelier::test::util::log( \
        "[", Catelier::test::util::getRelativeFile(__FILE__), ":", \
        Catelier::test::util::formatLineNumber(__LINE__), \
        "] ", \
        Catelier::test::util::getIndent(), \
        __VA_ARGS__ \
    )

#define PURE_LOG(...) \
    Catelier::test::util::log( \
        "[", Catelier::test::util::getRelativeFile(__FILE__), ":", \
        Catelier::test::util::formatLineNumber(__LINE__), \
        "] ", \
        __VA_ARGS__ \
    )