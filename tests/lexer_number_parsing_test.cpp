// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cmath>

#include "test_common.hpp"

class NumberParsingTest : public ::testing::Test {
   protected:
    SemiVM* vm;
    Compiler compiler;
    void SetUp() override {
        vm = semiCreateVM(NULL);
        semiCompilerInit(&vm->gc, &compiler);
    }

    void TearDown() override {
        semiCompilerCleanup(&compiler);
        semiDestroyVM(vm);
    }

    void InitLexer(const char* input) {
        semiTextInitLexer(&compiler.lexer, &compiler, input, strlen(input));
    }

    Token NextToken() {
        return semiTestNextToken(&compiler.lexer);
    }
};

TEST_F(NumberParsingTest, DecimalIntegers) {
    InitLexer("0 42 123 9999");

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 42);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 123);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 9999);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, BinaryIntegers) {
    InitLexer("0b0 0b1 0b101 0b1111");

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 1);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 5);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 15);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, OctalIntegers) {
    InitLexer("0o0 0o7 0o17 0o777");

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 7);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 15);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 511);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, HexadecimalIntegers) {
    InitLexer("0x0 0xA 0xFF 0xDEADBEEF");

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 10);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 255);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0xDEADBEEF);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, FloatingPointNumbers) {
    InitLexer("3.14 0.5 123.456");

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 3.14);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 0.5);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 123.456);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, ScientificNotation) {
    InitLexer("1e10 1.5e-3 2.5e+2 1e0");

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 1e10);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 1.5e-3);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 2.5e+2);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 1.0);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, NumbersWithUnderscores) {
    InitLexer("1_000 0b1010_1010 0x_FF_FF 3.14_159");

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 1000);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 170);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0xFFFF);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 3.14159);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, ZeroWithVariousFormats) {
    InitLexer("0 0b0 0o0 0x0 0.0 0e0");

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 0);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 0.0);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 0.0);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, InvalidNumberFormats) {
    InitLexer("0b2");
    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_NUMBER_LITERAL);

    InitLexer("0o8");
    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_NUMBER_LITERAL);

    InitLexer("0xG");
    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_NUMBER_LITERAL);

    InitLexer("1.");
    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_NUMBER_LITERAL);

    InitLexer("1e");
    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_INVALID_NUMBER_LITERAL);
}

TEST_F(NumberParsingTest, LargeNumbers) {
    InitLexer("9223372036854775807 1.7976931348623157e+308");

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 9223372036854775807LL);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 1.7976931348623157e+308);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(NumberParsingTest, Dots) {
    InitLexer("0.1..0.4");

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 0.1);

    EXPECT_EQ(NextToken(), TK_DOUBLE_DOTS);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 0.4);

    EXPECT_EQ(NextToken(), TK_EOF);

    InitLexer("10.1..10.4");

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 10.1);

    EXPECT_EQ(NextToken(), TK_DOUBLE_DOTS);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 10.4);

    EXPECT_EQ(NextToken(), TK_EOF);
}