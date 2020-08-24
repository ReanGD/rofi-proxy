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

#include <locale>
#include <cstring>
#include <charconv>

#include "json.h"
#include "exception.h"

static const auto userLocale = std::locale("");
static const char STRING_INVALID[] = "invalid character inside JSON string";
static const char JSON_INVALID[]  = "the string is not a full JSON packet, more bytes expected";

namespace {

static char* toUtf8(const char* data, char* writeIt) {
    static auto& facet = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(userLocale);
    uint16_t v;
    if (auto [p, ec] = std::from_chars(data, data + 4, v, 16); ((ec != std::errc()) || (p != data + 4))) {
        throw ProxyError(STRING_INVALID);
    }

    char* toNext;
    std::mbstate_t mb{};
    const char16_t* fromNext;
    const char16_t* wcArr = reinterpret_cast<const char16_t*>(&v);
    facet.out(mb, wcArr, wcArr + 1, fromNext, writeIt, writeIt + 4, toNext);

    return toNext;
}

}

std::string_view Token::AsString() {
    return std::string_view(start, end);
}

Json::Json()
    : m_tokens(new Token[64])
    , m_tokensCapacity(64) {

}

Json::~Json() {
    if (m_tokens != nullptr) {
        delete[] m_tokens;
    }
    if (m_text != nullptr) {
        delete[] m_text;
    }
}

void Json::Parse(const char* text) {
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

Token* Json::Next() {
    if (m_tokensIt < m_tokensCount) {
        return &m_tokens[m_tokensIt++];
    }

    return nullptr;
}

Token* Json::Next(TokenType expectedType) {
    Token* token = Next();

    if ((token == nullptr) || (token->type != expectedType)) {
        throw ProxyError("unexpected token type");
    }

    return token;
}

std::string_view Json::NextString() {
    return Next(TokenType::String)->AsString();
}

bool Json::NextBool() {
    auto text = Next(TokenType::Primitive)->AsString();
    if (text == "true") {
        return true;
    }

    if (text == "false") {
        return false;
    }

    throw ProxyError("unexpected bool token value");
}

std::string Json::EscapeString(const char* str) {
    static auto& facet = std::use_facet<std::codecvt<char16_t, char, std::mbstate_t>>(userLocale);
    char16_t wc;
    char16_t* toNext;
    const char* fromNext;
    std::mbstate_t mb{};

    std::string result;
    const char* it = str;
    const char* endIt = str + strlen(str) + 1;
    while(it != endIt) {
        int charCount = facet.length(mb, it, endIt, 1);
        if (charCount == 1) {
            switch (*it) {
            case '\"':
            case '/':
            case '\\':
            case '\b':
            case '\f':
            case '\r':
            case '\n':
            case '\t':
                result.push_back('\\');
            }
            result.push_back(*it);
        } else {
            facet.in(mb, it, it + charCount, fromNext, &wc, &wc + 1, toNext);
            uint16_t v = static_cast<uint16_t>(wc);

            char data[7] = {'\\', 'u', '0', '0', '0', '0', '\0'};
            char *dataBegin = data + 2;
            if (v == 0) {
                dataBegin = nullptr;
            } else if ((v >> 4) == 0) {
                dataBegin += 3;
            } else if ((v >> 8) == 0) {
                dataBegin += 2;
            } else if ((v >> 12) == 0) {
                dataBegin += 1;
            } else {
                dataBegin += 0;
            }

            if (dataBegin != nullptr) {
                if (auto [p, ec] = std::to_chars(dataBegin, data + 6, v, 16); ((ec != std::errc()) || (p != data + 6))) {
                    throw ProxyError("invalid string for escape");
                }
            }
            result.append(data);
        }
        it += charCount;
    }

    return result;
}

Token* Json::NewToken(TokenType type, char* start, char* end) {
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

void Json::ParsePrimitive() {
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
    // In strict mode primitive must be followed by a comma/object/array
    throw ProxyError(JSON_INVALID);
#endif
}

void Json::ParseString() {
    ++m_textIt;
    char* startIt = m_textIt;
    char* writeIt = m_textIt;

    while (*m_textIt != '\0') {
        if (*m_textIt == '\"') {
            NewToken(TokenType::String, startIt, writeIt);
            return;
        }

        if ((*m_textIt == '\\') && (m_textIt[1] != '\0')) {
            ++m_textIt;
            switch (*m_textIt++) {
            case '\"':
                *writeIt++ = '\"';
                break;
            case '/':
                *writeIt++ = '/';
                break;
            case '\\':
                *writeIt++ = '\\';
                break;
            case 'b':
                *writeIt++ = '\b';
                break;
            case 'f':
                *writeIt++ = '\f';
                break;
            case 'r':
                *writeIt++ = '\r';
                break;
            case 'n':
                *writeIt++ = '\b';
                break;
            case 't':
                *writeIt++ = '\t';
                break;
            // \uXXXX
            case 'u':
                writeIt = toUtf8(m_textIt, writeIt);
                m_textIt += 4;
                break;
            default:
                throw ProxyError(STRING_INVALID);
            }
        } else {
            *writeIt++ = *m_textIt++;
        }
    }

    throw ProxyError(JSON_INVALID);
}

void Json::ParseImpl() {
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
