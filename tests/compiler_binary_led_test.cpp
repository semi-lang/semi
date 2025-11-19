// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerBinaryLedTest : public CompilerTest {};

// ==========================================
// Tests that trigger loading large constants
// ==========================================

TEST_F(CompilerBinaryLedTest, OpAddLargeConstVar) {
    const char* source = "{ x := 400000; z := 300000 + x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_LOAD_CONSTANT         A=0x02 K=0x0001 i=F s=F
2: OP_ADD                   A=0x01 B=0x02 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
[Constants]
K[0]: Int 400000
K[1]: Int 300000
)");
}

TEST_F(CompilerBinaryLedTest, OpAddLargeVarConst) {
    const char* source = "{ x := 300000; z := x + 400000 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_LOAD_CONSTANT         A=0x02 K=0x0001 i=F s=F
2: OP_ADD                   A=0x01 B=0x00 C=0x02 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
[Constants]
K[0]: Int 300000
K[1]: Int 400000
)");
}

TEST_F(CompilerBinaryLedTest, OpAddLargeVarVar) {
    const char* source = "{ x := 300000; y := 400000; z := x + y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_LOAD_CONSTANT         A=0x01 K=0x0001 i=F s=F
2: OP_ADD                   A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
[Constants]
K[0]: Int 300000
K[1]: Int 400000
)");
}

// ==========================================
// Arithmetic Operators
// ==========================================

// ------------------------------------------
// Add (+)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpAddConstVar) {
    const char* source = "{ x := 2; z := 1 + x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_ADD                   A=0x01 B=0x81 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpAddVarConst) {
    const char* source = "{ x := 1; z := x + 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_ADD                   A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpAddVarVar) {
    const char* source = "{ x := 1; y := 2; z := x + y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_ADD                   A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Subtract (-)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpSubtractConstConst) {
    const char* source = "{ z := 3 - 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpSubtractConstVar) {
    const char* source = "{ x := 1; z := 3 - x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_SUBTRACT              A=0x01 B=0x83 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpSubtractVarConst) {
    const char* source = "{ x := 3; z := x - 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_SUBTRACT              A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpSubtractVarVar) {
    const char* source = "{ x := 3; y := 1; z := x - y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0001 i=T s=T
2: OP_SUBTRACT              A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Multiply (*)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpMultiplyConstConst) {
    const char* source = "{ z := 2 * 3 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0006 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpMultiplyConstVar) {
    const char* source = "{ x := 3; z := 2 * x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_MULTIPLY              A=0x01 B=0x82 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpMultiplyVarConst) {
    const char* source = "{ x := 2; z := x * 3 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_MULTIPLY              A=0x01 B=0x00 C=0x83 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpMultiplyVarVar) {
    const char* source = "{ x := 2; y := 3; z := x * y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0003 i=T s=T
2: OP_MULTIPLY              A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Power (**)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpPowerConstConst) {
    const char* source = "{ z := 2 ** 3 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0008 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpPowerConstVar) {
    const char* source = "{ x := 3; z := 2 ** x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_POWER                 A=0x01 B=0x82 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpPowerVarConst) {
    const char* source = "{ x := 2; z := x ** 3 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_POWER                 A=0x01 B=0x00 C=0x83 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpPowerVarVar) {
    const char* source = "{ x := 2; y := 3; z := x ** y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0003 i=T s=T
2: OP_POWER                 A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Divide (/)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpDivideConstConst) {
    const char* source = "{ z := 4 / 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpDivideConstVar) {
    const char* source = "{ x := 2; z := 4 / x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_DIVIDE                A=0x01 B=0x84 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpDivideVarConst) {
    const char* source = "{ x := 4; z := x / 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0004 i=T s=T
1: OP_DIVIDE                A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpDivideVarVar) {
    const char* source = "{ x := 4; y := 2; z := x / y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0004 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_DIVIDE                A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Floor Divide (//)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpFloorDivideConstConst) {
    const char* source = "{ z := 5 // 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpFloorDivideConstVar) {
    const char* source = "{ x := 2; z := 5 // x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_FLOOR_DIVIDE          A=0x01 B=0x85 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpFloorDivideVarConst) {
    const char* source = "{ x := 5; z := x // 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0005 i=T s=T
1: OP_FLOOR_DIVIDE          A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpFloorDivideVarVar) {
    const char* source = "{ x := 5; y := 2; z := x // y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0005 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_FLOOR_DIVIDE          A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Modulo (%)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpModuloConstConst) {
    const char* source = "{ z := 5 % 3 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpModuloConstVar) {
    const char* source = "{ x := 3; z := 5 % x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_MODULO                A=0x01 B=0x85 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpModuloVarConst) {
    const char* source = "{ x := 5; z := x % 3 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0005 i=T s=T
1: OP_MODULO                A=0x01 B=0x00 C=0x83 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpModuloVarVar) {
    const char* source = "{ x := 5; y := 3; z := x % y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0005 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0003 i=T s=T
2: OP_MODULO                A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ==========================================
// Bitwise Operators
// ==========================================

// ------------------------------------------
// Bitwise And (&)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpBitwiseAndConstConst) {
    const char* source = "{ z := 3 & 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseAndConstVar) {
    const char* source = "{ x := 1; z := 3 & x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_BITWISE_AND           A=0x01 B=0x83 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseAndVarConst) {
    const char* source = "{ x := 3; z := x & 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_BITWISE_AND           A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseAndVarVar) {
    const char* source = "{ x := 3; y := 1; z := x & y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0001 i=T s=T
2: OP_BITWISE_AND           A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Bitwise Or (|)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpBitwiseOrConstConst) {
    const char* source = "{ z := 1 | 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseOrConstVar) {
    const char* source = "{ x := 2; z := 1 | x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_BITWISE_OR            A=0x01 B=0x81 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseOrVarConst) {
    const char* source = "{ x := 1; z := x | 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_BITWISE_OR            A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseOrVarVar) {
    const char* source = "{ x := 1; y := 2; z := x | y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_BITWISE_OR            A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Bitwise Xor (^)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpBitwiseXorConstConst) {
    const char* source = "{ z := 3 ^ 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseXorConstVar) {
    const char* source = "{ x := 1; z := 3 ^ x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_BITWISE_XOR           A=0x01 B=0x83 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseXorVarConst) {
    const char* source = "{ x := 3; z := x ^ 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_BITWISE_XOR           A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseXorVarVar) {
    const char* source = "{ x := 3; y := 1; z := x ^ y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0001 i=T s=T
2: OP_BITWISE_XOR           A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Bitwise Left Shift (<<)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpBitwiseLShiftConstConst) {
    const char* source = "{ z := 1 << 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0004 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseLShiftConstVar) {
    const char* source = "{ x := 2; z := 1 << x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_BITWISE_L_SHIFT       A=0x01 B=0x81 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseLShiftVarConst) {
    const char* source = "{ x := 1; z := x << 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_BITWISE_L_SHIFT       A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseLShiftVarVar) {
    const char* source = "{ x := 1; y := 2; z := x << y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_BITWISE_L_SHIFT       A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Bitwise Right Shift (>>)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpBitwiseRShiftConstConst) {
    const char* source = "{ z := 4 >> 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseRShiftConstVar) {
    const char* source = "{ x := 1; z := 4 >> x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_BITWISE_R_SHIFT       A=0x01 B=0x84 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseRShiftVarConst) {
    const char* source = "{ x := 4; z := x >> 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0004 i=T s=T
1: OP_BITWISE_R_SHIFT       A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpBitwiseRShiftVarVar) {
    const char* source = "{ x := 4; y := 1; z := x >> y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0004 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0001 i=T s=T
2: OP_BITWISE_R_SHIFT       A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ==========================================
// Comparison Operators
// ==========================================

// ------------------------------------------
// Equal (==)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpEqConstConst) {
    const char* source = "{ z := 1 == 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=F s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpEqConstVar) {
    const char* source = "{ x := 2; z := 1 == x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_EQ                    A=0x01 B=0x81 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpEqVarConst) {
    const char* source = "{ x := 1; z := x == 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_EQ                    A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpEqVarVar) {
    const char* source = "{ x := 1; y := 2; z := x == y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_EQ                    A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Not Equal (!=)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpNeqConstConst) {
    const char* source = "{ z := 1 != 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpNeqConstVar) {
    const char* source = "{ x := 2; z := 1 != x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_NEQ                   A=0x01 B=0x81 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpNeqVarConst) {
    const char* source = "{ x := 1; z := x != 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_NEQ                   A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpNeqVarVar) {
    const char* source = "{ x := 1; y := 2; z := x != y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_NEQ                   A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Greater Than (>)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpGtConstConst) {
    const char* source = "{ z := 2 > 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpGtConstVar) {
    const char* source = "{ x := 1; z := 2 > x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_GT                    A=0x01 B=0x82 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpGtVarConst) {
    const char* source = "{ x := 2; z := x > 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_GT                    A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpGtVarVar) {
    const char* source = "{ x := 2; y := 1; z := x > y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0001 i=T s=T
2: OP_GT                    A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Greater Than Or Equal (>=)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpGeConstConst) {
    const char* source = "{ z := 2 >= 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpGeConstVar) {
    const char* source = "{ x := 1; z := 2 >= x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_GE                    A=0x01 B=0x82 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpGeVarConst) {
    const char* source = "{ x := 2; z := x >= 1 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_GE                    A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpGeVarVar) {
    const char* source = "{ x := 2; y := 1; z := x >= y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0001 i=T s=T
2: OP_GE                    A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Less Than (<) - Implemented as GT with swapped operands
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpLtConstConst) {
    const char* source = "{ z := 1 < 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpLtConstVar) {
    const char* source = "{ x := 2; z := 1 < x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_GT                    A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpLtVarConst) {
    const char* source = "{ x := 1; z := x < 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_GT                    A=0x01 B=0x82 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpLtVarVar) {
    const char* source = "{ x := 1; y := 2; z := x < y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_GT                    A=0x02 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Less Than Or Equal (<=) - Implemented as GE with swapped operands
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpLteConstConst) {
    const char* source = "{ z := 1 <= 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpLteConstVar) {
    const char* source = "{ x := 2; z := 1 <= x }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
1: OP_GE                    A=0x01 B=0x00 C=0x81 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpLteVarConst) {
    const char* source = "{ x := 1; z := x <= 2 }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_GE                    A=0x01 B=0x82 C=0x00 kb=T kc=F
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpLteVarVar) {
    const char* source = "{ x := 1; y := 2; z := x <= y }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_INLINE_INTEGER   A=0x01 K=0x0002 i=T s=T
2: OP_GE                    A=0x02 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ==========================================
// Type/Collection Operators
// ==========================================

// ------------------------------------------
// Check Type (is)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpCheckTypeConstConst) {
    const char* source = "{ z := 1 is Int }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpCheckTypeVarConst) {
    const char* source = "{ x := 1; z := x is Int }";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_CHECK_TYPE            A=0x01 B=0x00 C=0x82 kb=F kc=T
2: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

// ------------------------------------------
// Contain (in)
// ------------------------------------------

TEST_F(CompilerBinaryLedTest, OpContainConstConst) {
    const char* source = R"({ z := "a" in "abc" })";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(CompilerBinaryLedTest, OpContainConstVar) {
    const char* source = R"({ x := "abc"; z := "a" in x })";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_LOAD_INLINE_STRING    A=0x02 K=0x0061 i=T s=F
2: OP_CONTAIN               A=0x01 B=0x02 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
[Constants]
K[0]: String "abc" length=3
)");
}

TEST_F(CompilerBinaryLedTest, OpContainVarConst) {
    const char* source = R"({ x := "a"; z := x in "abc" })";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_STRING    A=0x00 K=0x0061 i=T s=F
1: OP_LOAD_CONSTANT         A=0x02 K=0x0000 i=F s=F
2: OP_CONTAIN               A=0x01 B=0x00 C=0x02 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
[Constants]
K[0]: String "abc" length=3
)");
}

TEST_F(CompilerBinaryLedTest, OpContainVarVar) {
    const char* source = R"({ x := "a"; y := "abc"; z := x in y })";
    EXPECT_EQ(ParseModule(source), 0);
    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_STRING    A=0x00 K=0x0061 i=T s=F
1: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=F
2: OP_CONTAIN               A=0x02 B=0x00 C=0x01 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
[Constants]
K[0]: String "abc" length=3
)");
}
