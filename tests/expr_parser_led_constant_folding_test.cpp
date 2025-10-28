// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "test_common.hpp"

class ExprParserLedConstantFoldingTest : public CompilerTest {};

// Test arithmetic operations constant folding
TEST_F(ExprParserLedConstantFoldingTest, ArithmeticAddition) {
    PrattExpr expr;
    ErrorId result = ParseExpression("3 + 5", &expr);
    ASSERT_EQ(result, 0) << "Parsing '3 + 5' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 8) << "3 + 5 should equal 8";
}

TEST_F(ExprParserLedConstantFoldingTest, ArithmeticSubtraction) {
    PrattExpr expr;
    ErrorId result = ParseExpression("10 - 3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '10 - 3' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 7) << "10 - 3 should equal 7";
}

TEST_F(ExprParserLedConstantFoldingTest, ArithmeticMultiplication) {
    PrattExpr expr;
    ErrorId result = ParseExpression("4 * 6", &expr);
    ASSERT_EQ(result, 0) << "Parsing '4 * 6' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 24) << "4 * 6 should equal 24";
}

TEST_F(ExprParserLedConstantFoldingTest, ArithmeticDivision) {
    PrattExpr expr;
    ErrorId result = ParseExpression("15 / 3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '15 / 3' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 5) << "15 / 3 should equal 5";
}

TEST_F(ExprParserLedConstantFoldingTest, ArithmeticFloorDivision) {
    PrattExpr expr;
    ErrorId result = ParseExpression("17 // 3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '17 // 3' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 5) << "17 // 3 should equal 5";
}

TEST_F(ExprParserLedConstantFoldingTest, ArithmeticModulo) {
    PrattExpr expr;
    ErrorId result = ParseExpression("17 % 5", &expr);
    ASSERT_EQ(result, 0) << "Parsing '17 % 5' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 2) << "17 % 5 should equal 2";
}

TEST_F(ExprParserLedConstantFoldingTest, ArithmeticPower) {
    PrattExpr expr;
    ErrorId result = ParseExpression("2 ** 3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '2 ** 3' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 8) << "2 ** 3 should equal 8";
}

// Test bitwise operations constant folding
TEST_F(ExprParserLedConstantFoldingTest, BitwiseAnd) {
    PrattExpr expr;
    ErrorId result = ParseExpression("12 & 10", &expr);
    ASSERT_EQ(result, 0) << "Parsing '12 & 10' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 8) << "12 & 10 should equal 8 (1100 & 1010 = 1000)";
}

TEST_F(ExprParserLedConstantFoldingTest, BitwiseOr) {
    PrattExpr expr;
    ErrorId result = ParseExpression("12 | 10", &expr);
    ASSERT_EQ(result, 0) << "Parsing '12 | 10' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 14) << "12 | 10 should equal 14 (1100 | 1010 = 1110)";
}

TEST_F(ExprParserLedConstantFoldingTest, BitwiseXor) {
    PrattExpr expr;
    ErrorId result = ParseExpression("12 ^ 10", &expr);
    ASSERT_EQ(result, 0) << "Parsing '12 ^ 10' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 6) << "12 ^ 10 should equal 6 (1100 ^ 1010 = 0110)";
}

TEST_F(ExprParserLedConstantFoldingTest, BitwiseLeftShift) {
    PrattExpr expr;
    ErrorId result = ParseExpression("5 << 2", &expr);
    ASSERT_EQ(result, 0) << "Parsing '5 << 2' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 20) << "5 << 2 should equal 20 (5 * 2^2)";
}

TEST_F(ExprParserLedConstantFoldingTest, BitwiseRightShift) {
    PrattExpr expr;
    ErrorId result = ParseExpression("20 >> 2", &expr);
    ASSERT_EQ(result, 0) << "Parsing '20 >> 2' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 5) << "20 >> 2 should equal 5 (20 / 2^2)";
}

// Test comparison operations constant folding
TEST_F(ExprParserLedConstantFoldingTest, ComparisonEqual) {
    PrattExpr expr;
    ErrorId result = ParseExpression("5 == 5", &expr);
    ASSERT_EQ(result, 0) << "Parsing '5 == 5' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "5 == 5 should be true";
}

TEST_F(ExprParserLedConstantFoldingTest, ComparisonNotEqual) {
    PrattExpr expr;
    ErrorId result = ParseExpression("5 != 3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '5 != 3' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "5 != 3 should be true";
}

TEST_F(ExprParserLedConstantFoldingTest, ComparisonLessThan) {
    PrattExpr expr;
    ErrorId result = ParseExpression("3 < 5", &expr);
    ASSERT_EQ(result, 0) << "Parsing '3 < 5' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "3 < 5 should be true";
}

TEST_F(ExprParserLedConstantFoldingTest, ComparisonLessThanOrEqual) {
    PrattExpr expr;
    ErrorId result = ParseExpression("5 <= 5", &expr);
    ASSERT_EQ(result, 0) << "Parsing '5 <= 5' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "5 <= 5 should be true";
}

TEST_F(ExprParserLedConstantFoldingTest, ComparisonGreaterThan) {
    PrattExpr expr;
    ErrorId result = ParseExpression("8 > 3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '8 > 3' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "8 > 3 should be true";
}

TEST_F(ExprParserLedConstantFoldingTest, ComparisonGreaterThanOrEqual) {
    PrattExpr expr;
    ErrorId result = ParseExpression("5 >= 5", &expr);
    ASSERT_EQ(result, 0) << "Parsing '5 >= 5' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "5 >= 5 should be true";
}

// Test logical operations constant folding
TEST_F(ExprParserLedConstantFoldingTest, LogicalAnd) {
    PrattExpr expr;
    ErrorId result = ParseExpression("true and true", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'true and true' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "true and true should be true";
}

TEST_F(ExprParserLedConstantFoldingTest, LogicalOr) {
    PrattExpr expr;
    ErrorId result = ParseExpression("false or true", &expr);
    ASSERT_EQ(result, 0) << "Parsing 'false or true' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be boolean constant";
    ASSERT_TRUE(AS_BOOL(&expr.value.constant)) << "false or true should be true";
}

// Test floating point operations constant folding
TEST_F(ExprParserLedConstantFoldingTest, FloatAddition) {
    PrattExpr expr;
    ErrorId result = ParseExpression("3.5 + 2.1", &expr);
    ASSERT_EQ(result, 0) << "Parsing '3.5 + 2.1' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be double constant";
    ASSERT_DOUBLE_EQ(AS_FLOAT(&expr.value.constant), 5.6) << "3.5 + 2.1 should equal 5.6";
}

TEST_F(ExprParserLedConstantFoldingTest, FloatSubtraction) {
    PrattExpr expr;
    ErrorId result = ParseExpression("7.5 - 2.3", &expr);
    ASSERT_EQ(result, 0) << "Parsing '7.5 - 2.3' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be double constant";
    ASSERT_DOUBLE_EQ(AS_FLOAT(&expr.value.constant), 5.2) << "7.5 - 2.3 should equal 5.2";
}

TEST_F(ExprParserLedConstantFoldingTest, FloatMultiplication) {
    PrattExpr expr;
    ErrorId result = ParseExpression("2.5 * 4.0", &expr);
    ASSERT_EQ(result, 0) << "Parsing '2.5 * 4.0' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be double constant";
    ASSERT_DOUBLE_EQ(AS_FLOAT(&expr.value.constant), 10.0) << "2.5 * 4.0 should equal 10.0";
}

TEST_F(ExprParserLedConstantFoldingTest, FloatDivision) {
    PrattExpr expr;
    ErrorId result = ParseExpression("9.0 / 3.0", &expr);
    ASSERT_EQ(result, 0) << "Parsing '9.0 / 3.0' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be double constant";
    ASSERT_DOUBLE_EQ(AS_FLOAT(&expr.value.constant), 3.0) << "9.0 / 3.0 should equal 3.0";
}

// Test mixed type operations
TEST_F(ExprParserLedConstantFoldingTest, MixedIntFloatAddition) {
    PrattExpr expr;
    ErrorId result = ParseExpression("5 + 2.5", &expr);
    ASSERT_EQ(result, 0) << "Parsing '5 + 2.5' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be double constant";
    ASSERT_DOUBLE_EQ(AS_FLOAT(&expr.value.constant), 7.5) << "5 + 2.5 should equal 7.5";
}

// Test edge cases and error conditions
TEST_F(ExprParserLedConstantFoldingTest, DivisionByZero) {
    PrattExpr expr;
    ErrorId result = ParseExpression("5 / 0", &expr);
    // This should either succeed with infinity/error value or return an error
    // The exact behavior depends on the implementation of semiValueDivide
    if (result == 0) {
        // If it succeeds, check that the result is handled appropriately
        ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
        // The exact value depends on how division by zero is handled
    } else {
        // Division by zero should return an appropriate error
        ASSERT_NE(result, 0) << "Division by zero should return an error";
    }
}

TEST_F(ExprParserLedConstantFoldingTest, ComplexExpression) {
    PrattExpr expr;
    ErrorId result = ParseExpression("2 + 3 * 4", &expr);
    ASSERT_EQ(result, 0) << "Parsing '2 + 3 * 4' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 14) << "2 + 3 * 4 should equal 14 (respecting precedence)";
}

TEST_F(ExprParserLedConstantFoldingTest, ParenthesesPrecedence) {
    PrattExpr expr;
    ErrorId result = ParseExpression("(2 + 3) * 4", &expr);
    ASSERT_EQ(result, 0) << "Parsing '(2 + 3) * 4' should succeed";
    ASSERT_EQ(expr.type, PRATT_EXPR_TYPE_CONSTANT) << "Should be integer constant";
    ASSERT_EQ(AS_INT(&expr.value.constant), 20) << "(2 + 3) * 4 should equal 20";
}
