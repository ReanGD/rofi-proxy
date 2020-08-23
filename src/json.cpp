/*
 * MIT License
 *
 * Copyright (c) 2010 Serge Zaitsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURm_POSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "json.h"
#include "exception.h"


static const uint32_t UNDEFINED = UINT32_MAX;
static const char STRING_INVALID[]  = "invalid character inside JSON string";
static const char JSON_INVALID[]  = "the string is not a full JSON packet, more bytes expected";

std::string_view Token::AsString(const char* text) {
    return std::string_view(text + start, text + end);
}

JsonParser::JsonParser()
    : m_tokens(new Token[64])
    , m_tokensCapacity(64) {

}

JsonParser::~JsonParser() {
    if (m_tokens != nullptr) {
        delete[] m_tokens;
    }
}

void JsonParser::Parse(const char* js, size_t len) {
    m_pos = 0;
    m_toksuper = UNDEFINED;
    m_tokensIt = 0;
    m_tokensCount = 0;

    ParseImpl(js, len);
}

Token* JsonParser::Next(TokenType expectedType) {
    if (m_tokensIt < m_tokensCount) {
        Token* token = &m_tokens[m_tokensIt++];
        if ((expectedType != TokenType::Undefined) && (token->type != expectedType)) {
            throw ProxyError("unexpected token type");
        }

        return token;
    }

    if (expectedType != TokenType::Undefined) {
        throw ProxyError("unexpected token type");
    }

    return nullptr;
}

Token* JsonParser::NewToken(TokenType type, uint32_t start, uint32_t end) {
    if (m_tokensCount >= m_tokensCapacity) {
        m_tokensCapacity *= 2;
        Token* tokens = new Token[m_tokensCapacity];
        for (uint32_t i=0; i!=m_tokensCount; ++i) {
            tokens[i] = m_tokens[i];
        }
        delete[] m_tokens;
        m_tokens = tokens;
    }
    Token* token = &m_tokens[m_tokensCount++];
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;

    return token;
}

void JsonParser::ParsePrimitive(const char *js, const size_t len) {
    auto start = m_pos;
    for (; (m_pos < len) && (js[m_pos] != '\0'); ++m_pos) {
        switch (js[m_pos]) {
#ifndef JSMN_STRICT
        /* In strict mode primitive must be followed by "," or "}" or "]" */
        case ':':
#endif
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
        case ']':
        case '}':
            NewToken(TokenType::Primitive, start, m_pos);
            m_pos--;
            return;
        default:
            break;
        }
        if (js[m_pos] < 32 || js[m_pos] >= 127) {
            throw ProxyError(STRING_INVALID);
        }
    }
#ifdef JSMN_STRICT
    /* In strict mode primitive must be followed by a comma/object/array */
    throw ProxyError(JSON_INVALID);
#endif
}

void JsonParser::ParseString(const char *js, const size_t len) {
    auto start = m_pos;
    m_pos++;

    /* Skip starting quote */
    for (; (m_pos < len) && (js[m_pos] != '\0'); ++m_pos) {
        char c = js[m_pos];

        /* Quote: end of string */
        if (c == '\"') {
            NewToken(TokenType::String, start + 1, m_pos);
            return;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && m_pos + 1 < len) {
            int i;
            m_pos++;
            switch (js[m_pos]) {
            /* Allowed escaped symbols */
            case '\"':
            case '/':
            case '\\':
            case 'b':
            case 'f':
            case 'r':
            case 'n':
            case 't':
                break;
            /* Allows escaped symbol \uXXXX */
            case 'u':
                m_pos++;
                for (i=0; (i < 4) && (m_pos < len) && (js[m_pos] != '\0'); ++i) {
                    /* If it isn't a hex character we have an error */
                    if (!(  (js[m_pos] >= 48 && js[m_pos] <= 57) ||   /* 0-9 */
                            (js[m_pos] >= 65 && js[m_pos] <= 70) ||   /* A-F */
                            (js[m_pos] >= 97 && js[m_pos] <= 102))) { /* a-f */
                        throw ProxyError(STRING_INVALID);
                    }
                    m_pos++;
                }
                m_pos--;
                break;
            /* Unexpected symbol */
            default:
                throw ProxyError(STRING_INVALID);
            }
        }
    }

    throw ProxyError(JSON_INVALID);
}

void JsonParser::ParseImpl(const char *js, const size_t len) {
    for (; (m_pos < len) && (js[m_pos] != '\0'); ++m_pos) {
        int32_t i;
        TokenType type;
        char c = js[m_pos];
        switch (c) {
        case '{':
        case '[':
            type = (c == '{' ? TokenType::Object : TokenType::Array);
            NewToken(type, m_pos, UNDEFINED);
            if (m_toksuper != UNDEFINED) {
                Token *t = &m_tokens[m_toksuper];
#ifdef JSMN_STRICT
                /* In strict mode an object or array can't become a key */
                if (t->type == TokenType::Object) {
                    throw ProxyError(STRING_INVALID);
                }
#endif
                t->size++;
            }
            m_toksuper = m_tokensCount - 1;
            break;
        case '}':
        case ']':
            type = (c == '}' ? TokenType::Object : TokenType::Array);
            for (i=static_cast<int32_t>(m_tokensCount - 1); i >= 0; --i) {
                Token* token = &m_tokens[i];
                if (token->end == UNDEFINED) {
                    if (token->type != type) {
                        throw ProxyError(STRING_INVALID);
                    }
                    m_toksuper = UNDEFINED;
                    token->end = m_pos + 1;
                    break;
                }
            }
            // Error if unmatched closing bracket
            if (i == -1) {
                throw ProxyError(STRING_INVALID);
            }
            for (; i >= 0; --i) {
                Token* token = &m_tokens[i];
                if (token->end == UNDEFINED) {
                    m_toksuper = static_cast<uint32_t>(i);
                    break;
                }
            }
            break;
        case '\"':
            ParseString(js, len);
            if (m_toksuper != UNDEFINED) {
                m_tokens[m_toksuper].size++;
            }
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            m_toksuper = m_tokensCount - 1;
            break;
        case ',':
            if ((m_toksuper != UNDEFINED) && (m_tokens[m_toksuper].type != TokenType::Array) && (m_tokens[m_toksuper].type != TokenType::Object)) {
                for (i=static_cast<int32_t>(m_tokensCount - 1); i >= 0; --i) {
                    if ((m_tokens[i].type == TokenType::Array) || (m_tokens[i].type == TokenType::Object)) {
                        if (m_tokens[i].end == UNDEFINED) {
                            m_toksuper = static_cast<uint32_t>(i);
                            break;
                        }
                    }
                }
            }
            break;
#ifdef JSMN_STRICT
        /* In strict mode primitives are: numbers and booleans */
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 't':
        case 'f':
        case 'n':
            /* And they must not be keys of the object */
            if (m_toksuper != UNDEFINED) {
                const Token *t = &m_tokens[m_toksuper];
                if ((t->type == TokenType::Object) || ((t->type == TokenType::String) && (t->size != 0))) {
                    throw ProxyError(STRING_INVALID);
                }
            }
#else
        /* In non-strict mode every unquoted value is a primitive */
        default:
#endif
            ParsePrimitive(js, len);
            if (m_toksuper != UNDEFINED) {
                m_tokens[m_toksuper].size++;
            }
            break;

#ifdef JSMN_STRICT
        // Unexpected char in strict mode
        default:
            throw ProxyError(STRING_INVALID);
#endif
        }
    }

    for (uint32_t i=0; i!=m_tokensCount; ++i) {
        // Unmatched opened object or array
        if (m_tokens[i].end == UNDEFINED) {
            throw ProxyError(JSON_INVALID);
        }
    }
}
