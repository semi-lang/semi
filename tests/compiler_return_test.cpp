// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=0 -> @test

[Instructions:test]
0: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=1 -> @test

[Instructions:test]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x002A i=T s=T
1: OP_RETURN                A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @test

[Instructions:test]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP                A=0x00 K=0x0002 i=F s=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
3: OP_CLOSE_UPVALUES        A=0x00 B=0x00 C=0x00 kb=F kc=F
4: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
5: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=1 -> @test

[Instructions:test]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP                A=0x00 K=0x0003 i=F s=T
2: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x002A i=T s=T
3: OP_RETURN                A=0x00 B=0x00 C=0x00 kb=F kc=F
4: OP_CLOSE_UPVALUES        A=0x00 B=0x00 C=0x00 kb=F kc=F
5: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0018 i=T s=T
6: OP_RETURN                A=0x00 B=0x00 C=0x00 kb=F kc=F
7: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0001 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=1 -> @inner
K[1]: FunctionProto arity=0 coarity=1 maxStackSize=2 -> @outer

[Instructions:outer]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_RETURN                A=0x01 B=0x00 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:inner]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_RETURN                A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0002 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: String "hello" length=5
K[1]: Float 3.14
K[2]: FunctionProto arity=0 coarity=1 maxStackSize=1 -> @test

[Instructions:test]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP                A=0x00 K=0x0003 i=F s=T
2: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
3: OP_RETURN                A=0x00 B=0x00 C=0x00 kb=F kc=F
4: OP_CLOSE_UPVALUES        A=0x00 B=0x00 C=0x00 kb=F kc=F
5: OP_LOAD_CONSTANT         A=0x00 K=0x0001 i=F s=F
6: OP_RETURN                A=0x00 B=0x00 C=0x00 kb=F kc=F
7: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0:  OP_LOAD_CONSTANT         A=0x00 K=0x0001 i=F s=F
1:  OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2:  OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: Range start=1 end=10 step=1
K[1]: FunctionProto arity=0 coarity=1 maxStackSize=3 -> @test

[Instructions:test]
0:  OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1:  OP_ITER_NEXT             A=0xFF B=0x01 C=0x00 kb=F kc=F
2:  OP_JUMP                  J=0x000006 s=T
3:  OP_EQ                    A=0x02 B=0x01 C=0x85 kb=F kc=T
4:  OP_C_JUMP                A=0x02 K=0x0002 i=F s=T
5:  OP_RETURN                A=0x01 B=0x00 C=0x00 kb=F kc=F
6:  OP_CLOSE_UPVALUES        A=0x02 B=0x00 C=0x00 kb=F kc=F
7:  OP_JUMP                  J=0x000006 s=F
8:  OP_CLOSE_UPVALUES        A=0x00 B=0x00 C=0x00 kb=F kc=F
9:  OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0000 i=T s=T
10: OP_RETURN                A=0x00 B=0x00 C=0x00 kb=F kc=F
11: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=3 -> @test

[Instructions:test]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0005 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x000A i=T s=T
2: OP_MULTIPLY              A=0x02 B=0x01 C=0x82 kb=F kc=T
3: OP_ADD                   A=0x02 B=0x00 C=0x02 kb=F kc=F
4: OP_RETURN                A=0x02 B=0x00 C=0x00 kb=F kc=F
5: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// Test Case: Empty function (implicit return)
TEST_F(CompilerReturnTest, EmptyFunction) {
    const char* source = R"(
        fn test() {
        }
    )";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Empty function should parse successfully";

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=0 -> @test

[Instructions:test]
0: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
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

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @test

[Instructions:test]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x002A i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}