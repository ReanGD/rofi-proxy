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


JsonParser::JsonParser()
    : m_tokens(new Token[m_tokensCount]) {

}

JsonParser::~JsonParser() {
    delete[] m_tokens;
}

int JsonParser::Parse(const char* js, size_t len) {
    pos = 0;
    toknext = 0;
    toksuper = -1;

    return ParseImpl(js, len);
}

Token* JsonParser::Tokens() {
    return m_tokens;
}

Token* JsonParser::NewToken(TokenType type, int start, int end) {
    if (toknext >= m_tokensCount) {
        return nullptr;
    }
    Token* token = &m_tokens[toknext++];
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;

    return token;
}

int JsonParser::ParsePrimitive(const char *js, const size_t len) {
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
            pos = start;
            return JSMN_ERROR_INVAL;
        }
    }
#ifdef JSMN_STRICT
    /* In strict mode primitive must be followed by a comma/object/array */
    pos = start;
    return JSMN_ERROR_PART;
#endif

found:
    Token* token = NewToken(TokenType::Primitive, start, pos);
    if (token == NULL) {
        pos = start;
        return JSMN_ERROR_NOMEM;
    }
    pos--;
    return 0;
}

int JsonParser::ParseString(const char *js, const size_t len) {
    int start = pos;

    pos++;

    /* Skip starting quote */
    for (; pos < len && js[pos] != '\0'; pos++) {
        char c = js[pos];

        /* Quote: end of string */
        if (c == '\"') {
            Token* token = NewToken(TokenType::String, start + 1, pos);
            if (token == NULL) {
                pos = start;
                return JSMN_ERROR_NOMEM;
            }
            return 0;
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
                        pos = start;
                        return JSMN_ERROR_INVAL;
                    }
                    pos++;
                }
                pos--;
                break;
            /* Unexpected symbol */
            default:
                pos = start;
                return JSMN_ERROR_INVAL;
            }
        }
    }
    pos = start;
    return JSMN_ERROR_PART;
}

int JsonParser::ParseImpl(const char *js, const size_t len) {
    int r;
    int i;
    int count = toknext;

    for (; pos < len && js[pos] != '\0'; pos++) {
        char c;
        TokenType type;

        c = js[pos];
        switch (c) {
        case '{':
        case '[': {
            count++;
            auto type = (c == '{' ? TokenType::Object : TokenType::Array);
            Token* token = NewToken(type, pos, -1);
            if (token == NULL) {
                return JSMN_ERROR_NOMEM;
            }
            if (toksuper != -1) {
                Token *t = &m_tokens[toksuper];
#ifdef JSMN_STRICT
                /* In strict mode an object or array can't become a key */
                if (t->type == TokenType::Object) {
                    return JSMN_ERROR_INVAL;
                }
#endif
                t->size++;
            }
            toksuper = toknext - 1;
            }
            break;
        case '}':
        case ']':
            type = (c == '}' ? TokenType::Object : TokenType::Array);
            for (i = toknext - 1; i >= 0; i--) {
                Token* token = &m_tokens[i];
                if (token->start != -1 && token->end == -1) {
                    if (token->type != type) {
                        return JSMN_ERROR_INVAL;
                    }
                    toksuper = -1;
                    token->end = pos + 1;
                    break;
                }
            }
            /* Error if unmatched closing bracket */
            if (i == -1) {
                return JSMN_ERROR_INVAL;
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
            r = ParseString(js, len);
            if (r < 0) {
                return r;
            }
            count++;
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
            toksuper = toknext - 1;
            break;
        case ',':
            if (toksuper != -1 &&
                    m_tokens[toksuper].type != TokenType::Array &&
                    m_tokens[toksuper].type != TokenType::Object) {
                for (i = toknext - 1; i >= 0; i--) {
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
                    return JSMN_ERROR_INVAL;
                }
            }
#else
        /* In non-strict mode every unquoted value is a primitive */
        default:
#endif
            r = ParsePrimitive(js, len);
            if (r < 0) {
                return r;
            }
            count++;
            if (toksuper != -1) {
                m_tokens[toksuper].size++;
            }
            break;

#ifdef JSMN_STRICT
        /* Unexpected char in strict mode */
        default:
            return JSMN_ERROR_INVAL;
#endif
        }
    }

    for (i = toknext - 1; i >= 0; i--) {
        /* Unmatched opened object or array */
        if (m_tokens[i].start != -1 && m_tokens[i].end == -1) {
            return JSMN_ERROR_PART;
        }
    }

    return count;
}
