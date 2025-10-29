// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "test_common.hpp"

class ExprParserBooleanLedTest : public CompilerTest {};

struct BooleanConstantTestCase {
    const char* expression;
    PrattExprType expected_type;
    Value expected_value;
    const char* description;
};

struct BooleanVariableTestCase {
    const char* expression;
    size_t expected_code_size;
    const char* description;
};

// Test cases for constant-constant boolean operations
TEST_F(ExprParserBooleanLedTest, ConstantAndConstant) {
    BooleanConstantTestCase test_cases[] = {
        // AND operations: returns lhs if lhs is falsy, returns rhs if lhs is truthy
        {     "0 and 3",PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(0),"0 and 3 should return 0 (lhs is falsy)"                                                                           },
        {     "3 and 0",
         PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(0),
         "3 and 0 should return 0 (lhs is truthy, return rhs)"                                                                 },
        {     "3 and 5",
         PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(5),
         "3 and 5 should return 5 (lhs is truthy, return rhs)"                                                                 },
        { "0 and false", PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(0),        "0 and false should return 0 (lhs is falsy)"},
        {  "true and 7",
         PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(7),
         "true and 7 should return 7 (lhs is truthy, return rhs)"                                                              },
        {"false and 42",
         PRATT_EXPR_TYPE_CONSTANT, semiValueNewBool(false),
         "false and 42 should return false (lhs is falsy)"                                                                     },

        // OR operations: returns lhs if lhs is truthy, returns rhs if lhs is falsy
        {      "0 or 3", PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(3), "0 or 3 should return 3 (lhs is falsy, return rhs)"},
        {      "3 or 0", PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(3),            "3 or 0 should return 3 (lhs is truthy)"},
        {  "0 or false",
         PRATT_EXPR_TYPE_CONSTANT, semiValueNewBool(false),
         "0 or false should return false (lhs is falsy, return rhs)"                                                           },
        {  "false or 0",
         PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(0),
         "false or 0 should return 0 (lhs is falsy, return rhs)"                                                               },
        {  "true or 42",
         PRATT_EXPR_TYPE_CONSTANT,  semiValueNewBool(true),
         "true or 42 should return true (lhs is truthy)"                                                                       },
        {     "5 or 10", PRATT_EXPR_TYPE_CONSTANT,      semiValueNewInt(5),           "5 or 10 should return 5 (lhs is truthy)"},
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();

        PrattExpr expr;
        ErrorId result = ParseExpression(test_case.expression, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";

        // Check the actual returned value and type
        ASSERT_EQ(expr.type, test_case.expected_type) << test_case.description << " - type mismatch";
        ASSERT_TRUE(semiBuiltInEquals(expr.value.constant, test_case.expected_value)) << test_case.description;

        ASSERT_EQ(GetCodeSize(), 0) << test_case.description << " - no code should be generated";
    }
}

// Test cases for constant-variable boolean operations
TEST_F(ExprParserBooleanLedTest, ConstantAndVariable) {
    BooleanVariableTestCase test_cases[] = {
        {    "3 and x", 0,        "3 and x should return x (truthy lhs, return rhs)"},
        {    "0 and x", 0,         "0 and x should return 0 (falsy lhs, return lhs)"},
        { "true and x", 0,     "true and x should return x (truthy lhs, return rhs)"},
        {"false and x", 0, "false and x should return false (falsy lhs, return lhs)"},

        {     "3 or x", 0,         "3 or x should return 3 (truthy lhs, return lhs)"},
        {     "0 or x", 0,          "0 or x should return x (falsy lhs, return rhs)"},
        {  "true or x", 0,   "true or x should return true (truthy lhs, return lhs)"},
        { "false or x", 0,      "false or x should return x (falsy lhs, return rhs)"},
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();
        InitializeVariable("x");
        PrattExpr expr;

        ErrorId result = ParseExpression(test_case.expression, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(GetCodeSize(), test_case.expected_code_size) << test_case.description;
    }
}

// Test cases for variable-constant boolean operations
TEST_F(ExprParserBooleanLedTest, VariableAndConstant) {
    BooleanVariableTestCase test_cases[] = {
        {    "x and 3", 3,     "x and 3 should generate move + conditional jump + load"},
        {    "x and 0", 3,     "x and 0 should generate move + conditional jump + load"},
        { "x and true", 3,  "x and true should generate move + conditional jump + load"},
        {"x and false", 3, "x and false should generate move + conditional jump + load"},

        {     "x or 3", 3,      "x or 3 should generate move + conditional jump + load"},
        {     "x or 0", 3,      "x or 0 should generate move + conditional jump + load"},
        {  "x or true", 3,   "x or true should generate move + conditional jump + load"},
        { "x or false", 3,  "x or false should generate move + conditional jump + load"},
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();
        InitializeVariable("x");
        PrattExpr expr;

        ErrorId result = ParseExpression(test_case.expression, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << test_case.description << " - should be register";
        ASSERT_EQ(GetCodeSize(), test_case.expected_code_size) << test_case.description;

        VariableDescription* var = FindVariable("x");
        ASSERT_NE(var, nullptr) << "Variable 'x' should exist";

        // First instruction should be MOVE
        Instruction instr0 = GetInstruction(0);
        ASSERT_EQ(GET_OPCODE(instr0), OP_MOVE) << test_case.description << " - first instruction should be MOVE";

        // Second instruction should be C_JUMP
        Instruction instr1 = GetInstruction(1);
        ASSERT_EQ(GET_OPCODE(instr1), OP_C_JUMP) << test_case.description << " - second instruction should be C_JUMP";
        // Note: C_JUMP checks the result register, not the original variable register

        // Third instruction should load the constant
        Instruction instr2 = GetInstruction(2);
        bool is_load_bool  = GET_OPCODE(instr2) == OP_LOAD_BOOL;
        bool is_load_int   = GET_OPCODE(instr2) == OP_LOAD_INLINE_INTEGER;
        ASSERT_TRUE(is_load_bool || is_load_int) << test_case.description << " - third instruction should be load";
    }
}

// Test cases for variable-variable boolean operations
TEST_F(ExprParserBooleanLedTest, VariableAndVariable) {
    BooleanVariableTestCase test_cases[] = {
        {"x and y", 3, "x and y should generate move + conditional jump + move"},
        { "x or y", 3,  "x or y should generate move + conditional jump + move"},
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();
        InitializeVariable("x");
        InitializeVariable("y");
        PrattExpr expr;

        ErrorId result = ParseExpression(test_case.expression, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << test_case.description << " - should be register";
        ASSERT_EQ(GetCodeSize(), test_case.expected_code_size) << test_case.description;

        VariableDescription* var_x = FindVariable("x");
        VariableDescription* var_y = FindVariable("y");
        ASSERT_NE(var_x, nullptr) << "Variable 'x' should exist";
        ASSERT_NE(var_y, nullptr) << "Variable 'y' should exist";

        // First instruction should be MOVE
        Instruction instr0 = GetInstruction(0);
        ASSERT_EQ(GET_OPCODE(instr0), OP_MOVE) << test_case.description << " - first instruction should be MOVE";

        // Second instruction should be C_JUMP
        Instruction instr1 = GetInstruction(1);
        ASSERT_EQ(GET_OPCODE(instr1), OP_C_JUMP) << test_case.description << " - second instruction should be C_JUMP";

        // Third instruction should be MOVE
        Instruction instr2 = GetInstruction(2);
        ASSERT_EQ(GET_OPCODE(instr2), OP_MOVE) << test_case.description << " - third instruction should be MOVE";
    }
}

// Test cases for expression-expression boolean operations
TEST_F(ExprParserBooleanLedTest, ExpressionAndExpression) {
    BooleanVariableTestCase test_cases[] = {
        {      "x and (y + z)", 3,      "x and (y + z) should generate MOVE + C_JUMP + ADD"},
        {      "(x + y) and z", 3,      "(x + y) and z should generate ADD + C_JUMP + MOVE"},
        {       "x or (y + z)", 3,       "x or (y + z) should generate MOVE + C_JUMP + ADD"},
        {       "(x + y) or z", 3,       "(x + y) or z should generate ADD + C_JUMP + MOVE"},
        {"(x + y) and (z + w)", 3, "(x + y) and (z + w) should generate ADD + C_JUMP + ADD"},
        { "(x + y) or (z + w)", 3,  "(x + y) or (z + w) should generate ADD + C_JUMP + ADD"},
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();
        InitializeVariable("x");
        InitializeVariable("y");
        InitializeVariable("z");
        InitializeVariable("w");
        PrattExpr expr;

        ErrorId result = ParseExpression(test_case.expression, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << test_case.description << " - should be register";
        ASSERT_EQ(GetCodeSize(), test_case.expected_code_size) << test_case.description;

        // Should contain at least one C_JUMP instruction
        bool found_c_jump = false;
        for (size_t i = 0; i < GetCodeSize(); i++) {
            Instruction instr = GetInstruction(i);
            if (GET_OPCODE(instr) == OP_C_JUMP) {
                found_c_jump = true;
                break;
            }
        }
        ASSERT_TRUE(found_c_jump) << test_case.description << " - should contain C_JUMP instruction";
    }
}

// Test cases for boolean operations with assignment
TEST_F(ExprParserBooleanLedTest, BooleanWithAssignment) {
    struct BooleanAssignmentWithOperandsTestCase {
        const char* expression;
        size_t expected_code_size;
        uint8_t expected_opcode;
        uint16_t expected_constant_value;
        bool expected_inline_flag;
        const char* description;
    };

    BooleanAssignmentWithOperandsTestCase test_cases[] = {
        {   "y := 3 and 5", 1, OP_LOAD_INLINE_INTEGER, 5,  true,            "y := 3 and 5 should return 5 (load 5)"},
        {   "y := 0 and 3", 1, OP_LOAD_INLINE_INTEGER, 0,  true,            "y := 0 and 3 should return 0 (load 0)"},
        {    "y := 3 or 0", 1, OP_LOAD_INLINE_INTEGER, 3,  true,             "y := 3 or 0 should return 3 (load 3)"},
        {"y := 0 or false", 1,           OP_LOAD_BOOL, 0, false, "y := 0 or false should return false (load false)"},
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();

        ErrorId result = ParseStatement(test_case.expression, true);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(GetCodeSize(), test_case.expected_code_size) << test_case.description;

        VariableDescription* var = FindVariable("y");
        ASSERT_NE(var, nullptr) << "Variable 'y' should exist";

        // Should generate appropriate load instruction
        Instruction instr = GetInstruction(0);
        ASSERT_EQ(GET_OPCODE(instr), test_case.expected_opcode)
            << test_case.description << " - should be correct opcode";
        ASSERT_EQ(OPERAND_K_A(instr), var->registerId)
            << test_case.description << " - should load into variable's register";
        ASSERT_EQ(OPERAND_K_K(instr), test_case.expected_constant_value)
            << test_case.description << " - should load correct constant value";
        ASSERT_EQ(OPERAND_K_I(instr), test_case.expected_inline_flag)
            << test_case.description << " - should have correct inline flag";
    }
}

// Test cases for variable-constant boolean operations with assignment
TEST_F(ExprParserBooleanLedTest, VariableAndConstantWithAssignment) {
    struct VariableConstantAssignmentTestCase {
        const char* expression;
        size_t expected_code_size;

        // Expected MOVE instruction
        TInstruction expected_move;

        // Expected C_JUMP instruction
        KInstruction expected_cjump;

        // Expected LOAD instruction
        KInstruction expected_load;

        const char* description;
    };

    VariableConstantAssignmentTestCase test_cases[] = {
        {"y := x and 3",
         3, // MOVE: y = x (move x to y's register)
 {.opcode = OP_MOVE, .destReg = 1, .srcReg1 = 0, .srcReg2 = 0, .constFlag1 = false, .constFlag2 = false},
         // C_JUMP: for AND, if y is truthy, skip ahead
         {.opcode = OP_C_JUMP, .destReg = 1, .constant = 2, .inlineFlag = true, .signFlag = true},
         // LOAD_INLINE_INTEGER: y = 3 (load 3 into y's register)
         {.opcode = OP_LOAD_INLINE_INTEGER, .destReg = 1, .constant = 3, .inlineFlag = true, .signFlag = true},
         "y := x and 3: if x is truthy return 3, if falsy return x"},
        { "y := x or 0",
         3, // MOVE: y = x (move x to y's register)
 {.opcode = OP_MOVE, .destReg = 1, .srcReg1 = 0, .srcReg2 = 0, .constFlag1 = false, .constFlag2 = false},
         // C_JUMP: for OR, if y is falsy, skip ahead
         {.opcode = OP_C_JUMP, .destReg = 1, .constant = 2, .inlineFlag = false, .signFlag = true},
         // LOAD_INLINE_INTEGER: y = 0 (load 0 into y's register)
         {.opcode = OP_LOAD_INLINE_INTEGER, .destReg = 1, .constant = 0, .inlineFlag = true, .signFlag = true},
         "y := x or 0: if x is truthy return x, if falsy return 0" }
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();
        InitializeVariable("x");

        ErrorId result = ParseStatement(test_case.expression, true);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(GetCodeSize(), test_case.expected_code_size) << test_case.description;

        VariableDescription* var_x = FindVariable("x");
        VariableDescription* var_y = FindVariable("y");
        ASSERT_NE(var_x, nullptr) << "Variable 'x' should exist";
        ASSERT_NE(var_y, nullptr) << "Variable 'y' should exist";

        // First instruction should be MOVE (x to y's register)
        Instruction instr0 = GetInstruction(0);
        ASSERT_T_INSTRUCTION_EQ(instr0, test_case.expected_move, test_case.description);

        // Second instruction should be C_JUMP
        Instruction instr1 = GetInstruction(1);
        ASSERT_K_INSTRUCTION_EQ(instr1, test_case.expected_cjump, test_case.description);

        // Third instruction should load the constant
        Instruction instr2 = GetInstruction(2);
        ASSERT_K_INSTRUCTION_EQ(instr2, test_case.expected_load, test_case.description);
    }
}

// Test cases for constant-variable boolean operations with assignment
TEST_F(ExprParserBooleanLedTest, ConstantAndVariableWithAssignment) {
    struct ConstantVariableAssignmentTestCase {
        const char* expression;
        size_t expected_code_size;
        TInstruction expected_move;
        const char* description;
    };

    ConstantVariableAssignmentTestCase test_cases[] = {
        {"y := 3 and x",
         1, // MOVE: y = x (constant folding optimization)
 {.opcode = OP_MOVE, .destReg = 1, .srcReg1 = 0, .srcReg2 = 0, .constFlag1 = false, .constFlag2 = false},
         "y := 3 and x should return x (truthy lhs, constant folding)"},
        { "y := 0 or x",
         1, // MOVE: y = x (constant folding optimization)
 {.opcode = OP_MOVE, .destReg = 1, .srcReg1 = 0, .srcReg2 = 0, .constFlag1 = false, .constFlag2 = false},
         "y = 0 or x should return x (falsy lhs, constant folding)"   }
    };

    for (const auto& test_case : test_cases) {
        TearDown();
        SetUp();
        InitializeVariable("x");

        ErrorId result = ParseStatement(test_case.expression, true);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(GetCodeSize(), test_case.expected_code_size) << test_case.description;

        VariableDescription* var_x = FindVariable("x");
        VariableDescription* var_y = FindVariable("y");
        ASSERT_NE(var_x, nullptr) << "Variable 'x' should exist";
        ASSERT_NE(var_y, nullptr) << "Variable 'y' should exist";

        // Should generate single MOVE instruction (constant folding optimization)
        Instruction instr0 = GetInstruction(0);
        ASSERT_T_INSTRUCTION_EQ(instr0, test_case.expected_move, test_case.description);
    }
}
