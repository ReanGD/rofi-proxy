#pragma once

#include <vector>
#include <string>

#include "json.h"


struct Line {
    bool filtering = true;
    std::string text;
};

class Protocol {
public:
    Protocol() = default;
    ~Protocol() = default;

    std::string CreateInputChangeRequest(const char* input);
    std::vector<Line> ParseRequest(const char* text);

private:
    std::vector<Line> ParseLines(uint32_t itemCount);
    Line ParseLine(uint32_t keyCount);

private:
    JsonParser m_parser;
};
