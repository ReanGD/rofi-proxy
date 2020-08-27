#include "protocol.h"

#include "exception.h"


std::string Protocol::CreateMessageInput(const char* text) {
    return detail::Format(
        "{\"name\": \"input\", \"value\": \"%s\"}",
        m_json.EscapeString(text).c_str());
}

std::string Protocol::CreateMessageSelectLine(const Line& line) {
    return detail::Format(
        "{\"name\": \"select_line\", \"value\": {\"id\": \"%s\", \"text\": \"%s\", \"group\": \"%s\"}}",
        m_json.EscapeString(line.id.c_str()).c_str(),
        m_json.EscapeString(line.text.c_str()).c_str(),
        m_json.EscapeString(line.group.c_str()).c_str());
}

std::string Protocol::CreateMessageKeyPress(const Line& line, const char* keyName) {
    return detail::Format(
        "{\"name\": \"key_press\", \"value\": {\"key\": \"%s\", \"line\": {\"id\": \"%s\", \"text\": \"%s\", \"group\": \"%s\"}}}",
        m_json.EscapeString(keyName).c_str(),
        m_json.EscapeString(line.id.c_str()).c_str(),
        m_json.EscapeString(line.text.c_str()).c_str(),
        m_json.EscapeString(line.group.c_str()).c_str());
}

UserRequest Protocol::ParseRequest(const char* text) {
    m_json.Parse(text);

    UserRequest result;
    uint32_t keyCount = m_json.Next(TokenType::Object)->size;
    for (uint32_t i=0; i!=keyCount; ++i) {
        auto key = m_json.NextString();
        if (key == "prompt") {
            result.prompt = m_json.NextString();
            result.updatePrompt = true;
        } else if (key == "input") {
            result.input = m_json.NextString();
            result.updateInput = true;
        } else if (key == "help") {
            result.help = m_json.NextString();
        } else if (key == "hide_combi_lines") {
            result.hideCombiLines = m_json.NextBool();
        } else if (key == "exit_by_cancel") {
            result.exitByCancel = m_json.NextBool();
        } else if (key == "lines") {
            ParseLines(m_json.Next(TokenType::Array)->size, result.lines);
        } else {
            throw ProxyError("unexpected key in root dict");
        }
    }

    return result;
}

void Protocol::ParseLines(uint32_t itemCount, std::vector<Line>& result) {
    for (uint32_t i=0; i!=itemCount; ++i) {
        result.push_back(ParseLine(m_json.Next(TokenType::Object)->size));
    }
}

Line Protocol::ParseLine(uint32_t keyCount) {
    Line result;

    for (uint32_t i=0; i!=keyCount; ++i) {
        auto key = m_json.NextString();
        if (key == "id") {
            result.id = m_json.NextString();
        } else if (key == "text") {
            result.text = m_json.NextString();
        } else if (key == "group") {
            result.group = m_json.NextString();
        } else if (key == "filtering") {
            result.filtering = m_json.NextBool();
        } else if (key == "markup") {
            result.markup = m_json.NextBool();
        } else {
            throw ProxyError("unexpected key in line item dict");
        }
    }

    if (result.text.empty()) {
        throw ProxyError("field \"text\" in section \"lines\" is empty");
    }

    return result;
}
