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

std::string Protocol::CreateMessageDeleteLine(const Line& line) {
    return detail::Format(
        "{\"name\": \"delete_line\", \"value\": {\"id\": \"%s\", \"text\": \"%s\", \"group\": \"%s\"}}",
        m_json.EscapeString(line.id.c_str()).c_str(),
        m_json.EscapeString(line.text.c_str()).c_str(),
        m_json.EscapeString(line.group.c_str()).c_str());
}

std::string Protocol::CreateMessageSelectCustomInput(const char* text) {
    return detail::Format(
        "{\"name\": \"select_custom_input\", \"value\": \"%s\"}",
        m_json.EscapeString(text).c_str());
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
            result.prompt = m_json.NextStringOrNull(result.updatePrompt);
        } else if (key == "input") {
            result.input = m_json.NextStringOrNull(result.updateInput);
        } else if (key == "overlay") {
            result.overlay = m_json.NextStringOrNull(result.updateOverlay);
        } else if (key == "help") {
            result.help = m_json.NextStringOrNull(result.updateHelp);
        } else if (key == "hide_combi_lines") {
            result.hideCombiLines = m_json.NextBoolOrNull(result.updateHideCombiLines);
        } else if (key == "exit_by_cancel") {
            result.exitByCancel = m_json.NextBoolOrNull(result.updateExitByCancel);
        } else if (key == "lines") {
            auto itemCount = m_json.NextOrNull(TokenType::Array, result.updateLines)->size;
            if (result.updateLines) {
                ParseLines(itemCount, result.lines);
            }
        } else {
            throw ProxyError("unexpected key \"%s\" in root dict", std::string(key).c_str());
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
    bool isValue;
    for (uint32_t i=0; i!=keyCount; ++i) {
        auto key = m_json.NextString();
        if (key == "id") {
            result.id = m_json.NextStringOrNull(isValue);
        } else if (key == "text") {
            result.text = m_json.NextString();
        } else if (key == "group") {
            result.group = m_json.NextStringOrNull(isValue);
        } else if (key == "icon") {
            result.icon = m_json.NextStringOrNull(isValue);
        } else if (key == "filtering") {
            bool value = m_json.NextBoolOrNull(isValue);
            if (isValue) {
                result.filtering = value;
            }
        } else if (key == "urgent") {
            bool value = m_json.NextBoolOrNull(isValue);
            if (isValue) {
                result.urgent = value;
            }
        } else if (key == "active") {
            bool value = m_json.NextBoolOrNull(isValue);
            if (isValue) {
                result.active = value;
            }
        } else if (key == "markup") {
            bool value = m_json.NextBoolOrNull(isValue);
            if (isValue) {
                result.markup = value;
            }
        } else {
            throw ProxyError("unexpected key \"%s\" in line item dict", std::string(key).c_str());
        }
    }

    if (result.text.empty()) {
        throw ProxyError("field \"text\" in section \"lines\" is empty");
    }

    return result;
}
