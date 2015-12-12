/*
Copyright 2015 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <cassert>

#include <string>
#include <sstream>

#include "lexer.h"
#include "static_error.h"
#include "unicode.h"

static bool is_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static bool is_lower(char c)
{
    return c >= 'a' && c <= 'z';
}

static bool is_number(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_identifier_first(char c)
{
    return is_upper(c) || is_lower(c) || c == '_';
}

static bool is_identifier(char c)
{
    return is_identifier_first(c) || is_number(c);
}

static bool is_symbol(char c)
{
    switch (c) {
        case '&': case '|': case '^':
        case '=': case '<': case '>':
        case '*': case '/': case '%':
        case '#':
        return true;
    }
    return false;
}

std::string lex_number(const char *&c, const std::string &filename, const Location &begin)
{
    // This function should be understood with reference to the linked image:
    // http://www.json.org/number.gif

    // Note, we deviate from the json.org documentation as follows:
    // There is no reason to lex negative numbers as atomic tokens, it is better to parse them
    // as a unary operator combined with a numeric literal.  This avoids x-1 being tokenized as
    // <identifier> <number> instead of the intended <identifier> <binop> <number>.

    enum State {
        BEGIN,
        AFTER_ZERO,
        AFTER_ONE_TO_NINE,
        AFTER_DOT,
        AFTER_DIGIT,
        AFTER_E,
        AFTER_EXP_SIGN,
        AFTER_EXP_DIGIT
    } state;

    std::string r;

    state = BEGIN;
    while (true) {
        switch (state) {
            case BEGIN:
            switch (*c) {
                case '0':
                state = AFTER_ZERO;
                break;

                case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                state = AFTER_ONE_TO_NINE;
                break;

                default:
                throw StaticError(filename, begin, "Couldn't lex number");
            }
            break;

            case AFTER_ZERO:
            switch (*c) {
                case '.':
                state = AFTER_DOT;
                break;

                case 'e': case 'E':
                state = AFTER_E;
                break;

                default:
                goto end;
            }
            break;

            case AFTER_ONE_TO_NINE:
            switch (*c) {
                case '.':
                state = AFTER_DOT;
                break;

                case 'e': case 'E':
                state = AFTER_E;
                break;

                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                state = AFTER_ONE_TO_NINE;
                break;

                default:
                goto end;
            }
            break;

            case AFTER_DOT:
            switch (*c) {
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                state = AFTER_DIGIT;
                break;

                default: {
                    std::stringstream ss;
                    ss << "Couldn't lex number, junk after decimal point: " << *c;
                    throw StaticError(filename, begin, ss.str());
                }
            }
            break;

            case AFTER_DIGIT:
            switch (*c) {
                case 'e': case 'E':
                state = AFTER_E;
                break;

                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                state = AFTER_DIGIT;
                break;

                default:
                goto end;
            }
            break;

            case AFTER_E:
            switch (*c) {
                case '+': case '-':
                state = AFTER_EXP_SIGN;
                break;

                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                state = AFTER_EXP_DIGIT;
                break;

                default: {
                    std::stringstream ss;
                    ss << "Couldn't lex number, junk after 'E': " << *c;
                    throw StaticError(filename, begin, ss.str());
                }
            }
            break;

            case AFTER_EXP_SIGN:
            switch (*c) {
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                state = AFTER_EXP_DIGIT;
                break;

                default: {
                    std::stringstream ss;
                    ss << "Couldn't lex number, junk after exponent sign: " << *c;
                    throw StaticError(filename, begin, ss.str());
                }
            }
            break;

            case AFTER_EXP_DIGIT:
            switch (*c) {
                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                state = AFTER_EXP_DIGIT;
                break;

                default:
                goto end;
            }
            break;
        }
        r += *c;
        c++;
    }
    end:
    c--;
    return r;
}

// Check that b has at least the same whitespace prefix as a and returns the amount of this
// whitespace, otherwise returns 0.  If a has no whitespace prefix than return 0.
static int whitespace_check(const char *a, const char *b)
{
    int i = 0;
    while (a[i] == ' ' || a[i] == '\t') {
        if (b[i] != a[i]) return 0;
        i++;
    }
    return i;
}

static void add_whitespace(Fodder &fodder, const char *s, size_t n)
{
    std::string ws(s, n);
    if (fodder.size() == 0 || fodder.back().kind != FodderElement::WHITESPACE) {
        fodder.emplace_back(FodderElement::WHITESPACE, ws);
    } else {
        fodder.back().data += ws;
    }
}

Tokens jsonnet_lex(const std::string &filename, const char *input)
{
    unsigned long line_number = 1;
    const char *line_start = input;

    Tokens r;

    const char *c = input;

    Fodder fodder;

    for ( ; *c!='\0' ; ++c) {
        Location begin(line_number, c - line_start + 1);
        Token::Kind kind;
        std::string data;
        std::string string_block_indent;
        std::string string_block_term_indent;

        switch (*c) {

            // Skip non-\n whitespace
            case ' ': case '\t': case '\r':
            add_whitespace(fodder, c, 1);
            continue;

            // Skip \n and maintain line numbers
            case '\n':
            add_whitespace(fodder, c, 1);
            line_number++;
            line_start = c+1;
            continue;

            case '{':
            kind = Token::BRACE_L;
            data = "{";
            break;

            case '}':
            kind = Token::BRACE_R;
            data = "}";
            break;

            case '[':
            kind = Token::BRACKET_L;
            data = "[";
            break;

            case ']':
            kind = Token::BRACKET_R;
            data = "]";
            break;

            case ':':
            kind = Token::COLON;
            data = ":";
            break;

            case ',':
            kind = Token::COMMA;
            data = ",";
            break;

            case '$':
            kind = Token::DOLLAR;
            data = "$";
            break;

            case '.':
            kind = Token::DOT;
            data = ".";
            break;

            case '(':
            kind = Token::PAREN_L;
            data = "(";
            break;

            case ')':
            kind = Token::PAREN_R;
            data = ")";
            break;

            case ';':
            kind = Token::SEMICOLON;
            data = ";";
            break;

            // Special cases for unary operators.
            case '!':
            kind = Token::OPERATOR;
            if (*(c+1) == '=') {
                c++;
                data = "!=";
            } else {
                data = "!";
            }
            break;

            case '~':
            kind = Token::OPERATOR;
            data = "~";
            break;

            case '+':
            kind = Token::OPERATOR;
            data = "+";

            break;
            case '-':
            kind = Token::OPERATOR;
            data = "-";
            break;

            // Numeric literals.
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            kind = Token::NUMBER;
            data = lex_number(c, filename, begin);
            break;

            // String literals.
            case '"': {
                c++;
                for (; ; ++c) {
                    if (*c == '\0') {
                        throw StaticError(filename, begin, "Unterminated string");
                    }
                    if (*c == '"') {
                        break;
                    }
                    if (*c == '\\' && *(c+1) != '\0') {
                        data += *c;
                        ++c;
                    }
                    if (*c == '\n') {
                        // Maintain line/column counters.
                        line_number++;
                        line_start = c+1;
                    }
                    data += *c;
                }
                kind = Token::STRING_DOUBLE;
            }
            break;

            // String literals.
            case '\'': {
                c++;
                for (; ; ++c) {
                    if (*c == '\0') {
                        throw StaticError(filename, begin, "Unterminated string");
                    }
                    if (*c == '\'') {
                        break;
                    }
                    if (*c == '\\' && *(c+1) != '\0') {
                        data += *c;
                        ++c;
                    }
                    if (*c == '\n') {
                        // Maintain line/column counters.
                        line_number++;
                        line_start = c+1;
                    }
                    data += *c;
                }
                kind = Token::STRING_SINGLE;
            }
            break;

            // Keywords
            default:
            if (is_identifier_first(*c)) {
                std::string id;
                for (; *c != '\0' ; ++c) {
                    if (!is_identifier(*c)) {
                        break;
                    }
                    id += *c;
                }
                --c;
                if (id == "assert") {
                    kind = Token::ASSERT;
                } else if (id == "else") {
                    kind = Token::ELSE;
                } else if (id == "error") {
                    kind = Token::ERROR;
                } else if (id == "false") {
                    kind = Token::FALSE;
                } else if (id == "for") {
                    kind = Token::FOR;
                } else if (id == "function") {
                    kind = Token::FUNCTION;
                } else if (id == "if") {
                    kind = Token::IF;
                } else if (id == "import") {
                    kind = Token::IMPORT;
                } else if (id == "importstr") {
                    kind = Token::IMPORTSTR;
                } else if (id == "in") {
                    kind = Token::IN;
                } else if (id == "local") {
                    kind = Token::LOCAL;
                } else if (id == "null") {
                    kind = Token::NULL_LIT;
                } else if (id == "self") {
                    kind = Token::SELF;
                } else if (id == "super") {
                    kind = Token::SUPER;
                } else if (id == "tailstrict") {
                    kind = Token::TAILSTRICT;
                } else if (id == "then") {
                    kind = Token::THEN;
                } else if (id == "true") {
                    kind = Token::TRUE;
                } else {
                    // Not a keyword, must be an identifier.
                    kind = Token::IDENTIFIER;
                }
                data = id;
            } else if (is_symbol(*c)) {

                // Single line C++ style comment
                if (*c == '/' && *(c+1) == '/') {
                    const char *start = c + 2;
                    while (*c != '\0' && *c != '\n') {
                        ++c;
                    }
                    // Do not include the // or \n in the comment.
                    fodder.emplace_back(FodderElement::COMMENT_CPP, std::string(start, c - start));
                    // Leaving it on the \n allows processing of \n on next iteration,
                    // i.e. managing of the line & column counter.
                    c--;  // Will be iterated again by the for loop.
                    continue;
                }

                // Single line # comment
                if (*c == '#') {
                    const char *start = c + 1;
                    while (*c != '\0' && *c != '\n') {
                        ++c;
                    }
                    // Do not include the # or \n in the comment.
                    fodder.emplace_back(FodderElement::COMMENT_HASH, std::string(start, c - start));
                    // Leaving it on the \n allows processing of \n on next iteration,
                    // I.e., managing of the line & column counter.
                    c--;  // Will be iterated again by the for loop.
                    continue;
                }

                // Multi-line comment.
                if (*c == '/' && *(c+1) == '*') {
                    c += 2;  // Avoid matching /*/: skip the /* before starting the search for */.
                    const char *start = c;
                    while (*c != '\0' && !(*c == '*' && *(c+1) == '/')) {
                        if (*c == '\n') {
                            // Just keep track of the line / column counters.
                            line_number++;
                            line_start = c+1;
                        }
                        ++c;
                    }
                    if (*c == '\0') {
                        auto msg = "Multi-line comment has no terminating */.";
                        throw StaticError(filename, begin, msg);
                    }
                    // Do not include the /* or the */ in the comment.
                    fodder.emplace_back(FodderElement::COMMENT_C,
                                        std::string(start, c - start));
                    // Move the counter from the * to the closing /.
                    c++;
                    continue;
                }
                // Text block
                if (*c == '|' && *(c+1) == '|' && *(c+2) == '|' && *(c+3) == '\n') {
                    std::stringstream block;
                    c += 4; // Skip the "|||\n"
                    line_number++;
                    line_start = c;
                    const char *first_line = c;
                    int ws_chars = whitespace_check(first_line, c);
                    string_block_indent = std::string(first_line, ws_chars);
                    if (ws_chars == 0) {
                        auto msg = "Text block's first line must start with whitespace.";
                        throw StaticError(filename, begin, msg);
                    }
                    while (true) {
                        assert(ws_chars > 0);
                        // Read up to the \n
                        for (c = &c[ws_chars]; *c != '\n' ; ++c) {
                            if (*c == '\0')
                                throw StaticError(filename, begin, "Unexpected EOF");
                            block << *c;
                        }
                        // Add the \n
                        block << '\n';
                        ++c;
                        line_number++;
                        line_start = c;
                        // Examine next line
                        ws_chars = whitespace_check(first_line, c);
                        if (ws_chars == 0) {
                            // End of text block
                            // Skip over any whitespace
                            while (*c == ' ' || *c == '\t') {
                                string_block_term_indent += *c;
                                ++c;
                            }
                            // Expect |||
                            if (!(*c == '|' && *(c+1) == '|' && *(c+2) == '|')) {
                                auto msg = "Text block not terminated with |||";
                                throw StaticError(filename, begin, msg);
                            }
                            c += 2;  // Leave on the last |
                            data = block.str();
                            kind = Token::STRING_BLOCK;
                            break;
                        }
                    }

                    break;  // Out of the switch.
                }

                for (; *c != '\0' ; ++c) {
                    if (!is_symbol(*c)) {
                        break;
                    }
                    data += *c;
                }
                --c;
                kind = Token::OPERATOR;
            } else {
                std::stringstream ss;
                ss << "Could not lex the character ";
                auto uc = (unsigned char)(*c);
                if (*c < 32)
                    ss << "code " << unsigned(uc);
                else
                    ss << "'" << *c << "'";
                throw StaticError(filename, begin, ss.str());
            }
            break;
        }

        Location end(line_number, c - line_start + 1);
        r.emplace_back(kind, fodder, data, string_block_indent, string_block_term_indent,
                       LocationRange(filename, begin, end));
        fodder.clear();
    }

    Location end(line_number, c - line_start + 1);
    r.emplace_back(Token::END_OF_FILE, fodder, "", "", "", LocationRange(filename, end, end));
    return r;
}

std::string jsonnet_unlex(const Tokens &tokens)
{
    std::stringstream ss;
    for (const auto &t : tokens) {
        for (const auto &f : t.fodder) {
            switch (f.kind) {
                case FodderElement::WHITESPACE:
                ss << f.data;
                break;

                case FodderElement::COMMENT_C:
                ss << "/*" << f.data << "*/";
                break;

                case FodderElement::COMMENT_CPP:
                ss << "//" << f.data;
                break;

                case FodderElement::COMMENT_HASH:
                ss << "#" << f.data;
                break;
            }
        }
        if (t.kind == Token::END_OF_FILE) {
            break;
        }
        if (t.kind == Token::STRING_DOUBLE) {
            ss << "\"" << t.data << "\"";
        } else if (t.kind == Token::STRING_SINGLE) {
            ss << "'" << t.data << "'";
        } else if (t.kind == Token::STRING_BLOCK) {
            ss << "|||\n";
            ss << t.stringBlockIndent;
            for (const char *cp = t.data.c_str() ; *cp != '\0' ; ++cp) {
                ss << *cp;
                if (*cp == '\n' && *(cp + 1) != U'\0') {
                    ss << t.stringBlockIndent;
                }
            }
            ss << t.stringBlockTermIndent << "|||";
        } else {
            ss << t.data;
        }
    }
    return ss.str();
}
