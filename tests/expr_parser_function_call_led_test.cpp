// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "test_common.hpp"

class ExprParserFunctionCallLedTest : public CompilerTest {};

TEST_F(ExprParserFunctionCallLedTest, FunctionCallWithoutArguments) {
    InitializeVariable("func");
    PrattExpr expr;

    ErrorId result = ParseExpression("func()", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'func()' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Should be register expression";

    VariableDescription* var = FindVariable("func");
    ASSERT_NE(var, nullptr) << "Variable 'func' should exist";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate 2 instructions for function call";

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_MOVE) << "Should move function to call register";
    ASSERT_EQ(OPERAND_T_B(instr0), var->registerId) << "Should move from func's register";

    Instruction instr1 = GetInstruction(1);
    ASSERT_EQ(GET_OPCODE(instr1), OP_CALL) << "Should be CALL instruction";
    ASSERT_EQ(OPERAND_T_A(instr1), OPERAND_T_A(instr0)) << "Should call function in the same register as MOVE target";
    ASSERT_EQ(OPERAND_T_C(instr1), 0) << "Should have 0 arguments";
    ASSERT_EQ(OPERAND_T_A(instr1), expr.value.reg) << "Call target should match the register of the function";
}

TEST_F(ExprParserFunctionCallLedTest, FunctionCallWithOneArgument) {
    InitializeVariable("func");
    PrattExpr expr;

    ErrorId result = ParseExpression("func(42)", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'func(42)' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Should be register expression";

    VariableDescription* var = FindVariable("func");
    ASSERT_NE(var, nullptr) << "Variable 'func' should exist";

    ASSERT_EQ(GetCodeSize(), 3) << "Should generate 3 instructions";

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_MOVE) << "Should move function to call register";
    ASSERT_EQ(OPERAND_T_B(instr0), var->registerId) << "Should move from func's register";

    Instruction instr1 = GetInstruction(1);
    ASSERT_EQ(GET_OPCODE(instr1), OP_LOAD_INLINE_INTEGER) << "Should load argument";
    ASSERT_EQ(OPERAND_K_K(instr1), 42) << "Should load constant 42";

    Instruction instr2 = GetInstruction(2);
    ASSERT_EQ(GET_OPCODE(instr2), OP_CALL) << "Should be CALL instruction";
    ASSERT_EQ(OPERAND_T_A(instr2), OPERAND_T_A(instr0)) << "Should call function in the same register as MOVE target";
    ASSERT_EQ(OPERAND_T_B(instr2), OPERAND_K_A(instr1))
        << "The stack should start with the first argument in the same register";
    ASSERT_EQ(OPERAND_T_C(instr2), 1) << "Should have 1 argument";
    ASSERT_EQ(OPERAND_T_A(instr2), expr.value.reg) << "Call target should match the register of the function";
}

TEST_F(ExprParserFunctionCallLedTest, FunctionCallWithMultipleArguments) {
    InitializeVariable("func");
    InitializeVariable("a");
    PrattExpr expr;

    ErrorId result = ParseExpression("func(42, a, 3 + 4)", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'func(42, a, 3 + 4)' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Should be register expression";

    VariableDescription* func_var = FindVariable("func");
    ASSERT_NE(func_var, nullptr) << "Variable 'func' should exist";

    VariableDescription* a_var = FindVariable("a");
    ASSERT_NE(a_var, nullptr) << "Variable 'a' should exist";

    ASSERT_EQ(GetCodeSize(), 5) << "Should generate 5 instructions";

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_MOVE) << "Should move function to call register";
    ASSERT_EQ(OPERAND_T_B(instr0), func_var->registerId) << "Should move from func's register";

    Instruction instr1 = GetInstruction(1);
    ASSERT_EQ(GET_OPCODE(instr1), OP_LOAD_INLINE_INTEGER) << "Should load first argument";
    ASSERT_EQ(OPERAND_K_K(instr1), 42) << "Should load constant 42";

    Instruction instr2 = GetInstruction(2);
    ASSERT_EQ(GET_OPCODE(instr2), OP_MOVE) << "Should move second argument";
    ASSERT_EQ(OPERAND_T_B(instr2), a_var->registerId) << "Should move from a's register";

    Instruction instr3 = GetInstruction(3);
    ASSERT_EQ(GET_OPCODE(instr3), OP_LOAD_INLINE_INTEGER) << "Should be optimized to load constant";
    ASSERT_EQ(OPERAND_K_K(instr3), 7) << "Should load constant 7 (3+4 folded)";

    Instruction instr4 = GetInstruction(4);
    ASSERT_EQ(GET_OPCODE(instr4), OP_CALL) << "Should be CALL instruction";
    ASSERT_EQ(OPERAND_T_A(instr4), OPERAND_T_A(instr0)) << "Should call function in the same register as MOVE target";
    ASSERT_EQ(OPERAND_T_B(instr4), OPERAND_K_A(instr1))
        << "The stack should start with the first argument in the same register";
    ASSERT_EQ(OPERAND_T_C(instr4), 3) << "Should have 3 arguments";
    ASSERT_EQ(OPERAND_T_A(instr4), expr.value.reg) << "Call target should match the register of the function";
}

TEST_F(ExprParserFunctionCallLedTest, FunctionCallWithComplexExpressions) {
    InitializeVariable("func");
    InitializeVariable("a");
    InitializeVariable("b");
    PrattExpr expr;

    ErrorId result = ParseExpression("func(a + b, 10 * 2)", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'func(a + b, 10 * 2)' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Should be register expression";

    VariableDescription* func_var = FindVariable("func");
    ASSERT_NE(func_var, nullptr) << "Variable 'func' should exist";

    VariableDescription* a_var = FindVariable("a");
    ASSERT_NE(a_var, nullptr) << "Variable 'a' should exist";

    VariableDescription* b_var = FindVariable("b");
    ASSERT_NE(b_var, nullptr) << "Variable 'b' should exist";

    ASSERT_GE(GetCodeSize(), 3) << "Should generate at least 3 instructions";

    // Find the CALL instruction (should be the last one)
    Instruction lastInstr = GetInstruction(GetCodeSize() - 1);
    ASSERT_EQ(GET_OPCODE(lastInstr), OP_CALL) << "Last instruction should be CALL";
    ASSERT_EQ(OPERAND_T_C(lastInstr), 2) << "Should have 2 arguments";

    // The first instruction should be MOVE to set up function register
    Instruction firstInstr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(firstInstr), OP_MOVE) << "First instruction should move function";
    ASSERT_EQ(OPERAND_T_B(firstInstr), func_var->registerId) << "Should move from func's register";

    // The CALL should target the same register as the MOVE
    ASSERT_EQ(OPERAND_T_A(lastInstr), OPERAND_T_A(firstInstr))
        << "Should call function in the same register as MOVE target";
}

TEST_F(ExprParserFunctionCallLedTest, NestedFunctionCalls) {
    InitializeVariable("outer");
    InitializeVariable("inner");
    PrattExpr expr;

    ErrorId result = ParseExpression("outer(inner(5))", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'outer(inner(5))' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Should be register expression";

    VariableDescription* outer_var = FindVariable("outer");
    ASSERT_NE(outer_var, nullptr) << "Variable 'outer' should exist";

    VariableDescription* inner_var = FindVariable("inner");
    ASSERT_NE(inner_var, nullptr) << "Variable 'inner' should exist";

    ASSERT_GE(GetCodeSize(), 3) << "Should generate at least 3 instructions";

    // Should have two CALL instructions
    int callCount = 0;
    for (size_t i = 0; i < GetCodeSize(); i++) {
        Instruction instr = GetInstruction(i);
        if (GET_OPCODE(instr) == OP_CALL) {
            callCount++;
        }
    }
    ASSERT_EQ(callCount, 2) << "Should have exactly 2 CALL instructions";

    // Last instruction should be the outer call
    Instruction lastInstr = GetInstruction(GetCodeSize() - 1);
    ASSERT_EQ(GET_OPCODE(lastInstr), OP_CALL) << "Last instruction should be CALL";
    ASSERT_EQ(OPERAND_T_C(lastInstr), 1) << "Should have 1 argument";

    // Find the first MOVE instruction (should set up outer function)
    Instruction firstInstr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(firstInstr), OP_MOVE) << "First instruction should move outer function";
    ASSERT_EQ(OPERAND_T_B(firstInstr), outer_var->registerId) << "Should move from outer's register";

    // The outer CALL should target the same register as the first MOVE
    ASSERT_EQ(OPERAND_T_A(lastInstr), OPERAND_T_A(firstInstr)) << "Should call outer function in correct register";
}

TEST_F(ExprParserFunctionCallLedTest, FunctionCallInAssignment) {
    InitializeVariable("func");

    ErrorId result = ParseStatement("result := func(42)", true);
    ASSERT_EQ(result, 0) << "Parsing 'result = func(42)' should succeed";

    VariableDescription* func_var = FindVariable("func");
    ASSERT_NE(func_var, nullptr) << "Variable 'func' should exist";

    VariableDescription* result_var = FindVariable("result");
    ASSERT_NE(result_var, nullptr) << "Variable 'result' should exist";

    ASSERT_GE(GetCodeSize(), 2) << "Should generate at least 2 instructions";

    // Should find a CALL instruction (should be the last one)
    Instruction lastInstr = GetInstruction(GetCodeSize() - 1);
    ASSERT_EQ(GET_OPCODE(lastInstr), OP_CALL) << "Last instruction should be CALL";
    ASSERT_EQ(OPERAND_T_A(lastInstr), result_var->registerId) << "Should call into result register";
    ASSERT_EQ(OPERAND_T_C(lastInstr), 1) << "Should have 1 argument";

    // Should find a MOVE instruction that moves func to result's register
    bool foundMove = false;
    for (size_t i = 0; i < GetCodeSize(); i++) {
        Instruction instr = GetInstruction(i);
        if (GET_OPCODE(instr) == OP_MOVE && OPERAND_T_A(instr) == result_var->registerId &&
            OPERAND_T_B(instr) == func_var->registerId) {
            foundMove = true;
            break;
        }
    }
    ASSERT_TRUE(foundMove) << "Should have a MOVE instruction copying func to result register";
}

TEST_F(ExprParserFunctionCallLedTest, ErrorHandling) {
    struct ErrorTestCase {
        const char* input;
        const char* description;
        bool needsVariable;
        const char* variableName;
    };

    ErrorTestCase errorCases[] = {
        {      "func(",     "Missing closing parenthesis", true, "func"},
        {   "func(42,",    "Missing argument after comma", true, "func"},
        {"func(42 43)", "Missing comma between arguments", true, "func"},
    };

    for (size_t i = 0; i < sizeof(errorCases) / sizeof(errorCases[0]); i++) {
        TearDown();
        SetUp();

        ErrorTestCase& testCase = errorCases[i];
        if (testCase.needsVariable) {
            InitializeVariable(testCase.variableName);
        }

        PrattExpr expr;
        ErrorId result = ParseExpression(testCase.input, &expr);
        ASSERT_NE(result, 0) << "Error case " << i << " (" << testCase.description << ") should fail";
    }
}