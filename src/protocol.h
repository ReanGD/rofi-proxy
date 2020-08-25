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

struct UserRequest {
    bool hideCombiLines = false;
    std::vector<Line> lines;
};

class Protocol {
public:
    Protocol() = default;
    ~Protocol() = default;

    std::string CreateInputChangeRequest(const char* text);
    std::string CreateOnSelectLineRequest(const char* text);
    UserRequest ParseRequest(const char* text);

private:
    void ParseLines(uint32_t itemCount, std::vector<Line>& result);
    Line ParseLine(uint32_t keyCount);

private:
    Json m_json;
};
