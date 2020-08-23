#include "protocol.h"

#include "exception.h"


std::string Protocol::CreateInputChangeRequest(const char* input) {
    return detail::Format("{\"name\": \"input\", \"value\": \"%s\"}", input);
}

std::vector<std::string> Protocol::ParseRequest(const char* text) {
    m_parser.Parse(text);
    m_parser.Next(TokenType::Object);
    if (m_parser.Next(TokenType::String)->AsString() != "lines") {
        throw ProxyError("unexpected token value");
    }
    m_parser.Next(TokenType::Array);

    return ParseLines();
}

std::vector<std::string> Protocol::ParseLines() {
    std::vector<std::string> result;
    for(auto* t = m_parser.Next(); t!=nullptr; t = m_parser.Next()) {
        if (t->type != TokenType::Object) {
            throw ProxyError("unexpected token type");
        }

        if (m_parser.Next(TokenType::String)->AsString() != "text") {
            throw ProxyError("unexpected token value");
        }

        result.push_back(std::string(m_parser.Next(TokenType::String)->AsString()));
    }

    return result;
}
