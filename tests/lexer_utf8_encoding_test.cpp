// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>
extern "C" {
#include "../src/symbol_table.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class UTF8EncodingTest : public CompilerTest {
   protected:
    void InitLexer(const char* input) {
        semiTextInitLexer(&compiler.lexer, &compiler, input, strlen(input));
    }

    Token NextToken() {
        return semiTestNextToken(&compiler.lexer);
    }

    void InitLexerBytes(const char* input, unsigned int length) {
        semiTextInitLexer(&compiler.lexer, &compiler, input, length);
    }
};

TEST_F(UTF8EncodingTest, ByteOrderMark) {
    const char bom_input[] = {'\xEF', '\xBB', '\xBF', 'a', 'b', 'c'};
    InitLexerBytes(bom_input, sizeof(bom_input));

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, 3);
    EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, "abc", 3), 0);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, ShebangLine) {
    InitLexer("#!/usr/bin/semi\nabc");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, 3);
    EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, "abc", 3), 0);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, ShebangWithBOM) {
    const char bom_shebang[] = {'\xEF', '\xBB', '\xBF', '#', '!', '/', 'u', 's',  'r', '/', 'b',
                                'i',    'n',    '/',    's', 'e', 'm', 'i', '\n', 'a', 'b', 'c'};
    InitLexerBytes(bom_shebang, sizeof(bom_shebang));

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, 3);
    EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, "abc", 3), 0);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, UTF8StringsInComments) {
    InitLexer("# This is a comment with UTF-8: 世界 🌍\nabc");

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, 3);
    EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, "abc", 3), 0);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, UTF8StringLiterals) {
    InitLexer("\"Hello 世界\" \"🌍\" \"café\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, InvalidUTF8Sequences) {
    const char invalid_utf8[] = {'\xFF', '\xFE', 'a', 'b', 'c'};
    InitLexerBytes(invalid_utf8, sizeof(invalid_utf8));

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(UTF8EncodingTest, InvalidContinuationBytes) {
    const char invalid_continuation[] = {'\xC2', '\x20', 'a', 'b', 'c'};
    InitLexerBytes(invalid_continuation, sizeof(invalid_continuation));

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(UTF8EncodingTest, OverlongUTF8Sequences) {
    const char overlong[] = {'\xC0', '\x80', 'a', 'b', 'c'};
    InitLexerBytes(overlong, sizeof(overlong));

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(UTF8EncodingTest, SurrogateCodepoints) {
    const char surrogate[] = {'\xED', '\xA0', '\x80', 'a', 'b', 'c'};
    InitLexerBytes(surrogate, sizeof(surrogate));

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(UTF8EncodingTest, IncompleteUTF8Sequence) {
    const char incomplete[] = {'\xC2', 'a', 'b', 'c'};
    InitLexerBytes(incomplete, sizeof(incomplete));

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(UTF8EncodingTest, ValidASCIIOnly) {
    InitLexer("hello world 123");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, LineCountingWithUTF8) {
    InitLexer("line1\n# Comment with 世界\nline3");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.line, 0);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(compiler.lexer.line, 2);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.line, 2);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, UTF8InStringEscapes) {
    InitLexer("\"Hello\\nworld\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, EmptyFileWithBOM) {
    const char bom_only[] = {'\xEF', '\xBB', '\xBF'};
    InitLexerBytes(bom_only, sizeof(bom_only));

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, ShebangOnly) {
    InitLexer("#!/usr/bin/semi\n");

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(UTF8EncodingTest, ShebangWithoutNewline) {
    InitLexer("#!/usr/bin/semi");

    EXPECT_EQ(NextToken(), TK_EOF);
}