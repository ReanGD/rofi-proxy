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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "json.h"
#include "exception.h"


static const char STRING_INVALID[]  = "invalid character inside JSON string";
static const char JSON_INVALID[]  = "the string is not a full JSON packet, more bytes expected";

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
    pos = 0;
    toksuper = -1;

    m_tokensIt = 0;
    m_tokensCount = 0;

    ParseImpl(js, len);
}

Token* JsonParser::Next() {
    if (m_tokensIt < m_tokensCount) {
        return &m_tokens[m_tokensIt++];
    }

    return nullptr;
}

Token* JsonParser::NewToken(TokenType type, int start, int end) {
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
    int start = pos;
    for (; pos < len && js[pos] != '\0'; pos++) {
        switch (js[pos]) {
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
            goto found;
        default:
            break;
        }
        if (js[pos] < 32 || js[pos] >= 127) {
            throw ProxyError(STRING_INVALID);
        }
    }
#ifdef JSMN_STRICT
    /* In strict mode primitive must be followed by a comma/object/array */
    throw ProxyError(JSON_INVALID);
#endif

found:
    Token* token = NewToken(TokenType::Primitive, start, pos);
    pos--;
}

void JsonParser::ParseString(const char *js, const size_t len) {
    int start = pos;

    pos++;

    /* Skip starting quote */
    for (; pos < len && js[pos] != '\0'; pos++) {
        char c = js[pos];

        /* Quote: end of string */
        if (c == '\"') {
            NewToken(TokenType::String, start + 1, pos);
            return;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\' && pos + 1 < len) {
            int i;
            pos++;
            switch (js[pos]) {
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
                pos++;
                for (i = 0; i < 4 && pos < len && js[pos] != '\0';
                         i++) {
                    /* If it isn't a hex character we have an error */
                    if (!((js[pos] >= 48 && js[pos] <= 57) ||   /* 0-9 */
                                (js[pos] >= 65 && js[pos] <= 70) ||   /* A-F */
                                (js[pos] >= 97 && js[pos] <= 102))) { /* a-f */
                        throw ProxyError(STRING_INVALID);
                    }
                    pos++;
                }
                pos--;
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
    int r;
    int i;

    for (; pos < len && js[pos] != '\0'; pos++) {
        char c;
        TokenType type;

        c = js[pos];
        switch (c) {
        case '{':
        case '[': {
            auto type = (c == '{' ? TokenType::Object : TokenType::Array);
            Token* token = NewToken(type, pos, -1);
            if (toksuper != -1) {
                Token *t = &m_tokens[toksuper];
#ifdef JSMN_STRICT
                /* In strict mode an object or array can't become a key */
                if (t->type == TokenType::Object) {
                    throw ProxyError(STRING_INVALID);
                }
#endif
                t->size++;
            }
            toksuper = m_tokensCount - 1;
            }
            break;
        case '}':
        case ']':
            type = (c == '}' ? TokenType::Object : TokenType::Array);
            for (i = m_tokensCount - 1; i >= 0; i--) {
                Token* token = &m_tokens[i];
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type) {
                        throw ProxyError(STRING_INVALID);
                    }
                    toksuper = -1;
                    token->end = pos + 1;
                    break;
                }
            }
            /* Error if unmatched closing bracket */
            if (i == -1) {
                throw ProxyError(STRING_INVALID);
            }
            for (; i >= 0; i--) {
                Token* token = &m_tokens[i];
                if (token->start != -1 && token->end == -1) {
                    toksuper = i;
                    break;
                }
            }
            break;
        case '\"':
            ParseString(js, len);
            if (toksuper != -1) {
                m_tokens[toksuper].size++;
            }
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ':':
            toksuper = m_tokensCount - 1;
            break;
        case ',':
            if (toksuper != -1 &&
                    m_tokens[toksuper].type != TokenType::Array &&
                    m_tokens[toksuper].type != TokenType::Object) {
                for (i = m_tokensCount - 1; i >= 0; i--) {
                    if (m_tokens[i].type == TokenType::Array || m_tokens[i].type == TokenType::Object) {
                        if (m_tokens[i].start != -1 && m_tokens[i].end == -1) {
                            toksuper = i;
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
            if (toksuper != -1) {
                const Token *t = &m_tokens[toksuper];
                if (t->type == TokenType::Object ||
                        (t->type == TokenType::String && t->size != 0)) {
                    throw ProxyError(STRING_INVALID);
                }
            }
#else
        /* In non-strict mode every unquoted value is a primitive */
        default:
#endif
            ParsePrimitive(js, len);
            if (toksuper != -1) {
                m_tokens[toksuper].size++;
            }
            break;

#ifdef JSMN_STRICT
        /* Unexpected char in strict mode */
        default:
            throw ProxyError(STRING_INVALID);
#endif
        }
    }

    for (i = m_tokensCount - 1; i >= 0; i--) {
        /* Unmatched opened object or array */
        if (m_tokens[i].start != -1 && m_tokens[i].end == -1) {
            throw ProxyError(JSON_INVALID);
        }
    }
}
