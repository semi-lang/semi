// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerIfTest : public CompilerTest {};

// Basic If Statement Variations
TEST_F(CompilerIfTest, SimpleIfStatement) {
    const char* source = "if true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Simple if statement should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL         A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP            A=0x00 K=0x0001 i=F s=T
2: OP_CLOSE_UPVALUES    A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, IfElseStatement) {
    const char* source = "if true { } else { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "If-else statement should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL         A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP            A=0x00 K=0x0002 i=F s=T
2: OP_JUMP              J=0x000001 s=T
3: OP_CLOSE_UPVALUES    A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, IfElifStatement) {
    const char* source = "if false { } elif true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "If-elif statement should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL         A=0x00 K=0x0000 i=F s=F
1: OP_C_JUMP            A=0x00 K=0x0002 i=F s=T
2: OP_JUMP              J=0x000003 s=T
3: OP_LOAD_BOOL         A=0x00 K=0x0000 i=T s=F
4: OP_C_JUMP            A=0x00 K=0x0001 i=F s=T
5: OP_CLOSE_UPVALUES    A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, IfElifElseStatement) {
    const char* source = "if false { } elif false { } else { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "If-elif-else statement should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL         A=0x00 K=0x0000 i=F s=F
1: OP_C_JUMP            A=0x00 K=0x0002 i=F s=T
2: OP_JUMP              J=0x000004 s=T
3: OP_LOAD_BOOL         A=0x00 K=0x0000 i=F s=F
4: OP_C_JUMP            A=0x00 K=0x0002 i=F s=T
5: OP_JUMP              J=0x000001 s=T
6: OP_CLOSE_UPVALUES    A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

// Condition Expression Types
TEST_F(CompilerIfTest, ConstantTrueCondition) {
    const char* source = "if true { x := 5 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Constant true condition should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL              A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
2: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0005 i=T s=T
3: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, ConstantFalseCondition) {
    const char* source = "if false { } else { y := 10 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Constant false condition should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
1: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
2: OP_JUMP                   J=0x000002 s=T
3: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x000A i=T s=T
4: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}
TEST_F(CompilerIfTest, VariableCondition) {
    InitializeVariable("x");
    const char* source = "if x { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable condition should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_C_JUMP            A=0x00 K=0x0001 i=F s=T
1: OP_CLOSE_UPVALUES    A=0x01 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, ComplexExpressionCondition) {
    InitializeVariable("x");
    const char* source = "if x > 5 { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Complex expression condition should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_GT                A=0x01 B=0x00 C=0x85 kb=F kc=T
1: OP_C_JUMP            A=0x01 K=0x0001 i=F s=T
2: OP_CLOSE_UPVALUES    A=0x01 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, VariableBindingInElifBlock) {
    const char* source = "if false { } elif true { z := 15 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable binding in elif block should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
1: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
2: OP_JUMP                   J=0x000004 s=T
3: OP_LOAD_BOOL              A=0x00 K=0x0000 i=T s=F
4: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
5: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x000F i=T s=T
6: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, VariableShadowingPrevention) {
    const char* source = "{ x := 1\nif true { x := 2 } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Variable shadowing should be prevented";
}
TEST_F(CompilerIfTest, SiblingScopeIsolation) {
    InitializeVariable("someCondition");
    const char* source = "if someCondition { x := 5 } else { x := 10 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Sibling scope isolation should work";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_C_JUMP                 A=0x00 K=0x0003 i=F s=T
1: OP_LOAD_INLINE_INTEGER    A=0x01 K=0x0005 i=T s=T
2: OP_JUMP                   J=0x000002 s=T
3: OP_LOAD_INLINE_INTEGER    A=0x01 K=0x000A i=T s=T
4: OP_CLOSE_UPVALUES         A=0x01 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, VariableAccessFromParentScope) {
    const char* source = "{ x := 5\nif true { y := x } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable access from parent scope should work";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0005 i=T s=T
1: OP_LOAD_BOOL              A=0x01 K=0x0000 i=T s=F
2: OP_C_JUMP                 A=0x01 K=0x0002 i=F s=T
3: OP_MOVE                   A=0x01 B=0x00 C=0x00 kb=F kc=F
4: OP_CLOSE_UPVALUES         A=0x01 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, VariableAssignmentInBlocks) {
    const char* source = "{ x := 5\nif true { x = 10 } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable assignment in blocks should work";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0005 i=T s=T
1: OP_LOAD_BOOL              A=0x01 K=0x0000 i=T s=F
2: OP_C_JUMP                 A=0x01 K=0x0002 i=F s=T
3: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x000A i=T s=T
4: OP_CLOSE_UPVALUES         A=0x01 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, VariableOutOfScopeAssignment) {
    const char* source = "{ if true { x := 5 }\nx = 5 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_BINDING_ERROR) << "Variable out of scope assignment should fail";
}

TEST_F(CompilerIfTest, UnbindVariableAfterScope) {
    const char* source = "{ if true { x := 2 }\nx := 3 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable binding after scope should work (variables in different scopes)";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL              A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
2: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0002 i=T s=T
3: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
4: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0003 i=T s=T
)");
}

TEST_F(CompilerIfTest, UnbindVariableAfterElseScope) {
    const char* source = "{ if false { } else { y := 10 }\ny := 20 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable binding after else scope should work (variables in different scopes)";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
1: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
2: OP_JUMP                   J=0x000002 s=T
3: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x000A i=T s=T
4: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
5: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0014 i=T s=T
)");
}

// Negative Test Cases
TEST_F(CompilerIfTest, MissingOpeningBrace) {
    const char* source = "if true x := 5";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_UNEXPECTED_TOKEN) << "Missing opening brace should fail";
}

TEST_F(CompilerIfTest, MissingCondition) {
    const char* source = "if { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_UNEXPECTED_TOKEN) << "Missing condition should fail";
}

TEST_F(CompilerIfTest, UnclosedBraces) {
    const char* source = "if true { x := 5";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_UNEXPECTED_TOKEN) << "Unclosed braces should fail";
}

TEST_F(CompilerIfTest, VariableAlreadyDefinedInSameScope) {
    const char* source = "if true { x := 5\nx := 10 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_VARIABLE_ALREADY_DEFINED)
        << "Variable already defined in same scope should fail";
}

// Instruction Generation Verification
TEST_F(CompilerIfTest, JumpInstructionVerification) {
    const char* source = "if false { } else { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL         A=0x00 K=0x0000 i=F s=F
1: OP_C_JUMP            A=0x00 K=0x0002 i=F s=T
2: OP_JUMP              J=0x000001 s=T
3: OP_CLOSE_UPVALUES    A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerIfTest, CloseUpvaluesInstruction) {
    const char* source = "if true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL         A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP            A=0x00 K=0x0001 i=F s=T
2: OP_CLOSE_UPVALUES    A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

// Complex Nested Cases
TEST_F(CompilerIfTest, NestedIfStatements) {
    const char* source = "if true { if true { x := 5 } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Nested if statements should parse successfully";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL              A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP                 A=0x00 K=0x0005 i=F s=T
2: OP_LOAD_BOOL              A=0x00 K=0x0000 i=T s=F
3: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
4: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0005 i=T s=T
5: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
6: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

// Edge Cases
TEST_F(CompilerIfTest, EmptyBlocks) {
    const char* source = "if true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Empty blocks should work";

    // This is the same as SimpleIfStatement test case - already verified above
}

TEST_F(CompilerIfTest, LongElifChainsWithinLimits) {
    // Create a source with 10 elif statements (within limits)
    std::string source = "if false { }";
    for (int i = 0; i < 10; i++) {
        source += " elif false { }";
    }
    source += " else { x := 1 }";

    ErrorId result = ParseStatement(source.c_str(), false);
    EXPECT_EQ(result, 0) << "Long elif chains within limits should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0:  OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
1:  OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
2:  OP_JUMP                   J=0x000020 s=T
3:  OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
4:  OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
5:  OP_JUMP                   J=0x00001D s=T
6:  OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
7:  OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
8:  OP_JUMP                   J=0x00001A s=T
9:  OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
10: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
11: OP_JUMP                   J=0x000017 s=T
12: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
13: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
14: OP_JUMP                   J=0x000014 s=T
15: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
16: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
17: OP_JUMP                   J=0x000011 s=T
18: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
19: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
20: OP_JUMP                   J=0x00000E s=T
21: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
22: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
23: OP_JUMP                   J=0x00000B s=T
24: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
25: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
26: OP_JUMP                   J=0x000008 s=T
27: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
28: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
29: OP_JUMP                   J=0x000005 s=T
30: OP_LOAD_BOOL              A=0x00 K=0x0000 i=F s=F
31: OP_C_JUMP                 A=0x00 K=0x0002 i=F s=T
32: OP_JUMP                   J=0x000002 s=T
33: OP_LOAD_INLINE_INTEGER    A=0x00 K=0x0001 i=T s=T
34: OP_CLOSE_UPVALUES         A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}
