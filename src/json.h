#pragma once

#include <cstddef>
#include <cstdint>


enum class TokenType : uint8_t {
    Undefined = 0,
    Object = 1,
    Array = 2,
    String = 3,
    Primitive = 4 // Other primitive: number, boolean (true/false) or null
};

enum jsmnerr {
    /* Invalid character inside JSON string */
    JSMN_ERROR_INVAL = -2,
    /* The string is not a full JSON packet, more bytes expected */
    JSMN_ERROR_PART = -3
};

struct Token {
    TokenType type;
    int start;
    int end;
    int size;
};

class JsonParser {
public:
    JsonParser();
    ~JsonParser();

    int Parse(const char* js, size_t len);
    Token* Tokens();

    Token* NewToken(TokenType type, int start, int end);
    int ParsePrimitive(const char *js, const size_t len);
    int ParseString(const char *js, const size_t len);
    int ParseImpl(const char *js, const size_t len);

public:
    unsigned int pos;     /* offset in the JSON string */
    int toksuper;         /* superior token node, e.g. parent object or array */

private:
    Token* m_tokens = nullptr;
    uint32_t m_tokensCount = 0;
    uint32_t m_tokensCapacity = 0;
};
