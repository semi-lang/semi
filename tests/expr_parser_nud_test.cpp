// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "test_common.hpp"

class ExprParserNudTest : public CompilerTest {};

// Test constantNud function
TEST_F(ExprParserNudTest, ConstantNonStringNud) {
    struct {
        const char* input;
        Value expected_value;
    } test_cases[] = {
        { "true",  semiValueBoolCreate(true)},
        {"false", semiValueBoolCreate(false)},
        {   "42",     semiValueIntCreate(42)},
        { "-123",   semiValueIntCreate(-123)},
        { "3.14", semiValueFloatCreate(3.14)},
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        PrattExpr expr;
        ErrorId result = ParseExpression(test_cases[i].input, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_cases[i].input << "' should succeed";
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT)
            << "Expression type mismatch for input '" << test_cases[i].input << "'";
        ASSERT_TRUE(semiBuiltInEquals(expr.value.constant, test_cases[i].expected_value))
            << "Value mismatch for input '" << test_cases[i].input << "'";
    }
}

TEST_F(ExprParserNudTest, ConstantStringNud) {
    const char* input    = "\"hello\"";
    const char* raw_data = "hello";
    Value expected_value = semiValueStringCreate(&vm->gc, (char*)raw_data, strlen(raw_data));

    PrattExpr expr;
    ErrorId result = ParseExpression(input, &expr);
    ASSERT_EQ(result, 0) << "Parsing '" << input << "' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Expression type mismatch for input '" << input << "'";
    ASSERT_TRUE(semiBuiltInEquals(expr.value.constant, expected_value)) << "Value mismatch for input '" << input << "'";
}

// Test unaryNud function - constant folding
TEST_F(ExprParserNudTest, UnaryNudConstantFolding) {
    struct ConstantTestCase {
        const char* operator_str;
        const char* operandExpr;
        PrattExprType expected_type;
        Value expected_value;
        ErrorId expected_error;
        const char* description;
    };

    ConstantTestCase test_cases[] = {
        // Boolean NOT operations
        {"!","true", PRATT_EXPR_TYPE_CONSTANT,semiValueBoolCreate(false),0,      "!true constant folding"                                                                                 },
        {"!",     "false", PRATT_EXPR_TYPE_CONSTANT,   semiValueBoolCreate(true),     0,     "!false constant folding"},
        {"!",        "42", PRATT_EXPR_TYPE_CONSTANT,  semiValueBoolCreate(false),     0,        "!42 constant folding"},
        {"!",         "0", PRATT_EXPR_TYPE_CONSTANT,   semiValueBoolCreate(true),     0,         "!0 constant folding"},
        {"!",      "3.14", PRATT_EXPR_TYPE_CONSTANT,  semiValueBoolCreate(false),     0,      "!3.14 constant folding"},
        {"!",       "0.0", PRATT_EXPR_TYPE_CONSTANT,   semiValueBoolCreate(true),     0,       "!0.0 constant folding"},
        {"!", "\"hello\"", PRATT_EXPR_TYPE_CONSTANT,  semiValueBoolCreate(false),     0, "!\"hello\" constant folding"},
        {"!",      "\"\"", PRATT_EXPR_TYPE_CONSTANT,   semiValueBoolCreate(true),     0,      "!\"\" constant folding"},

        // Arithmetic negation operations
        {"-",        "42", PRATT_EXPR_TYPE_CONSTANT,     semiValueIntCreate(-42),     0,        "-42 constant folding"},
        {"-",         "0", PRATT_EXPR_TYPE_CONSTANT,       semiValueIntCreate(0),     0,         "-0 constant folding"},
        {"-",      "3.14", PRATT_EXPR_TYPE_CONSTANT, semiValueFloatCreate(-3.14),     0,      "-3.14 constant folding"},
        {"-",       "0.0", PRATT_EXPR_TYPE_CONSTANT,   semiValueFloatCreate(0.0),     0,       "-0.0 constant folding"},

        // Bitwise NOT operations
        {"~",        "42", PRATT_EXPR_TYPE_CONSTANT,     semiValueIntCreate(~42),     0,        "~42 constant folding"},
        {"~",         "0", PRATT_EXPR_TYPE_CONSTANT,      semiValueIntCreate(~0),     0,         "~0 constant folding"},
        {"~",       "255", PRATT_EXPR_TYPE_CONSTANT,    semiValueIntCreate(~255),     0,       "~255 constant folding"},

        // Invalid operations should fail
        {"-",
         "true",    PRATT_EXPR_TYPE_UNSET,
         semiValueBoolCreate(false),
         SEMI_ERROR_UNEXPECTED_TYPE,       "- on bool should fail"                                                    },
        {"-",
         "\"hello\"",    PRATT_EXPR_TYPE_UNSET,
         semiValueBoolCreate(false),
         SEMI_ERROR_UNEXPECTED_TYPE,     "- on string should fail"                                                    },
        {"~",
         "true",    PRATT_EXPR_TYPE_UNSET,
         semiValueBoolCreate(false),
         SEMI_ERROR_UNEXPECTED_TYPE,       "~ on bool should fail"                                                    },
        {"~",
         "3.14",    PRATT_EXPR_TYPE_UNSET,
         semiValueBoolCreate(false),
         SEMI_ERROR_UNEXPECTED_TYPE,     "~ on double should fail"                                                    },
        {"~",
         "\"hello\"",    PRATT_EXPR_TYPE_UNSET,
         semiValueBoolCreate(false),
         SEMI_ERROR_UNEXPECTED_TYPE,     "~ on string should fail"                                                    },
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        const ConstantTestCase& test_case = test_cases[i];

        // Construct full expression
        std::string full_expr = std::string(test_case.operator_str) + test_case.operandExpr;
        PrattExpr expr;

        ErrorId result = ParseExpression(full_expr.c_str(), &expr);

        if (test_case.expected_error != 0) {
            ASSERT_EQ(result, test_case.expected_error)
                << "Expected error " << test_case.expected_error << " for test case: " << test_case.description;
            continue;
        }

        ASSERT_EQ(result, 0) << "Parsing should succeed for test case: " << test_case.description;

        ASSERT_EQ(expr.type, test_case.expected_type)
            << "Expression type mismatch for test case: " << test_case.description;

        ASSERT_TRUE(semiBuiltInEquals(expr.value.constant, test_case.expected_value))
            << "Value mismatch for test case: " << test_case.description;

        // Verify no instructions are generated for constant folding
        ASSERT_EQ(GetCodeSize(), 0) << "Constant folding should generate no instructions for test case: "
                                    << test_case.description;
    }
}

// Test unaryNud function - variables and code generation
TEST_F(ExprParserNudTest, UnaryNudVariablesAndCodeGen) {
    struct VariableTestCase {
        const char* operator_str;
        const char* variable_name;
        ErrorId expected_error;
        uint32_t expected_opcode;
        const char* description;
    };

    VariableTestCase test_cases[] = {
        // Operations on initialized variables should succeed
        {"!", "x", 0,       OP_BOOL_NOT, "!x variable"},
        {"-", "x", 0,         OP_NEGATE, "-x variable"},
        {"~", "x", 0, OP_BITWISE_INVERT, "~x variable"},
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        const VariableTestCase& test_case = test_cases[i];

        // Reset test environment
        TearDown();
        SetUp();

        // Initialize the variable with the specified state
        InitializeVariable(test_case.variable_name);

        // Construct full expression
        std::string full_expr = std::string(test_case.operator_str) + test_case.variable_name;
        PrattExpr expr;

        ErrorId result = ParseExpression(full_expr.c_str(), &expr);

        if (test_case.expected_error != 0) {
            ASSERT_EQ(result, test_case.expected_error)
                << "Expected error " << test_case.expected_error << " for test case: " << test_case.description;

            // Verify no code was generated for failed operations
            ASSERT_EQ(GetCodeSize(), 0) << "Should not generate any instructions for test case: "
                                        << test_case.description;
            continue;
        }

        ASSERT_EQ(result, 0) << "Parsing should succeed for test case: " << test_case.description;
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG)
            << "Result should be in register for test case: " << test_case.description;
        ASSERT_NE(expr.value.reg, INVALID_LOCAL_REGISTER_ID)
            << "Register should be valid for test case: " << test_case.description;

        // Verify exactly 2 instructions: MOVE + unary operation
        ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions for test case: "
                                    << test_case.description;

        Instruction moveInst  = GetInstruction(0);
        Instruction unaryInst = GetInstruction(1);

        // Verify instruction opcodes
        ASSERT_EQ(moveInst & OPCODE_MASK, OP_MOVE)
            << "First instruction should be MOVE for test case: " << test_case.description;
        ASSERT_EQ(unaryInst & OPCODE_MASK, test_case.expected_opcode)
            << "Second instruction opcode mismatch for test case: " << test_case.description;

        // Verify operands - destination and source should be the same register
        uint8_t move_dst  = OPERAND_T_A(moveInst);
        uint8_t unary_dst = OPERAND_T_A(unaryInst);
        uint8_t unary_src = OPERAND_T_B(unaryInst);

        ASSERT_EQ(move_dst, unary_dst) << "MOVE destination should match unary destination for test case: "
                                       << test_case.description;
        ASSERT_EQ(move_dst, unary_src) << "MOVE destination should match unary source for test case: "
                                       << test_case.description;
    }
}

// Test unaryNud function - double unary operations (!!x, --x, ~~x) which generate 3 opcodes
TEST_F(ExprParserNudTest, UnaryNudRegisterAndCodeGen) {
    struct DoubleUnaryTestCase {
        const char* expression;
        uint32_t expected_opcode;
        const char* description;
    };

    DoubleUnaryTestCase test_cases[] = {
        {"!!x",       OP_BOOL_NOT,    "!!x double boolean negation"},
        {"--x",         OP_NEGATE, "--x double arithmetic negation"},
        {"~~x", OP_BITWISE_INVERT,   "~~x double bitwise inversion"},
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        const DoubleUnaryTestCase& test_case = test_cases[i];

        // Reset test environment
        TearDown();
        SetUp();

        // Initialize a variable
        InitializeVariable("x");

        // Test the double unary operation
        PrattExpr expr;
        ErrorId result = ParseExpression(test_case.expression, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_case.expression << "' should succeed";
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Result should be in register for " << test_case.description;
        ASSERT_NE(expr.value.reg, INVALID_LOCAL_REGISTER_ID)
            << "Register should be valid for " << test_case.description;

        // Verify exactly 3 instructions are generated
        ASSERT_EQ(GetCodeSize(), 3) << "Should generate exactly 3 instructions for " << test_case.description;

        Instruction moveInst  = GetInstruction(0);
        Instruction first_op  = GetInstruction(1);
        Instruction second_op = GetInstruction(2);

        // Verify instruction opcodes
        ASSERT_EQ(moveInst & OPCODE_MASK, OP_MOVE) << "First instruction should be MOVE for " << test_case.description;
        ASSERT_EQ(first_op & OPCODE_MASK, test_case.expected_opcode)
            << "Second instruction should be " << test_case.expected_opcode << " for " << test_case.description;
        ASSERT_EQ(second_op & OPCODE_MASK, test_case.expected_opcode)
            << "Third instruction should be " << test_case.expected_opcode << " for " << test_case.description;

        // Verify the instruction sequence forms a valid chain:
        // MOVE: temp1 = x
        // OP1:  temp2 = op(temp1)
        // OP2:  temp3 = op(temp2)
        uint8_t move_dst   = OPERAND_T_A(moveInst);
        uint8_t move_src   = OPERAND_T_B(moveInst);
        uint8_t first_dst  = OPERAND_T_A(first_op);
        uint8_t first_src  = OPERAND_T_B(first_op);
        uint8_t second_dst = OPERAND_T_A(second_op);
        uint8_t second_src = OPERAND_T_B(second_op);

        // The chain should be: move_dst -> first_src, first_dst -> second_src
        ASSERT_EQ(move_dst, first_src) << "MOVE destination should match first op source for " << test_case.description;
        ASSERT_EQ(first_dst, second_src) << "First op destination should match second op source for "
                                         << test_case.description;

        // The final result should be in the register returned by the expression
        ASSERT_EQ(second_dst, expr.value.reg)
            << "Second op destination should match expression result register for " << test_case.description;
    }
}

// Test parenthesisNud function
TEST_F(ExprParserNudTest, ParenthesisNudSimple) {
    PrattExpr expr;
    ErrorId result = ParseExpression("(42)", &expr);
    ASSERT_EQ(result, 0) << "Parsing '(42)' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 42) << "Value should be 42";
}

TEST_F(ExprParserNudTest, ParenthesisNudNested) {
    PrattExpr expr;
    ErrorId result = ParseExpression("((true))", &expr);
    ASSERT_EQ(result, 0) << "Parsing '((true))' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "Value should be true";
}

TEST_F(ExprParserNudTest, ParenthesisNudUnaryExpression) {
    PrattExpr expr;
    ErrorId result = ParseExpression("(-42)", &expr);
    ASSERT_EQ(result, 0) << "Parsing '(-42)' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), -42) << "Value should be -42";
}

TEST_F(ExprParserNudTest, ParenthesisNudMissingClose) {
    PrattExpr expr;
    ErrorId result = ParseExpression("(42", &expr);
    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN) << "Should fail on missing closing parenthesis";
}

TEST_F(ExprParserNudTest, ParenthesisNudEmpty) {
    PrattExpr expr;
    ErrorId result = ParseExpression("()", &expr);
    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN) << "Should fail on empty parentheses";
}

TEST_F(ExprParserNudTest, ParenthesisNudMaxBracketDepth) {
    // Create a string with maximum bracket depth
    std::string deep_parens = std::string(MAX_BRACKET_COUNT, '(') + "42" + std::string(MAX_BRACKET_COUNT, ')');
    PrattExpr expr;
    ErrorId result = ParseExpression(deep_parens.c_str(), &expr);
    ASSERT_EQ(result, 0) << "Parsing maximum bracket depth should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 42) << "Value should be 42";
}

TEST_F(ExprParserNudTest, ParenthesisNudExceedMaxBracketDepth) {
    // Create a string with one more than maximum bracket depth
    std::string too_deep_parens =
        std::string(MAX_BRACKET_COUNT + 1, '(') + "42" + std::string(MAX_BRACKET_COUNT + 1, ')');
    PrattExpr expr;
    ErrorId result = ParseExpression(too_deep_parens.c_str(), &expr);
    ASSERT_EQ(result, SEMI_ERROR_MAXMUM_BRACKET_REACHED) << "Should fail on exceeding bracket depth";
}

// Test edge cases and error conditions
TEST_F(ExprParserNudTest, UnexpectedTokenError) {
    PrattExpr expr;
    ErrorId result = ParseExpression("}", &expr);
    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN) << "Should fail on unexpected token";
}

TEST_F(ExprParserNudTest, UnexpectedEndOfFileError) {
    PrattExpr expr;
    ErrorId result = ParseExpression("", &expr);
    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_END_OF_FILE) << "Should fail on empty input";
}

TEST_F(ExprParserNudTest, TypeIdentifierNud) {
    struct {
        const char* input;
        BaseValueType expected_type;
    } test_cases[] = {
        {  "Bool",   BASE_VALUE_TYPE_BOOL},
        {   "Int",    BASE_VALUE_TYPE_INT},
        { "Float",  BASE_VALUE_TYPE_FLOAT},
        {"String", BASE_VALUE_TYPE_STRING},
        {  "List",   BASE_VALUE_TYPE_LIST},
        {  "Dict",   BASE_VALUE_TYPE_DICT},
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        PrattExpr expr;
        ErrorId result = ParseExpression(test_cases[i].input, &expr);
        ASSERT_EQ(result, 0) << "Parsing '" << test_cases[i].input << "' should succeed";
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_TYPE) << "Should be type for '" << test_cases[i].input << "'";
        ASSERT_EQ(expr.value.type, test_cases[i].expected_type) << "Type mismatch for '" << test_cases[i].input << "'";
        ASSERT_EQ(GetCodeSize(), 0) << "Should generate no instructions for '" << test_cases[i].input << "'";
    }
}

TEST_F(ExprParserNudTest, CodeGen_ConstantFolding_NoInstructionsGenerated) {
    // Test that constants are folded at compile time and generate no instructions

    // Test -42
    PrattExpr expr;
    ErrorId result = ParseExpression("-42", &expr);
    ASSERT_EQ(result, 0) << "Parsing '-42' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be constant integer";
    ASSERT_EQ(AS_INT(&expr.value.constant), -42) << "Value should be -42";
    ASSERT_EQ(GetCodeSize(), 0) << "Should generate no instructions";

    // Test !true
    result = ParseExpression("!true", &expr);
    ASSERT_EQ(result, 0) << "Parsing '!true' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be constant boolean";
    ASSERT_FALSE(AS_BOOL(&expr.value.constant)) << "Value should be false";
    ASSERT_EQ(GetCodeSize(), 0) << "Should generate no instructions";

    // Test ~255
    result = ParseExpression("~255", &expr);
    ASSERT_EQ(result, 0) << "Parsing '~255' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be constant integer";
    ASSERT_EQ(AS_INT(&expr.value.constant), ~255) << "Value should be ~255";
    ASSERT_EQ(GetCodeSize(), 0) << "Should generate no instructions";
}

TEST_F(ExprParserNudTest, CodeGen_DirectVariableInitialization) {
    // Now test unary operations on the properly initialized variable
    struct {
        const char* source;
        uint32_t expected_opcode;
    } test_cases[] = {
        {"-x",         OP_NEGATE},
        {"~x", OP_BITWISE_INVERT},
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        TearDown();
        SetUp();

        // Initialize a variable directly using the helper method
        InitializeVariable("x");

        PrattExpr expr;
        ErrorId errorId = ParseExpression(test_cases[i].source, &expr);

        ASSERT_EQ(errorId, 0) << "Parsing '" << test_cases[i].source << "' should succeed";
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_REG) << "Result should be in local register";

        // Verify exactly 2 instructions: MOVE + unary operation
        ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

        Instruction moveInst  = GetInstruction(0);
        Instruction unaryInst = GetInstruction(1);

        ASSERT_EQ(moveInst & OPCODE_MASK, OP_MOVE) << "First instruction should be MOVE";
        ASSERT_EQ(unaryInst & OPCODE_MASK, test_cases[i].expected_opcode)
            << "Second instruction should be " << test_cases[i].expected_opcode;

        // Verify that both instructions use the same register
        uint8_t move_dst  = OPERAND_T_A(moveInst);
        uint8_t unary_dst = OPERAND_T_A(unaryInst);
        uint8_t unary_src = OPERAND_T_B(unaryInst);

        ASSERT_EQ(move_dst, unary_dst) << "MOVE destination should match unary operation destination";
        ASSERT_EQ(move_dst, unary_src) << "MOVE destination should match unary operation source";
    }
}
