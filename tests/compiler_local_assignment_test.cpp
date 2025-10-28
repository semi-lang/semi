// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "test_common.hpp"

class CompilerLocalAssignmentTest : public CompilerTest {};

TEST_F(CompilerLocalAssignmentTest, LocalIntegerAssignment) {
    ErrorId result = ParseStatement("x := 42", true);
    ASSERT_EQ(result, 0) << "Parsing 'x := 42' should succeed";

    VariableDescription* var = FindVariable("x");
    ASSERT_NE(var, nullptr) << "Variable 'x' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'x' should be in register 0";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr), OP_LOAD_INLINE_INTEGER) << "Should be LOAD_INLINE_INTEGER instruction";
    ASSERT_EQ(OPERAND_K_A(instr), var->registerId) << "Should load into variable's register";
    ASSERT_EQ(OPERAND_K_K(instr), 42) << "Should load constant 42";
    ASSERT_TRUE(OPERAND_K_I(instr)) << "Should be inline constant";
    ASSERT_TRUE(OPERAND_K_S(instr)) << "Should be positive";
}

TEST_F(CompilerLocalAssignmentTest, LocalDoubleAssignment) {
    ErrorId result = ParseStatement("y := 3.14", true);
    ASSERT_EQ(result, 0) << "Parsing 'y := 3.14' should succeed";

    VariableDescription* var = FindVariable("y");
    ASSERT_NE(var, nullptr) << "Variable 'y' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'y' should be in register 0";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr), OP_LOAD_CONSTANT) << "Should be LOAD_CONSTANT instruction";
    ASSERT_EQ(OPERAND_K_A(instr), var->registerId) << "Should load into variable's register";
    ASSERT_FALSE(OPERAND_K_I(instr)) << "Floats always use constant table";

    // The value should be stored in the constant table
    uint16_t constIdx = OPERAND_K_K(instr);
    ASSERT_LT(constIdx, semiConstantTableSize(&compiler.artifactModule->constantTable))
        << "Constant index should be valid";
    Value constValue = semiConstantTableGet(&compiler.artifactModule->constantTable, constIdx);
    ASSERT_DOUBLE_EQ(AS_FLOAT(&constValue), 3.14) << "Constant value should be 3.14";
}

TEST_F(CompilerLocalAssignmentTest, LocalBooleanAssignment) {
    ErrorId result = ParseStatement("flag := true", true);
    ASSERT_EQ(result, 0) << "Parsing 'flag := true' should succeed";

    VariableDescription* var = FindVariable("flag");
    ASSERT_NE(var, nullptr) << "Variable 'flag' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'flag' should be in register 0";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr), OP_LOAD_BOOL) << "Should be LOAD_BOOL instruction";
    ASSERT_EQ(OPERAND_K_A(instr), var->registerId) << "Should load into variable's register";
    ASSERT_EQ(OPERAND_K_I(instr), 1) << "Should load true";
    ASSERT_FALSE(OPERAND_K_S(instr)) << "Should not jump (K=0)";
}

TEST_F(CompilerLocalAssignmentTest, LocalStringAssignment) {
    ErrorId result = ParseStatement("name := \"hello\"", true);
    ASSERT_EQ(result, 0) << "Parsing 'name := \"hello\"' should succeed";

    VariableDescription* var = FindVariable("name");
    ASSERT_NE(var, nullptr) << "Variable 'name' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'name' should be in register 0";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr), OP_LOAD_CONSTANT) << "Should be LOAD_CONSTANT instruction";
    ASSERT_EQ(OPERAND_K_A(instr), var->registerId) << "Should load into variable's register";

    // The value should be stored in the constant table
    uint16_t constIdx = OPERAND_K_K(instr);
    ASSERT_LT(constIdx, semiConstantTableSize(&compiler.artifactModule->constantTable))
        << "Constant index should be valid";

    Value constValue = semiConstantTableGet(&compiler.artifactModule->constantTable, constIdx);
    ASSERT_EQ(VALUE_TYPE(&constValue), VALUE_TYPE_OBJECT_STRING) << "Should be string constant";
    ASSERT_EQ(AS_OBJECT_STRING(&constValue)->length, 5) << "String should have correct size";
    ASSERT_EQ(strncmp((const char*)AS_OBJECT_STRING(&constValue)->str, "hello", 5), 0) << "String content should match";
}

TEST_F(CompilerLocalAssignmentTest, LocalExpressionAssignment) {
    ErrorId result = ParseStatement("result := 10 + 5", true);
    ASSERT_EQ(result, 0) << "Parsing 'result := 10 + 5' should succeed";

    VariableDescription* var = FindVariable("result");
    ASSERT_NE(var, nullptr) << "Variable 'result' should exist";
    ASSERT_EQ(var->registerId, 0) << "Variable 'result' should be in register 0";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr), OP_LOAD_INLINE_INTEGER) << "Should be LOAD_INLINE_INTEGER instruction";
    ASSERT_EQ(OPERAND_K_A(instr), var->registerId) << "Should load into variable's register";
    ASSERT_EQ(OPERAND_K_K(instr), 15) << "Should load constant 15";
    ASSERT_TRUE(OPERAND_K_I(instr)) << "Should be inline constant";
    ASSERT_TRUE(OPERAND_K_S(instr)) << "Should be positive";
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

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr), OP_MOVE) << "Should be MOVE instruction";
    ASSERT_EQ(OPERAND_T_A(instr), var_y->registerId) << "Should move into y's register";
    ASSERT_EQ(OPERAND_T_B(instr), var_x->registerId) << "Should move from x's register";
    ASSERT_EQ(OPERAND_T_C(instr), 0) << "Should not have conditional jump";
    ASSERT_FALSE(OPERAND_T_KB(instr)) << "Source should be register";
    ASSERT_FALSE(OPERAND_T_KC(instr)) << "Jump offset should be register";
}

TEST_F(CompilerLocalAssignmentTest, LocalVariableReassignment) {
    InitializeVariable("counter");
    ErrorId result;

    VariableDescription* var   = FindVariable("counter");
    LocalRegisterId registerId = var->registerId;

    result = ParseStatement("counter = 100", true);
    ASSERT_EQ(result, 0) << "Assignment should succeed";

    ASSERT_EQ(compiler.variables.size, 1) << "Should have only one variable";
    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    // Both instructions should target the same register
    Instruction inst = GetInstruction(0);

    ASSERT_EQ(get_opcode(inst), OP_LOAD_INLINE_INTEGER) << "Should be LOAD_INLINE_INTEGER";
    ASSERT_EQ(OPERAND_K_A(inst), var->registerId) << "Should use same register";
    ASSERT_EQ(OPERAND_K_K(inst), 100) << "Should load 100";
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
