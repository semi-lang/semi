// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <string>

#include "test_common.hpp"

class CompilerCollectionInitializerTest : public CompilerTest {};

// TODO: Verify the exact instructions generated for collection initializers

TEST_F(CompilerCollectionInitializerTest, EmptyListInitializer) {
    const char* input = "List[]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 1);
    Instruction instr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr), expr.value.reg);
    ASSERT_EQ(OPERAND_T_C(instr), 0);
}

TEST_F(CompilerCollectionInitializerTest, EmptyDictInitializer) {
    const char* input = "Dict[]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 1);
    Instruction instr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr), expr.value.reg);
    ASSERT_EQ(OPERAND_T_C(instr), 0);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerSingleElement) {
    const char* input = "List[1]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 3);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr2 = GetInstruction(2);
    ASSERT_EQ(GET_OPCODE(instr2), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr2), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr2), 1);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerMultipleElements) {
    const char* input = "List[1, 2, 3]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 5);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr4 = GetInstruction(4);
    ASSERT_EQ(GET_OPCODE(instr4), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr4), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr4), 3);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerWithTrailingComma) {
    const char* input = "List[1, 2, 3,]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 5);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr4 = GetInstruction(4);
    ASSERT_EQ(GET_OPCODE(instr4), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr4), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr4), 3);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerExactly16Elements) {
    const char* input = "List[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 18);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr17 = GetInstruction(17);
    ASSERT_EQ(GET_OPCODE(instr17), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr17), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr17), 16);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializer17ElementsRequiresBatching) {
    const char* input = "List[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 20);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr17 = GetInstruction(17);
    ASSERT_EQ(GET_OPCODE(instr17), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr17), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr17), 16);

    Instruction instr19 = GetInstruction(19);
    ASSERT_EQ(GET_OPCODE(instr19), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr19), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr19), 1);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializer32ElementsRequiresDoubleBatching) {
    const char* input =
        "List[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, "
        "17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 35);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr17 = GetInstruction(17);
    ASSERT_EQ(GET_OPCODE(instr17), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr17), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr17), 16);

    Instruction instr34 = GetInstruction(34);
    ASSERT_EQ(GET_OPCODE(instr34), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(instr34), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr34), 16);
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerSinglePair) {
    const char* input = "Dict[1: 10]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 4);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr3 = GetInstruction(3);
    ASSERT_EQ(GET_OPCODE(instr3), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr3), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr3), 1);
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerMultiplePairs) {
    const char* input = "Dict[1: 10, 2: 20, 3: 30]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 8);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr7 = GetInstruction(7);
    ASSERT_EQ(GET_OPCODE(instr7), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr7), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr7), 3);
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerWithTrailingComma) {
    const char* input = "Dict[1: 10, 2: 20,]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 6);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr5 = GetInstruction(5);
    ASSERT_EQ(GET_OPCODE(instr5), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr5), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr5), 2);
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerExactly8Pairs) {
    const char* input = "Dict[1: 10, 2: 20, 3: 30, 4: 40, 5: 50, 6: 60, 7: 70, 8: 80]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 18);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr17 = GetInstruction(17);
    ASSERT_EQ(GET_OPCODE(instr17), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr17), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr17), 8);
}

TEST_F(CompilerCollectionInitializerTest, DictInitializer9PairsRequiresBatching) {
    const char* input = "Dict[1: 10, 2: 20, 3: 30, 4: 40, 5: 50, 6: 60, 7: 70, 8: 80, 9: 90]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 21);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr17 = GetInstruction(17);
    ASSERT_EQ(GET_OPCODE(instr17), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr17), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr17), 8);

    Instruction instr20 = GetInstruction(20);
    ASSERT_EQ(GET_OPCODE(instr20), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr20), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr20), 1);
}

TEST_F(CompilerCollectionInitializerTest, DictInitializer16PairsRequiresDoubleBatching) {
    const char* input =
        "Dict[1: 10, 2: 20, 3: 30, 4: 40, 5: 50, 6: 60, 7: 70, 8: 80, "
        "9: 90, 10: 100, 11: 110, 12: 120, 13: 130, 14: 140, 15: 150, 16: 160]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    ASSERT_EQ(GetCodeSize(), 35);

    Instruction instr0 = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr0), OP_NEW_COLLECTION);
    ASSERT_EQ(OPERAND_T_A(instr0), expr.value.reg);

    Instruction instr17 = GetInstruction(17);
    ASSERT_EQ(GET_OPCODE(instr17), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr17), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr17), 8);

    Instruction instr34 = GetInstruction(34);
    ASSERT_EQ(GET_OPCODE(instr34), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(instr34), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(instr34), 8);
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxColonInList) {
    const char* input = "List[1, 2: 20]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxNoColonInDict) {
    const char* input = "Dict[1: 10, 2]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxStartWithListThenDict) {
    const char* input = "List[1, 2, 3: 30]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, MixingListAndMapSyntaxStartWithDictThenList) {
    const char* input = "Dict[1: 10, 2: 20, 3]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN);
}

TEST_F(CompilerCollectionInitializerTest, ListInitializerWithComplexExpressions) {
    const char* input = "List[1 + 2, 3 * 4, 5 - 6]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    size_t codeSize = GetCodeSize();
    ASSERT_GT(codeSize, 2);

    Instruction lastInstr = GetInstruction(codeSize - 1);
    ASSERT_EQ(GET_OPCODE(lastInstr), OP_APPEND_LIST);
    ASSERT_EQ(OPERAND_T_A(lastInstr), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(lastInstr), 3);
}

TEST_F(CompilerCollectionInitializerTest, DictInitializerWithComplexExpressions) {
    const char* input = "Dict[1 + 1: 10 * 2, 2 - 1: 20 + 5]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    size_t codeSize = GetCodeSize();
    ASSERT_GT(codeSize, 2);

    Instruction lastInstr = GetInstruction(codeSize - 1);
    ASSERT_EQ(GET_OPCODE(lastInstr), OP_APPEND_MAP);
    ASSERT_EQ(OPERAND_T_A(lastInstr), expr.value.reg);
    ASSERT_EQ(OPERAND_T_B(lastInstr), 2);
}

TEST_F(CompilerCollectionInitializerTest, NestedListInitializers) {
    const char* input = "List[List[1, 2], List[3, 4]]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    size_t codeSize = GetCodeSize();
    ASSERT_GT(codeSize, 6);
}

TEST_F(CompilerCollectionInitializerTest, NestedDictInitializers) {
    const char* input = "Dict[1: Dict[10: 100], 2: Dict[20: 200]]";
    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);

    ASSERT_EQ(result, 0);
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG);

    size_t codeSize = GetCodeSize();
    ASSERT_GT(codeSize, 6);
}
