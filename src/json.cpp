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
 * FITNESS FOR A PARTICULAR PURm_textPosE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstring>

#include "json.h"
#include "exception.h"


static const char STRING_INVALID[]  = "invalid character inside JSON string";
static const char JSON_INVALID[]  = "the string is not a full JSON packet, more bytes expected";

std::string_view Token::AsString() {
    return std::string_view(start, end);
}

JsonParser::JsonParser()
    : m_tokens(new Token[64])
    , m_tokensCapacity(64) {

}

JsonParser::~JsonParser() {
    if (m_tokens != nullptr) {
        delete[] m_tokens;
    }
    if (m_text != nullptr) {
        delete[] m_text;
    }
}

void JsonParser::Parse(const char* text) {
    if (m_text != nullptr) {
        delete[] m_text;
    }
    auto len = strlen(text) + 1;
    m_text = new char[len];
    strncpy(m_text, text, len);
    m_textIt = m_text;

    m_parent = nullptr;
    m_tokensIt = 0;
    m_tokensCount = 0;

    ParseImpl();
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

Token* JsonParser::NewToken(TokenType type, char* start, char* end) {
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

void JsonParser::ParsePrimitive() {
    for (char* start = m_textIt; *m_textIt != '\0'; ++m_textIt) {
        switch (*m_textIt) {
#ifndef JSMN_STRICT
        // In strict mode primitive must be followed by "," or "}" or "]"
        case ':':
#endif
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
        case ']':
        case '}':
            NewToken(TokenType::Primitive, start, m_textIt);
            --m_textIt;
            return;
        default:
            break;
        }
        if ((*m_textIt < 32) || (*m_textIt >= 127)) {
            throw ProxyError(STRING_INVALID);
        }
    }
#ifdef JSMN_STRICT
    /* In strict mode primitive must be followed by a comma/object/array */
    throw ProxyError(JSON_INVALID);
#endif
}

void JsonParser::ParseString() {
    for (char* start = ++m_textIt; *m_textIt != '\0'; ++m_textIt) {
        // Quote: end of string
        if (*m_textIt == '\"') {
            NewToken(TokenType::String, start, m_textIt);
            return;
        }

        /* Backslash: Quoted symbol expected */
        if ((*m_textIt == '\\') && (m_textIt[1] != '\0')) {
            int i;
            ++m_textIt;
            switch (*m_textIt) {
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
            // Allows escaped symbol \uXXXX
            case 'u':
                ++m_textIt;
                for (i=0; (i != 4) && (*m_textIt != '\0'); ++i, ++m_textIt) {
                    // If it isn't a hex character we have an error
                    if (!(  (*m_textIt >= 48 && *m_textIt <= 57) ||   // 0-9
                            (*m_textIt >= 65 && *m_textIt <= 70) ||   // A-F
                            (*m_textIt >= 97 && *m_textIt <= 102))) { // a-f
                        throw ProxyError(STRING_INVALID);
                    }
                }
                --m_textIt;
                break;
            // Unexpected symbol
            default:
                throw ProxyError(STRING_INVALID);
            }
        }
    }

    throw ProxyError(JSON_INVALID);
}

void JsonParser::ParseImpl() {
    for (; *m_textIt != '\0'; ++m_textIt) {
        int32_t i;
        TokenType type;
        char c = *m_textIt;
        switch (c) {
        case '{':
        case '[':
            type = (c == '{' ? TokenType::Object : TokenType::Array);
            NewToken(type, m_textIt, nullptr);
            if (m_parent != nullptr) {
#ifdef JSMN_STRICT
                // In strict mode an object or array can't become a key
                if (m_parent->type == TokenType::Object) {
                    throw ProxyError(STRING_INVALID);
                }
#endif
                m_parent->size++;
            }
            m_parent = &m_tokens[m_tokensCount - 1];
            break;
        case '}':
        case ']':
            type = (c == '}' ? TokenType::Object : TokenType::Array);
            for (i=static_cast<int32_t>(m_tokensCount - 1); i >= 0; --i) {
                Token* token = &m_tokens[i];
                if (token->end == nullptr) {
                    if (token->type != type) {
                        throw ProxyError(STRING_INVALID);
                    }
                    m_parent = nullptr;
                    token->end = m_textIt + 1;
                    break;
                }
            }
            // Error if unmatched closing bracket
            if (i == -1) {
                throw ProxyError(STRING_INVALID);
            }
            for (; i >= 0; --i) {
                Token* token = &m_tokens[i];
                if (token->end == nullptr) {
                    m_parent = &m_tokens[i];
                    break;
                }
            }
            break;
        case '\"':
            ParseString();
            if (m_parent != nullptr) {
                m_parent->size++;
            }
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            m_parent = &m_tokens[m_tokensCount - 1];
            break;
        case ',':
            if ((m_parent != nullptr) && (m_parent->type != TokenType::Array) && (m_parent->type != TokenType::Object)) {
                for (i=static_cast<int32_t>(m_tokensCount - 1); i >= 0; --i) {
                    Token* t = &m_tokens[i];
                    if ((t->type == TokenType::Array) || (t->type == TokenType::Object)) {
                        if (t->end == nullptr) {
                            m_parent = t;
                            break;
                        }
                    }
                }
            }
            break;
#ifdef JSMN_STRICT
        // In strict mode primitives are: numbers and booleans
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
            // And they must not be keys of the object
            if (m_parent != nullptr) {
                if ((m_parent->type == TokenType::Object) || ((m_parent->type == TokenType::String) && (m_parent->size != 0))) {
                    throw ProxyError(STRING_INVALID);
                }
            }
#else
        // In non-strict mode every unquoted value is a primitive
        default:
#endif
            ParsePrimitive();
            if (m_parent != nullptr) {
                m_parent->size++;
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
        if (m_tokens[i].end == nullptr) {
            throw ProxyError(JSON_INVALID);
        }
    }
}
