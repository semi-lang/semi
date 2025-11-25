// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerForTest : public CompilerTest {};

// Basic For Loop Tests
TEST_F(CompilerForTest, SimpleForLoopWithRange) {
    const char* source = "for i in 0..10 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Simple for loop with range should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1: OP_RANGE_NEXT      A=0x00 K=0x0002 i=F s=F
2: OP_JUMP            J=0x000001 s=F
3: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=10 step=1
)");
}

TEST_F(CompilerForTest, ForLoopWithExplicitStep) {
    const char* source = "for i in 0..10 by 2 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with explicit step should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1: OP_RANGE_NEXT      A=0x00 K=0x0002 i=F s=F
2: OP_JUMP            J=0x000001 s=F
3: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=10 step=2
)");
}

TEST_F(CompilerForTest, ForLoopWithIndexAndItem) {
    const char* source = "for i, item in 0..5 { index := i; curr := item }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with index and item should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT        A=0x00 K=0x0000 i=F s=F
1: OP_LOAD_INLINE_INTEGER  A=0x01 K=0x0000 i=T s=T
2: OP_RANGE_NEXT           A=0x00 K=0x0004 i=T s=F
3: OP_MOVE                 A=0x04 B=0x03 C=0x00 kb=F kc=F
4: OP_MOVE                 A=0x05 B=0x02 C=0x00 kb=F kc=F
5: OP_JUMP                 J=0x000003 s=F
6: OP_CLOSE_UPVALUES       A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=5 step=1
)");
}

TEST_F(CompilerForTest, ForLoopWithVariableInRange) {
    InitializeVariable("start");
    InitializeVariable("end");
    const char* source = "for i in start..end { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with variables in range should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_MOVE           A=0x02 B=0x00 C=0x00 kb=F kc=F
1: OP_MAKE_RANGE     A=0x02 B=0x01 C=0x81 kb=F kc=T
2: OP_RANGE_NEXT     A=0x02 K=0x0002 i=F s=F
3: OP_JUMP           J=0x000001 s=F
4: OP_CLOSE_UPVALUES A=0x02 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerForTest, NestedForLoops) {
    const char* source = "for i in 0..3 { for j in 0..2 { } }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Nested for loops should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1: OP_RANGE_NEXT      A=0x00 K=0x0006 i=F s=F
2: OP_LOAD_CONSTANT   A=0x02 K=0x0001 i=F s=F
3: OP_RANGE_NEXT      A=0x02 K=0x0002 i=F s=F
4: OP_JUMP            J=0x000001 s=F
5: OP_CLOSE_UPVALUES  A=0x02 B=0x00 C=0x00 kb=F kc=F
6: OP_JUMP            J=0x000005 s=F
7: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=3 step=1
K[1]: Range start=0 end=2 step=1
)");
}

// Range Expression Tests
TEST_F(CompilerForTest, NegativeRangeStep) {
    const char* source = "for i in 10..0 by -1 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Range with negative step should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1: OP_RANGE_NEXT      A=0x00 K=0x0002 i=F s=F
2: OP_JUMP            J=0x000001 s=F
3: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=10 end=0 step=-1
)");
}

TEST_F(CompilerForTest, ExpressionInRange) {
    InitializeVariable("x");
    const char* source = "for i in x-1..x+1 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Range with expressions should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
1: OP_SUBTRACT       A=0x01 B=0x00 C=0x81 kb=F kc=T
3: OP_ADD            A=0x02 B=0x00 C=0x81 kb=F kc=T
4: OP_MAKE_RANGE     A=0x01 B=0x02 C=0x81 kb=F kc=T
5: OP_RANGE_NEXT     A=0x01 K=0x0002 i=F s=F
6: OP_JUMP           J=0x000001 s=F
7: OP_CLOSE_UPVALUES A=0x01 B=0x00 C=0x00 kb=F kc=F
)");
}

// Break and Continue Tests
TEST_F(CompilerForTest, ForLoopWithBreak) {
    const char* source = "for i in 0..10 { break; }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with break should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1: OP_RANGE_NEXT      A=0x00 K=0x0003 i=F s=F
3: OP_JUMP            J=0x000002 s=T
4: OP_JUMP            J=0x000002 s=F
5: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=10 step=1
)");
}

TEST_F(CompilerForTest, ForLoopWithContinue) {
    const char* source = "for i in 0..10 { continue; }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with continue should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1: OP_RANGE_NEXT      A=0x00 K=0x0003 i=F s=F
3: OP_JUMP            J=0x000001 s=F
4: OP_JUMP            J=0x000002 s=F
5: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=10 step=1
)");
}

TEST_F(CompilerForTest, ForLoopWithBreakAndContinue) {
    const char* source = "for i in 0..5 { if i == 2 { continue; } if i == 4 { break; } }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with both break and continue should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0:  OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1:  OP_RANGE_NEXT      A=0x00 K=0x000A i=F s=F
2:  OP_EQ              A=0x02 B=0x01 C=0x82 kb=F kc=T
3:  OP_C_JUMP          A=0x02 K=0x0002 i=F s=T
4:  OP_JUMP            J=0x000003 s=F
5:  OP_CLOSE_UPVALUES  A=0x02 B=0x00 C=0x00 kb=F kc=F
6:  OP_EQ              A=0x02 B=0x01 C=0x84 kb=F kc=T
7:  OP_C_JUMP          A=0x02 K=0x0002 i=F s=T
8:  OP_JUMP            J=0x000003 s=T
9:  OP_CLOSE_UPVALUES  A=0x02 B=0x00 C=0x00 kb=F kc=F
10: OP_JUMP            J=0x000009 s=F
11: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=5 step=1
)");
}

TEST_F(CompilerForTest, NestedForLoopsWithBreakAndContinue) {
    const char* source = "for i in 0..3 { for j in 0..2 { if j == 1 { continue; } if i == 2 { break; } } }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Nested for loops with break and continue should parse successfully";

    VerifyCompiler(&compiler, R"(
[Instructions]
0:  OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F
1:  OP_RANGE_NEXT      A=0x00 K=0x000E i=F s=F
2:  OP_LOAD_CONSTANT   A=0x02 K=0x0001 i=F s=F
3:  OP_RANGE_NEXT      A=0x02 K=0x000A i=F s=F
4:  OP_EQ              A=0x04 B=0x03 C=0x81 kb=F kc=T
5:  OP_C_JUMP          A=0x04 K=0x0002 i=F s=T
6:  OP_JUMP            J=0x000003 s=F
7:  OP_CLOSE_UPVALUES  A=0x04 B=0x00 C=0x00 kb=F kc=F
8:  OP_EQ              A=0x04 B=0x01 C=0x82 kb=F kc=T
9:  OP_C_JUMP          A=0x04 K=0x0002 i=F s=T
10: OP_JUMP            J=0x000003 s=T
11: OP_CLOSE_UPVALUES  A=0x04 B=0x00 C=0x00 kb=F kc=F
12: OP_JUMP            J=0x000009 s=F
13: OP_CLOSE_UPVALUES  A=0x02 B=0x00 C=0x00 kb=F kc=F
14: OP_JUMP            J=0x00000D s=F
15: OP_CLOSE_UPVALUES  A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=0 end=3 step=1
K[1]: Range start=0 end=2 step=1
)");
}

// Error Cases
TEST_F(CompilerForTest, MissingInKeyword) {
    const char* source = "for i 0..10 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Missing 'in' keyword should cause parse error";
}

TEST_F(CompilerForTest, MissingOpeningBrace) {
    const char* source = "for i in 0..10 }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Missing opening brace should cause parse error";
}

TEST_F(CompilerForTest, MissingIterable) {
    const char* source = "for i in { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Missing iterable expression should cause parse error";
}

TEST_F(CompilerForTest, DuplicateVariableNames) {
    const char* source = "for i, i in 0..5 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Duplicate variable names should cause parse error";
}

TEST_F(CompilerForTest, TooManyVariables) {
    const char* source = "for i, j, k in 0..5 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Too many variables should cause parse error";
}

// Error Cases for Break and Continue
TEST_F(CompilerForTest, BreakOutsideLoop) {
    const char* source = "break;";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Break outside of loop should cause parse error";
}

TEST_F(CompilerForTest, ContinueOutsideLoop) {
    const char* source = "continue;";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Continue outside of loop should cause parse error";
}
