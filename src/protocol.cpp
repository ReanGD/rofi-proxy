#include "protocol.h"

#include "exception.h"


std::string Protocol::CreateInputChangeRequest(const char* input) {
    return detail::Format("{\"name\": \"input\", \"value\": \"%s\"}", input);
}

std::vector<Line> Protocol::ParseRequest(const char* text) {
    m_parser.Parse(text);
    m_parser.Next(TokenType::Object);
    if (m_parser.NextString() != "lines") {
        throw ProxyError("unexpected token value");
    }

    return ParseLines(m_parser.Next(TokenType::Array)->size);
}

std::vector<Line> Protocol::ParseLines(uint32_t itemCount) {
    std::vector<Line> result;
    for (uint32_t i=0; i!=itemCount; ++i) {
        result.push_back(ParseLine(m_parser.Next(TokenType::Object)->size));
    }

    return result;
}

Line Protocol::ParseLine(uint32_t keyCount) {
    Line result;

    for (uint32_t i=0; i!=keyCount; ++i) {
        auto key = m_parser.NextString();
        if (key == "text") {
            result.text = m_parser.NextString();
        } else if (key == "filtering") {
            result.filtering = m_parser.NextBool();
        } else {
            throw ProxyError("unexpected key in line item");
        }
    }

    if (result.text.empty()) {
        throw ProxyError("line.text is empty");
    }

    return result;
}
