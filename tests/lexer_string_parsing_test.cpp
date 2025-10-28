// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>
extern "C" {
#include "../src/symbol_table.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class StringParsingTest : public CompilerTest {
   protected:
    void InitLexer(const char* input) {
        semiTextInitLexer(&compiler.lexer, &compiler, input, strlen(input));
    }

    Token NextToken() {
        return semiTestNextToken(&compiler.lexer);
    }
};

TEST_F(StringParsingTest, BasicStrings) {
    InitLexer("\"hello\" \"world\" \"\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, StringsWithSpaces) {
    InitLexer("\"hello world\" \"  spaces  \"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, EscapeSequences) {
    // "\\" "\n" "\r" "\t" "\0" "\'"
    InitLexer("\"\\\"\" \"\\n\" \"\\r\" \"\\t\" \"\\0\" \"\\'\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, UTF8Strings) {
    InitLexer(u8"\"Hello 世界\" \"🌍\" \"café\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, StringsWithNumbers) {
    InitLexer("\"123\" \"3.14\" \"0xFF\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, StringsWithSpecialChars) {
    InitLexer("\"!@#$%^&*()\" \"[]{};:,.<>?\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, UnclosedString) {
    InitLexer("\"unclosed");

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(StringParsingTest, StringWithNewline) {
    InitLexer("\"hello\nworld\"");

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(StringParsingTest, StringWithCarriageReturn) {
    InitLexer("\"hello\rworld\"");

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(StringParsingTest, StringWithNullCharacter) {
    const char input[] = "\"hello\0world\"";
    semiTextInitLexer(&compiler.lexer, &compiler, input, sizeof(input) - 1);

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(StringParsingTest, IncompleteEscapeSequence) {
    InitLexer("\"hello\\");

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INCOMPLETE_STIRNG_ESCAPE);
}

TEST_F(StringParsingTest, UnknownEscapeSequence) {
    InitLexer("\"hello\\z\"");

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_UNKNOWN_STIRNG_ESCAPE);
}

TEST_F(StringParsingTest, MultipleStrings) {
    InitLexer("\"first\" \"second\" \"third\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, StringsWithTokensInside) {
    InitLexer("\"if else for while\" \"+ - * /\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, EmptyString) {
    InitLexer("\"\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StringParsingTest, StringsAroundOtherTokens) {
    InitLexer("\"hello\" + \"world\"");

    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_PLUS);
    EXPECT_EQ(NextToken(), TK_STRING);
    EXPECT_EQ(NextToken(), TK_EOF);
}