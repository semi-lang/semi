// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerLocalAssignmentTest : public CompilerTest {};

TEST_F(CompilerLocalAssignmentTest, LocalIntegerAssignment) {
    ErrorId result = ParseStatement("x := 42", true);
    ASSERT_EQ(result, 0) << "Parsing 'x := 42' should succeed";

    VariableDescription* var = FindVariable("x");
    ASSERT_NE(var, nullptr) << "Variable 'x' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'x' should be in register 0";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x002A i=T s=T
)");
}

TEST_F(CompilerLocalAssignmentTest, LocalDoubleAssignment) {
    ErrorId result = ParseStatement("y := 3.14", true);
    ASSERT_EQ(result, 0) << "Parsing 'y := 3.14' should succeed";

    VariableDescription* var = FindVariable("y");
    ASSERT_NE(var, nullptr) << "Variable 'y' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'y' should be in register 0";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Float 3.14
)");
}

TEST_F(CompilerLocalAssignmentTest, LocalBooleanAssignment) {
    ErrorId result = ParseStatement("flag := true", true);
    ASSERT_EQ(result, 0) << "Parsing 'flag := true' should succeed";

    VariableDescription* var = FindVariable("flag");
    ASSERT_NE(var, nullptr) << "Variable 'flag' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'flag' should be in register 0";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL   A=0x00 K=0x0000 i=T s=F
)");
}

TEST_F(CompilerLocalAssignmentTest, LocalStringAssignment) {
    ErrorId result = ParseStatement("name := \"hello\"", true);
    ASSERT_EQ(result, 0) << "Parsing 'name := \"hello\"' should succeed";

    VariableDescription* var = FindVariable("name");
    ASSERT_NE(var, nullptr) << "Variable 'name' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'name' should be in register 0";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT   A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: String "hello" length=5
)");
}

TEST_F(CompilerLocalAssignmentTest, LocalExpressionAssignment) {
    ErrorId result = ParseStatement("result := 10 + 5", true);
    ASSERT_EQ(result, 0) << "Parsing 'result := 10 + 5' should succeed";

    VariableDescription* var = FindVariable("result");
    ASSERT_NE(var, nullptr) << "Variable 'result' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'result' should be in register 0";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x000F i=T s=T
)");
}

TEST_F(CompilerLocalAssignmentTest, LocalVariableToLocalVariableAssignment) {
    // Initialize variable x using helper function
    InitializeVariable("x");

    VariableDescription* var_x = FindVariable("x");
    ASSERT_NE(var_x, nullptr) << "Variable 'x' should exist";

    ErrorId result = ParseStatement("y := x", true);
    ASSERT_EQ(result, 0) << "Parsing 'y := x' should succeed";

    VariableDescription* var_y = FindVariable("y");
    ASSERT_NE(var_y, nullptr) << "Variable 'y' should exist";

    ASSERT_NE(var_x->registerId, var_y->registerId) << "Variables should have different registers";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_MOVE   A=0x01 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerLocalAssignmentTest, LocalVariableReassignment) {
    InitializeVariable("counter");
    ErrorId result;

    VariableDescription* var   = FindVariable("counter");
    LocalRegisterId registerId = var->registerId;

    result = ParseStatement("counter = 100", true);
    ASSERT_EQ(result, 0) << "Assignment should succeed";

    ASSERT_EQ(compiler.variables.size, 1) << "Should have only one variable";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0064 i=T s=T
)");
}

TEST_F(CompilerLocalAssignmentTest, MultipleLocalVariablesUniqueRegisters) {
    ErrorId result = ParseStatement("a := 1", true);
    ASSERT_EQ(result, 0) << "First binding should succeed";

    result = ParseStatement("b := 2", true);
    ASSERT_EQ(result, 0) << "Second binding should succeed";

    result = ParseStatement("c := 3", true);
    ASSERT_EQ(result, 0) << "Third binding should succeed";

    VariableDescription* var_a = FindVariable("a");
    VariableDescription* var_b = FindVariable("b");
    VariableDescription* var_c = FindVariable("c");

    ASSERT_NE(var_a, nullptr) << "Variable 'a' should exist";
    ASSERT_NE(var_b, nullptr) << "Variable 'b' should exist";
    ASSERT_NE(var_c, nullptr) << "Variable 'c' should exist";

    ASSERT_NE(var_a->registerId, var_b->registerId) << "Variables should have different registers";
    ASSERT_NE(var_b->registerId, var_c->registerId) << "Variables should have different registers";
    ASSERT_NE(var_a->registerId, var_c->registerId) << "Variables should have different registers";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0003 i=T s=T
)");
}

TEST_F(CompilerLocalAssignmentTest, AssignmentToConstant) {
    ErrorId result = ParseStatement("42 := x", true);
    ASSERT_EQ(result, SEMI_ERROR_EXPECT_LVALUE) << "Assignment to constant should fail";
}

TEST_F(CompilerLocalAssignmentTest, AssignmentToLiteral) {
    ErrorId result = ParseStatement("42 := 10", true);
    ASSERT_EQ(result, SEMI_ERROR_EXPECT_LVALUE) << "Assignment to literal should fail";
}

TEST_F(CompilerLocalAssignmentTest, AssignmentToExpression) {
    // Initialize variables x and y directly
    InitializeVariable("x");
    InitializeVariable("y");

    // Try to assign to an arithmetic expression - should fail
    ErrorId result = ParseStatement("x + y := 10", true);
    ASSERT_EQ(result, SEMI_ERROR_EXPECT_LVALUE) << "Assignment to expression should fail";
}

TEST_F(CompilerLocalAssignmentTest, VariableRedefinitionInSameScope) {
    ErrorId result = ParseStatement("x := 42", true);
    ASSERT_EQ(result, 0) << "First declaration should succeed";

    // Try to redeclare the same variable in the same scope
    result = ParseStatement("x := 100", true);
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Variable redefinition in same scope should fail";
}

TEST_F(CompilerLocalAssignmentTest, VariableRedefinitionInInnerScope) {
    ErrorId result = ParseStatement("x := 42", true);
    ASSERT_EQ(result, 0) << "Outer scope declaration should succeed";

    // Enter an inner block scope
    BlockScope innerBlock;
    enterTestBlock(&compiler, &innerBlock);

    // Try to redeclare the same variable in inner scope - should fail
    result = ParseStatement("x := 100", true);
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Variable redefinition in inner scope should fail";
}
