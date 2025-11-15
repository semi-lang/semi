// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>
extern "C" {
#include "../src/symbol_table.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class StateManagementTest : public ::testing::Test {
   protected:
    SemiVM* vm;
    Compiler compiler;
    void SetUp() override {
        vm = semiCreateVM(NULL);
        semiCompilerInit(&compiler);
        compiler.gc = &vm->gc;
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

    Token PeekToken() {
        return semiTestPeekToken(&compiler.lexer);
    }
};

TEST_F(StateManagementTest, BasicTokenPeeking) {
    InitLexer("abc 123");

    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);

    EXPECT_EQ(PeekToken(), TK_INTEGER);
    EXPECT_EQ(NextToken(), TK_INTEGER);

    EXPECT_EQ(PeekToken(), TK_EOF);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, PeekAfterNext) {
    InitLexer("a b c");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(PeekToken(), TK_EOF);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, MultiplePeeks) {
    InitLexer("test");

    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, PeekEOF) {
    InitLexer("");

    EXPECT_EQ(PeekToken(), TK_EOF);
    EXPECT_EQ(PeekToken(), TK_EOF);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, NewlineStateDefault) {
    InitLexer("a\nb");

    compiler.lexer.ignoreSeparators = false;
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, NewlineStateEnabled) {
    InitLexer("a\nb");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, BracketStateRound) {
    InitLexer("a\nb");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, BracketStateSquare) {
    InitLexer("a\nb");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, BracketStateCurly) {
    InitLexer("a\nb");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, MultipleBracketTypes) {
    InitLexer("a\nb");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, BracketStateReset) {
    InitLexer("a\nb\nc");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);

    compiler.lexer.ignoreSeparators = false;
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, LineCountingAccuracy) {
    InitLexer("line0\nline1\nline2");

    EXPECT_EQ(compiler.lexer.line, 0);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.line, 0);

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(compiler.lexer.line, 1);

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.line, 1);

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(compiler.lexer.line, 2);

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.line, 2);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, TokenValuePersistence) {
    InitLexer("test 42 3.14");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, 4);
    EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, "test", 4), 0);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 42);

    EXPECT_EQ(NextToken(), TK_DOUBLE);
    EXPECT_DOUBLE_EQ(AS_FLOAT(&compiler.lexer.tokenValue.constant), 3.14);

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, PeekTokenValueConsistency) {
    InitLexer("hello 123");

    EXPECT_EQ(PeekToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, 5);
    EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, "hello", 5), 0);

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, 5);
    EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, "hello", 5), 0);

    EXPECT_EQ(PeekToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 123);

    EXPECT_EQ(NextToken(), TK_INTEGER);
    EXPECT_EQ(AS_INT(&compiler.lexer.tokenValue.constant), 123);
}

TEST_F(StateManagementTest, LexerBufferBoundaries) {
    InitLexer("a");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(StateManagementTest, ComplexNewlineStateManagement) {
    InitLexer("if (condition\nand other)\nresult");

    EXPECT_EQ(NextToken(), TK_IF);
    EXPECT_EQ(NextToken(), TK_OPEN_PAREN);

    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_AND);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_CLOSE_PAREN);

    compiler.lexer.ignoreSeparators = false;

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}
