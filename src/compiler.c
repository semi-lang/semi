// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./compiler.h"

#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "./darray.h"
#include "./instruction.h"
#include "./primitives.h"
#include "./semi_common.h"
#include "./symbol_table.h"
#include "./utf8.h"
#include "./value.h"
#include "./vm.h"
#include "semi/error.h"

/*
 │ Error Handling
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region Error Handling

#ifdef SEMI_DEBUG_MSG
#define SEMI_COMPILE_ABORT(compiler, errId, msg)   \
    do {                                           \
        (compiler)->errorJmpBuf.message = (msg);   \
        (compiler)->errorJmpBuf.errorId = (errId); \
        longjmp((compiler)->errorJmpBuf.env, 1);   \
        SEMI_UNREACHABLE();                        \
    } while (0)

#define SEMI_LEXER_ERROR(lexer, errId, msg)               \
    do {                                                  \
        (lexer)->compiler->errorJmpBuf.errorId = (errId); \
        (lexer)->compiler->errorJmpBuf.message = (msg);   \
    } while (0)
#else
#define SEMI_COMPILE_ABORT(compiler, errId, msg)   \
    do {                                           \
        (compiler)->errorJmpBuf.errorId = (errId); \
        longjmp((compiler)->errorJmpBuf.env, 1);   \
        SEMI_UNREACHABLE();                        \
    } while (0)

#define SEMI_LEXER_ERROR(lexer, errId, msg)               \
    do {                                                  \
        (lexer)->compiler->errorJmpBuf.errorId = (errId); \
    } while (0)
#endif  // SEMI_DEBUG_MSG

#pragma endregion
/*
 │ Lexer
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region Lexer

typedef enum BracketType {
    BRACKET_ROUND = 0,
    BRACKET_SQUARE,
    BRACKET_CURLY,
    BRACKET_ANGLE,
} BracketType;

#define BRACKET_ROUND_MASK  ((uint32_t)(0x000000FF))
#define BRACKET_SQUARE_MASK ((uint32_t)(0x0000FF00))
#define BRACKET_CURLY_MASK  ((uint32_t)(0x00FF0000))
#define BRACKET_ANGLE_MASK  ((uint32_t)(0xFF000000))

// End of stream
#define EOZ 0

#define IS_EOF(lexer) (lexer->curr == lexer->end)

// Get the current character that is always in the range of 0-127.
// Always use this macro to get safe ascii character from the lexer.
//
// Only use `lexer->curr` directly when getting the raw unsigned char.
#define SAFE_PEEK(lexer) ((IS_EOF(lexer) || (*lexer->curr) <= 0 || (*lexer->curr) > 127) ? ((char)EOZ) : (*lexer->curr))
#define SAFE_PEEK_NEXT(lexer)                                                                   \
    ((lexer->curr + 1 == lexer->end || (*(lexer->curr + 1)) <= 0 || (*(lexer->curr + 1)) > 127) \
         ? ((char)EOZ)                                                                          \
         : (*(lexer->curr + 1)))

// Advance the lexer to the next character assuming that the current character
// is not the end of the stream and the current character is not a newline character.
#define ADVANCE_CHAR(lexer) \
    do {                    \
        lexer->curr++;      \
    } while (0)

#define IS_TYPE_IDENTIFIER(s) (((s)[0]) >= 'A' && ((s)[0]) <= 'Z')

// Map keyword strings to their corresponding Token enum values.
typedef struct {
    const char* str;
    Token token;
} KeywordMap;

typedef struct {
    const KeywordMap* keywords;
    unsigned short count;
} KeywordLengthMap;

static const KeywordMap KEYWORDS_LEN_2[] = {
    {"or", TK_OR},
    {"in", TK_IN},
    {"is", TK_IS},
    {"if", TK_IF},
    {"as", TK_AS},
    {"fn", TK_FN},
};

static const KeywordMap KEYWORDS_LEN_3[] = {
    {"and", TK_AND},
    {"for", TK_FOR},
};

static const KeywordMap KEYWORDS_LEN_4[] = {
    {"elif", TK_ELIF},
    {"else", TK_ELSE},
    {"step", TK_STEP},
    {"true", TK_TRUE},
};

static const KeywordMap KEYWORDS_LEN_5[] = {
    {"defer", TK_DEFER},
    {"raise", TK_RAISE},
    {"break", TK_BREAK},
    {"false", TK_FALSE},
    {"unset", TK_UNSET},
};

static const KeywordMap KEYWORDS_LEN_6[] = {
    {"export", TK_EXPORT},
    {"return", TK_RETURN},
    {"import", TK_IMPORT},
    {"struct", TK_STRUCT},
};

static const KeywordMap KEYWORDS_LEN_8[] = {
    {"continue", TK_CONTINUE},
};

static const KeywordLengthMap KEYWORD_MAP[] = {
    {          NULL,                                                  0}, // length 0
    {          NULL,                                                  0}, // length 1
    {KEYWORDS_LEN_2, sizeof(KEYWORDS_LEN_2) / sizeof(KEYWORDS_LEN_2[0])},
    {KEYWORDS_LEN_3, sizeof(KEYWORDS_LEN_3) / sizeof(KEYWORDS_LEN_3[0])},
    {KEYWORDS_LEN_4, sizeof(KEYWORDS_LEN_4) / sizeof(KEYWORDS_LEN_4[0])},
    {KEYWORDS_LEN_5, sizeof(KEYWORDS_LEN_5) / sizeof(KEYWORDS_LEN_5[0])},
    {KEYWORDS_LEN_6, sizeof(KEYWORDS_LEN_6) / sizeof(KEYWORDS_LEN_6[0])},
    {          NULL,                                                  0}, // length 7
    {KEYWORDS_LEN_8, sizeof(KEYWORDS_LEN_8) / sizeof(KEYWORDS_LEN_8[0])},
};

// Look up a keyword string and return its corresponding Token.
// If the string is not a keyword, return TK_NON_TOKEN.
static Token lookupKeyword(const char* str, unsigned short length) {
    if (length == 0 || length >= (sizeof(KEYWORD_MAP) / sizeof(KEYWORD_MAP[0]))) {
        return TK_NON_TOKEN;
    }

    const KeywordLengthMap* map = &KEYWORD_MAP[length];
    for (int i = 0; i < map->count; i++) {
        if (memcmp(str, map->keywords[i].str, (size_t)length) == 0) {
            return map->keywords[i].token;
        }
    }
    return TK_NON_TOKEN;
}

static void consumeHeader(Lexer* lexer) {
    // Check for Byte Order Mark (BOM)
    // BOM in UTF-8 is represented as EF BB BF (0xEF 0xBB 0xBF)
    if (lexer->end - lexer->curr >= 3 &&
        ((unsigned char)lexer->curr[0] == 0xEF && (unsigned char)lexer->curr[1] == 0xBB &&
         (unsigned char)lexer->curr[2] == 0xBF)) {
        lexer->curr += 3;
    }

    // Check for shebang (#!)
    if (lexer->end - lexer->curr >= 2 && (lexer->curr[0] == '#' && lexer->curr[1] == '!')) {
        lexer->curr += 2;

        while (lexer->curr < lexer->end) {
            uint32_t codepoint = semiUTF8NextCodepoint(&lexer->curr, (unsigned int)(lexer->end - lexer->curr));
            if (codepoint == 0) {
                SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INVALID_UTF_8, "Invalid UTF-8 sequence in shebang");
                return;
            }

            if (codepoint == (uint32_t)'\n') {
                lexer->line++;
                lexer->lineStart = lexer->curr;
                return;
            }
        }
    }
}

static void initLexer(Lexer* lexer, Compiler* compiler, const char* source, uint32_t length) {
    memset(lexer, 0, sizeof(Lexer));

    lexer->compiler         = compiler;
    lexer->lineStart        = source;
    lexer->curr             = source;
    lexer->end              = source + length;
    lexer->token            = TK_NON_TOKEN;
    lexer->ignoreSeparators = false;

    consumeHeader(lexer);
}

// Read a number until we reach a non-number character or EOF.
//
// If it is an integer, we accept the following forms:
//   1. Binary matching the regexp `0b[01_]*[01][01_]*`.
//   1. Octal matching the regexp `0o[0-7_]*[0-7][0-7_]*`.
//   2. Hexadecimal matching the regexp
//   `0x[0-9a-fA-F_]*[0-9a-fA-F][0-9a-fA-F_]*`.
//   3. Decimal matching the regexp `\d[\d_]*`
//
// If it is a floating number, we only accept the following forms:
//   1. The normal way `\d[\d_]*\.[\d_]*\d[\d_]*`.
//   1. Scientific notation `\d[\d_]*(\.[\d_]*\d[\d_]*)?e[+-]?[\d_]*\d[\d_]*`.
//
// TODO: Currently, setting the decimal point other than '.' in the locale will
//       cause error. In the future, we don't want to support decimal points
//       other that '.'.
//
// TODO: There exists a fast algorithm for parsing floating points
//       (Eisel-Lemire ParseFloat algorithm)[https://arxiv.org/abs/2101.11408].
//       We can implement it and provide a compilation option to enable it.
//
// TODO: In certain use cases, it would be more beneficial if floating numbers
//       are stored and operated as "fractions". We can implement it and provide
//       a compilation option to enable it. Alternative, we can create a new
//       type for fractions.
static Token readNumber(Lexer* lexer) {
    int base            = 10;
    bool hasExponent    = false;
    bool hasDot         = false;
    unsigned int length = 0;

    if (SAFE_PEEK(lexer) == '0') {
        lexer->buffer[length++] = '0';
        ADVANCE_CHAR(lexer);

        char c;
        while ((c = SAFE_PEEK(lexer)) == '_') {
            ADVANCE_CHAR(lexer);
        }
        switch (c) {
            case EOZ:
                lexer->tokenValue.constant = semiValueNewInt(0);
                return TK_INTEGER;
            case 'b':
                base = 2;
                break;
            case 'o':
                base = 8;
                break;
            case 'x':
                base = 16;
                break;
            case 'e':
                base                    = 10;
                lexer->buffer[length++] = c;
                hasExponent             = true;
                break;
            case '.': {
                // rule out the case of ranges like `0..10`
                char c2 = SAFE_PEEK_NEXT(lexer);
                if (c2 == '.') {
                    lexer->tokenValue.constant = semiValueNewInt(0);
                    return TK_INTEGER;
                }

                base                    = 10;
                lexer->buffer[length++] = c;
                hasDot                  = true;
                break;
            }

            default:
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                    SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INVALID_NUMBER_LITERAL, "Invalid number literal");
                    return TK_EOF;
                } else {
                    lexer->tokenValue.constant = semiValueNewInt(0);
                    return TK_INTEGER;
                }
        }
        ADVANCE_CHAR(lexer);
    }

    bool isError   = false;
    bool needDigit = true;
    char c;
    while (!isError && (c = SAFE_PEEK(lexer)) != EOZ) {
        if (c == '_') {
            ADVANCE_CHAR(lexer);
            continue;
        }

        if (length >= MAX_NUMBER_CHAR - 1) {
            isError = true;
            continue;
        }

        if (base != 10) {
            if ((base == 2 && (c == '0' || c == '1')) || (base == 8 && (c >= '0' && c <= '7')) ||
                (base == 16 && ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))) {
                lexer->buffer[length++] = c;
                needDigit               = false;
                ADVANCE_CHAR(lexer);
                continue;

            } else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                isError = true;
                continue;

            } else {
                break;
            }
        }

        if (c >= '0' && c <= '9') {
            lexer->buffer[length++] = c;
            needDigit               = false;
            ADVANCE_CHAR(lexer);
            continue;
        }

        if (needDigit) {
            isError = true;
            continue;
        }

        if (c == '.') {
            // Since ".." is also a valid token (e.g. 1..10), we first check if the next character
            // is also a dot. If it is, we return the integer without consuming the dots.
            char c2 = SAFE_PEEK_NEXT(lexer);
            if (c2 == '.') {
                break;
            }

            if (hasDot || hasExponent) {
                isError = true;
                continue;
            }
            hasDot                  = true;
            lexer->buffer[length++] = c;
            ADVANCE_CHAR(lexer);
            needDigit = true;
            continue;
        }

        if (c == 'e') {
            if (hasExponent) {
                isError = true;
                continue;
            }
            hasExponent             = true;
            lexer->buffer[length++] = c;
            ADVANCE_CHAR(lexer);
            c = SAFE_PEEK(lexer);
            if (c == '+' || c == '-') {
                lexer->buffer[length++] = c;
                ADVANCE_CHAR(lexer);
            }
            needDigit = true;
            continue;
        }

        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            isError = true;
        }
        break;
    }

    if (isError || needDigit || length == 0) {
        SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INVALID_NUMBER_LITERAL, "Invalid number literal");
        return TK_EOF;
    }

    lexer->buffer[length] = '\0';
    if (hasDot || hasExponent) {
        char* endptr;
        double d = strtod(lexer->buffer, &endptr);
        if (*endptr != '\0') {
            SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INVALID_NUMBER_LITERAL, "Invalid number literal");
            return TK_EOF;
        }
        lexer->tokenValue.constant = semiValueNewFloat(d);
        return TK_DOUBLE;
    } else {
        char* endptr;
        // TODO: Overflow and underflow handling
        int64_t i = (int64_t)strtoimax(lexer->buffer, &endptr, base);
        if (*endptr != '\0') {
            SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INVALID_NUMBER_LITERAL, "Invalid number literal");
            return TK_EOF;
        }
        lexer->tokenValue.constant = semiValueNewInt(i);
        return TK_INTEGER;
    }
}

// Read an identifier until we reach a non-identifier character or EOF.
// Return NULL if there is an error (e.g., invalid identifier).
//
// An identifier starts with a letter or underscore followed by
// letters, digits, or underscores. An identifier without letters
// are valid and considered placeholders. They can appear at the
// right side of an assignment or as a standalone expression.
static Token readIdentifier(Lexer* lexer) {
    uint8_t length   = 0;
    const char* head = lexer->curr;
    char c;
    while ((c = SAFE_PEEK(lexer)) != EOZ) {
        if ((c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
            if (length == UINT8_MAX) {
                SEMI_LEXER_ERROR(lexer, SEMI_ERROR_IDENTIFIER_TOO_LONG, "Identifier too long");
                return TK_EOF;
            }

            length++;
            ADVANCE_CHAR(lexer);
        } else {
            break;
        }
    }

    lexer->tokenValue.identifier.name   = head;
    lexer->tokenValue.identifier.length = length;

    return TK_IDENTIFIER;
}

DECLARE_DARRAY(ByteBuffer, char, uint32_t)
DEFINE_DARRAY(ByteBuffer, char, uint32_t, UINT32_MAX)

// Read a string until we reach the closing quote or EOF. Return TK_EOF
// if there is an error (e.g., unclosed string).
static Token readString(Lexer* lexer) {
    ADVANCE_CHAR(lexer);
    GC* gc           = lexer->compiler->gc;
    const char* head = lexer->curr;

    ByteBuffer buffer;
    ByteBufferInit(&buffer);

    char c;
    ErrorId errId;
    while (lexer->curr < lexer->end) {
        c = *lexer->curr;
        if (c == '\0' || c == '\n' || c == '\r') {
            break;
        }

        if (c == '"') {
            lexer->tokenValue.constant = semiValueStringCreate(gc, buffer.data, buffer.size);
            ByteBufferCleanup(gc, &buffer);

            if (SEMI_UNLIKELY(IS_INVALID(&lexer->tokenValue.constant))) {
                SEMI_LEXER_ERROR(
                    lexer, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Memory allocation failure duing lexing string");
                return TK_EOF;
            } else {
                ADVANCE_CHAR(lexer);
                return TK_STRING;
            }
        }

        if (c == '\\') {
            ADVANCE_CHAR(lexer);
            if (IS_EOF(lexer)) {
                ByteBufferCleanup(gc, &buffer);
                SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INCOMPLETE_STIRNG_ESCAPE, "Incomplete string escape");
                return TK_EOF;
            }

            char c2 = SAFE_PEEK(lexer);
            switch (c2) {
                case '"':
                    errId = ByteBufferAppend(gc, &buffer, '"');
                    break;
                case '\'':
                    errId = ByteBufferAppend(gc, &buffer, '\'');
                    break;
                case '0':
                    errId = ByteBufferAppend(gc, &buffer, '\0');
                    break;
                case 'n':
                    errId = ByteBufferAppend(gc, &buffer, '\n');
                    break;
                case 'r':
                    errId = ByteBufferAppend(gc, &buffer, '\r');
                    break;
                case 't':
                    errId = ByteBufferAppend(gc, &buffer, '\t');
                    break;

                default: {
                    ByteBufferCleanup(gc, &buffer);
                    SEMI_LEXER_ERROR(lexer, SEMI_ERROR_UNKNOWN_STIRNG_ESCAPE, "Unknown string escape");
                    return TK_EOF;
                }
            }
            if (SEMI_UNLIKELY(errId != 0)) {
                ByteBufferCleanup(gc, &buffer);
                SEMI_LEXER_ERROR(lexer, errId, "Memory allocation failure duing lexing string");
                return TK_EOF;
            }
            ADVANCE_CHAR(lexer);
            continue;
        }

        const char* curr   = lexer->curr;
        uint32_t codepoint = semiUTF8NextCodepoint(&lexer->curr, (unsigned int)(lexer->end - lexer->curr));
        if (codepoint == EOZ) {
            ByteBufferCleanup(gc, &buffer);
            SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INVALID_UTF_8, "Invalid UTF-8 sequence in string");
            return TK_EOF;
        }
        while (curr < lexer->curr) {
            if (SEMI_UNLIKELY(errId = ByteBufferAppend(gc, &buffer, *curr)) != 0) {
                ByteBufferCleanup(gc, &buffer);
                SEMI_LEXER_ERROR(lexer, errId, "Memory allocation failure duing lexing string");
                return TK_EOF;
            }
            curr++;
        }
    }

    ByteBufferCleanup(gc, &buffer);
    SEMI_LEXER_ERROR(lexer, SEMI_ERROR_UNCLOSED_STRING, "Unclosed string");
    return TK_EOF;
}

static Token consumeSpaces(Lexer* lexer) {
    if (IS_EOF(lexer)) {
        return TK_EOF;
    }

    char c;
    bool hasNewline = false;
    while ((c = SAFE_PEEK(lexer)) != EOZ) {
        switch (c) {
            case '#': {
                ADVANCE_CHAR(lexer);
                uint32_t cp;
                while ((cp = semiUTF8NextCodepoint(&lexer->curr, (unsigned int)(lexer->end - lexer->curr))) != EOZ) {
                    if (cp == '\n') {
                        lexer->curr--;  // roll back one character to behave
                                        // like peek
                        break;
                    }
                }
                break;
            }
            case '\n': {
                if (!lexer->ignoreSeparators) {
                    hasNewline = true;
                }
                ADVANCE_CHAR(lexer);
                // NOTE: line number never overflow because the maximum number of lines is limited by the length of the
                //       source code.
                lexer->line++;
                lexer->lineStart = lexer->curr;
                break;
            }
            case ' ':
            case '\t':
            case '\r':
                ADVANCE_CHAR(lexer);
                break;

            default:
                return hasNewline ? TK_SEPARATOR : TK_NON_TOKEN;
        }
    }
    if (lexer->curr != lexer->end) {
        // not end of stream, but SAFE_PEEK returned EOZ -> invalid ASCII character
        SEMI_LEXER_ERROR(lexer, SEMI_ERROR_INVALID_UTF_8, "Invalid UTF-8 sequence");
        return TK_EOF;
    }
    return hasNewline ? TK_SEPARATOR : TK_NON_TOKEN;
}

static Token nextToken(Lexer* lexer) {
    Token t;
    if (lexer->token != TK_NON_TOKEN) {
        t            = lexer->token;
        lexer->token = TK_NON_TOKEN;
        return t;
    }
    if (IS_EOF(lexer)) {
        return TK_EOF;
    }

    t = consumeSpaces(lexer);
    if (t != TK_NON_TOKEN) {
        return t;
    }
    char c = SAFE_PEEK(lexer);

    switch (c) {
        case '\n': {
            ADVANCE_CHAR(lexer);
            lexer->line++;
            lexer->lineStart = lexer->curr;
            return TK_SEPARATOR;
        }
        case '~':
            ADVANCE_CHAR(lexer);
            return TK_TILDE;

        case '?': {
            ADVANCE_CHAR(lexer);
            char next_char = SAFE_PEEK(lexer);
            if (next_char == '.') {
                ADVANCE_CHAR(lexer);
                return TK_QUESTION_DOT;
            }
            return TK_QUESTION;
        }
        case ':':
            ADVANCE_CHAR(lexer);
            if (SAFE_PEEK(lexer) == '=') {
                ADVANCE_CHAR(lexer);
                return TK_BINDING;
            }
            return TK_COLON;
        case ';':
            ADVANCE_CHAR(lexer);
            return TK_SEMICOLON;
        case '=': {
            ADVANCE_CHAR(lexer);
            if (SAFE_PEEK(lexer) == '=') {
                ADVANCE_CHAR(lexer);
                return TK_EQ;
            }
            return TK_ASSIGN;
        }
        case '!': {
            ADVANCE_CHAR(lexer);
            if (SAFE_PEEK(lexer) == '=') {
                ADVANCE_CHAR(lexer);
                return TK_NOT_EQ;
            }
            return TK_BANG;
        }
        case '<': {
            ADVANCE_CHAR(lexer);
            char next_char = SAFE_PEEK(lexer);
            if (next_char == '=') {
                ADVANCE_CHAR(lexer);
                return TK_LTE;
            } else if (next_char == '<') {
                ADVANCE_CHAR(lexer);
                return TK_DOUBLE_LEFT_ARROW;
            }
            return TK_LT;
        }
        case '>': {
            ADVANCE_CHAR(lexer);
            char next_char = SAFE_PEEK(lexer);
            if (next_char == '=') {
                ADVANCE_CHAR(lexer);
                return TK_GTE;
            } else if (next_char == '>') {
                ADVANCE_CHAR(lexer);
                return TK_DOUBLE_RIGHT_ARROW;
            }
            return TK_GT;
        }
        case '&':
            ADVANCE_CHAR(lexer);
            return TK_AMPERSAND;
        case '|':
            ADVANCE_CHAR(lexer);
            return TK_VERTICAL_BAR;
        case '+':
            ADVANCE_CHAR(lexer);
            return TK_PLUS;
        case '-':
            ADVANCE_CHAR(lexer);
            return TK_MINUS;
        case '*': {
            ADVANCE_CHAR(lexer);
            if (SAFE_PEEK(lexer) == '*') {
                ADVANCE_CHAR(lexer);
                return TK_DOUBLE_STAR;
            }
            return TK_STAR;
        }
        case '/': {
            ADVANCE_CHAR(lexer);
            if (SAFE_PEEK(lexer) == '/') {
                ADVANCE_CHAR(lexer);
                return TK_DOUBLE_SLASH;
            }
            return TK_SLASH;
        }
        case '^':
            ADVANCE_CHAR(lexer);
            return TK_CARET;
        case '%':
            ADVANCE_CHAR(lexer);
            return TK_PERCENT;
        case ',':
            ADVANCE_CHAR(lexer);
            return TK_COMMA;
        case '.':
            ADVANCE_CHAR(lexer);
            if (SAFE_PEEK(lexer) == '.') {
                ADVANCE_CHAR(lexer);
                return TK_DOUBLE_DOTS;
            }
            return TK_DOT;
        case '(':
            ADVANCE_CHAR(lexer);
            return TK_OPEN_PAREN;
        case ')':
            ADVANCE_CHAR(lexer);
            return TK_CLOSE_PAREN;
        case '{':
            ADVANCE_CHAR(lexer);
            return TK_OPEN_BRACE;
        case '}':
            ADVANCE_CHAR(lexer);
            return TK_CLOSE_BRACE;
        case '[':
            ADVANCE_CHAR(lexer);
            return TK_OPEN_BRACKET;
        case ']':
            ADVANCE_CHAR(lexer);
            return TK_CLOSE_BRACKET;

        case '"': {
            Token str = readString(lexer);
            if (str == TK_EOF) {
                return TK_EOF;
            }
            return TK_STRING;
        }

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            return readNumber(lexer);
        }

        default: {
            if (c == EOZ) {
                return TK_EOF;
            }

            Token token = readIdentifier(lexer);
            if (token == TK_EOF) {
                return TK_EOF;
            }

            token = lookupKeyword(lexer->tokenValue.identifier.name, lexer->tokenValue.identifier.length);
            if (token != TK_NON_TOKEN) {
                return token;
            }

            return IS_TYPE_IDENTIFIER(lexer->tokenValue.identifier.name) ? TK_TYPE_IDENTIFIER : TK_IDENTIFIER;
        }
    }
}

static Token peekToken(Lexer* lexer) {
    if (lexer->token == TK_NON_TOKEN) {
        lexer->token = nextToken(lexer);
    }
    return lexer->token;
}

#define MATCH_NEXT_TOKEN_OR_ABORT(compiler, expected, msg)                  \
    do {                                                                    \
        if (nextToken(&compiler->lexer) != expected) {                      \
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, msg); \
            SEMI_UNREACHABLE();                                             \
        }                                                                   \
    } while (0)

#define MATCH_PEEK_TOKEN_OR_ABORT(compiler, expected, msg)                  \
    do {                                                                    \
        if (peekToken(&compiler->lexer) != expected) {                      \
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, msg); \
            SEMI_UNREACHABLE();                                             \
        }                                                                   \
    } while (0)

static void updateBracketCount(Compiler* compiler, BracketType bracketType, bool increment) {
    uint32_t mask;
    unsigned short shift;
    switch (bracketType) {
        case BRACKET_ROUND:
            mask  = BRACKET_ROUND_MASK;
            shift = 0;
            break;
        case BRACKET_SQUARE:
            mask  = BRACKET_SQUARE_MASK;
            shift = 8;
            break;
        case BRACKET_CURLY:
            mask  = BRACKET_CURLY_MASK;
            shift = 16;
            break;
        case BRACKET_ANGLE:
            mask  = BRACKET_ANGLE_MASK;
            shift = 24;
            break;
        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Invalid bracket type");
    }
    uint32_t bracketCount = (compiler->newlineState & mask) >> shift;
    if (increment) {
        if (bracketCount >= MAX_BRACKET_COUNT) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_MAXMUM_BRACKET_REACHED, "Maximum bracket count reached");
        }
        bracketCount++;
    } else {
        if (bracketCount == 0) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Bracket count below zero");
        }
        bracketCount--;
    }
    compiler->newlineState           = (compiler->newlineState & ~mask) | (bracketCount << shift);
    compiler->lexer.ignoreSeparators = (compiler->newlineState != 0);
}

#pragma endregion

/*
 │ Code Emission
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region Code Emission

DEFINE_DARRAY(Chunk, Instruction, uint32_t, UINT32_MAX)

static inline PCLocation currentPCLocation(Compiler* compiler) {
    return compiler->currentFunction->chunk.size;
}

#include <stdio.h>

static PCLocation emitCode(Compiler* compiler, Instruction instruction) {
    PCLocation pcLocation = compiler->currentFunction->chunk.size;
    ErrorId errId         = ChunkAppend(compiler->gc, &compiler->currentFunction->chunk, instruction);
    if (errId == SEMI_ERROR_MEMORY_ALLOCATION_FAILURE) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Memory allocation failure when emitting code");
    } else if (errId == SEMI_ERROR_REACH_ALLOCATION_LIMIT) {
        SEMI_COMPILE_ABORT(compiler,
                           SEMI_ERROR_TOO_MANY_INSTRUCTIONS_FOR_JUMP,
                           "Function too large (exceeds maximum instruction count)");
    }

    return pcLocation;
}

static void rewindCode(Compiler* compiler, PCLocation pc) {
    Chunk* chunk = &compiler->currentFunction->chunk;
    if (pc > chunk->size) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Rewind PC out of bounds");
    }
    chunk->size = pc;
}

static void patchCode(Compiler* compiler, PCLocation pc, Instruction instruction) {
    Chunk* chunk = &compiler->currentFunction->chunk;
    if (pc >= chunk->size) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Patch PC out of bounds");
    }
    chunk->data[pc] = instruction;
}

static inline PCLocation emitPlaceholder(Compiler* compiler) {
    return emitCode(compiler, INSTRUCTION_NOOP());
}

static inline void overrideJumpHere(Compiler* compiler, PCLocation previous) {
    // Invariant: currentPCLocation(compiler) >= previous
    uint16_t diff = (uint16_t)(currentPCLocation(compiler) - previous);

    Instruction instruction = INSTRUCTION_JUMP(diff, true);
    patchCode(compiler, previous, instruction);
}

static inline void overrideConditionalJumpHere(Compiler* compiler,
                                               PCLocation previous,
                                               LocalRegisterId condReg,
                                               bool jumpIfTrue) {
    // Invariant: currentPCLocation(compiler) >= previous
    uint16_t diff = (uint16_t)(currentPCLocation(compiler) - previous);

    Instruction instruction = INSTRUCTION_C_JUMP(condReg, diff, jumpIfTrue, true);
    patchCode(compiler, previous, instruction);
}

static inline void emitJumpBack(Compiler* compiler, PCLocation previous) {
    // Invariant: currentPCLocation(compiler) >= previous
    uint16_t diff = (uint16_t)(currentPCLocation(compiler) - previous);

    Instruction instruction = INSTRUCTION_JUMP(diff, false);
    emitCode(compiler, instruction);
}

static inline void emitConditionalJumpBack(Compiler* compiler,
                                           PCLocation previous,
                                           LocalRegisterId condReg,
                                           bool jumpIfTrue) {
    // Invariant: currentPCLocation(compiler) >= previous
    uint16_t diff = (uint16_t)(currentPCLocation(compiler) - previous);

    Instruction instruction = INSTRUCTION_C_JUMP(condReg, diff, jumpIfTrue, false);
    emitCode(compiler, instruction);
}

#pragma endregion

/*
 │ Register Management & Variable Resolution
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region Register Management & Variable Resolution

#define IS_TOP_LEVEL(compiler) ((compiler)->currentFunction->currentBlock == &(compiler)->rootFunction.rootBlock)

DEFINE_DARRAY(VariableList, VariableDescription, uint16_t, UINT16_MAX)
DEFINE_DARRAY(UpvalueList, UpvalueDescription, uint8_t, MAX_UPVALUE_COUNT)

static LocalRegisterId reserveTempRegister(Compiler* compiler) {
    FunctionScope* currentFunction = compiler->currentFunction;
    if (currentFunction->nextRegisterId == INVALID_LOCAL_REGISTER_ID) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_LOCAL_VARIABLES, "Too many temporary registers");
    }

    LocalRegisterId registerId = currentFunction->nextRegisterId++;
    if (currentFunction->nextRegisterId > currentFunction->maxUsedRegisterCount) {
        currentFunction->maxUsedRegisterCount = currentFunction->nextRegisterId;
    }
    return registerId;
}

static inline LocalRegisterId getNextRegisterId(Compiler* compiler) {
    return compiler->currentFunction->nextRegisterId;
}

static inline void restoreNextRegisterId(Compiler* compiler, LocalRegisterId registerId) {
    compiler->currentFunction->nextRegisterId = registerId;
}

static void enterFunctionScope(Compiler* compiler, bool isDeferredFunction) {
    if (isDeferredFunction && compiler->currentFunction->isDeferredFunction) {
        // Make sure deferred functions are not nested
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_NESTED_DEFER, "Nested defer block is not allowed");
    }
    FunctionScope* newFunction = semiMalloc(compiler->gc, sizeof(FunctionScope));
    if (newFunction == NULL) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Failed to allocate memory for function scope");
    }

    FunctionScope* currentFunction = compiler->currentFunction;
    BlockScope* currentBlock       = currentFunction->currentBlock;

    newFunction->rootBlock.type               = BLOCK_SCOPE_TYPE_FN;
    newFunction->rootBlock.parent             = NULL;
    newFunction->rootBlock.variableStackStart = currentBlock->variableStackEnd;
    newFunction->rootBlock.variableStackEnd   = currentBlock->variableStackEnd;
    newFunction->currentBlock                 = &newFunction->rootBlock;

    newFunction->parent               = currentFunction;
    newFunction->nextRegisterId       = 0;
    newFunction->maxUsedRegisterCount = 0;
    newFunction->nReturns             = UINT8_MAX;
    newFunction->isDeferredFunction   = isDeferredFunction;
    ChunkInit(&newFunction->chunk);
    UpvalueListInit(&newFunction->upvalues);

    compiler->currentFunction = newFunction;
}

static void leaveFunctionScope(Compiler* compiler) {
    FunctionScope* currentFunction = compiler->currentFunction;
    FunctionScope* parentFunction  = currentFunction->parent;

    ChunkCleanup(compiler->gc, &currentFunction->chunk);
    UpvalueListCleanup(compiler->gc, &currentFunction->upvalues);
    semiFree(compiler->gc, currentFunction, sizeof(FunctionScope));
    compiler->currentFunction = parentFunction;
    compiler->variables.size  = parentFunction->currentBlock->variableStackEnd;
}

static void enterBlockScope(Compiler* compiler, BlockScope* newBlock, BlockScopeType type) {
    FunctionScope* currentFunction = compiler->currentFunction;
    BlockScope* currentBlock       = currentFunction->currentBlock;
    currentFunction->currentBlock  = newBlock;

    newBlock->parent             = currentBlock;
    newBlock->variableStackStart = currentBlock->variableStackEnd;
    newBlock->variableStackEnd   = currentBlock->variableStackEnd;
    newBlock->type               = type;
}

static void leaveBlockScope(Compiler* compiler) {
    FunctionScope* currentFunction = compiler->currentFunction;
    BlockScope* currentBlock       = currentFunction->currentBlock;
    BlockScope* parentBlock        = currentBlock->parent;

    currentFunction->currentBlock = parentBlock;
    compiler->variables.size      = parentBlock->variableStackEnd;
}

static ModuleVariableId resolveGlobalVariable(Compiler* compiler, IdentifierId identifierId) {
    IdentifierId* globalIdentifiers = compiler->globalIdentifiers->data;
    for (ModuleVariableId i = 0; i < compiler->globalIdentifiers->size; i++) {
        if (globalIdentifiers[i] == identifierId) {
            return i;
        }
    }

    return INVALID_MODULE_VARIABLE_ID;
}

static bool hasModuleVariable(Compiler* compiler, IdentifierId identifierId) {
    ValueHash hash = semiHash64Bits(identifierId);
    Value v        = semiValueNewInt(identifierId);
    return semiDictHasWithHash(&compiler->artifactModule->exports, v, hash) ||
           semiDictHasWithHash(&compiler->artifactModule->globals, v, hash);
}

static ModuleVariableId bindModuleVariable(Compiler* compiler, IdentifierId identifierId, bool isExport) {
    SemiModule* module     = compiler->artifactModule;
    ObjectDict* targetDict = isExport ? &module->exports : &module->globals;
    Value v                = semiValueNewInt(identifierId);
    ValueHash hash         = semiHash64Bits(identifierId);

    if (semiDictHasWithHash(&module->exports, semiValueNewInt(identifierId), hash)) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_VARIABLE_ALREADY_DEFINED, "Variable already defined in module exports");
    }
    if (semiDictHasWithHash(&module->globals, semiValueNewInt(identifierId), hash)) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_VARIABLE_ALREADY_DEFINED, "Variable already defined in module globals");
    }

    if (semiDictLen(targetDict) >= UINT32_MAX - 1) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_VARIABLES, "Too many module variables");
    }

    if (!semiDictSetWithHash(
            compiler->gc, targetDict, semiValueNewInt(identifierId), semiValueNewInt(identifierId), hash)) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Memory allocation failure when binding module variable");
    }

    TupleId tupleId = semiDictFindTupleId(targetDict, v, hash);
    if (tupleId < 0 && tupleId > UINT16_MAX) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_VARIABLES, "Too many module variables");
    }

    return (ModuleVariableId)tupleId;
}

static ModuleVariableId resolveModuleVariable(Compiler* compiler, IdentifierId identifierId, bool* isExport) {
    SemiModule* module = compiler->artifactModule;
    ValueHash hash     = semiHash64Bits(identifierId);
    Value v            = semiValueNewInt(identifierId);
    TupleId tupleId    = semiDictFindTupleId(&module->exports, v, hash);
    if (tupleId >= 0 && tupleId <= UINT32_MAX) {
        *isExport = true;
        return (ModuleVariableId)tupleId;
    }
    tupleId = semiDictFindTupleId(&module->globals, v, hash);
    if (tupleId >= 0 && tupleId <= UINT32_MAX) {
        *isExport = false;
        return (ModuleVariableId)tupleId;
    }

    return INVALID_MODULE_VARIABLE_ID;
}

static void bindLocalVariable(Compiler* compiler, IdentifierId identifierId, LocalRegisterId registerId) {
    for (uint16_t i = 0; i < compiler->variables.size; i++) {
        if (compiler->variables.data[i].identifierId == identifierId) {
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_VARIABLE_ALREADY_DEFINED, "Variable already defined in local scope");
        }
    }

    if (resolveGlobalVariable(compiler, identifierId) != INVALID_MODULE_VARIABLE_ID) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_VARIABLE_ALREADY_DEFINED, "Variable already defined in VM globals");
    }

    if (hasModuleVariable(compiler, identifierId)) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_VARIABLE_ALREADY_DEFINED, "Variable already defined in module scope");
    }

    VariableDescription varDesc = {
        .identifierId = identifierId,
        .registerId   = registerId,
    };
    ErrorId errId = VariableListAppend(compiler->gc, &compiler->variables, varDesc);
    if (errId == SEMI_ERROR_MEMORY_ALLOCATION_FAILURE) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Memory allocation failure when binding local variable");
    } else if (errId == SEMI_ERROR_REACH_ALLOCATION_LIMIT) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_VARIABLES, "Too many local variables in a module");
    }

    compiler->currentFunction->currentBlock->variableStackEnd = compiler->variables.size;
}

static LocalRegisterId resolveLocalVariable(Compiler* compiler, IdentifierId identifierId) {
    uint16_t variableStackStart = compiler->currentFunction->rootBlock.variableStackStart;
    uint16_t variableStackEnd   = compiler->currentFunction->currentBlock->variableStackEnd;

    for (uint16_t i = variableStackStart; i < variableStackEnd; i++) {
        if (compiler->variables.data[i].identifierId == identifierId) {
            return compiler->variables.data[i].registerId;
        }
    }

    return INVALID_LOCAL_REGISTER_ID;
}

static uint8_t addUpvalue(Compiler* compiler, FunctionScope* functionScope, uint8_t index, bool isLocal) {
    uint8_t upvalueIndex = (uint8_t)functionScope->upvalues.size;

    UpvalueDescription upvalueDesc = {
        .index   = index,
        .isLocal = isLocal,
    };
    ErrorId errId = UpvalueListAppend(compiler->gc, &functionScope->upvalues, upvalueDesc);
    if (errId == SEMI_ERROR_MEMORY_ALLOCATION_FAILURE) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Memory allocation failure when adding upvalue");
    } else if (errId == SEMI_ERROR_REACH_ALLOCATION_LIMIT) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_UPVALUES, "Too many upvalues in a function");
    }

    return upvalueIndex;
}

static uint8_t resolveUpvalue(Compiler* compiler, FunctionScope* functionScope, IdentifierId identifierId) {
    FunctionScope* parentFunction = functionScope->parent;
    if (parentFunction == NULL) {
        // No parent function scope, so no upvalue
        return INVALID_UPVALUE_ID;
    }

    // Check if the identifier is a local variable in the parent function scope
    uint16_t variableStackStart = parentFunction->rootBlock.variableStackStart;
    uint16_t variableStackEnd   = parentFunction->currentBlock->variableStackEnd;
    for (uint16_t i = variableStackStart; i < variableStackEnd; i++) {
        if (compiler->variables.data[i].identifierId == identifierId) {
            LocalRegisterId registerId = compiler->variables.data[i].registerId;
            return addUpvalue(compiler, functionScope, registerId, true);
        }
    }

    // Check if the identifier is an upvalue in the parent function scope
    uint8_t parentUpvalueIndex = resolveUpvalue(compiler, parentFunction, identifierId);
    if (parentUpvalueIndex != INVALID_UPVALUE_ID) {
        return addUpvalue(compiler, functionScope, parentUpvalueIndex, false);
    }

    return INVALID_UPVALUE_ID;
}

#pragma endregion

/*
 │ Operand Helpers
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region Operand Helpers

static void saveConstantToRegister(Compiler* compiler, Value value, LocalRegisterId reg) {
    ConstantIndex idx;
    ValueType valueType = VALUE_TYPE(&value);
    switch (valueType) {
        case VALUE_TYPE_BOOL: {
            emitCode(compiler, INSTRUCTION_LOAD_BOOL(reg, 0, AS_BOOL(&value), false));
            return;
        }
        case VALUE_TYPE_INT: {
            IntValue intValue = AS_INT(&value);
            if (intValue >= -UINT16_MAX && intValue <= UINT16_MAX) {
                emitCode(compiler,
                         INSTRUCTION_LOAD_INLINE_INTEGER(
                             reg, intValue >= 0 ? (uint16_t)intValue : (uint16_t)(-intValue), true, intValue >= 0));
                return;
            }
            goto save_to_constant_table;
        }
        case VALUE_TYPE_FLOAT: {
            goto save_to_constant_table;
        }
        case VALUE_TYPE_INLINE_STRING: {
            InlineString inlineString = AS_INLINE_STRING(&value);
            if (inlineString.length == 0) {
                emitCode(compiler, INSTRUCTION_LOAD_INLINE_STRING(reg, 0, true, false));
            } else if (inlineString.length == 1) {
                emitCode(compiler, INSTRUCTION_LOAD_INLINE_STRING(reg, (uint16_t)(inlineString.c[0]), true, false));
            } else if (inlineString.length == 2) {
                emitCode(compiler,
                         INSTRUCTION_LOAD_INLINE_STRING(
                             reg, (uint16_t)(inlineString.c[0] | (inlineString.c[1] << 8)), true, false));
            }
            return;
        }
        case VALUE_TYPE_OBJECT_STRING: {
            goto save_to_constant_table;
        }
        case VALUE_TYPE_INLINE_RANGE: {
            goto save_to_constant_table;
        }

        case VALUE_TYPE_OBJECT_RANGE: {
            goto save_to_constant_table;
        }

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Invalid constant type when saving to register");
    }

save_to_constant_table:
    idx = semiConstantTableInsert(&compiler->artifactModule->constantTable, value);
    if (idx == CONST_INDEX_INVALID) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Memory allocation failure when saving integer constant");
    }
    if (idx > MAX_OPERAND_K) {
        // TODO: Spill with OP_EXTRA_ARG
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_CONSTANTS, "Too many constants in a module");
    }

    emitCode(compiler, INSTRUCTION_LOAD_CONSTANT(reg, (uint16_t)idx, false, false));
    return;
}

// Save expressions to the given register.
static void saveExprToRegister(Compiler* compiler, PrattExpr* expr, LocalRegisterId reg) {
    switch (expr->type) {
        case PRATT_EXPR_TYPE_CONSTANT: {
            saveConstantToRegister(compiler, expr->value.constant, reg);
            return;
        }
        case PRATT_EXPR_TYPE_VAR: {
            emitCode(compiler, INSTRUCTION_MOVE(reg, expr->value.reg, 0, false, false));
            return;
        }

        case PRATT_EXPR_TYPE_REG: {
            if (reg != expr->value.reg) {
                emitCode(compiler, INSTRUCTION_MOVE(reg, expr->value.reg, 0, false, false));
                expr->value.reg = reg;
            }
            return;
        }

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Invalid expression type when saving to register");
    }
    SEMI_UNREACHABLE();
}

// First try to embed the constant in the instruction. If not, save it to a register and set
// `allocatedReg` to the register ID to allow callers to free the register.
static void saveConstantExprToOperand(Compiler* compiler, Value value, uint8_t* operand, bool* isInlineOperand) {
    switch (VALUE_TYPE(&value)) {
        case VALUE_TYPE_BOOL: {
            *operand         = AS_BOOL(&value) ? (uint8_t)(1 - INT8_MIN) : (uint8_t)(0 - INT8_MIN);
            *isInlineOperand = true;
            return;
        }

        case VALUE_TYPE_INT: {
            IntValue intValue = AS_INT(&value);
            if (intValue >= INT8_MIN && intValue <= INT8_MAX) {
                *operand         = (uint8_t)(intValue - INT8_MIN);
                *isInlineOperand = true;
                return;
            }
            break;
        }

        case VALUE_TYPE_FLOAT:
        case VALUE_TYPE_INLINE_STRING:
        case VALUE_TYPE_OBJECT_STRING:
            break;

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Invalid constant type for operand");
    }

    LocalRegisterId operandReg = reserveTempRegister(compiler);
    saveConstantToRegister(compiler, value, operandReg);
}

static void saveNonConstantExprToOperand(Compiler* compiler, PrattExpr* expr, LocalRegisterId* retReg) {
    (void)compiler;
    // If the expression is not a constant, we need to save it to a register and set the operand to
    // the register ID.
    switch (expr->type) {
        case PRATT_EXPR_TYPE_REG:
            *retReg = expr->value.reg;
            return;

        case PRATT_EXPR_TYPE_VAR: {
            *retReg = expr->value.reg;
            return;
        }

        default:
            SEMI_COMPILE_ABORT(compiler,
                               SEMI_ERROR_INTERNAL_ERROR,
                               "Invalid expression type when saving non-constant expression to operand");
    }
    SEMI_UNREACHABLE();
}

#pragma endregion

/*
 │ Parser Functions
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region Parser

typedef void (*NudFn)(Compiler* compiler, const PrattState state, PrattExpr* expr);
typedef void (*LedFn)(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* retExpr);
typedef ErrorId (*ConstantFoldingFn)(Value* a, Value* b, Value* c);
typedef Instruction (*MakeTTypeInstructionFn)(uint8_t a, uint8_t b, uint8_t c, bool kb, bool kc);

typedef struct TokenPrecedence {
    const Precedence rightPrecedence;
    const LedFn ledFn;
} TokenPrecedence;

static void accessLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* retExpr);
static void binaryBooleanLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* retExpr);
static void ternaryLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* retExpr);
static void binaryLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* retExpr);
static void indexLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* retExpr);
static void functionCallLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* retExpr);

// TOKEN_ORDER_NOTE: The size of this table is determined by the biggest token value. Please order
//                   the tokens by their value, and be sure to access the precedence only with
//                   `LEFT_PRECEDENCE` and `RIGHT_PRECEDENCE`.
static const TokenPrecedence tokenPrecedences[] = {
    [TK_PLUS]               = {    PRECEDENCE_TERM,        binaryLed},
    [TK_MINUS]              = {    PRECEDENCE_TERM,        binaryLed},
    [TK_STAR]               = {  PRECEDENCE_FACTOR,        binaryLed},
    [TK_DOUBLE_STAR]        = {PRECEDENCE_EXPONENT,        binaryLed},
    [TK_SLASH]              = {  PRECEDENCE_FACTOR,        binaryLed},
    [TK_DOUBLE_SLASH]       = {  PRECEDENCE_FACTOR,        binaryLed},
    [TK_PERCENT]            = {  PRECEDENCE_FACTOR,        binaryLed},
    [TK_AMPERSAND]          = {    PRECEDENCE_TERM,        binaryLed},
    [TK_VERTICAL_BAR]       = {    PRECEDENCE_TERM,        binaryLed},
    [TK_CARET]              = {    PRECEDENCE_TERM,        binaryLed},
    [TK_DOUBLE_LEFT_ARROW]  = {  PRECEDENCE_FACTOR,        binaryLed},
    [TK_DOUBLE_RIGHT_ARROW] = {  PRECEDENCE_FACTOR,        binaryLed},
    [TK_EQ]                 = {      PRECEDENCE_EQ,        binaryLed},
    [TK_NOT_EQ]             = {      PRECEDENCE_EQ,        binaryLed},
    [TK_LT]                 = {     PRECEDENCE_CMP,        binaryLed},
    [TK_LTE]                = {     PRECEDENCE_CMP,        binaryLed},
    [TK_GT]                 = {     PRECEDENCE_CMP,        binaryLed},
    [TK_GTE]                = {     PRECEDENCE_CMP,        binaryLed},
    [TK_QUESTION]           = { PRECEDENCE_TERNARY,       ternaryLed},
    [TK_DOT]                = {  PRECEDENCE_ACCESS,        accessLed},
    [TK_OPEN_PAREN]         = {  PRECEDENCE_ACCESS,  functionCallLed},
    [TK_OPEN_BRACKET]       = {  PRECEDENCE_ACCESS,         indexLed},
    [TK_AND]                = {     PRECEDENCE_AND, binaryBooleanLed},
    [TK_OR]                 = {      PRECEDENCE_OR, binaryBooleanLed},
    [TK_IS]                 = {      PRECEDENCE_IS,        binaryLed},
    [TK_IN]                 = {      PRECEDENCE_IN,        binaryLed},
};
#define LEFT_PRECEDENCE(token)                                                                             \
    ((token >= 0 && token < sizeof(tokenPrecedences) / sizeof(tokenPrecedences[0]))                        \
         ? tokenPrecedences[token].rightPrecedence - ((token == TK_DOUBLE_STAR || token == TK_EQ) ? 1 : 0) \
         : PRECEDENCE_INVALID)
#define RIGHT_PRECEDENCE(token)                                                     \
    ((token >= 0 && token < sizeof(tokenPrecedences) / sizeof(tokenPrecedences[0])) \
         ? tokenPrecedences[token].rightPrecedence                                  \
         : PRECEDENCE_INVALID)
#define LED_FN(token)                                                                                               \
    ((token >= 0 && token < sizeof(tokenPrecedences) / sizeof(tokenPrecedences[0])) ? tokenPrecedences[token].ledFn \
                                                                                    : NULL)

static MagicMethodsTable* getMagicMethodsTable(Compiler* compiler, Value* value) {
    BaseValueType baseType = BASE_TYPE(value);
    if (baseType >= MIN_CUSTOM_BASE_VALUE_TYPE) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Invalid type for constant folding");
    }
    return &compiler->classes->classMethods[baseType];
}

static bool isConstantExprTruthy(Compiler* compiler, PrattExpr* expr) {
    BaseValueType baseType = BASE_TYPE(&expr->value.constant);
    if (baseType >= compiler->classes->classCount) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Invalid type for constant folding");
    }

    MagicMethodsTable* table = getMagicMethodsTable(compiler, &expr->value.constant);

    Value result;
    ErrorId errorId = table->conversionMethods->toBool(compiler->gc, &result, &expr->value.constant);
    if (errorId != 0) {
        SEMI_COMPILE_ABORT(compiler, errorId, "Error during constant folding for boolean conversion");
    }

    return AS_BOOL(&result);
}

static void constantNud(Compiler* compiler, const PrattState state, PrattExpr* restrict expr) {
    (void)state;

    switch (compiler->lexer.token) {
        case TK_TRUE:
            expr->value.constant = semiValueNewBool(true);
            break;

        case TK_FALSE:
            expr->value.constant = semiValueNewBool(false);
            break;

        case TK_INTEGER:
        case TK_DOUBLE:
        case TK_STRING:
            expr->value.constant = compiler->lexer.tokenValue.constant;
            break;

        default:
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Unexpected token when parsing constant expression");
    }

    expr->type = PRATT_EXPR_TYPE_CONSTANT;
    nextToken(&compiler->lexer);  // Consume the constant token
}

static void variableNud(Compiler* compiler, const PrattState state, PrattExpr* restrict expr) {
    InternedChar* identifier  = semiSymbolTableInsert(compiler->symbolTable,
                                                     compiler->lexer.tokenValue.identifier.name,
                                                     compiler->lexer.tokenValue.identifier.length);
    IdentifierId identifierId = semiSymbolTableGetId(identifier);
    nextToken(&compiler->lexer);  // Consume the identifier token

    bool isExport;
    ModuleVariableId moduleVarId;
    if ((moduleVarId = resolveGlobalVariable(compiler, identifierId)) != INVALID_MODULE_VARIABLE_ID) {
        emitCode(compiler, INSTRUCTION_LOAD_CONSTANT(state.targetRegister, moduleVarId, false, true));
        *expr = PRATT_EXPR_REG(state.targetRegister);
        return;
    }

    if ((moduleVarId = resolveModuleVariable(compiler, identifierId, &isExport)) != INVALID_MODULE_VARIABLE_ID) {
        emitCode(compiler, INSTRUCTION_GET_MODULE_VAR(state.targetRegister, moduleVarId, false, isExport));
        *expr = PRATT_EXPR_REG(state.targetRegister);
        return;
    }

    LocalRegisterId registerId = resolveLocalVariable(compiler, identifierId);
    if (registerId != INVALID_LOCAL_REGISTER_ID) {
        *expr = PRATT_EXPR_VAR(registerId);
        return;
    }

    uint8_t upvalueId = resolveUpvalue(compiler, compiler->currentFunction, identifierId);
    if (upvalueId != INVALID_UPVALUE_ID) {
        emitCode(compiler, INSTRUCTION_GET_UPVALUE(state.targetRegister, upvalueId, 0, false, false));
        *expr = PRATT_EXPR_REG(state.targetRegister);
        return;
    }

    SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNINITIALIZED_VARIABLE, "Uninitialized variable");
}

static void typeIdentifierNud(Compiler* compiler, const PrattState state, PrattExpr* expr) {
    (void)compiler;
    (void)state;
    (void)expr;

    SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Type identifiers are not implemented yet");
}

static IdentifierId newIdentifierNud(Compiler* compiler) {
    if (nextToken(&compiler->lexer) != TK_IDENTIFIER) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected identifier");
    }
    InternedChar* identifier  = semiSymbolTableInsert(compiler->symbolTable,
                                                     compiler->lexer.tokenValue.identifier.name,
                                                     compiler->lexer.tokenValue.identifier.length);
    IdentifierId identifierId = semiSymbolTableGetId(identifier);

    return identifierId;
}

static void unaryNud(Compiler* compiler, const PrattState state, PrattExpr* restrict expr) {
    Token token = nextToken(&compiler->lexer);

    LocalRegisterId destReg = state.targetRegister;
    PrattState innerState   = {
          .targetRegister    = destReg,
          .rightBindingPower = PRECEDENCE_UNARY,
    };
    semiParseExpression(compiler, innerState, expr);

    LocalRegisterId srcReg;
    switch (expr->type) {
        case PRATT_EXPR_TYPE_CONSTANT: {
            MagicMethodsTable* table = getMagicMethodsTable(compiler, &expr->value.constant);
            ErrorId errorId;
            Value result;

            if (token == TK_BANG) {
                result = semiValueNewBool(!isConstantExprTruthy(compiler, expr));
            } else if (token == TK_MINUS) {
                if ((errorId = table->numericMethods->negate(compiler->gc, &result, &expr->value.constant)) != 0) {
                    SEMI_COMPILE_ABORT(compiler, errorId, "Error during constant folding for unary minus");
                }
            } else if (token == TK_TILDE) {
                if ((errorId = table->numericMethods->bitwiseInvert(compiler->gc, &result, &expr->value.constant)) !=
                    0) {
                    SEMI_COMPILE_ABORT(compiler, errorId, "Error during constant folding for unary bitwise invert");
                }
            } else {
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Unexpected token in unary expression");
            }

            expr->value.constant = result;
            expr->type           = PRATT_EXPR_TYPE_CONSTANT;
            return;
        }

        case PRATT_EXPR_TYPE_VAR: {
            emitCode(compiler, INSTRUCTION_MOVE(destReg, expr->value.reg, 0, false, false));
            srcReg = destReg;
            break;
        }

        case PRATT_EXPR_TYPE_REG: {
            srcReg = expr->value.reg;
            break;
        }

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected expression type in unary expression");
    }

    expr->type      = PRATT_EXPR_TYPE_REG;
    expr->value.reg = destReg;
    switch (token) {
        case TK_BANG:
            emitCode(compiler, INSTRUCTION_BOOL_NOT(destReg, srcReg, 0, false, false));
            break;
        case TK_MINUS:
            emitCode(compiler, INSTRUCTION_NEGATE(destReg, srcReg, 0, false, false));
            break;
        case TK_TILDE:
            emitCode(compiler, INSTRUCTION_BITWISE_INVERT(destReg, srcReg, 0, false, false));
            break;

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Unexpected token in unary expression");
    }
}

static void parenthesisNud(Compiler* compiler, const PrattState state, PrattExpr* restrict expr) {
    updateBracketCount(compiler, BRACKET_ROUND, true);
    nextToken(&compiler->lexer);  // Consume '('

    PrattState innerState = {
        .targetRegister    = state.targetRegister,
        .rightBindingPower = PRECEDENCE_NONE,
    };
    semiParseExpression(compiler, innerState, expr);

    if (nextToken(&compiler->lexer) != TK_CLOSE_PAREN) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected closing parenthesis");
    }

    updateBracketCount(compiler, BRACKET_ROUND, false);
}

static void ternaryLed(Compiler* compiler, const PrattState state, PrattExpr* condExpr, PrattExpr* restrict retExpr) {
    nextToken(&compiler->lexer);  // Consume the ternary operator token
    PCLocation pcAfterCond = currentPCLocation(compiler);
    PrattState innerState  = {.rightBindingPower = PRECEDENCE_TERNARY, .targetRegister = state.targetRegister};

    // We have special treatment for constant condition expressions. For truthy constant condition expression like `true
    // ? truthy_expr : falsy_expr`, we reduce it to just the truthy branch, and vice versa. Similar to constant folding,
    // this is reduced at the lexical level.
    if (condExpr->type == PRATT_EXPR_TYPE_CONSTANT) {
        if (isConstantExprTruthy(compiler, condExpr)) {
            semiParseExpression(compiler, innerState, retExpr);
            if (nextToken(&compiler->lexer) != TK_COLON) {
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected colon after truthy branch");
            }
            PCLocation pcAfterTruthyBranch = currentPCLocation(compiler);

            // We parse the falsy branch, but we rewind the code to the point after the
            // truthy branch, so the falsy branch is effectively ignored.
            PrattExpr falsyBranch;
            semiParseExpression(compiler, innerState, &falsyBranch);

            rewindCode(compiler, pcAfterTruthyBranch);
        } else {
            // We parse the truthy branch, but we rewind the code to the point after the
            // truthy branch, so the truthy branch is effectively ignored.
            PrattExpr truthyBranch;
            semiParseExpression(compiler, innerState, &truthyBranch);
            rewindCode(compiler, pcAfterCond);

            if (nextToken(&compiler->lexer) != TK_COLON) {
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected colon after truthy branch");
            }
            semiParseExpression(compiler, innerState, retExpr);
        }
        return;
    }

    // When condition is not a constant, the result must be saved to a register.
    /*
     pc ┐        ┌─
        │   cond │
        │        └─ save to R[Cond]
        │        c-jmp if R[Cond] is falsy ───────────────┐    // pcAfterCond
        v        ┌─                                       │
             lhs │                                        │
                 └─ save to R[A]                          │
                 jump ──────────────────────────────────────┐  // pcAfterTruthy
                 ┌─      <────────────────────────────────┘ │
             rhs │                                          │
                 └─ save to R[A]                            │
                 <──────────────────────────────────────────┘
     */
    LocalRegisterId condReg;
    saveNonConstantExprToOperand(compiler, condExpr, &condReg);
    pcAfterCond = emitPlaceholder(compiler);

    PrattExpr truthyBranch;
    semiParseExpression(compiler, innerState, &truthyBranch);
    saveExprToRegister(compiler, &truthyBranch, innerState.targetRegister);
    PCLocation pcAfterTruthy = emitPlaceholder(compiler);
    overrideConditionalJumpHere(compiler, pcAfterCond, condReg, false);

    if (nextToken(&compiler->lexer) != TK_COLON) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected colon after truthy branch");
    }

    PrattExpr falsyBranch;
    semiParseExpression(compiler, innerState, &falsyBranch);
    saveExprToRegister(compiler, &falsyBranch, innerState.targetRegister);
    overrideJumpHere(compiler, pcAfterTruthy);

    *retExpr = PRATT_EXPR_REG(innerState.targetRegister);
}

typedef struct BinaryLedTokenData {
    const Opcode opcode;
    const MakeTTypeInstructionFn instFn;
} BinaryLedTokenData;

static const BinaryLedTokenData binaryLedTokenData[] = {
    [TK_PLUS]               = {            OP_ADD,             INSTRUCTION_ADD},
    [TK_MINUS]              = {       OP_SUBTRACT,        INSTRUCTION_SUBTRACT},
    [TK_STAR]               = {       OP_MULTIPLY,        INSTRUCTION_MULTIPLY},
    [TK_DOUBLE_STAR]        = {          OP_POWER,           INSTRUCTION_POWER},
    [TK_SLASH]              = {         OP_DIVIDE,          INSTRUCTION_DIVIDE},
    [TK_DOUBLE_SLASH]       = {   OP_FLOOR_DIVIDE,    INSTRUCTION_FLOOR_DIVIDE},
    [TK_PERCENT]            = {         OP_MODULO,          INSTRUCTION_MODULO},
    [TK_AMPERSAND]          = {    OP_BITWISE_AND,     INSTRUCTION_BITWISE_AND},
    [TK_VERTICAL_BAR]       = {     OP_BITWISE_OR,      INSTRUCTION_BITWISE_OR},
    [TK_CARET]              = {    OP_BITWISE_XOR,     INSTRUCTION_BITWISE_XOR},
    [TK_DOUBLE_LEFT_ARROW]  = {OP_BITWISE_L_SHIFT, INSTRUCTION_BITWISE_L_SHIFT},
    [TK_DOUBLE_RIGHT_ARROW] = {OP_BITWISE_R_SHIFT, INSTRUCTION_BITWISE_R_SHIFT},
    [TK_EQ]                 = {             OP_EQ,              INSTRUCTION_EQ},
    [TK_NOT_EQ]             = {            OP_NEQ,             INSTRUCTION_NEQ},
    [TK_LT]                 = {             OP_GT,              INSTRUCTION_GT},
    [TK_LTE]                = {             OP_GE,              INSTRUCTION_GE},
    [TK_GT]                 = {             OP_GT,              INSTRUCTION_GT},
    [TK_GTE]                = {             OP_GE,              INSTRUCTION_GE},
    [TK_IS]                 = {     OP_CHECK_TYPE,      INSTRUCTION_CHECK_TYPE},
    [TK_IN]                 = {        OP_CONTAIN,         INSTRUCTION_CONTAIN},
};

static void constantFolding(Compiler* compiler, Value* left, Value* right, Value* result, Token token) {
    if (IS_OBJECT_STRING(left) || IS_OBJECT_STRING(right)) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Constant folding for strings is not implemented");
    }

    MagicMethodsTable* table = getMagicMethodsTable(compiler, left);

    ErrorId errorId;
    if (token == TK_LTE) {
        errorId = semiPrimitivesDispatch2Operands(table, compiler->gc, OP_GE, result, right, left);
    } else if (token == TK_LT) {
        errorId = semiPrimitivesDispatch2Operands(table, compiler->gc, OP_GT, result, right, left);
    } else {
        errorId =
            semiPrimitivesDispatch2Operands(table, compiler->gc, binaryLedTokenData[token].opcode, result, left, right);
    }

    if (errorId != 0) {
        SEMI_COMPILE_ABORT(compiler, errorId, "Error during constant folding");
    }
}

// |  LHS   |  token  |  RHS  |
// | truthy |   and   |  any  | -> return RHS
// | truthy |   or    |  any  | -> return LHS
// | falsy  |   and   |  any  | -> return LHS
// | falsy  |   or    |  any  | -> return RHS
static void binaryBooleanLed(Compiler* compiler,
                             const PrattState state,
                             PrattExpr* leftExpr,
                             PrattExpr* restrict retExpr) {
    Token token = compiler->lexer.token;
    nextToken(&compiler->lexer);  // Consume the binary operator token

    if (leftExpr->type == PRATT_EXPR_TYPE_CONSTANT) {
        bool isLeftTruthy = isConstantExprTruthy(compiler, leftExpr);

        if ((token == TK_OR) ^ isLeftTruthy) {
            // Return RHS e.g., `0 or foo` or `2 and foo`
            //
            // Since left is a constant (no code emitted), we just ignore it.

            PrattExpr rightExpr;
            PrattState innerState = {
                .targetRegister    = state.targetRegister,
                .rightBindingPower = token == TK_AND ? PRECEDENCE_AND : PRECEDENCE_OR,
            };
            semiParseExpression(compiler, innerState, retExpr);
        } else {
            // Return LHS e.g., `3 or foo` or `0 and foo`
            //
            // Parse the RHS and ignore it.

            PCLocation pcAfterLeft = currentPCLocation(compiler);

            PrattExpr rightExpr;
            PrattState innerState = {
                .targetRegister    = state.targetRegister,
                .rightBindingPower = token == TK_AND ? PRECEDENCE_AND : PRECEDENCE_OR,
            };
            semiParseExpression(compiler, innerState, &rightExpr);
            rewindCode(compiler, pcAfterLeft);
            *retExpr = *leftExpr;
        }
    } else {
        // When LHS is not a constant, the result must be saved to a register.
        /*
         pc ┐       ┌─
            │   lhs │
            │       └─ save to R[A]
            │       c-jmp if (R[A] is truthy) ^ (token == TK_AND) ─┐
            v       ┌─                                             │
                rhs │                                              │
                    └─ save to R[A]                                │
                    <──────────────────────────────────────────────┘
         */

        saveExprToRegister(compiler, leftExpr, state.targetRegister);

        PCLocation pcAfterLeft = emitPlaceholder(compiler);

        PrattExpr rightExpr;
        PrattState innerState = {
            .targetRegister    = state.targetRegister,
            .rightBindingPower = token == TK_AND ? PRECEDENCE_AND : PRECEDENCE_OR,
        };
        semiParseExpression(compiler, innerState, &rightExpr);
        saveExprToRegister(compiler, &rightExpr, state.targetRegister);

        PCLocation pcAfterRight = currentPCLocation(compiler);
        if (pcAfterRight - pcAfterLeft >= MAX_OPERAND_K) {
            // TODO: Spill with OP_EXTRA_ARG
            SEMI_COMPILE_ABORT(compiler,
                               SEMI_ERROR_TOO_MANY_INSTRUCTIONS_FOR_JUMP,
                               "Too many instructions between logical expression and its branches");
        }
        overrideConditionalJumpHere(compiler, pcAfterLeft, state.targetRegister, token == TK_AND);

        *retExpr = PRATT_EXPR_REG(state.targetRegister);
    }
}

// This function handles binary expressions with left expressions already parsed. We are essentially creating a table
// `N(LHS expr types) * N(RHS expr types)` to cover all scenarios.
static void binaryLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* restrict retExpr) {
    Token token = nextToken(&compiler->lexer);

    Precedence rightBindingPower = RIGHT_PRECEDENCE(token);
    if (rightBindingPower == PRECEDENCE_INVALID) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Invalid token for binary expression");
    }

    LocalRegisterId regB, regC, rightTargetRegister;
    bool kb, kc;

    switch (leftExpr->type) {
        case PRATT_EXPR_TYPE_CONSTANT: {
            // We don't calculate regB and kb here because of possible constant folding.
            rightTargetRegister = state.targetRegister;
            break;
        }

        case PRATT_EXPR_TYPE_VAR: {
            regB                = leftExpr->value.reg;
            kb                  = false;
            rightTargetRegister = state.targetRegister;
            break;
        }

        case PRATT_EXPR_TYPE_REG: {
            regB                = state.targetRegister;
            kb                  = false;
            rightTargetRegister = reserveTempRegister(compiler);
            break;
        }

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected expression type in binary expression");
    }

    PrattState innerState = {
        .targetRegister    = rightTargetRegister,
        .rightBindingPower = rightBindingPower,
    };
    PrattExpr rightExpr;
    semiParseExpression(compiler, innerState, &rightExpr);

    // Handle constant folding as the special case that returns constant expression.
    if (leftExpr->type == PRATT_EXPR_TYPE_CONSTANT && rightExpr.type == PRATT_EXPR_TYPE_CONSTANT) {
        constantFolding(
            compiler, &leftExpr->value.constant, &rightExpr.value.constant, &retExpr->value.constant, token);
        retExpr->type = PRATT_EXPR_TYPE_CONSTANT;
        restoreNextRegisterId(compiler, state.targetRegister);
        return;
    }

    // RHS is not a constant, so now it's the time we save the LHS to an insturction operand and populated regB and kb.
    if (leftExpr->type == PRATT_EXPR_TYPE_CONSTANT) {
        saveConstantExprToOperand(compiler, leftExpr->value.constant, &regB, &kb);
    }
    switch (rightExpr.type) {
        case PRATT_EXPR_TYPE_CONSTANT: {
            // We already handle constant folding above, so here it can only be
            // the case for non-constant LHS.
            saveConstantExprToOperand(compiler, rightExpr.value.constant, &regC, &kc);
            break;
        }

        case PRATT_EXPR_TYPE_VAR: {
            regC = rightExpr.value.reg;
            kc   = false;
            break;
        }

        case PRATT_EXPR_TYPE_REG: {
            regC = rightTargetRegister;
            kc   = false;
            break;
        }

        default:
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected RHS expression type in binary expression");
    }

    if (token == TK_LT || token == TK_LTE) {
        emitCode(compiler, binaryLedTokenData[token].instFn(state.targetRegister, regC, regB, kc, kb));
    } else {
        emitCode(compiler, binaryLedTokenData[token].instFn(state.targetRegister, regB, regC, kb, kc));
    }
    *retExpr = PRATT_EXPR_REG(state.targetRegister);
    restoreNextRegisterId(compiler, state.targetRegister + 1);
}

static void accessLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* restrict retExpr) {
    (void)state;
    (void)leftExpr;
    (void)retExpr;

    Token token = nextToken(&compiler->lexer);  // Consume the access token
    SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Accessing object fields is not implemented");
}

static void indexLed(Compiler* compiler, const PrattState state, PrattExpr* leftExpr, PrattExpr* restrict retExpr) {
    updateBracketCount(compiler, BRACKET_SQUARE, true);
    nextToken(&compiler->lexer);  // Consume [

    LocalRegisterId indexReg, targetReg;
    switch (leftExpr->type) {
        case PRATT_EXPR_TYPE_CONSTANT:
            targetReg = state.targetRegister;
            saveExprToRegister(compiler, leftExpr, state.targetRegister);
            indexReg = reserveTempRegister(compiler);
            break;

        case PRATT_EXPR_TYPE_VAR:
            targetReg = leftExpr->value.reg;
            indexReg  = state.targetRegister;
            break;

        case PRATT_EXPR_TYPE_REG:
            targetReg = state.targetRegister;
            indexReg  = reserveTempRegister(compiler);
            break;

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected expression type in index expression");
    }

    PrattState innerState = {
        .targetRegister    = indexReg,
        .rightBindingPower = PRECEDENCE_NONE,
    };
    PrattExpr indexExpr;
    semiParseExpression(compiler, innerState, &indexExpr);

    if (indexExpr.type == PRATT_EXPR_TYPE_CONSTANT) {
        uint8_t indexOperand;
        bool isInlineOperand;
        saveConstantExprToOperand(compiler, indexExpr.value.constant, &indexOperand, &isInlineOperand);
        emitCode(compiler, INSTRUCTION_GET_ITEM(state.targetRegister, targetReg, indexOperand, false, isInlineOperand));
    } else {
        saveNonConstantExprToOperand(compiler, &indexExpr, &indexReg);
        emitCode(compiler, INSTRUCTION_GET_ITEM(state.targetRegister, targetReg, indexReg, false, false));
    }

    updateBracketCount(compiler, BRACKET_SQUARE, false);
    MATCH_NEXT_TOKEN_OR_ABORT(compiler, TK_CLOSE_BRACKET, "Expected closing bracket for index expression");

    restoreNextRegisterId(compiler, state.targetRegister + 1);
    *retExpr = PRATT_EXPR_REG(state.targetRegister);
}

// TODO: Currently we only support calling a function and returning at most one result. No error handling.
//       No varargs.
static void functionCallLed(Compiler* compiler,
                            const PrattState state,
                            PrattExpr* leftExpr,
                            PrattExpr* restrict retExpr) {
    // OPEN_PAREN ( EXPR ( COMMA EXPR )* COMMA )? CLOSE_PAREN

    saveExprToRegister(compiler, leftExpr, state.targetRegister);
    uint8_t argCount = 0;

    nextToken(&compiler->lexer);
    updateBracketCount(compiler, BRACKET_ROUND, true);

    Token t = peekToken(&compiler->lexer);
    if (t == TK_CLOSE_PAREN) {
        goto parsed_arguments;
    }

    // we have at least one argument
    while ((t = peekToken(&compiler->lexer)) != TK_EOF) {
        if (compiler->currentFunction->nextRegisterId == INVALID_LOCAL_REGISTER_ID) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_ARGUMENTS, "Too many arguments in function call");
        }
        LocalRegisterId argReg = reserveTempRegister(compiler);
        argCount++;

        PrattExpr argExpr;
        PrattState innerState = {
            .targetRegister    = argReg,
            .rightBindingPower = PRECEDENCE_NONE,
        };
        semiParseExpression(compiler, innerState, &argExpr);
        saveExprToRegister(compiler, &argExpr, argReg);
        argReg++;

        switch (t = peekToken(&compiler->lexer)) {
            case TK_COMMA:
                nextToken(&compiler->lexer);  // Consume ","
                t = peekToken(&compiler->lexer);
                if (t == TK_CLOSE_PAREN) {
                    // Allow trailing comma
                    goto parsed_arguments;
                }
                break;

            case TK_CLOSE_PAREN:
                goto parsed_arguments;

            default:
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected comma or closing parenthesis");
        }
    }

parsed_arguments:
    // Expected close parenthesis
    MATCH_NEXT_TOKEN_OR_ABORT(compiler, TK_CLOSE_PAREN, "Expected closing parenthesis for function call");
    updateBracketCount(compiler, BRACKET_ROUND, false);

    emitCode(compiler, INSTRUCTION_CALL(state.targetRegister, argCount, 0, false, false));
    *retExpr = PRATT_EXPR_REG(state.targetRegister);
    restoreNextRegisterId(compiler, state.targetRegister + 1);
}

// The algorithm for Pratt parsing. This is the entry point for expression parsing in Semi.
//
// ```python
// def parse_expr(rbp=0):
//     token = next_token()
//     left = token.nud()
//     while rbp < peek().lbp:
//         token = next_token()
//         left = token.led(left)
//     return left
// ```
//
// A big assumption about an expression is that it uses at most ONE register to store the result.
// There might be multiple temporary registers used during the evaluation of the expression,
// but they must be released before the expression evaluation is done. If the result is a temporary
// register, it must be the `state.targetRegister`, pre-allocated by the caller. All of the
// dependent functions, such as nud and led functions, must follow this rule.
void semiParseExpression(Compiler* compiler, const PrattState state, PrattExpr* restrict expr) {
    ErrorId errorId = 0;

    // 1. Create null denotation from the current token.
    Token token;
    NudFn nudFn = NULL;
    switch (token = peekToken(&compiler->lexer)) {
        case TK_STRING:
        case TK_INTEGER:
        case TK_DOUBLE:
        case TK_TRUE:
        case TK_FALSE: {
            nudFn = constantNud;
            break;
        }

        case TK_IDENTIFIER:
            nudFn = variableNud;
            break;

        case TK_BANG:
        case TK_MINUS:
        case TK_TILDE:
            nudFn = unaryNud;
            break;

        case TK_OPEN_PAREN:
            nudFn = parenthesisNud;
            break;

        case TK_TYPE_IDENTIFIER:
            nudFn = typeIdentifierNud;
            break;

        case TK_EOF:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_END_OF_FILE, "Unexpected end of file");

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Unexpected token for expression");
    }
    nudFn(compiler, state, expr);

    // 2. Compare the binding power of the current token with the minimum precedence.
    LedFn ledFn;
    Precedence leftBindingPower;
    while (true) {
        switch (token = peekToken(&compiler->lexer)) {
            // Stop tokens for expressions
            case TK_EOF:
            case TK_COLON:
            case TK_COMMA:
            case TK_SEMICOLON:
            case TK_OPEN_BRACE:
            case TK_CLOSE_BRACE:
            case TK_CLOSE_BRACKET:
            case TK_CLOSE_PAREN:
            case TK_SEPARATOR:
            case TK_DOUBLE_DOTS:
            case TK_STEP:
            case TK_BINDING:
            case TK_ASSIGN: {
                if (compiler->errorJmpBuf.errorId != 0) {
                    SEMI_COMPILE_ABORT(compiler, compiler->errorJmpBuf.errorId, compiler->errorJmpBuf.message);
                }
                return;
            }

            default: {
                ledFn            = LED_FN(token);
                leftBindingPower = LEFT_PRECEDENCE(token);
            }
        }

        if (leftBindingPower == PRECEDENCE_INVALID) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Invalid infix token for expression");
        }
        if (leftBindingPower < state.rightBindingPower) {
            return;
        }

        PrattExpr ledExpr;
        ledFn(compiler, state, expr, &ledExpr);
        *expr = ledExpr;
    }

    SEMI_UNREACHABLE();
}

static PrattExpr parseAndSaveLhsOperand(Compiler* compiler, const LhsExpr lhsExpr) {
    LocalRegisterId targetRegister;
    ModuleVariableId moduleVarId;
    switch (lhsExpr.type) {
        case LHS_EXPR_TYPE_VAR: {
            targetRegister = lhsExpr.baseRegister;
            break;
        }
        case LHS_EXPR_TYPE_MODULE_VAR:
        case LHS_EXPR_TYPE_UPVALUE:
        case LHS_EXPR_TYPE_INDEX:
        case LHS_EXPR_TYPE_FIELD: {
            targetRegister = reserveTempRegister(compiler);
            break;
        }

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected lhs expression type");
    }

    PrattState state = {
        .targetRegister    = targetRegister,
        .rightBindingPower = PRECEDENCE_NONE,
    };
    PrattExpr expr;
    semiParseExpression(compiler, state, &expr);
    saveExprToRegister(compiler, &expr, targetRegister);

    switch (lhsExpr.type) {
        case LHS_EXPR_TYPE_VAR: {
            break;
        }

        case LHS_EXPR_TYPE_MODULE_VAR: {
            emitCode(compiler,
                     INSTRUCTION_SET_MODULE_VAR(
                         targetRegister, lhsExpr.value.moduleVar.id, false, lhsExpr.value.moduleVar.isExport));
            break;
        }

        case LHS_EXPR_TYPE_UPVALUE: {
            emitCode(compiler, INSTRUCTION_SET_UPVALUE(lhsExpr.baseRegister, targetRegister, 0, false, false));
            break;
        }
        case LHS_EXPR_TYPE_INDEX: {
            emitCode(compiler,
                     INSTRUCTION_SET_ITEM(lhsExpr.baseRegister,
                                          lhsExpr.value.index.operand,
                                          targetRegister,
                                          lhsExpr.value.index.operandInlined,
                                          false));
            break;
        }
        case LHS_EXPR_TYPE_GLOBAL_VAR: {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_VARIABLE_ALREADY_DEFINED, " Cannot assign to global variable");
        }
        case LHS_EXPR_TYPE_FIELD: {
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Assigning to object fields is not implemented");
        }
        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected lhs expression type");
    }

    return expr;
}

static LocalRegisterId dereferenceLhsExpr(Compiler* compiler, LhsExpr* expr) {
    switch (expr->type) {
        case LHS_EXPR_TYPE_MODULE_VAR: {
            LocalRegisterId outReg       = reserveTempRegister(compiler);
            ModuleVariableId moduleVarId = expr->value.moduleVar.id;
            bool isExport                = expr->value.moduleVar.isExport;
            emitCode(compiler, INSTRUCTION_GET_MODULE_VAR(outReg, moduleVarId, false, isExport));
            return outReg;
        }
        case LHS_EXPR_TYPE_VAR: {
            LocalRegisterId outReg = reserveTempRegister(compiler);
            emitCode(compiler, INSTRUCTION_MOVE(outReg, expr->baseRegister, 0, false, false));
            return outReg;
        }

        case LHS_EXPR_TYPE_GLOBAL_VAR:
        case LHS_EXPR_TYPE_UPVALUE: {
            return expr->baseRegister;
        }

        case LHS_EXPR_TYPE_INDEX: {
            emitCode(compiler,
                     INSTRUCTION_GET_ITEM(expr->baseRegister,
                                          expr->baseRegister,
                                          expr->value.index.operand,
                                          false,
                                          expr->value.index.operandInlined));
            restoreNextRegisterId(compiler, expr->baseRegister + 1);
            return expr->baseRegister;
        }

        case LHS_EXPR_TYPE_FIELD: {
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Accessing object fields is not implemented");
        }

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected lhs expression type");
    }

    SEMI_UNREACHABLE();
}

void lhsIndexLed(Compiler* compiler, LhsExpr* expr, LhsExpr* restrict retExpr) {
    nextToken(&compiler->lexer);  // Consume '['
    updateBracketCount(compiler, BRACKET_SQUARE, true);

    retExpr->type         = LHS_EXPR_TYPE_INDEX;
    retExpr->baseRegister = dereferenceLhsExpr(compiler, expr);

    LocalRegisterId indexReg = reserveTempRegister(compiler);
    PrattExpr indexExpr;
    PrattState state = {
        .targetRegister    = indexReg,
        .rightBindingPower = PRECEDENCE_NONE,
    };
    semiParseExpression(compiler, state, &indexExpr);

    if (indexExpr.type == PRATT_EXPR_TYPE_CONSTANT && AS_INT(&indexExpr.value.constant) >= INT8_MIN &&
        AS_INT(&indexExpr.value.constant) <= INT8_MAX) {
        retExpr->value.index.operand        = (uint8_t)(AS_INT(&indexExpr.value.constant) - INT8_MIN);
        retExpr->value.index.operandInlined = true;
        restoreNextRegisterId(compiler, indexReg);
    } else {
        // Save indexExpr to indexReg
        saveExprToRegister(compiler, &indexExpr, indexReg);
        retExpr->value.index.operand        = indexReg;
        retExpr->value.index.operandInlined = false;
    }

    MATCH_NEXT_TOKEN_OR_ABORT(compiler, TK_CLOSE_BRACKET, "Expected closing bracket for index expression");
    updateBracketCount(compiler, BRACKET_SQUARE, false);
}

void lhsFieldLed(Compiler* compiler, LhsExpr* expr, LhsExpr* restrict retExpr) {
    nextToken(&compiler->lexer);  // Consume '.'
    MATCH_NEXT_TOKEN_OR_ABORT(compiler, TK_IDENTIFIER, "Expected identifier after '.'");

    retExpr->type         = LHS_EXPR_TYPE_FIELD;
    retExpr->baseRegister = dereferenceLhsExpr(compiler, expr);

    InternedChar* identifier       = semiSymbolTableInsert(compiler->symbolTable,
                                                     compiler->lexer.tokenValue.identifier.name,
                                                     compiler->lexer.tokenValue.identifier.length);
    IdentifierId identifierId      = semiSymbolTableGetId(identifier);
    retExpr->value.field.fieldName = identifierId;
}

// parseLhsNud may return these possible outcomes:
//  1. Uninitialized variable that can only be used for binding.
//  2. Expression.
//  3. Initialized variable to be assigned to.
//  4. Initialized module variable to be assigned to.
//  5. An lvalue representing a field to be assigned to.
//  6. An lvalue representing an index to be assigned to.
//
// Parsed result is lazily dereferenced. For example, when encountering an index access, the expression is initially
// marked as an LValueIndex. Parsing then continues, and if a subsequent field or index access is found, the previous
// result is dereferenced. Ultimately, the function returns the appropriate lvalue target (either field or index).
void parseLhsNud(Compiler* compiler, LhsExpr* restrict lhsExpr) {
    if (peekToken(&compiler->lexer) != TK_IDENTIFIER) {
        LocalRegisterId targetRegister = reserveTempRegister(compiler);
        PrattState state               = (PrattState){
                          .targetRegister    = targetRegister,
                          .rightBindingPower = PRECEDENCE_NONE,
        };
        PrattExpr expr;
        semiParseExpression(compiler, state, &expr);
        *lhsExpr = LHS_EXPR_UNASSIGNABLE(expr);
        return;
    }

    nextToken(&compiler->lexer);  // Consume the identifier
    InternedChar* identifier  = semiSymbolTableInsert(compiler->symbolTable,
                                                     compiler->lexer.tokenValue.identifier.name,
                                                     compiler->lexer.tokenValue.identifier.length);
    IdentifierId identifierId = semiSymbolTableGetId(identifier);

    bool isExport;
    ModuleVariableId moduleVarId;
    LocalRegisterId registerId;
    if ((moduleVarId = resolveGlobalVariable(compiler, identifierId)) != INVALID_MODULE_VARIABLE_ID) {
        registerId = reserveTempRegister(compiler);
        emitCode(compiler, INSTRUCTION_LOAD_CONSTANT(registerId, moduleVarId, false, true));
        *lhsExpr = LHS_EXPR_GLOBAL_VAR(registerId);
        goto parse_lhs;
    }
    if ((moduleVarId = resolveModuleVariable(compiler, identifierId, &isExport)) != INVALID_MODULE_VARIABLE_ID) {
        *lhsExpr = LHS_EXPR_MODULE_VAR(moduleVarId, isExport);
        goto parse_lhs;
    }

    registerId = resolveLocalVariable(compiler, identifierId);
    if (registerId != INVALID_LOCAL_REGISTER_ID) {
        *lhsExpr = LHS_EXPR_VAR(registerId);
        goto parse_lhs;
    }

    uint8_t upvalueId = resolveUpvalue(compiler, compiler->currentFunction, identifierId);
    if (upvalueId != INVALID_UPVALUE_ID) {
        registerId = reserveTempRegister(compiler);
        emitCode(compiler, INSTRUCTION_GET_UPVALUE(registerId, upvalueId, 0, false, false));
        *lhsExpr = LHS_EXPR_UPVALUE(registerId);
        goto parse_lhs;
    }

    // Don't bind the variable so the identifier can not be used at the right hand side.
    *lhsExpr    = LHS_EXPR_UNINIT_VAR(identifierId);
    Token token = peekToken(&compiler->lexer);
    if (token != TK_BINDING) {
        if (token == TK_ASSIGN) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_BINDING_ERROR, "Expected ':=' for new variable binding");
        } else {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNINITIALIZED_VARIABLE, "Uninitialized variable");
        }
    }
    return;

parse_lhs:
    while (true) {
        // Inside the loop, the cases for expr->type are:
        //  1. LHS_EXPR_TYPE_VAR
        //  2. LHS_EXPR_TYPE_MODULE_VAR
        //  3. LHS_EXPR_TYPE_UPVALUE
        //  4. LHS_EXPR_TYPE_FIELD
        //  5. LHS_EXPR_TYPE_INDEX
        //
        // This implementation relies on the fact that special tokens we want to handle have the highest precedence
        // than other tokens, so if the result is not an lvalue, we can still parse the PrattExpr without worrying about
        // binding powers.

        switch (token = peekToken(&compiler->lexer)) {
            case TK_OPEN_BRACKET: {
                LhsExpr temp;
                lhsIndexLed(compiler, lhsExpr, &temp);
                *lhsExpr = temp;
                break;
            }

            case TK_DOT: {
                LhsExpr temp;
                lhsFieldLed(compiler, lhsExpr, &temp);
                *lhsExpr = temp;
                break;
            }

            case TK_BINDING: {
                SEMI_COMPILE_ABORT(
                    compiler, SEMI_ERROR_VARIABLE_ALREADY_DEFINED, "Left-hand side of \":=\" must be a new variable");
            }

            case TK_ASSIGN: {
                return;
            }

            default: {
                if (lhsExpr->type == LHS_EXPR_TYPE_VAR) {
                    *lhsExpr = LHS_EXPR_UNASSIGNABLE(PRATT_EXPR_VAR(lhsExpr->baseRegister));
                } else {
                    LocalRegisterId reg = dereferenceLhsExpr(compiler, lhsExpr);
                    *lhsExpr            = LHS_EXPR_UNASSIGNABLE(PRATT_EXPR_REG(reg));
                }
                return;
            }
        }
    }

    SEMI_UNREACHABLE();
}

static PrattExpr semiParseAssignmentOrExpr(Compiler* compiler, bool isModuleExport) {
    Token token;
    LocalRegisterId currentNextRegisterId = compiler->currentFunction->nextRegisterId;
    LocalRegisterId targetRegister;
    LhsExpr lhsExpr;
    PrattExpr expr;
    parseLhsNud(compiler, &lhsExpr);

    if (isModuleExport && lhsExpr.type != LHS_EXPR_TYPE_UNINIT_VAR) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INVALID_MODULE_EXPORT, "Only new variables can be exported");
    }

    if (lhsExpr.type != LHS_EXPR_TYPE_UNASSIGNABLE) {
        token = nextToken(&compiler->lexer);  // Consume the assignment token
        if (lhsExpr.type == LHS_EXPR_TYPE_UNINIT_VAR && token != TK_BINDING) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected binding token");
        } else if (lhsExpr.type != LHS_EXPR_TYPE_UNINIT_VAR && token != TK_ASSIGN) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected assignment token");
        }

        if (lhsExpr.type == LHS_EXPR_TYPE_UNINIT_VAR) {
            if (IS_TOP_LEVEL(compiler)) {
                ModuleVariableId moduleVarId = bindModuleVariable(compiler, lhsExpr.value.identifierId, isModuleExport);
                lhsExpr                      = LHS_EXPR_MODULE_VAR(moduleVarId, isModuleExport);
            } else {
                targetRegister = reserveTempRegister(compiler);
                bindLocalVariable(compiler, lhsExpr.value.identifierId, targetRegister);
                lhsExpr = LHS_EXPR_VAR(targetRegister);
                currentNextRegisterId += 1;
            }
        }
        // Now lhsExpr is not LHS_EXPR_TYPE_UNINIT_VAR.
        expr = parseAndSaveLhsOperand(compiler, lhsExpr);

        // Stop tokens for rvalue
        token = peekToken(&compiler->lexer);
        if (token != TK_SEMICOLON && token != TK_EOF && token != TK_CLOSE_BRACE && token != TK_SEPARATOR) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Unexpected token after assignment");
        }

        restoreNextRegisterId(compiler, currentNextRegisterId);
        return expr;
    }

    expr = lhsExpr.value.rvalue;
    LedFn ledFn;
    Precedence leftBindingPower;
    PrattState state;
    switch (expr.type) {
        case PRATT_EXPR_TYPE_REG:
            targetRegister = expr.value.reg;
            break;

        case PRATT_EXPR_TYPE_VAR:
        case PRATT_EXPR_TYPE_CONSTANT:
            targetRegister = reserveTempRegister(compiler);
            break;

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected expression type");
    }

    state = (PrattState){
        .targetRegister    = targetRegister,
        .rightBindingPower = PRECEDENCE_NONE,
    };

    while (true) {
        switch (token = peekToken(&compiler->lexer)) {
            // Stop tokens for expression statement
            case TK_EOF:
            case TK_SEMICOLON:
            case TK_CLOSE_BRACE:
            case TK_SEPARATOR: {
                if (compiler->errorJmpBuf.errorId != 0) {
                    SEMI_COMPILE_ABORT(compiler, compiler->errorJmpBuf.errorId, compiler->errorJmpBuf.message);
                }
                goto parse_unassignable_end;
            }

            case TK_BINDING:
            case TK_ASSIGN: {
                SEMI_COMPILE_ABORT(compiler,
                                   SEMI_ERROR_EXPECT_LVALUE,
                                   "Left-hand side of assignment must be a variable, field, or index");
            }

            default:
                ledFn            = LED_FN(token);
                leftBindingPower = LEFT_PRECEDENCE(token);
        }

        if (leftBindingPower == PRECEDENCE_INVALID) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Invalid infix token for expression");
        }

        // Statements has lower precedence than any expression, so we don't need to compare
        // leftBindingPower with state.rightBindingPower.
        PrattExpr ledExpr;
        ledFn(compiler, state, &expr, &ledExpr);
        expr = ledExpr;
    }

parse_unassignable_end:

    token = peekToken(&compiler->lexer);
    while (token == TK_SEMICOLON || token == TK_SEPARATOR) {
        nextToken(&compiler->lexer);  // Consume the semicolon or separator
        token = peekToken(&compiler->lexer);
    }

    if (compiler->artifactModule->moduleId == SEMI_REPL_MODULE_ID && token == TK_EOF &&
        compiler->errorJmpBuf.errorId == 0) {
        saveExprToRegister(compiler, &expr, targetRegister);
        emitCode(compiler, INSTRUCTION_RETURN(targetRegister, 0, 0, false, false));
    }
    restoreNextRegisterId(compiler, currentNextRegisterId);
    return expr;
}

static int structFieldCompare(const void* a, const void* b) {
    StructField* fa = (StructField*)a;
    StructField* fb = (StructField*)b;

    IdentifierId fa_id = semiSymbolTableGetId(fa->name);
    IdentifierId fb_id = semiSymbolTableGetId(fb->name);
    return (fa_id > fb_id) - (fa_id < fb_id);
}

static void parseStruct(Compiler* compiler) {
    SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Structs are not implemented");
}

static void parseScopedStatements(Compiler* compiler);

static void parseIf(Compiler* compiler) {
    /*
     pc ┐          calculate the condition for if
        │       ┌─ c-jmp if the condition is falsy ───────┐      // pcAfterCond
        │       │                                         │
        │    if │                                         │
        │       └─                                        │
        v          jump to end ────────────────────────── │ ─>┐  // pcJumpLocation_1
                   calculate the condition for elif      <┘   │
                ┌─ c-jmp if the condition is falsy ────┐      │  // pcAfterCond
           elif │                                      │      │
                └─                                     │      │
                   jump to end ─────────────────────── │ ────>┤  // pcJumpLocation_2
                ┌─  <──────────────────────────────────┘      │
                │                                             │
           else │                                             │
                └─                                            │
                   close_upvalues <───────────────────────────┘
    */

    Token t;
    PCLocation patchHead = INVALID_PC_LOCATION;

    // If-blocks don't increase or decrease the number of variables and registers used, so we choose
    // to write a single CLOSE_UPVALUES instruction at the end of the if-elif-else chain to close
    // all upvalues opened in the chain. If there are other control flow statements that leave the
    // scope, they need to take care of closing upvalues of this scope. Since block scopes have
    // stack semantics, their implementation should just work.
    LocalRegisterId currentNextRegisterId = getNextRegisterId(compiler);

    do {
        nextToken(&compiler->lexer);  // Consume if / elif

        LocalRegisterId condReg, targetReg;
        condReg              = reserveTempRegister(compiler);
        PrattState condState = {
            .targetRegister    = condReg,
            .rightBindingPower = PRECEDENCE_NONE,
        };
        PrattExpr condExpr;
        semiParseExpression(compiler, condState, &condExpr);
        switch (condExpr.type) {
            case PRATT_EXPR_TYPE_CONSTANT: {
                targetReg = condReg;
                saveExprToRegister(compiler, &condExpr, condReg);
                break;
            }
            case PRATT_EXPR_TYPE_VAR:
                targetReg = condExpr.value.reg;
                break;
            case PRATT_EXPR_TYPE_REG:
                targetReg = condReg;
                break;
            default:
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Unexpected expression type in if condition");
        }
        restoreNextRegisterId(compiler, currentNextRegisterId);

        PCLocation pcAfterCond = emitPlaceholder(compiler);

        if (peekToken(&compiler->lexer) != TK_OPEN_BRACE) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected opening brace for if body");
        }

        BlockScope blockScope;
        enterBlockScope(compiler, &blockScope, BLOCK_SCOPE_TYPE_IF);
        parseScopedStatements(compiler);
        leaveBlockScope(compiler);
        restoreNextRegisterId(compiler, currentNextRegisterId);

        // If it's not the end of the if-elif-else chain, we need to provide the jump-to-end
        // instruction.
        t = peekToken(&compiler->lexer);
        if (t == TK_ELIF || t == TK_ELSE) {
            patchHead = emitCode(compiler, INSTRUCTION_JUMP(patchHead, false));
        }

        overrideConditionalJumpHere(compiler, pcAfterCond, targetReg, false);
    } while (t == TK_ELIF);

    if (t == TK_ELSE) {
        MATCH_NEXT_TOKEN_OR_ABORT(compiler, TK_ELSE, "Expected 'else' token");

        IfScope ifScope;
        enterBlockScope(compiler, (BlockScope*)&ifScope, BLOCK_SCOPE_TYPE_IF);
        parseScopedStatements(compiler);
        leaveBlockScope(compiler);
        restoreNextRegisterId(compiler, currentNextRegisterId);
    }

    while (patchHead != INVALID_PC_LOCATION) {
        PCLocation nextPatch = OPERAND_J_J(compiler->currentFunction->chunk.data[patchHead]);
        overrideJumpHere(compiler, patchHead);
        patchHead = nextPatch;
    }

    emitCode(compiler, INSTRUCTION_CLOSE_UPVALUES(currentNextRegisterId, 0, 0, false, false));
}

static void saveRangeOperand(
    Compiler* compiler, PrattExpr expr, LocalRegisterId reg, uint8_t* restrict operand, bool* restrict operandInline) {
    if (expr.type == PRATT_EXPR_TYPE_CONSTANT) {
        if (IS_INT(&expr.value.constant)) {
            IntValue intValue = AS_INT(&expr.value.constant);
            if (intValue >= INT8_MIN && intValue <= INT8_MAX) {
                *operand       = (uint8_t)(intValue - INT8_MIN);
                *operandInline = true;
                return;
            }
        } else if (!IS_NUMBER(&expr.value.constant)) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INVALID_VALUE, "Range operands must be numbers");
        }
    } else if (expr.type == PRATT_EXPR_TYPE_VAR) {
        *operand       = (uint8_t)expr.value.reg;
        *operandInline = false;
        return;
    }

    saveExprToRegister(compiler, &expr, reg);
    *operand       = (uint8_t)reg;
    *operandInline = false;
}

static void parseRangeOrIter(Compiler* compiler, LocalRegisterId iterReg) {
    PrattState state = {
        .targetRegister    = iterReg,
        .rightBindingPower = PRECEDENCE_NON_KEYWORD,
    };
    PrattExpr iterExpr;
    semiParseExpression(compiler, state, &iterExpr);
    if (peekToken(&compiler->lexer) == TK_DOUBLE_DOTS) {
        // for ... in startExpr..endExpr [step stepExpr]
        nextToken(&compiler->lexer);  // Consume ".."

        PCLocation pcBeforeIter = currentPCLocation(compiler);

        saveExprToRegister(compiler, &iterExpr, iterReg);
        uint8_t endOperand, stepOperand;
        bool endOperandInline, stepOperandInline;

        LocalRegisterId endReg, stepReg;
        endReg              = reserveTempRegister(compiler);
        stepReg             = reserveTempRegister(compiler);
        PrattState endState = {
            .targetRegister    = endReg,
            .rightBindingPower = PRECEDENCE_NON_KEYWORD,
        };
        PrattExpr endExpr, stepExpr;
        semiParseExpression(compiler, endState, &endExpr);
        saveRangeOperand(compiler, endExpr, endReg, &endOperand, &endOperandInline);

        if (peekToken(&compiler->lexer) == TK_STEP) {
            nextToken(&compiler->lexer);  // Consume "step"
            PrattState stepState = {
                .targetRegister    = stepReg,
                .rightBindingPower = PRECEDENCE_NON_KEYWORD,
            };
            semiParseExpression(compiler, stepState, &stepExpr);
        } else {
            stepExpr = PRATT_EXPR_CONSTANT(semiValueNewInt(1));
        }
        saveRangeOperand(compiler, stepExpr, stepReg, &stepOperand, &stepOperandInline);

        if (iterExpr.type == PRATT_EXPR_TYPE_CONSTANT && endExpr.type == PRATT_EXPR_TYPE_CONSTANT &&
            stepExpr.type == PRATT_EXPR_TYPE_CONSTANT) {
            rewindCode(compiler, pcBeforeIter);

            Value rangeConstant = semiValueRangeCreate(
                compiler->gc, iterExpr.value.constant, endExpr.value.constant, stepExpr.value.constant);
            if (IS_INVALID(&rangeConstant)) {
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INVALID_VALUE, "Failed to allocate range");
            }
            iterExpr = PRATT_EXPR_CONSTANT(rangeConstant);
            saveExprToRegister(compiler, &iterExpr, iterReg);
        } else {
            emitCode(compiler,
                     INSTRUCTION_MAKE_RANGE(iterReg, endOperand, stepOperand, endOperandInline, stepOperandInline));
        }
    } else {
        saveExprToRegister(compiler, &iterExpr, iterReg);
    }

    restoreNextRegisterId(compiler, iterReg + 1);
}

typedef struct ForHeader {
    LocalRegisterId indexReg;
    LocalRegisterId itemReg;
} ForHeader;

static ForHeader parseForHeader(Compiler* compiler, LocalRegisterId iterReg) {
    nextToken(&compiler->lexer);  // Consume "for"

    if (peekToken(&compiler->lexer) == TK_OPEN_BRACE) {
        // for { ... }
        return (ForHeader){
            .indexReg = INVALID_LOCAL_REGISTER_ID,
            .itemReg  = INVALID_LOCAL_REGISTER_ID,
        };
    }

    LocalRegisterId firstReg, secondReg;
    IdentifierId firstIdentifierId, secondIdentifierId;
    firstIdentifierId = newIdentifierNud(compiler);

    bool hasIndexVar;
    if (peekToken(&compiler->lexer) == TK_COMMA) {
        // for i, item in ...
        hasIndexVar = true;
        nextToken(&compiler->lexer);  // Consume ","
        secondIdentifierId = newIdentifierNud(compiler);
    } else {
        // for item in ...
        hasIndexVar = false;
    }
    MATCH_NEXT_TOKEN_OR_ABORT(compiler, TK_IN, "Expect 'in'");

    parseRangeOrIter(compiler, iterReg);

    firstReg = reserveTempRegister(compiler);
    bindLocalVariable(compiler, firstIdentifierId, firstReg);

    if (hasIndexVar) {
        secondReg = reserveTempRegister(compiler);
        bindLocalVariable(compiler, secondIdentifierId, secondReg);

        return (ForHeader){
            .indexReg = secondReg,
            .itemReg  = firstReg,
        };
    } else {
        return (ForHeader){
            .indexReg = INVALID_LOCAL_REGISTER_ID,
            .itemReg  = firstReg,
        };
    }
}

static void parseFor(Compiler* compiler) {
    /*
     pc ┐       make_range / get_item
        │       iter_next ─┐ <──┬────┐
        │       jump ───── │ ── │ ── │ ───┐
        │       ┌─  <──────┘    │    │    │
        v       │               │    │    │
                │   continue ───┘    │    │
           loop │                    │    │
                │   break ────────── │ ── │ ──┐
                │                    │    │   │
                └─                   │    │   │
                jump ────────────────┘    │   │
                close_upvalues <──────────┘ <─┘

    Note that `iter_next` also closes upvalues for each iteration. This is crucial to ensure
    upvalues referencing unique variable instances if they are introduced in the loop body.

    If it is an infinite for-loop, the structure is simpler:

     pc ┐       ┌─  <───────────┬────┐
        │       │               │    │
        │       │   continue ───┘    │
        │  loop │                    │
        v       │   break ────────── │ ──┐
                │                    │   │
                └─                   │   │
                jump ────────────────┘   │
                close_upvalues <─────────┘
    */

    // For-blocks don't increase or decrease the number of variables and registers used, so we
    // choose to write a single CLOSE_UPVALUES instruction at the end of the block to close all
    // opened upvalues. If there are other control flow statements that leave the scope, they need
    // to take care of closing upvalues of this scope. Since block scopes have stack semantics,
    // their implementation should just work.
    LocalRegisterId currentNextRegisterId = getNextRegisterId(compiler);
    LocalRegisterId iterReg               = reserveTempRegister(compiler);

    LoopScope loopScope;
    enterBlockScope(compiler, (BlockScope*)&loopScope, BLOCK_SCOPE_TYPE_LOOP);

    ForHeader forHeader = parseForHeader(compiler, iterReg);

    if (forHeader.indexReg == INVALID_LOCAL_REGISTER_ID && forHeader.itemReg == INVALID_LOCAL_REGISTER_ID) {
        loopScope.loopStartLocation    = currentPCLocation(compiler);
        loopScope.previousJumpLocation = INVALID_PC_LOCATION;
    } else {
        loopScope.loopStartLocation =
            emitCode(compiler, INSTRUCTION_ITER_NEXT(forHeader.indexReg, forHeader.itemReg, iterReg, false, false));
        loopScope.previousJumpLocation = emitCode(compiler, INSTRUCTION_JUMP(INVALID_PC_LOCATION, false));
    }

    MATCH_PEEK_TOKEN_OR_ABORT(compiler, TK_OPEN_BRACE, "Expected opening brace for for body");
    parseScopedStatements(compiler);

    emitJumpBack(compiler, loopScope.loopStartLocation);
    leaveBlockScope(compiler);

    while (loopScope.previousJumpLocation != INVALID_PC_LOCATION) {
        PCLocation temp                = loopScope.previousJumpLocation;
        loopScope.previousJumpLocation = OPERAND_J_J(compiler->currentFunction->chunk.data[temp]);
        overrideJumpHere(compiler, temp);
    }
    emitCode(compiler, INSTRUCTION_CLOSE_UPVALUES(currentNextRegisterId, 0, 0, false, false));

    restoreNextRegisterId(compiler, currentNextRegisterId);
}

static void parseFunction(Compiler* compiler, bool isModuleExport) {
    nextToken(&compiler->lexer);  // Consume "fn"

    // Get function name
    IdentifierId fnIdentifierId = newIdentifierNud(compiler);
    if (nextToken(&compiler->lexer) != TK_OPEN_PAREN) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected opening parenthesis for function parameters");
    }
    updateBracketCount(compiler, BRACKET_ROUND, true);

    // Add the symbol before parsing the body so that the function can be recursive.
    // This also avoid using the same name in the parameters.
    LocalRegisterId fnReg        = reserveTempRegister(compiler);
    ModuleVariableId moduleVarId = INVALID_MODULE_VARIABLE_ID;
    if (IS_TOP_LEVEL(compiler)) {
        moduleVarId = bindModuleVariable(compiler, fnIdentifierId, isModuleExport);
    } else {
        bindLocalVariable(compiler, fnIdentifierId, fnReg);
    }

    enterFunctionScope(compiler, false);

    // Collect function parameters one by one
    //
    // OPEN_PAREN CLOSE_PAREN
    // OPEN_PAREN [ IDENTIFIER COMMA ] IDENTIFIER (COMMA)? CLOSE_PAREN
    uint8_t paramCount = 0;
    Token t            = peekToken(&compiler->lexer);
    if (t == TK_CLOSE_PAREN) {
        goto parsed_arguments;
    }
    while (true) {
        if (t == TK_EOF) {
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_UNEXPECTED_END_OF_FILE, "Unexpected end of file in function parameters");
        }
        IdentifierId paramIdentifierId = newIdentifierNud(compiler);
        LocalRegisterId paramReg       = reserveTempRegister(compiler);
        bindLocalVariable(compiler, paramIdentifierId, paramReg);
        paramCount++;

        switch (t = peekToken(&compiler->lexer)) {
            case TK_COMMA:
                nextToken(&compiler->lexer);  // Consume ","
                t = peekToken(&compiler->lexer);
                if (t == TK_CLOSE_PAREN) {
                    // Allow trailing comma
                    goto parsed_arguments;
                }
                break;

            case TK_CLOSE_PAREN:
                goto parsed_arguments;

            default:
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expect comma or close bracket");
        }
    }

parsed_arguments:
    nextToken(&compiler->lexer);  // Consume ")"
    updateBracketCount(compiler, BRACKET_ROUND, false);
    parseScopedStatements(compiler);

    // The mandatory return marking the end of the function.
    // Since we cannot fully verify the coarity of the function if there are phi nodes, we simply return without
    // values here. The VM will check the coarity with the current frame when it reaches this instruction.
    emitCode(compiler, INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false));

    FunctionProto* fn = semiFunctionProtoCreate(compiler->gc, compiler->currentFunction->upvalues.size);
    if (fn == NULL) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Failed to allocate function object");
    }

    fn->arity   = paramCount;
    fn->coarity = compiler->currentFunction->nReturns == UINT8_MAX ? 0 : compiler->currentFunction->nReturns;
    memcpy(fn->upvalues,
           compiler->currentFunction->upvalues.data,
           sizeof(UpvalueDescription) * compiler->currentFunction->upvalues.size);

    // Change the owner of the chunk to the function.
    fn->chunk    = compiler->currentFunction->chunk;
    fn->moduleId = compiler->artifactModule->moduleId;
    ChunkInit(&compiler->currentFunction->chunk);
    leaveFunctionScope(compiler);

    Value fnValue         = semiValueNewFunctionProto(fn);
    ConstantIndex fnIndex = semiConstantTableInsert(&compiler->artifactModule->constantTable, fnValue);

    // Load the function proto from the constant table. This makes the register a function.
    if (fnIndex > MAX_OPERAND_K) {
        // TODO: Spill with OP_EXTRA_ARG
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_CONSTANTS, "Too many constants in a module");
    }
    emitCode(compiler, INSTRUCTION_LOAD_CONSTANT(fnReg, (uint16_t)fnIndex, false, false));

    if (IS_TOP_LEVEL(compiler)) {
        emitCode(compiler, INSTRUCTION_SET_MODULE_VAR(fnReg, moduleVarId, false, isModuleExport));
        restoreNextRegisterId(compiler, fnReg);
    }
}

static void parseImport(Compiler* compiler) {
    SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Import statement is not implemented yet");
}

static void parseExport(Compiler* compiler) {
    nextToken(&compiler->lexer);

    if (!IS_TOP_LEVEL(compiler)) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Export statement inside a function or block scope");
    }

    switch (peekToken(&compiler->lexer)) {
        case TK_FN:
            parseFunction(compiler, true);
            return;

        case TK_STRUCT:
            parseStruct(compiler);
            return;

        case TK_IDENTIFIER: {
            // only `export <identifier> := <expression>` is allowed.
            semiParseAssignmentOrExpr(compiler, true);
            return;
        }

        default:
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected 'fn', 'struct', or identifier after 'export'");
    }
}

static void parseReturn(Compiler* compiler) {
    nextToken(&compiler->lexer);  // Consume "return"

    if (compiler->currentFunction->parent == NULL) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Return statement outside of function");
    }

    uint8_t coarity = 0;
    Token t         = peekToken(&compiler->lexer);
    if (t == TK_CLOSE_BRACE || t == TK_SEPARATOR || t == TK_SEMICOLON) {
        // Return without value
        emitCode(compiler, INSTRUCTION_RETURN(255, 0, 0, false, false));
        goto check_coarity;
    }

    if (compiler->currentFunction->isDeferredFunction) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_RETURN_VALUE_IN_DEFER, "Cannot return values in defer blocks");
    }

    // Currently we only support single return value.
    coarity = 1;

    LocalRegisterId reg = reserveTempRegister(compiler);
    PrattState state    = {
           .targetRegister    = reg,
           .rightBindingPower = PRECEDENCE_NONE,
    };
    PrattExpr expr;
    semiParseExpression(compiler, state, &expr);
    switch (expr.type) {
        case PRATT_EXPR_TYPE_CONSTANT: {
            saveConstantToRegister(compiler, expr.value.constant, reg);
            emitCode(compiler, INSTRUCTION_RETURN(reg, 0, 0, false, false));
            break;
        }
        case PRATT_EXPR_TYPE_REG:
        case PRATT_EXPR_TYPE_VAR: {
            emitCode(compiler, INSTRUCTION_RETURN(expr.value.reg, 0, 0, false, false));
            break;
        }

        default:
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_INTERNAL_ERROR, "Invalid expression type when saving to register");
    }
    restoreNextRegisterId(compiler, reg);

    t = peekToken(&compiler->lexer);
    if (t != TK_SEPARATOR && t != TK_SEMICOLON && t != TK_CLOSE_BRACE) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected a separator after return statement");
    }

check_coarity:
    if (compiler->currentFunction->nReturns == UINT8_MAX) {
        compiler->currentFunction->nReturns = coarity;
    } else if (compiler->currentFunction->nReturns != coarity) {
        SEMI_COMPILE_ABORT(
            compiler, SEMI_ERROR_INCONSISTENT_RETURN_COUNT, "Inconsistent number of return values in function");
    }
}

static void parseRaise(Compiler* compiler) {
    SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Raise statement is not implemented yet");
}

static void parseContinue(Compiler* compiler) {
    nextToken(&compiler->lexer);
    BlockScope* currentBlock = compiler->currentFunction->currentBlock;
    while (currentBlock->type != BLOCK_SCOPE_TYPE_LOOP) {
        if (currentBlock->parent == NULL) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Continue statement outside of loop");
        }
        currentBlock = currentBlock->parent;
    }

    LoopScope* loopScope = (LoopScope*)currentBlock;
    emitJumpBack(compiler, loopScope->loopStartLocation);
}

static void parseBreak(Compiler* compiler) {
    nextToken(&compiler->lexer);

    BlockScope* currentBlock = compiler->currentFunction->currentBlock;
    while (currentBlock->type != BLOCK_SCOPE_TYPE_LOOP) {
        if (currentBlock->parent == NULL) {
            SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Break statement outside of loop");
        }
        currentBlock = currentBlock->parent;
    }

    LoopScope* loopScope            = (LoopScope*)currentBlock;
    PCLocation pcJump               = emitCode(compiler, INSTRUCTION_JUMP(loopScope->previousJumpLocation, false));
    loopScope->previousJumpLocation = pcJump;
}

static void parseDefer(Compiler* compiler) {
    nextToken(&compiler->lexer);  // Consume "defer"

    enterFunctionScope(compiler, true);
    parseScopedStatements(compiler);

    emitCode(compiler, INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false));

    FunctionProto* fn = semiFunctionProtoCreate(compiler->gc, compiler->currentFunction->upvalues.size);
    if (fn == NULL) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Failed to allocate function object");
    }

    fn->arity   = 0;
    fn->coarity = 0;
    memcpy(fn->upvalues,
           compiler->currentFunction->upvalues.data,
           sizeof(UpvalueDescription) * compiler->currentFunction->upvalues.size);

    // Change the owner of the chunk to the function.
    fn->chunk    = compiler->currentFunction->chunk;
    fn->moduleId = compiler->artifactModule->moduleId;
    ChunkInit(&compiler->currentFunction->chunk);
    leaveFunctionScope(compiler);

    Value fnValue         = semiValueNewFunctionProto(fn);
    ConstantIndex fnIndex = semiConstantTableInsert(&compiler->artifactModule->constantTable, fnValue);

    // Load the function proto from the constant table. This makes the register a function.
    if (fnIndex > MAX_OPERAND_K) {
        // TODO: Spill with OP_EXTRA_ARG
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_TOO_MANY_CONSTANTS, "Too many constants in a module");
    }
    emitCode(compiler, INSTRUCTION_DEFER_CALL(0, (uint16_t)fnIndex, false, false));
}

static void parseBlock(Compiler* compiler) {
    BlockScope blockScope;
    enterBlockScope(compiler, &blockScope, BLOCK_SCOPE_TYPE_NORMAL);
    parseScopedStatements(compiler);
    leaveBlockScope(compiler);
}

void semiParseStatement(Compiler* compiler) {
    Token t = peekToken(&compiler->lexer);
    switch (t) {
        case TK_IF:
            parseIf(compiler);
            return;
        case TK_FOR:
            parseFor(compiler);
            return;
        case TK_FN:
            parseFunction(compiler, false);
            return;
        case TK_IMPORT:
            parseImport(compiler);
            return;
        case TK_EXPORT:
            parseExport(compiler);
            return;
        case TK_RETURN:
            parseReturn(compiler);
            return;
        case TK_RAISE:
            parseRaise(compiler);
            return;
        case TK_CONTINUE:
            parseContinue(compiler);
            return;
        case TK_BREAK:
            parseBreak(compiler);
            return;
        case TK_DEFER:
            parseDefer(compiler);
            return;
        case TK_OPEN_BRACE:
            parseBlock(compiler);
            return;
        case TK_STRUCT:
            parseStruct(compiler);
            return;

        default: {
            semiParseAssignmentOrExpr(compiler, false);
            return;
        }
    }

    SEMI_UNREACHABLE();
}

static void parseStatements(Compiler* compiler) {
    Token t;
    while ((t = peekToken(&compiler->lexer)) != TK_EOF) {
        if (t == TK_SEPARATOR || t == TK_SEMICOLON) {
            nextToken(&compiler->lexer);
            continue;  // Skip whitespace and separators
        }
        if (t == TK_CLOSE_BRACE) {
            return;  // End of scoped statements
        }

        semiParseStatement(compiler);
    }
}

static void parseScopedStatements(Compiler* compiler) {
    nextToken(&compiler->lexer);  // Consume the opening brace

    parseStatements(compiler);

    Token t;
    if ((t = peekToken(&compiler->lexer)) != TK_CLOSE_BRACE) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected closing brace for scoped statements");
    }
    nextToken(&compiler->lexer);  // Consume the closing brace
    return;
}

static SemiModule* finalizeCompiler(struct Compiler* compiler) {
    emitCode(compiler, INSTRUCTION_RETURN(255, 0, 0, false, false));

    FunctionProto* fn = semiFunctionProtoCreate(compiler->gc, 0);
    if (fn == NULL) {
        SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Failed to allocate function object");
    }

    fn->chunk        = compiler->rootFunction.chunk;
    fn->maxStackSize = compiler->rootFunction.maxUsedRegisterCount;
    fn->arity        = 0;
    fn->upvalueCount = 0;
    fn->moduleId     = compiler->artifactModule->moduleId;

    ChunkInit(&compiler->rootFunction.chunk);

    SemiModule* module       = compiler->artifactModule;
    module->moduleInit       = fn;
    compiler->artifactModule = NULL;
    return module;
}

SemiModule* semiCompilerCompileModule(Compiler* compiler, SemiVM* vm, SemiModuleSource* moduleSource) {
    if (setjmp(compiler->errorJmpBuf.env) == 0) {
        compiler->gc                = &vm->gc;
        compiler->symbolTable       = &vm->symbolTable;
        compiler->classes           = &vm->classes;
        compiler->globalIdentifiers = &vm->globalIdentifiers;

        if (compiler->artifactModule == NULL) {
            SemiModule* artifactModule = semiVMModuleCreate(compiler->gc, vm->nextModuleId);
            if (artifactModule == NULL) {
                SEMI_COMPILE_ABORT(compiler, SEMI_ERROR_MEMORY_ALLOCATION_FAILURE, "Failed to allocate module");
            }
            compiler->artifactModule = artifactModule;
        }

        initLexer(&compiler->lexer, compiler, moduleSource->source, moduleSource->length);

        parseStatements(compiler);
        if (nextToken(&compiler->lexer) != TK_EOF) {
            SEMI_COMPILE_ABORT(
                compiler, SEMI_ERROR_UNEXPECTED_TOKEN, "Expected end of file after parsing all statements");
        }

        SemiModule* artifact = finalizeCompiler(compiler);

        vm->nextModuleId++;
        return artifact;
    }

    return NULL;
}

bool semiCompilerInheritMainModule(Compiler* compiler, SemiVM* vm) {
    if (vm->modules.size == 0) {
        compiler->errorJmpBuf.errorId = SEMI_ERROR_MODULE_NOT_FOUND;
#ifdef SEMI_DEBUG_MSG
        compiler->errorJmpBuf.message = "No main module found";
#endif
        return false;
    }

    compiler->artifactModule = semiVMModuleCreateFrom(compiler->gc, vm->modules.data[0]);
    if (compiler->artifactModule == NULL) {
        compiler->errorJmpBuf.errorId = SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
#ifdef SEMI_DEBUG_MSG
        compiler->errorJmpBuf.message = "Failed to allocate module";
#endif
        return false;
    }
    return true;
}

void semiCompilerInit(GC* gc, Compiler* compiler) {
    memset(compiler, 0, sizeof(Compiler));
    compiler->gc             = gc;
    compiler->artifactModule = NULL;

    FunctionScope* rootFunction                = &compiler->rootFunction;
    rootFunction->rootBlock.parent             = NULL;
    rootFunction->rootBlock.variableStackStart = 0;
    rootFunction->rootBlock.variableStackEnd   = 0;
    rootFunction->currentBlock                 = &compiler->rootFunction.rootBlock;
    rootFunction->parent                       = NULL;
    rootFunction->nextRegisterId               = 0;
    rootFunction->maxUsedRegisterCount         = 0;
    UpvalueListInit(&rootFunction->upvalues);
    ChunkInit(&rootFunction->chunk);

    VariableListInit(&compiler->variables);
    compiler->currentFunction     = &compiler->rootFunction;
    compiler->newlineState        = 0;
    compiler->newlineState        = 0;
    compiler->errorJmpBuf.errorId = 0;
#ifdef SEMI_DEBUG_MSG
    compiler->errorJmpBuf.message = NULL;
#endif
}

void semiCompilerCleanup(struct Compiler* compiler) {
    while (compiler->currentFunction != &compiler->rootFunction) {
        leaveFunctionScope(compiler);
    }
    ChunkCleanup(compiler->gc, &compiler->rootFunction.chunk);
    UpvalueListCleanup(compiler->gc, &compiler->rootFunction.upvalues);

    if (compiler->artifactModule != NULL) {
        semiVMModuleDestroy(compiler->gc, compiler->artifactModule);
    }
    VariableListCleanup(compiler->gc, &compiler->variables);
    ChunkCleanup(compiler->gc, &compiler->rootFunction.chunk);
}

#pragma endregion

/*
 │ Test-only functions
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region Test-only functions

#ifdef SEMI_TEST

void semiTextInitLexer(struct Lexer* lexer, Compiler* compiler, const char* source, uint32_t length) {
    initLexer(lexer, compiler, source, length);
}

Token semiTestNextToken(struct Lexer* lexer) {
    return nextToken(lexer);
}

Token semiTestPeekToken(struct Lexer* lexer) {
    return peekToken(lexer);
}

#endif  // SEMI_TEST

#pragma endregion
