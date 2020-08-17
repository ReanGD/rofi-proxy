#pragma once

#include <string>
#include <vector>
#include <cstdio>
#include <rofi/rofi-types.h>


class Proxy {
public:
    Proxy();
    ~Proxy();

public:
    void Init();
    size_t GetLinesCount() const;
    const char* GetLine(size_t index) const;
    bool TokenMatch(rofi_int_matcher **tokens, size_t index) const;

private:
    FILE* m_log = nullptr;
    std::vector<std::string> m_lines;
};
