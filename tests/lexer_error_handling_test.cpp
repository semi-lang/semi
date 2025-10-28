// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include "test_common.hpp"

class ErrorHandlingTest : public CompilerTest {
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

    void ExpectError(unsigned int expected_error_code) {
        EXPECT_EQ(compiler.errorJmpBuf.errorId, expected_error_code);
    }
};

TEST_F(ErrorHandlingTest, InvalidUTF8Sequences) {
    const char invalid_utf8[] = {'\xFF', '\xFE', 'a', 'b', 'c'};
    InitLexerBytes(invalid_utf8, sizeof(invalid_utf8));

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(ErrorHandlingTest, UnclosedStringAtEOF) {
    InitLexer("\"unclosed string");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(ErrorHandlingTest, UnclosedStringWithNewline) {
    InitLexer("\"unclosed\nstring\"");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(ErrorHandlingTest, UnclosedStringWithCarriageReturn) {
    InitLexer("\"unclosed\rstring\"");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(ErrorHandlingTest, UnclosedStringWithNullByte) {
    const char input[] = "\"unclosed\0string\"";
    InitLexerBytes(input, sizeof(input) - 1);

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(ErrorHandlingTest, IncompleteStringEscape) {
    InitLexer("\"incomplete\\");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INCOMPLETE_STIRNG_ESCAPE);
}

TEST_F(ErrorHandlingTest, UnknownStringEscape) {
    InitLexer("\"unknown\\z\"");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNKNOWN_STIRNG_ESCAPE);
}

TEST_F(ErrorHandlingTest, InvalidBinaryNumber) {
    InitLexer("0b2");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, InvalidOctalNumber) {
    InitLexer("0o8");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, InvalidHexNumber) {
    InitLexer("0xG");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, IncompleteFloatingPoint) {
    InitLexer("1.");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, IncompleteScientificNotation) {
    InitLexer("1e");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, InvalidScientificNotation) {
    InitLexer("1e+");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, IdentifierTooLong) {
    std::string long_identifier(300, 'a');
    InitLexer(long_identifier.c_str());

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_IDENTIFIER_TOO_LONG);
}

TEST_F(ErrorHandlingTest, ErrorStatePersistence) {
    InitLexer("\"unclosed");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);
}

TEST_F(ErrorHandlingTest, InvalidUTF8InComment) {
    const char invalid_comment[] = {'#', ' ', '\xFF', '\xFE', '\n', 'a'};
    InitLexerBytes(invalid_comment, sizeof(invalid_comment));

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(ErrorHandlingTest, InvalidUTF8InShebang) {
    const char invalid_shebang[] = {'#', '!', '\xFF', '\xFE', '\n', 'a'};
    InitLexerBytes(invalid_shebang, sizeof(invalid_shebang));

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_UTF_8);
}

TEST_F(ErrorHandlingTest, NoErrorState) {
    InitLexer("abc 123");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, 0);

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, 0);
}

TEST_F(ErrorHandlingTest, ErrorLineReporting) {
    InitLexer("line1\nline2\n\"unclosed");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_EOF);

    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_UNCLOSED_STRING);
    EXPECT_EQ(compiler.lexer.line, 2);
}

TEST_F(ErrorHandlingTest, MultipleErrorTypes) {
    InitLexer("0b2");
    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);

    InitLexer("\"unclosed");
    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);

    InitLexer("\"escape\\z\"");
    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNKNOWN_STIRNG_ESCAPE);
}

TEST_F(ErrorHandlingTest, InvalidNumberWithUnderscores) {
    InitLexer("0b_");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, InvalidFloatDoubleDecimal) {
    InitLexer("1.2.3");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, InvalidFloatMultipleExponents) {
    InitLexer("1e2e3");

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(ErrorHandlingTest, ValidTokensAfterError) {
    InitLexer("valid \"unclosed");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, 0);

    EXPECT_EQ(NextToken(), TK_EOF);
    ExpectError(SEMI_ERROR_UNCLOSED_STRING);
}
