#include "protocol.h"

#include "exception.h"


std::string Protocol::CreateInputChangeRequest(const char* text) {
    return detail::Format("{\"name\": \"input\", \"value\": \"%s\"}", m_json.EscapeString(text).c_str());
}

std::string Protocol::CreateOnSelectLineRequest(const char* text) {
    return detail::Format("{\"name\": \"selected_line\", \"value\": \"%s\"}", m_json.EscapeString(text).c_str());
}

std::vector<Line> Protocol::ParseRequest(const char* text) {
    m_json.Parse(text);
    m_json.Next(TokenType::Object);
    if (m_json.NextString() != "lines") {
        throw ProxyError("unexpected token value");
    }

    return ParseLines(m_json.Next(TokenType::Array)->size);
}

std::vector<Line> Protocol::ParseLines(uint32_t itemCount) {
    std::vector<Line> result;
    for (uint32_t i=0; i!=itemCount; ++i) {
        result.push_back(ParseLine(m_json.Next(TokenType::Object)->size));
    }

    return result;
}

Line Protocol::ParseLine(uint32_t keyCount) {
    Line result;

    for (uint32_t i=0; i!=keyCount; ++i) {
        auto key = m_json.NextString();
        if (key == "text") {
            result.text = m_json.NextString();
        } else if (key == "filtering") {
            result.filtering = m_json.NextBool();
        } else {
            throw ProxyError("unexpected key in line item");
        }
    }

    if (result.text.empty()) {
        throw ProxyError("line.text is empty");
    }

    return result;
}
