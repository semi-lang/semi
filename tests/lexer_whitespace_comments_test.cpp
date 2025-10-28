// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>
extern "C" {
#include "../src/symbol_table.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class WhitespaceCommentsTest : public ::testing::Test {
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

TEST_F(WhitespaceCommentsTest, BasicWhitespaceHandling) {
    InitLexer("   a    b   \t  c   ");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, TabsAndSpaces) {
    InitLexer("\t\t  a  \t\t  b  \t\t");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, CarriageReturnHandling) {
    InitLexer("a\rb\rc");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, SimpleComments) {
    InitLexer("a # this is a comment\nb");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, CommentAtEndOfFile) {
    InitLexer("a # comment at end");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, CommentWithUTF8) {
    InitLexer("a # Comment with 世界 and 🌍\nb");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, MultipleComments) {
    InitLexer("a # first comment\nb # second comment\nc");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, CommentOnlyLine) {
    InitLexer("a\n# just a comment\nb");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, NewlineConsumeStateDisabled) {
    InitLexer("a\nb");
    compiler.lexer.ignoreSeparators = false;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, NewlineConsumeStateEnabled) {
    InitLexer("a\nb");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, BracketStateConsumeNewlines) {
    InitLexer("(a\nb)");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_OPEN_PAREN);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_CLOSE_PAREN);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, MultipleBracketLevels) {
    InitLexer("([a\nb])");
    compiler.lexer.ignoreSeparators = true;

    EXPECT_EQ(NextToken(), TK_OPEN_PAREN);
    EXPECT_EQ(NextToken(), TK_OPEN_BRACKET);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_CLOSE_BRACKET);
    EXPECT_EQ(NextToken(), TK_CLOSE_PAREN);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, LineCountingWithNewlines) {
    InitLexer("a\nb\nc");

    EXPECT_EQ(compiler.lexer.line, 0);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(compiler.lexer.line, 1);

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(compiler.lexer.line, 2);

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, LineCountingWithComments) {
    InitLexer("a # comment\nb # another comment\nc");

    EXPECT_EQ(compiler.lexer.line, 0);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(compiler.lexer.line, 1);

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);

    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(compiler.lexer.line, 2);

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, EmptyComments) {
    InitLexer("a #\nb");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, CommentWithSpecialCharacters) {
    InitLexer("a # !@#$%^&*()[]{}+=<>?/\nb");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, WhitespaceOnlyFile) {
    InitLexer("   \t   \r   ");

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, CommentOnlyFile) {
    InitLexer("# just a comment");

    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(WhitespaceCommentsTest, MixedWhitespaceAndNewlines) {
    InitLexer("a  \t  \n  \t  b");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}
