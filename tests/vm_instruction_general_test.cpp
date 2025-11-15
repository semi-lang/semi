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
    Instruction code[1];
    code[0] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->error = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 1, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "OP_TRAP with code 0 should return error code 0";
    ASSERT_EQ(vm->error, 0) << "VM error should be 0";
}

TEST_F(VMInstructionGeneralTest, OpTrapWithNonZeroCode) {
    Instruction code[1];
    code[0] = INSTRUCTION_TRAP(0, 42, false, false);

    vm->error = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 1, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 42) << "OP_TRAP with code 42 should return error code 42";
    ASSERT_EQ(vm->error, 42) << "VM error should be 42";
}

TEST_F(VMInstructionGeneralTest, OpJumpPositiveOffset) {
    Instruction code[4];
    code[0] = INSTRUCTION_JUMP(2, true);
    code[1] = INSTRUCTION_TRAP(0, 99, false, false);
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);
    code[3] = INSTRUCTION_TRAP(0, 88, false, false);

    vm->error = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 4, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpJumpNegativeOffset) {
    Instruction code[3];
    code[0] = INSTRUCTION_TRAP(0, 0, false, false);
    code[1] = INSTRUCTION_JUMP(1, false);
    code[2] = INSTRUCTION_TRAP(0, 99, false, false);

    vm->error = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 3, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpJumpZeroOffsetNoop) {
    Instruction code[2];
    code[0] = INSTRUCTION_JUMP(0, true);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->error = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, InvalidPCAfterJump) {
    Instruction code[2];
    code[0] = INSTRUCTION_JUMP(10, true);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->error = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, SEMI_ERROR_INVALID_PC) << "VM should return INVALID_PC error";
    ASSERT_EQ(vm->error, SEMI_ERROR_INVALID_PC) << "VM error should be INVALID_PC";
}

TEST_F(VMInstructionGeneralTest, OpCJumpLargerPositiveOffset) {
    Instruction code[6];
    code[0] = INSTRUCTION_C_JUMP(1, 4, true, true);
    code[1] = INSTRUCTION_TRAP(0, 99, false, false);
    code[2] = INSTRUCTION_TRAP(0, 99, false, false);
    code[3] = INSTRUCTION_TRAP(0, 99, false, false);
    code[4] = INSTRUCTION_TRAP(0, 0, false, false);
    code[5] = INSTRUCTION_TRAP(0, 88, false, false);

    vm->values[1].header = VALUE_TYPE_BOOL;
    vm->values[1].as.b   = true;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 6, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpCJumpZeroOffset) {
    Instruction code[2];
    code[0] = INSTRUCTION_C_JUMP(0, 0, true, true);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[0].header = VALUE_TYPE_BOOL;
    vm->values[0].as.b   = true;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpCJumpNegativeOffset) {
    Instruction code[3];
    code[0] = INSTRUCTION_TRAP(0, 0, false, false);
    code[1] = INSTRUCTION_C_JUMP(0, 1, true, false);
    code[2] = INSTRUCTION_TRAP(0, 99, false, false);

    vm->error            = 0;
    vm->values[0].header = VALUE_TYPE_BOOL;
    vm->values[0].as.b   = true;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 3, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

// Test OP_C_JUMP with different truthy value types
TEST_F(VMInstructionGeneralTest, OpCJumpTruthyValues) {
    struct {
        const char* name;
        ValueType value_type;
        union {
            bool b;
            int64_t i;
            double f;
        } value;
        bool i_flag;  // What boolean value we're checking for
        bool should_jump;
    } test_cases[] = {
        // Boolean values
        {     "bool_true_check_true",  VALUE_TYPE_BOOL,  {.b = true},  true, true},
        {   "bool_false_check_false",  VALUE_TYPE_BOOL, {.b = false}, false, true},

        // Integer values
        {  "int_positive_check_true",   VALUE_TYPE_INT,    {.i = 42},  true, true},
        {     "int_zero_check_false",   VALUE_TYPE_INT,     {.i = 0}, false, true},

        // Float values
        {"float_positive_check_true", VALUE_TYPE_FLOAT,  {.f = 3.14},  true, true},
        {   "float_zero_check_false", VALUE_TYPE_FLOAT,   {.f = 0.0}, false, true},
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        auto& test_case = test_cases[i];

        Instruction code[4];
        if (test_case.should_jump) {
            // Jump case: skip the TRAP(99) and execute TRAP(0)
            code[0] = INSTRUCTION_C_JUMP(0, 2, test_case.i_flag, true);
            code[1] = INSTRUCTION_TRAP(0, 99, false, false);  // Skip this
            code[2] = INSTRUCTION_TRAP(0, 0, false, false);   // Execute this
            code[3] = INSTRUCTION_TRAP(0, 88, false, false);
        } else {
            // No jump case: execute the next instruction (TRAP(0))
            code[0] = INSTRUCTION_C_JUMP(0, 2, test_case.i_flag, true);
            code[1] = INSTRUCTION_TRAP(0, 0, false, false);   // Execute this
            code[2] = INSTRUCTION_TRAP(0, 99, false, false);  // Don't reach this
            code[3] = INSTRUCTION_TRAP(0, 88, false, false);
        }

        vm->error            = 0;
        vm->values[0].header = test_case.value_type;

        switch (test_case.value_type) {
            case VALUE_TYPE_BOOL:
                vm->values[0].as.b = test_case.value.b;
                break;
            case VALUE_TYPE_INT:
                vm->values[0].as.i = test_case.value.i;
                break;
            case VALUE_TYPE_FLOAT:
                vm->values[0].as.f = test_case.value.f;
                break;
            default:
                FAIL() << "Unsupported value type in test case '" << test_case.name << "'";
        }

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 4, 8, 0, 0);
        module->moduleInit  = func;

        int result = RunModule(module);

        ASSERT_EQ(result, 0) << "Test case '" << test_case.name << "' should complete successfully";

        if (test_case.should_jump) {
        } else {
        }
    }
}

TEST_F(VMInstructionGeneralTest, OpCJumpMixedTruthyAndFalsy) {
    Instruction code[6];
    code[0] = INSTRUCTION_C_JUMP(0, 2, true, true);   // Jump if R[0] is truthy
    code[1] = INSTRUCTION_TRAP(0, 99, false, false);  // Should skip this
    code[2] = INSTRUCTION_C_JUMP(1, 2, false, true);  // Jump if R[1] is falsy
    code[3] = INSTRUCTION_TRAP(0, 99, false, false);  // Should skip this
    code[4] = INSTRUCTION_TRAP(0, 0, false, false);   // Success
    code[5] = INSTRUCTION_TRAP(0, 88, false, false);  // Should not reach

    vm->error = 0;

    // R[0] = non-zero int (truthy)
    vm->values[0].header = VALUE_TYPE_INT;
    vm->values[0].as.i   = 123;

    // R[1] = zero int (falsy)
    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 6, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

TEST_F(VMInstructionGeneralTest, OpNoopBasic) {
    Instruction code[2];
    code[0] = INSTRUCTION_NOOP();
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->error = 0;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
}

// OP_MOVE Tests: R[A] := R[B]
TEST_F(VMInstructionGeneralTest, OpMoveSequential) {
    Instruction code[4];
    code[0] = INSTRUCTION_MOVE(2, 1, 0, false, false);
    code[1] = INSTRUCTION_MOVE(0, 2, 0, false, false);
    code[2] = INSTRUCTION_MOVE(3, 0, 0, false, false);
    code[3] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 111;

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 4, 8, 0, 0);
    module->moduleInit  = func;

    int result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[2].header, VALUE_TYPE_INT) << "Register 2 should have int type";
    ASSERT_EQ(vm->values[2].as.i, 111) << "Register 2 should contain value 111";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Register 0 should have int type";
    ASSERT_EQ(vm->values[0].as.i, 111) << "Register 0 should contain value 111";
    ASSERT_EQ(vm->values[3].header, VALUE_TYPE_INT) << "Register 3 should have int type";
    ASSERT_EQ(vm->values[3].as.i, 111) << "Register 3 should contain value 111";
}