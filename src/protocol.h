#pragma once

#include <vector>
#include <string>

#include "json.h"


class Protocol {
public:
    Protocol() = default;
    ~Protocol() = default;

    std::string CreateInputChangeRequest(const char* input);
    std::vector<std::string> ParseRequest(const char* text);

private:
    std::vector<std::string> ParseLines(const char* text);

private:
    JsonParser m_parser;
};
