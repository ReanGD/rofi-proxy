#pragma once

#include <string>
#include <vector>

#include "json.h"


struct Line {
    bool filtering = true;
    bool markup = false;
    std::string id;
    std::string text;
};

class Protocol {
public:
    Protocol() = default;
    ~Protocol() = default;

    std::string CreateInputChangeRequest(const char* text);
    std::string CreateOnSelectLineRequest(const char* text);
    std::vector<Line> ParseRequest(const char* text);

private:
    std::vector<Line> ParseLines(uint32_t itemCount);
    Line ParseLine(uint32_t keyCount);

private:
    Json m_json;
};
