// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include "test_common.hpp"

class IdentifierParsingTest : public ::testing::Test {
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

    void ExpectIdentifier(const char* expected) {
        EXPECT_EQ(NextToken(), TK_IDENTIFIER);
        EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, strlen(expected));
        EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, expected, strlen(expected)), 0);
    }

    void ExpectTypeIdentifier(const char* expected) {
        EXPECT_EQ(NextToken(), TK_TYPE_IDENTIFIER);
        EXPECT_EQ(compiler.lexer.tokenValue.identifier.length, strlen(expected));
        EXPECT_EQ(memcmp(compiler.lexer.tokenValue.identifier.name, expected, strlen(expected)), 0);
    }
};

TEST_F(IdentifierParsingTest, SimpleIdentifiers) {
    InitLexer("a abc hello world123");

    ExpectIdentifier("a");
    ExpectIdentifier("abc");
    ExpectIdentifier("hello");
    ExpectIdentifier("world123");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, IdentifiersWithUnderscores) {
    InitLexer("_test test_ _private_ __special__");

    ExpectIdentifier("_test");
    ExpectIdentifier("test_");
    ExpectIdentifier("_private_");
    ExpectIdentifier("__special__");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, IdentifiersWithNumbers) {
    InitLexer("var1 test123 a1b2c3 _123");

    ExpectIdentifier("var1");
    ExpectIdentifier("test123");
    ExpectIdentifier("a1b2c3");
    ExpectIdentifier("_123");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, KeywordsVsIdentifiers) {
    InitLexer("if ifx for forloop and andor");

    EXPECT_EQ(NextToken(), TK_IF);
    ExpectIdentifier("ifx");
    EXPECT_EQ(NextToken(), TK_FOR);
    ExpectIdentifier("forloop");
    EXPECT_EQ(NextToken(), TK_AND);
    ExpectIdentifier("andor");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, KeywordEdgeCases) {
    InitLexer("truex falsey defery exportable");

    ExpectIdentifier("truex");
    ExpectIdentifier("falsey");
    ExpectIdentifier("defery");
    ExpectIdentifier("exportable");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, PlaceholderIdentifiers) {
    InitLexer("_ _1 ___ _123_456");

    ExpectIdentifier("_");
    ExpectIdentifier("_1");
    ExpectIdentifier("___");
    ExpectIdentifier("_123_456");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, CamelCaseIdentifiers) {
    InitLexer("camelCase PascalCase XMLHttpRequest");

    ExpectIdentifier("camelCase");
    ExpectTypeIdentifier("PascalCase");
    ExpectTypeIdentifier("XMLHttpRequest");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, SnakeCaseIdentifiers) {
    InitLexer("snake_case UPPER_CASE mixed_Snake_Case");

    ExpectIdentifier("snake_case");
    ExpectTypeIdentifier("UPPER_CASE");
    ExpectIdentifier("mixed_Snake_Case");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, SingleCharacterIdentifiers) {
    InitLexer("a b c x y z A B C X Y Z");

    ExpectIdentifier("a");
    ExpectIdentifier("b");
    ExpectIdentifier("c");
    ExpectIdentifier("x");
    ExpectIdentifier("y");
    ExpectIdentifier("z");
    ExpectTypeIdentifier("A");
    ExpectTypeIdentifier("B");
    ExpectTypeIdentifier("C");
    ExpectTypeIdentifier("X");
    ExpectTypeIdentifier("Y");
    ExpectTypeIdentifier("Z");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, IdentifiersInExpressions) {
    InitLexer("x + y * z");

    ExpectIdentifier("x");
    EXPECT_EQ(NextToken(), TK_PLUS);
    ExpectIdentifier("y");
    EXPECT_EQ(NextToken(), TK_STAR);
    ExpectIdentifier("z");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, LongIdentifiers) {
    const char* long_name = "very_long_identifier_name_that_goes_on_and_on_and_on";
    InitLexer(long_name);

    ExpectIdentifier(long_name);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, IdentifierTooLong) {
    std::string too_long(300, 'a');
    InitLexer(too_long.c_str());

    EXPECT_EQ(NextToken(), TK_EOF);
    EXPECT_EQ(compiler.errorJmpBuf.errorId, SEMI_ERROR_IDENTIFIER_TOO_LONG);
}

TEST_F(IdentifierParsingTest, AllKeywords) {
    InitLexer(
        "and or in is if elif else for import export as defer fn return raise break step struct "
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
    EXPECT_EQ(NextToken(), TK_STEP);
    EXPECT_EQ(NextToken(), TK_STRUCT);
    EXPECT_EQ(NextToken(), TK_TRUE);
    EXPECT_EQ(NextToken(), TK_FALSE);
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, NumericOnlyIdentifiers) {
    InitLexer("_123 _456_789 _0");

    ExpectIdentifier("_123");
    ExpectIdentifier("_456_789");
    ExpectIdentifier("_0");
    EXPECT_EQ(NextToken(), TK_EOF);
}

TEST_F(IdentifierParsingTest, IdentifiersAfterOperators) {
    InitLexer("test(param1, param2)");

    ExpectIdentifier("test");
    EXPECT_EQ(NextToken(), TK_OPEN_PAREN);
    ExpectIdentifier("param1");
    EXPECT_EQ(NextToken(), TK_COMMA);
    ExpectIdentifier("param2");
    EXPECT_EQ(NextToken(), TK_CLOSE_PAREN);
    EXPECT_EQ(NextToken(), TK_EOF);
}