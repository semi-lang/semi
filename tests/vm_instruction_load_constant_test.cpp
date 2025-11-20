// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "../src/const_table.h"
#include "../src/value.h"
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionLoadConstantTest : public VMTest {};

// OP_LOAD_BOOL Tests
TEST_F(VMInstructionLoadConstantTest, OpLoadBoolInlineTrue) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_BOOL  A=0x00 K=0x0000 i=T s=F
1: OP_TRAP       A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_BOOL) << "Register 0 should have bool type";
    ASSERT_EQ(vm->values[0].as.i, 1) << "Register 0 should contain true (1)";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadBoolInlineFalse) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_BOOL  A=0x01 K=0x0000 i=F s=F
1: OP_TRAP       A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[1].header, VALUE_TYPE_BOOL) << "Register 1 should have bool type";
    ASSERT_EQ(vm->values[1].as.i, 0) << "Register 1 should contain false (0)";
}

// OP_LOAD_INLINE_INTEGER Tests
TEST_F(VMInstructionLoadConstantTest, OpLoadIntegerInline) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_INLINE_INTEGER  A=0x00 K=0x002A i=T s=T
1: OP_TRAP                 A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Register 0 should have int type";
    ASSERT_EQ(vm->values[0].as.i, 42) << "Register 0 should contain value 42";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadIntegerInlineZero) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_INLINE_INTEGER  A=0x01 K=0x0000 i=T s=T
1: OP_TRAP                 A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[1].header, VALUE_TYPE_INT) << "Register 1 should have int type";
    ASSERT_EQ(vm->values[1].as.i, 0) << "Register 1 should contain value 0";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadIntegerInlineMaxValue) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_INLINE_INTEGER  A=0x02 K=0xFFFF i=T s=T
1: OP_TRAP                 A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[2].header, VALUE_TYPE_INT) << "Register 2 should have int type";
    ASSERT_EQ(vm->values[2].as.i, 65535) << "Register 2 should contain value 65535";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadIntegerInlineNegative) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_INLINE_INTEGER  A=0x00 K=0x002A i=T s=F
1: OP_TRAP                 A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Register 0 should have int type";
    ASSERT_EQ(vm->values[0].as.i, -42) << "Register 0 should contain value -42";
}

// OP_LOAD_CONST Tests
TEST_F(VMInstructionLoadConstantTest, OpLoadIntegerFromConstantTable) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Int 123456
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Register 0 should have int type";
    ASSERT_EQ(vm->values[0].as.i, 123456) << "Register 0 should contain value 123456";
}

TEST_F(VMInstructionLoadConstantTest, LoadFloatFromConstantTable) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Float 3.14159
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Register 0 should have float type";
    ASSERT_DOUBLE_EQ(vm->values[0].as.f, 3.14159) << "Register 0 should contain value 3.14159";
}

TEST_F(VMInstructionLoadConstantTest, LoadFloatNegativeValue) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x01 K=0x0000 i=F s=F
1: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Float -2.718
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[1].header, VALUE_TYPE_FLOAT) << "Register 1 should have float type";
    ASSERT_DOUBLE_EQ(vm->values[1].as.f, -2.718) << "Register 1 should contain value -2.718";
}

TEST_F(VMInstructionLoadConstantTest, LoadFloatZero) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x02 K=0x0000 i=F s=F
1: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Float 0.0
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[2].header, VALUE_TYPE_FLOAT) << "Register 2 should have float type";
    ASSERT_DOUBLE_EQ(vm->values[2].as.f, 0.0) << "Register 2 should contain value 0.0";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadStringFromConstantTable) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x01 K=0x0000 i=F s=F
1: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: String "Hello, World!" length=13
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(VALUE_TYPE(&vm->values[1]), VALUE_TYPE_OBJECT_STRING) << "Register 0 should have allocated string type";
    ASSERT_EQ(AS_OBJECT_STRING(&vm->values[1])->length, 13) << "String should have length 13";
    ASSERT_EQ(memcmp(AS_OBJECT_STRING(&vm->values[1])->str, "Hello, World!", 13), 0)
        << "String should contain 'Hello, World!'";
}

// OP_LOAD_INLINE_STRING Tests
TEST_F(VMInstructionLoadConstantTest, OpLoadStringInlineEmpty) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_INLINE_STRING(0, 0, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error = 0;

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INLINE_STRING) << "Register 0 should have inline string type";
    ASSERT_EQ(vm->values[0].as.is.length, 0) << "String should have length 0";
    ASSERT_EQ(vm->values[0].as.is.c[0], '\0') << "First char should be null";
    ASSERT_EQ(vm->values[0].as.is.c[1], '\0') << "Second char should be null";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadStringInlineOneChar) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_INLINE_STRING(0, 65, true, false);  // 'A' = 65
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error = 0;

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INLINE_STRING) << "Register 0 should have inline string type";
    ASSERT_EQ(vm->values[0].as.is.length, 1) << "String should have length 1";
    ASSERT_EQ(vm->values[0].as.is.c[0], 'A') << "First char should be 'A'";
    ASSERT_EQ(vm->values[0].as.is.c[1], '\0') << "Second char should be null";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadStringInlineTwoChars) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    uint16_t k = (uint16_t)('B' << 8) | 'A';  // "AB"
    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_INLINE_STRING(0, k, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error = 0;

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INLINE_STRING) << "Register 0 should have inline string type";
    ASSERT_EQ(vm->values[0].as.is.length, 2) << "String should have length 2";
    ASSERT_EQ(vm->values[0].as.is.c[0], 'A') << "First char should be 'A'";
    ASSERT_EQ(vm->values[0].as.is.c[1], 'B') << "Second char should be 'B'";
}

TEST_F(VMInstructionLoadConstantTest, OpLoadStringEmptyFromConstantTable) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Value s           = semiValueStringCreate(&vm->gc, "", 0);
    ConstantIndex idx = semiConstantTableInsert(&module->constantTable, s);
    ASSERT_NE(idx, CONST_INDEX_INVALID) << "Failed to insert empty string constant";

    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_INLINE_STRING(1, idx, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error = 0;

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(VALUE_TYPE(&vm->values[1]), VALUE_TYPE_INLINE_STRING) << "Register 1 should have allocated string type";
    ASSERT_EQ(AS_INLINE_STRING(&vm->values[1]).length, 0) << "String should have length 0";
}

// Edge Cases and Error Tests
TEST_F(VMInstructionLoadConstantTest, LoadConstantsInDifferentRegisters) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    ConstantIndex intIdx   = semiConstantTableInsert(&module->constantTable, semiValueIntCreate(100));
    ConstantIndex floatIdx = semiConstantTableInsert(&module->constantTable, semiValueFloatCreate(2.5));
    Value s                = semiValueStringCreate(&vm->gc, "test", 4);
    ConstantIndex strIdx   = semiConstantTableInsert(&module->constantTable, s);

    ASSERT_NE(intIdx, CONST_INDEX_INVALID) << "Failed to insert int constant";
    ASSERT_NE(floatIdx, CONST_INDEX_INVALID) << "Failed to insert float constant";
    ASSERT_NE(strIdx, CONST_INDEX_INVALID) << "Failed to insert string constant";

    Instruction code[5];
    code[0] = INSTRUCTION_LOAD_BOOL(0, 0, true, false);
    code[1] = INSTRUCTION_LOAD_CONSTANT(1, intIdx, false, false);
    code[2] = INSTRUCTION_LOAD_CONSTANT(2, floatIdx, false, false);
    code[3] = INSTRUCTION_LOAD_CONSTANT(3, strIdx, false, false);
    code[4] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 5, 254, 0, 0);

    vm->error = 0;

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Register 0 should have bool type";
    ASSERT_EQ(AS_INT(&vm->values[0]), 1) << "Register 0 should contain true";

    ASSERT_EQ(vm->values[1].header, VALUE_TYPE_INT) << "Register 1 should have int type";
    ASSERT_EQ(vm->values[1].as.i, 100) << "Register 1 should contain 100";

    ASSERT_EQ(vm->values[2].header, VALUE_TYPE_FLOAT) << "Register 2 should have float type";
    ASSERT_DOUBLE_EQ(vm->values[2].as.f, 2.5) << "Register 2 should contain 2.5";

    ASSERT_EQ(VALUE_TYPE(&vm->values[3]), VALUE_TYPE_OBJECT_STRING) << "Register 3 should have allocated string type";
    ASSERT_EQ(AS_OBJECT_STRING(&vm->values[3])->length, 4) << "String should have length 4";
    ASSERT_EQ(memcmp(AS_OBJECT_STRING(&vm->values[3])->str, "test", 4), 0) << "Register 3 should contain 'test'";
}

TEST_F(VMInstructionLoadConstantTest, LoadIntoHighRegisters) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_INLINE_INTEGER(254, 42, true, true);  // Max register index, s=true
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 2, 255, 0, 0);

    vm->error = 0;

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[254].header, VALUE_TYPE_INT) << "Register 255 should have int type";
    ASSERT_EQ(vm->values[254].as.i, 42) << "Register 255 should contain value 42";
}
