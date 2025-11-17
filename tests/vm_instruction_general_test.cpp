// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionGeneralTest : public VMTest {};

TEST_F(VMInstructionGeneralTest, OpTrapWithZeroCode) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_TRAP  A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "OP_TRAP with code 0 should return error code 0";
    ASSERT_EQ(vm->error, 0) << "VM error should be 0";
}

TEST_F(VMInstructionGeneralTest, OpTrapWithNonZeroCode) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_TRAP  A=0x00 K=0x002A i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 42) << "OP_TRAP with code 42 should return error code 42";
    ASSERT_EQ(vm->error, 42) << "VM error should be 42";
}

TEST_F(VMInstructionGeneralTest, OpJumpPositiveOffset) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_JUMP  J=0x000002 s=T
1: OP_TRAP  A=0x00 K=0x0063 i=F s=F
2: OP_TRAP  A=0x00 K=0x0000 i=F s=F
3: OP_TRAP  A=0x00 K=0x0058 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpJumpNegativeOffset) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_TRAP  A=0x00 K=0x0000 i=F s=F
1: OP_JUMP  J=0x000001 s=F
2: OP_TRAP  A=0x00 K=0x0063 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpJumpZeroOffsetNoop) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_JUMP  J=0x000000 s=T
1: OP_TRAP  A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, InvalidPCAfterJump) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_JUMP  J=0x00000A s=T
1: OP_TRAP  A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, SEMI_ERROR_INVALID_PC) << "VM should return INVALID_PC error";
    ASSERT_EQ(vm->error, SEMI_ERROR_INVALID_PC) << "VM error should be INVALID_PC";
}

TEST_F(VMInstructionGeneralTest, OpCJumpLargerPositiveOffset) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[1]: Bool true

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x01 K=0x0004 i=T s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_TRAP    A=0x00 K=0x0063 i=F s=F
3: OP_TRAP    A=0x00 K=0x0063 i=F s=F
4: OP_TRAP    A=0x00 K=0x0000 i=F s=F
5: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpCJumpZeroOffset) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[0]: Bool true

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0000 i=T s=T
1: OP_TRAP    A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpCJumpNegativeOffset) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[0]: Bool true

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_TRAP    A=0x00 K=0x0000 i=F s=F
1: OP_C_JUMP  A=0x00 K=0x0001 i=T s=F
2: OP_TRAP    A=0x00 K=0x0063 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

// Test OP_C_JUMP with different truthy value types
TEST_F(VMInstructionGeneralTest, OpCJumpTruthyValues) {
    struct TestCase {
        const char* name;
        const char* dsl_spec;
    } test_cases[] = {
        // Boolean values
        {     "bool_true_check_true", R"(
[PreDefine:Registers]
R[0]: Bool true

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0002 i=T s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_TRAP    A=0x00 K=0x0000 i=F s=F
3: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)"},
        {   "bool_false_check_false", R"(
[PreDefine:Registers]
R[0]: Bool false

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0002 i=F s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_TRAP    A=0x00 K=0x0000 i=F s=F
3: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)"},
        // Integer values
        {  "int_positive_check_true", R"(
[PreDefine:Registers]
R[0]: Int 42

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0002 i=T s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_TRAP    A=0x00 K=0x0000 i=F s=F
3: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)"},
        {     "int_zero_check_false", R"(
[PreDefine:Registers]
R[0]: Int 0

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0002 i=F s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_TRAP    A=0x00 K=0x0000 i=F s=F
3: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)"},
        // Float values
        {"float_positive_check_true", R"(
[PreDefine:Registers]
R[0]: Float 3.14

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0002 i=T s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_TRAP    A=0x00 K=0x0000 i=F s=F
3: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)"},
        {   "float_zero_check_false", R"(
[PreDefine:Registers]
R[0]: Float 0.0

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0002 i=F s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_TRAP    A=0x00 K=0x0000 i=F s=F
3: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)"},
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        auto& test_case = test_cases[i];

        SemiModule* module;
        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, test_case.dsl_spec, &module);

        ASSERT_EQ(result, 0) << "Test case '" << test_case.name << "' should complete successfully";
    }
}

TEST_F(VMInstructionGeneralTest, OpCJumpMixedTruthyAndFalsy) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[0]: Int 123
R[1]: Int 0

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_C_JUMP  A=0x00 K=0x0002 i=T s=T
1: OP_TRAP    A=0x00 K=0x0063 i=F s=F
2: OP_C_JUMP  A=0x01 K=0x0002 i=F s=T
3: OP_TRAP    A=0x00 K=0x0063 i=F s=F
4: OP_TRAP    A=0x00 K=0x0000 i=F s=F
5: OP_TRAP    A=0x00 K=0x0058 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpNoopBasic) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NOOP  A=0x00 B=0x00 C=0x00 kb=F kc=F
1: OP_TRAP  A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

// OP_MOVE Tests: R[A] := R[B]
TEST_F(VMInstructionGeneralTest, OpMoveSequential) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[1]: Int 111

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_MOVE  A=0x02 B=0x01 C=0x00 kb=F kc=F
1: OP_MOVE  A=0x00 B=0x02 C=0x00 kb=F kc=F
2: OP_MOVE  A=0x03 B=0x00 C=0x00 kb=F kc=F
3: OP_TRAP  A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[2].header, VALUE_TYPE_INT) << "Register 2 should have int type";
    ASSERT_EQ(vm->values[2].as.i, 111) << "Register 2 should contain value 111";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Register 0 should have int type";
    ASSERT_EQ(vm->values[0].as.i, 111) << "Register 0 should contain value 111";
    ASSERT_EQ(vm->values[3].header, VALUE_TYPE_INT) << "Register 3 should have int type";
    ASSERT_EQ(vm->values[3].as.i, 111) << "Register 3 should contain value 111";
}