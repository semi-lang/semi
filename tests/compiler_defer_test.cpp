// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <string>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerDeferTest : public CompilerTest {};

// Test Case 1: Basic defer block compilation - exact instruction verification
TEST_F(CompilerDeferTest, BasicDeferBlockExactInstructions) {
    const char* source = "defer { a := 1 }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Basic defer block should compile successfully";

    VerifyCompilation(module, R"(
[Instructions]
0: OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @deferFunc

[Instructions:deferFunc]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// Test Case 2: Multiple defer blocks - exact instruction verification
TEST_F(CompilerDeferTest, MultipleDeferBlocksExactInstructions) {
    const char* source = "defer { a := 1 }\ndefer { b := 2 }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Multiple defer blocks should compile successfully";

    VerifyCompilation(module, R"(
[Instructions]
0: OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
1: OP_DEFER_CALL            A=0x00 K=0x0001 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @deferFunc0
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @deferFunc1

[Instructions:deferFunc0]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:deferFunc1]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// Test Case 3: Defer in function scope - exact instruction verification
TEST_F(CompilerDeferTest, DeferInFunctionExactInstructions) {
    const char* source = "fn test() { defer { cleanup := true } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Defer in function should compile successfully";

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0001 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @deferFunc
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=0 -> @testFunc

[Instructions:testFunc]
0: OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:deferFunc]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// Test Case 4: Nested defer error - verify compilation fails
TEST_F(CompilerDeferTest, NestedDeferError) {
    const char* source = "defer { defer { x := 42 } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_NESTED_DEFER) << "Nested defer should fail with SEMI_ERROR_NESTED_DEFER";
}

// Test Case 5: Return with value in defer block - verify compilation fails
TEST_F(CompilerDeferTest, ReturnValueInDeferError) {
    const char* source = "fn test() { defer { return 42 } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_RETURN_VALUE_IN_DEFER)
        << "Return with value in defer should fail with SEMI_ERROR_RETURN_VALUE_IN_DEFER";
}

// Test Case 6: Return without value in defer block - verify exact instructions
TEST_F(CompilerDeferTest, ReturnWithoutValueInDeferExactInstructions) {
    const char* source = "fn test() { defer { x := 1; return } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Return without value in defer should compile successfully";

    VerifyCompilation(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0001 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @deferFunc
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=0 -> @testFunc

[Instructions:testFunc]
0: OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:deferFunc]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// Test Case 7: Empty defer block - verify minimal instructions
TEST_F(CompilerDeferTest, EmptyDeferBlockExactInstructions) {
    const char* source = "defer { }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Empty defer block should compile successfully";

    VerifyCompilation(module, R"(
[Instructions]
0: OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=0 -> @deferFunc

[Instructions:deferFunc]
0: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}
