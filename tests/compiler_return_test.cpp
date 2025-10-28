// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "test_common.hpp"

class CompilerReturnTest : public CompilerTest {};

// Test Case 1: No previous return, now return without value
TEST_F(CompilerReturnTest, FirstReturnWithoutValue) {
    const char* source = R"(
        fn test() {
            return
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Function with return without value should parse successfully";
}

// Test Case 2: No previous return, now return one value
TEST_F(CompilerReturnTest, FirstReturnWithValue) {
    const char* source = R"(
        fn test() {
            return 42
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Function with return with value should parse successfully";
}

// Test Case 3: Previously returns 0 values, now returns 0 values (consistent)
TEST_F(CompilerReturnTest, ConsistentZeroValueReturns) {
    const char* source = R"(
        fn test() {
            if true {
                return
            }
            return
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Consistent zero-value returns should parse successfully";
}

// Test Case 4: Previously returns 1 value, now returns 1 value (consistent)
TEST_F(CompilerReturnTest, ConsistentOneValueReturns) {
    const char* source = R"(
        fn test() {
            if true {
                return 42
            }
            return 24
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Consistent one-value returns should parse successfully";
}

// Test Case 5: Previously returns 0 values, but now returns 1 value (inconsistent)
TEST_F(CompilerReturnTest, InconsistentZeroToOneValue) {
    const char* source = R"(
        fn test() {
            if true {
                return
            }
            return 42
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_INCONSISTENT_RETURN_COUNT)
        << "Inconsistent return count (0 -> 1) should fail with SEMI_ERROR_INCONSISTENT_RETURN_COUNT";
}

// Test Case 6: Previously returns 1 value, but now returns 0 values (inconsistent)
TEST_F(CompilerReturnTest, InconsistentOneToZeroValue) {
    const char* source = R"(
        fn test() {
            if true {
                return 42
            }
            return
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_INCONSISTENT_RETURN_COUNT)
        << "Inconsistent return count (1 -> 0) should fail with SEMI_ERROR_INCONSISTENT_RETURN_COUNT";
}

// Test Case: Return statement outside of function should fail
TEST_F(CompilerReturnTest, ReturnOutsideFunction) {
    const char* source = "return 42";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN)
        << "Return statement outside function should fail with SEMI_ERROR_UNEXPECTED_TOKEN";
}

// Test Case: Return statement in nested function scopes
TEST_F(CompilerReturnTest, NestedFunctionReturns) {
    const char* source = R"(
        fn outer() {
            fn inner() {
                return 1
            }
            return 2
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Nested function returns should parse successfully";
}

// Test Case: Multiple inconsistent returns in complex control flow
TEST_F(CompilerReturnTest, ComplexControlFlowInconsistentReturns) {
    const char* source = R"(
        fn test() {
            if true {
                if false {
                    return 1
                }
                return
            } else {
                return 2
            }
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_INCONSISTENT_RETURN_COUNT)
        << "Complex control flow with inconsistent returns should fail";
}

// Test Case: Return with different expression types
TEST_F(CompilerReturnTest, ReturnWithDifferentExpressionTypes) {
    const char* source = R"(
        fn test() {
            if true {
                return "hello"
            }
            return 3.14
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Return with different expression types should parse successfully";
}

// Test Case: Return in for loop
TEST_F(CompilerReturnTest, ReturnInForLoop) {
    const char* source = R"(
        fn test() {
            for i in 1..10 {
                if i == 5 {
                    return i
                }
            }
            return 0
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Return in for loop should parse successfully";
}

// Test Case: Return without proper separator should fail
TEST_F(CompilerReturnTest, ReturnWithoutSeparator) {
    const char* source = R"(
        fn test() {
            return 42 x := 1
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_UNEXPECTED_TOKEN)
        << "Return without proper separator should fail with SEMI_ERROR_UNEXPECTED_TOKEN";
}

// Test Case: Return with complex expression
TEST_F(CompilerReturnTest, ReturnWithComplexExpression) {
    const char* source = R"(
        fn test() {
            x := 5
            y := 10
            return x + y * 2
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Return with complex expression should parse successfully";
}

// Test Case: Empty function (implicit return)
TEST_F(CompilerReturnTest, EmptyFunction) {
    const char* source = R"(
        fn test() {
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Empty function should parse successfully";
}

// Test Case: Function with only explicit void return
TEST_F(CompilerReturnTest, ExplicitVoidReturn) {
    const char* source = R"(
        fn test() {
            x := 42
            return
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Function with explicit void return should parse successfully";
}