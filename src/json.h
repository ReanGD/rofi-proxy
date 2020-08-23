#pragma once

#include <cstddef>
#include <cstdint>
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
    uint32_t start;
    uint32_t end;
    uint32_t size;

    std::string_view AsString(const char* text);
};

class JsonParser {
public:
    JsonParser();
    ~JsonParser();

    void Parse(const char* js, size_t len);
    [[maybe_unused]] Token* Next(TokenType expectedType = TokenType::Undefined);

protected:
    Token* NewToken(TokenType type, uint32_t start, uint32_t end);
    void ParsePrimitive(const char *js, const size_t len);
    void ParseString(const char *js, const size_t len);
    void ParseImpl(const char *js, const size_t len);

private:
    uint32_t m_pos;  // offset in the JSON string
    uint32_t m_toksuper;  // superior token node, e.g. parent object or array

    Token* m_tokens = nullptr;
    uint32_t m_tokensIt = 0;
    uint32_t m_tokensCount = 0;
    uint32_t m_tokensCapacity = 0;
};
