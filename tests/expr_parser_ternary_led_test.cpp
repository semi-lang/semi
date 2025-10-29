// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "test_common.hpp"

class ExprParserTernaryLedTest : public CompilerTest {};

TEST_F(ExprParserTernaryLedTest, ConstantConditionTrueExpression) {
    PrattExpr expr;

    ErrorId result = ParseExpression("true ? 42 : 0", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'true ? 42 : 0' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be constant";
    ASSERT_EQ(VALUE_TYPE(&expr.value.constant), VALUE_TYPE_INT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 42) << "Should return truthy branch value";

    ASSERT_EQ(GetCodeSize(), 0) << "Constant condition should generate no code";
}

TEST_F(ExprParserTernaryLedTest, ConstantConditionFalseExpression) {
    PrattExpr expr;

    ErrorId result = ParseExpression("false ? 42 : 99", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'false ? 42 : 99' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be constant";
    ASSERT_EQ(VALUE_TYPE(&expr.value.constant), VALUE_TYPE_INT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 99) << "Should return falsy branch value";

    ASSERT_EQ(GetCodeSize(), 0) << "Constant condition should generate no code";
}

TEST_F(ExprParserTernaryLedTest, VariableConditionExpression) {
    InitializeVariable("x");
    PrattExpr expr;

    ErrorId result = ParseExpression("x ? 10 : 20", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'x ? 10 : 20' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Should be register expression";

    ASSERT_EQ(GetCodeSize(), 4) << "Should generate 4 instructions";

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_C_JUMP) << "First instruction should be C_JUMP";
    ASSERT_FALSE(OPERAND_K_I(instr0)) << "Should jump if falsy (i=false)";
    ASSERT_TRUE(OPERAND_K_S(instr0)) << "Should be positive jump";
    ASSERT_EQ(OPERAND_K_K(instr0), 3) << "Should jump 3 instructions ahead to falsy branch";

    Instruction instr1 = GetInstruction(1);
    ASSERT_EQ(GET_OPCODE(instr1), OP_LOAD_INLINE_INTEGER) << "Second instruction should load truthy value";
    ASSERT_EQ(OPERAND_K_K(instr1), 10) << "Should load value 10";

    Instruction instr2 = GetInstruction(2);
    ASSERT_EQ(GET_OPCODE(instr2), OP_JUMP) << "Third instruction should be unconditional JUMP";
    ASSERT_TRUE(OPERAND_J_S(instr2)) << "Should be positive jump";
    ASSERT_EQ(OPERAND_J_J(instr2), 2) << "Should jump 2 instructions ahead to end";

    Instruction instr3 = GetInstruction(3);
    ASSERT_EQ(GET_OPCODE(instr3), OP_LOAD_INLINE_INTEGER) << "Fourth instruction should load falsy value";
    ASSERT_EQ(OPERAND_K_K(instr3), 20) << "Should load value 20";
}

TEST_F(ExprParserTernaryLedTest, RegisterConditionExpression) {
    InitializeVariable("x");
    PrattExpr expr;

    ErrorId result = ParseExpression("(x + 1) ? 2 : 3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '(x + 1) ? 2 : 3' should succeed";

    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Should be register expression";

    ASSERT_EQ(GetCodeSize(), 5) << "Should generate 5 instructions";

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_ADD) << "First instruction should be ADD for (x + 1)";

    Instruction instr1 = GetInstruction(1);
    ASSERT_EQ(GET_OPCODE(instr1), OP_C_JUMP) << "Second instruction should be C_JUMP";
    ASSERT_FALSE(OPERAND_K_I(instr1)) << "Should jump if falsy (i=false)";
    ASSERT_TRUE(OPERAND_K_S(instr1)) << "Should be positive jump";
    ASSERT_EQ(OPERAND_K_K(instr1), 3) << "Should jump 3 instructions ahead to falsy branch";

    Instruction instr2 = GetInstruction(2);
    ASSERT_EQ(GET_OPCODE(instr2), OP_LOAD_INLINE_INTEGER) << "Third instruction should load truthy value";
    ASSERT_EQ(OPERAND_K_K(instr2), 2) << "Should load value 2";

    Instruction instr3 = GetInstruction(3);
    ASSERT_EQ(GET_OPCODE(instr3), OP_JUMP) << "Fourth instruction should be unconditional JUMP";
    ASSERT_TRUE(OPERAND_J_S(instr3)) << "Should be positive jump";
    ASSERT_EQ(OPERAND_J_J(instr3), 2) << "Should jump 2 instructions ahead to end";

    Instruction instr4 = GetInstruction(4);
    ASSERT_EQ(GET_OPCODE(instr4), OP_LOAD_INLINE_INTEGER) << "Fifth instruction should load falsy value";
    ASSERT_EQ(OPERAND_K_K(instr4), 3) << "Should load value 3";
}
