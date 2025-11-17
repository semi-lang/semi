// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/vm.h"
#include "semi/error.h"
}
#include "test_common.hpp"

class VMInstructionComparisonTest : public VMTest {};

// OP_GT Tests: R[A] := RK(B, kb) > RK(C, kc)

TEST_F(VMInstructionComparisonTest, OpGtIntegerRegistersTrue) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 10
R[2]: Int 5

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GT   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "10 > 5 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtIntegerRegistersFalse) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 3
R[2]: Int 8

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GT   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "3 > 8 should be false";
}

TEST_F(VMInstructionComparisonTest, OpGtFloatRegistersTrue) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Float 7.5
R[2]: Float 3.2

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GT   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "7.5 > 3.2 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtMixedNumberTypes) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 10
R[2]: Float 5.5

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GT   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "10 > 5.5 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtWithConstantLeft) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int -120

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GT   A=0x00 B=0x0A C=0x01 kb=T kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "(10 + INT8_MIN) > -120 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtWithConstantRight) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 100

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GT   A=0x00 B=0x01 C=0xC8 kb=F kc=T
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "100 > (200 + INT8_MIN) should be true";
}

// OP_GE Tests: R[A] := RK(B, kb) >= RK(C, kc)

TEST_F(VMInstructionComparisonTest, OpGeIntegerRegistersTrue) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 10
R[2]: Int 10

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GE   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "10 >= 10 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGeIntegerRegistersGreater) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 15
R[2]: Int 10

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GE   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "15 >= 10 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGeIntegerRegistersFalse) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 5
R[2]: Int 10

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GE   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "5 >= 10 should be false";
}

TEST_F(VMInstructionComparisonTest, OpGeFloatRegisters) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Float 3.14
R[2]: Float 3.14

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GE   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "3.14 >= 3.14 should be true";
}

// OP_EQ Tests: R[A] := RK(B, kb) == RK(C, kc)

TEST_F(VMInstructionComparisonTest, OpEqIntegerRegistersTrue) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 42
R[2]: Int 42

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_EQ   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "42 == 42 should be true";
}

TEST_F(VMInstructionComparisonTest, OpEqIntegerRegistersFalse) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 42
R[2]: Int 13

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_EQ   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "42 == 13 should be false";
}

TEST_F(VMInstructionComparisonTest, OpEqBooleanRegistersTrue) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Bool true
R[2]: Bool true

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_EQ   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "true == true should be true";
}

TEST_F(VMInstructionComparisonTest, OpEqBooleanRegistersFalse) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Bool true
R[2]: Bool false

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_EQ   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "true == false should be false";
}

TEST_F(VMInstructionComparisonTest, OpEqFloatRegisters) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Float 2.5
R[2]: Float 2.5

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_EQ   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "2.5 == 2.5 should be true";
}

TEST_F(VMInstructionComparisonTest, OpEqMixedNumberTypes) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 5
R[2]: Float 5.0

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_EQ   A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "5 == 5.0 should be true";
}

// OP_NEQ Tests: R[A] := RK(B, kb) != RK(C, kc)

TEST_F(VMInstructionComparisonTest, OpNeqIntegerRegistersTrue) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 42
R[2]: Int 13

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEQ  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "42 != 13 should be true";
}

TEST_F(VMInstructionComparisonTest, OpNeqIntegerRegistersFalse) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 42
R[2]: Int 42

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEQ  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "42 != 42 should be false";
}

TEST_F(VMInstructionComparisonTest, OpNeqBooleanRegisters) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Bool true
R[2]: Bool false

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEQ  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "true != false should be true";
}

TEST_F(VMInstructionComparisonTest, OpNeqFloatRegisters) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Float 2.5
R[2]: Float 3.7

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEQ  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 K=0x0000 i=F s=F
)");

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "2.5 != 3.7 should be true";
}
