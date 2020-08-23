#include "protocol.h"

#include <cstring>
#include "exception.h"


std::string Protocol::CreateInputChangeRequest(const char* input) {
    return detail::Format("{\"name\": \"input\", \"value\": \"%s\"}", input);
}

std::vector<std::string> Protocol::ParseRequest(const char* text) {
    m_parser.Parse(text, strlen(text));
    m_parser.Next(TokenType::Object);
    if (m_parser.Next(TokenType::String)->AsString(text) != "lines") {
        throw ProxyError("unexpected token value");
    }
    m_parser.Next(TokenType::Array);

    return ParseLines(text);
}

std::vector<std::string> Protocol::ParseLines(const char* text) {
    std::vector<std::string> result;
    for(auto* t = m_parser.Next(); t!=nullptr; t = m_parser.Next()) {
        if (t->type != TokenType::Object) {
            throw ProxyError("unexpected token type");
        }

        if (m_parser.Next(TokenType::String)->AsString(text) != "text") {
            throw ProxyError("unexpected token value");
        }

        result.push_back(std::string(m_parser.Next(TokenType::String)->AsString(text)));
    }

    return result;
}
