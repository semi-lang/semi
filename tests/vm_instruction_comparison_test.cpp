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
    Instruction code[2];
    code[0] = INSTRUCTION_GT(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 10;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 5;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "10 > 5 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtIntegerRegistersFalse) {
    Instruction code[2];
    code[0] = INSTRUCTION_GT(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 3;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 8;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "3 > 8 should be false";
}

TEST_F(VMInstructionComparisonTest, OpGtFloatRegistersTrue) {
    Instruction code[2];
    code[0] = INSTRUCTION_GT(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_FLOAT;
    vm->values[1].as.f   = 7.5;
    vm->values[2].header = VALUE_TYPE_FLOAT;
    vm->values[2].as.f   = 3.2;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "7.5 > 3.2 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtMixedNumberTypes) {
    Instruction code[2];
    code[0] = INSTRUCTION_GT(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 10;
    vm->values[2].header = VALUE_TYPE_FLOAT;
    vm->values[2].as.f   = 5.5;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "10 > 5.5 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtWithConstantLeft) {
    Instruction code[2];
    code[0] = INSTRUCTION_GT(0, 10, 1, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = -120;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "(10 + INT8_MIN) > -120 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGtWithConstantRight) {
    Instruction code[2];
    code[0] = INSTRUCTION_GT(0, 1, 200, false, true);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 100;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "100 > (200 + INT8_MIN) should be true";
}

// OP_GE Tests: R[A] := RK(B, kb) >= RK(C, kc)

TEST_F(VMInstructionComparisonTest, OpGeIntegerRegistersTrue) {
    Instruction code[2];
    code[0] = INSTRUCTION_GE(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 10;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 10;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "10 >= 10 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGeIntegerRegistersGreater) {
    Instruction code[2];
    code[0] = INSTRUCTION_GE(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 15;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 10;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "15 >= 10 should be true";
}

TEST_F(VMInstructionComparisonTest, OpGeIntegerRegistersFalse) {
    Instruction code[2];
    code[0] = INSTRUCTION_GE(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 5;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 10;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "5 >= 10 should be false";
}

TEST_F(VMInstructionComparisonTest, OpGeFloatRegisters) {
    Instruction code[2];
    code[0] = INSTRUCTION_GE(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_FLOAT;
    vm->values[1].as.f   = 3.14;
    vm->values[2].header = VALUE_TYPE_FLOAT;
    vm->values[2].as.f   = 3.14;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "3.14 >= 3.14 should be true";
}

// OP_EQ Tests: R[A] := RK(B, kb) == RK(C, kc)

TEST_F(VMInstructionComparisonTest, OpEqIntegerRegistersTrue) {
    Instruction code[2];
    code[0] = INSTRUCTION_EQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 42;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 42;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "42 == 42 should be true";
}

TEST_F(VMInstructionComparisonTest, OpEqIntegerRegistersFalse) {
    Instruction code[2];
    code[0] = INSTRUCTION_EQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 42;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 13;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "42 == 13 should be false";
}

TEST_F(VMInstructionComparisonTest, OpEqBooleanRegistersTrue) {
    Instruction code[2];
    code[0] = INSTRUCTION_EQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_BOOL;
    vm->values[1].as.b   = true;
    vm->values[2].header = VALUE_TYPE_BOOL;
    vm->values[2].as.b   = true;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "true == true should be true";
}

TEST_F(VMInstructionComparisonTest, OpEqBooleanRegistersFalse) {
    Instruction code[2];
    code[0] = INSTRUCTION_EQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_BOOL;
    vm->values[1].as.b   = true;
    vm->values[2].header = VALUE_TYPE_BOOL;
    vm->values[2].as.b   = false;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "true == false should be false";
}

TEST_F(VMInstructionComparisonTest, OpEqFloatRegisters) {
    Instruction code[2];
    code[0] = INSTRUCTION_EQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_FLOAT;
    vm->values[1].as.f   = 2.5;
    vm->values[2].header = VALUE_TYPE_FLOAT;
    vm->values[2].as.f   = 2.5;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "2.5 == 2.5 should be true";
}

TEST_F(VMInstructionComparisonTest, OpEqMixedNumberTypes) {
    Instruction code[2];
    code[0] = INSTRUCTION_EQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 5;
    vm->values[2].header = VALUE_TYPE_FLOAT;
    vm->values[2].as.f   = 5.0;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "5 == 5.0 should be true";
}

// OP_NEQ Tests: R[A] := RK(B, kb) != RK(C, kc)

TEST_F(VMInstructionComparisonTest, OpNeqIntegerRegistersTrue) {
    Instruction code[2];
    code[0] = INSTRUCTION_NEQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 42;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 13;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "42 != 13 should be true";
}

TEST_F(VMInstructionComparisonTest, OpNeqIntegerRegistersFalse) {
    Instruction code[2];
    code[0] = INSTRUCTION_NEQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 42;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 42;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, false) << "42 != 42 should be false";
}

TEST_F(VMInstructionComparisonTest, OpNeqBooleanRegisters) {
    Instruction code[2];
    code[0] = INSTRUCTION_NEQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_BOOL;
    vm->values[1].as.b   = true;
    vm->values[2].header = VALUE_TYPE_BOOL;
    vm->values[2].as.b   = false;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "true != false should be true";
}

TEST_F(VMInstructionComparisonTest, OpNeqFloatRegisters) {
    Instruction code[2];
    code[0] = INSTRUCTION_NEQ(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_FLOAT;
    vm->values[1].as.f   = 2.5;
    vm->values[2].header = VALUE_TYPE_FLOAT;
    vm->values[2].as.f   = 3.7;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Result should be boolean type";
    ASSERT_EQ(vm->values[0].as.b, true) << "2.5 != 3.7 should be true";
}
