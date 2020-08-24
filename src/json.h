#pragma once

#include <cstddef>
#include <string_view>


enum class TokenType : uint8_t {
    Undefined = 0,
    Object = 1,
    Array = 2,
    String = 3,
    Primitive = 4 // Other primitive: number, boolean (true/false) or null
};

struct Token {
    TokenType type;
    char* start;
    char* end;
    uint32_t size;

    std::string_view AsString();
};

class JsonParser {
public:
    JsonParser();
    ~JsonParser();

    void Parse(const char* text);
    [[maybe_unused]] Token* Next();
    [[maybe_unused]] Token* Next(TokenType expectedType);
    std::string_view NextString();
    bool NextBool();

    std::string EscapeString(const char* str);

protected:
    Token* NewToken(TokenType type, char* start, char* end);
    void ParsePrimitive();
    void ParseString();
    void ParseImpl();

private:
    char* m_text = nullptr;
    char* m_textIt = nullptr;

    Token* m_tokens = nullptr;
    Token* m_parent = nullptr;
    uint32_t m_tokensIt = 0;
    uint32_t m_tokensCount = 0;
    uint32_t m_tokensCapacity = 0;
};
