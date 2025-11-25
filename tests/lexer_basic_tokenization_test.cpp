// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include "test_common.hpp"

class BasicTokenizationTest : public ::testing::Test {
   protected:
    SemiVM* vm;
    Compiler compiler;
    Lexer lexer;

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
        semiTextInitLexer(&lexer, &compiler, input, strlen(input));
    }

    Token NextToken() {
        return semiTestNextToken(&lexer);
    }
};

TEST_F(BasicTokenizationTest, SingleCharacterOperators) {
    InitLexer("+ - * / % & | ^ ~ ! ? : ; = , . ( ) { } [ ]");

    EXPECT_EQ(NextToken(), TK_PLUS);
    EXPECT_EQ(NextToken(), TK_MINUS);
    EXPECT_EQ(NextToken(), TK_STAR);
    EXPECT_EQ(NextToken(), TK_SLASH);
    EXPECT_EQ(NextToken(), TK_PERCENT);
    EXPECT_EQ(NextToken(), TK_AMPERSAND);
    EXPECT_EQ(NextToken(), TK_VERTICAL_BAR);
    EXPECT_EQ(NextToken(), TK_CARET);
    EXPECT_EQ(NextToken(), TK_TILDE);
    EXPECT_EQ(NextToken(), TK_BANG);
    EXPECT_EQ(NextToken(), TK_QUESTION);
    EXPECT_EQ(NextToken(), TK_COLON);
    EXPECT_EQ(NextToken(), TK_SEMICOLON);
    EXPECT_EQ(NextToken(), TK_ASSIGN);
    EXPECT_EQ(NextToken(), TK_COMMA);
    EXPECT_EQ(NextToken(), TK_DOT);
    EXPECT_EQ(NextToken(), TK_OPEN_PAREN);
    EXPECT_EQ(NextToken(), TK_CLOSE_PAREN);
    EXPECT_EQ(NextToken(), TK_OPEN_BRACE);
    EXPECT_EQ(NextToken(), TK_CLOSE_BRACE);
    EXPECT_EQ(NextToken(), TK_OPEN_BRACKET);
    EXPECT_EQ(NextToken(), TK_CLOSE_BRACKET);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(BasicTokenizationTest, MultiCharacterOperators) {
    InitLexer("** // == != <= >= ?. >> <<");

    EXPECT_EQ(NextToken(), TK_DOUBLE_STAR);
    EXPECT_EQ(NextToken(), TK_DOUBLE_SLASH);
    EXPECT_EQ(NextToken(), TK_EQ);
    EXPECT_EQ(NextToken(), TK_NOT_EQ);
    EXPECT_EQ(NextToken(), TK_LTE);
    EXPECT_EQ(NextToken(), TK_GTE);
    EXPECT_EQ(NextToken(), TK_QUESTION_DOT);
    EXPECT_EQ(NextToken(), TK_DOUBLE_RIGHT_ARROW);
    EXPECT_EQ(NextToken(), TK_DOUBLE_LEFT_ARROW);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(BasicTokenizationTest, Keywords) {
    InitLexer(
        "and or in is if elif else for import export as defer fn return raise break by struct "
        "true false");

    EXPECT_EQ(NextToken(), TK_AND);
    EXPECT_EQ(NextToken(), TK_OR);
    EXPECT_EQ(NextToken(), TK_IN);
    EXPECT_EQ(NextToken(), TK_IS);
    EXPECT_EQ(NextToken(), TK_IF);
    EXPECT_EQ(NextToken(), TK_ELIF);
    EXPECT_EQ(NextToken(), TK_ELSE);
    EXPECT_EQ(NextToken(), TK_FOR);
    EXPECT_EQ(NextToken(), TK_IMPORT);
    EXPECT_EQ(NextToken(), TK_EXPORT);
    EXPECT_EQ(NextToken(), TK_AS);
    EXPECT_EQ(NextToken(), TK_DEFER);
    EXPECT_EQ(NextToken(), TK_FN);
    EXPECT_EQ(NextToken(), TK_RETURN);
    EXPECT_EQ(NextToken(), TK_RAISE);
    EXPECT_EQ(NextToken(), TK_BREAK);
    EXPECT_EQ(NextToken(), TK_BY);
    EXPECT_EQ(NextToken(), TK_STRUCT);
    EXPECT_EQ(NextToken(), TK_TRUE);
    EXPECT_EQ(NextToken(), TK_FALSE);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(BasicTokenizationTest, NewlineSeparator) {
    InitLexer("a\nb");

    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEPARATOR);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(BasicTokenizationTest, EmptyInput) {
    InitLexer("");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(BasicTokenizationTest, WhitespaceHandling) {
    InitLexer("  \t  +  \t  -  \t  ");

    EXPECT_EQ(NextToken(), TK_PLUS);
    EXPECT_EQ(NextToken(), TK_MINUS);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(BasicTokenizationTest, MixedTokenTypes) {
    InitLexer("fn test(x, y) { return x + y; }");

    EXPECT_EQ(NextToken(), TK_FN);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_OPEN_PAREN);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_COMMA);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_CLOSE_PAREN);
    EXPECT_EQ(NextToken(), TK_OPEN_BRACE);
    EXPECT_EQ(NextToken(), TK_RETURN);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_PLUS);
    EXPECT_EQ(NextToken(), TK_IDENTIFIER);
    EXPECT_EQ(NextToken(), TK_SEMICOLON);
    EXPECT_EQ(NextToken(), TK_CLOSE_BRACE);
    EXPECT_EQ(NextToken(), TK_EOF);
}