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
    std::string prompt;
    bool updatePrompt = false;
    std::string input;
    bool updateInput = false;
    std::string help;
    bool hideCombiLines = false;
    bool exitByCancel = true;
    std::vector<Line> lines;
};

class Protocol {
public:
    Protocol() = default;
    ~Protocol() = default;

    std::string CreateOnKeyPressRequest(const char* text);
    std::string CreateInputChangeRequest(const char* text);
    std::string CreateOnSelectLineRequest(const char* text);
    UserRequest ParseRequest(const char* text);

private:
    void ParseLines(uint32_t itemCount, std::vector<Line>& result);
    Line ParseLine(uint32_t keyCount);

private:
    Json m_json;
};
