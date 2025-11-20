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

class RuntimeGlobalVariableTest : public VMTest {};

TEST_F(RuntimeGlobalVariableTest, AccessGlobalIntegerVariable) {
    // Add a global integer variable
    Value globalValue = semiValueIntCreate(123);
    AddGlobalVariable("globalInt", globalValue);

    // Create code that loads the global variable into register 0 and then traps
    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, 0, false, true);  // Load global constant at index 0
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error      = 0;
    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Register 0 should have int type";
    ASSERT_EQ(AS_INT(&vm->values[0]), 123) << "Register 0 should contain the global value";
}

TEST_F(RuntimeGlobalVariableTest, AccessGlobalFloatVariable) {
    // Add a global float variable
    Value globalValue = semiValueFloatCreate(3.14159);
    AddGlobalVariable("globalFloat", globalValue);

    // Create code that loads the global variable
    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);  // Load global constant at index 0 into register 1
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error      = 0;
    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[1].header, VALUE_TYPE_FLOAT) << "Register 1 should have float type";
    ASSERT_DOUBLE_EQ(AS_FLOAT(&vm->values[1]), 3.14159) << "Register 1 should contain the global value";
}

TEST_F(RuntimeGlobalVariableTest, AccessGlobalBooleanVariable) {
    // Add a global boolean variable
    Value globalValue = semiValueBoolCreate(false);
    AddGlobalVariable("globalBool", globalValue);

    // Create code that loads the global variable
    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_CONSTANT(2, 0, false, true);  // Load global constant at index 0 into register 2
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error      = 0;
    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[2].header, VALUE_TYPE_BOOL) << "Register 2 should have bool type";
    ASSERT_EQ(AS_BOOL(&vm->values[2]), false) << "Register 2 should contain the global value";
}

TEST_F(RuntimeGlobalVariableTest, AccessGlobalStringVariable) {
    // Add a global string variable
    Value globalValue = semiValueStringCreate(&vm->gc, "Hello, World!", 13);
    AddGlobalVariable("globalString", globalValue);

    // Create code that loads the global variable
    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_CONSTANT(3, 0, false, true);  // Load global constant at index 0 into register 3
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error      = 0;
    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[3].header, VALUE_TYPE_OBJECT_STRING) << "Register 3 should have object type";

    ObjectString* loadedStr = AS_OBJECT_STRING(&vm->values[3]);
    ASSERT_EQ(loadedStr->length, 13) << "String length should match";
    ASSERT_EQ(strncmp(loadedStr->str, "Hello, World!", 13), 0) << "String content should match";
}

TEST_F(RuntimeGlobalVariableTest, AccessMultipleGlobalVariables) {
    // Add multiple global variables
    Value intValue   = semiValueIntCreate(42);
    Value floatValue = semiValueFloatCreate(2.718);
    Value boolValue  = semiValueBoolCreate(true);

    AddGlobalVariable("first", intValue);
    AddGlobalVariable("second", floatValue);
    AddGlobalVariable("third", boolValue);

    // Create code that loads all three global variables
    Instruction code[4];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, 0, false, true);  // Load global[0] into register 0
    code[1] = INSTRUCTION_LOAD_CONSTANT(1, 1, false, true);  // Load global[1] into register 1
    code[2] = INSTRUCTION_LOAD_CONSTANT(2, 2, false, true);  // Load global[2] into register 2
    code[3] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    module->moduleInit = CreateFunctionObject(0, code, 4, 254, 0, 0);

    vm->error      = 0;
    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(AS_INT(&vm->values[0]), 42) << "First global should be loaded correctly";
    ASSERT_DOUBLE_EQ(AS_FLOAT(&vm->values[1]), 2.718) << "Second global should be loaded correctly";
    ASSERT_EQ(AS_BOOL(&vm->values[2]), true) << "Third global should be loaded correctly";
}

static ErrorId testGlobalNativeFunction(SemiVM* vm, uint8_t argsCount, Value* args, Value* returnValue) {
    (void)vm;
    (void)argsCount;
    (void)args;

    // Set a simple return value for testing
    *returnValue = semiValueIntCreate(12345);
    return 0;
}

TEST_F(RuntimeGlobalVariableTest, AccessGlobalNativeFunctionVariable) {
    // Add a global native function variable
    Value funcValue = semiValueNativeFunctionCreate(testGlobalNativeFunction);
    AddGlobalVariable("globalFunc", funcValue);

    // Create code that loads the global function
    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, 0, false, true);  // Load global constant at index 0
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    vm->error      = 0;
    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_NATIVE_FUNCTION) << "Register 0 should have native function type";
    ASSERT_EQ(AS_NATIVE_FUNCTION(&vm->values[0]), testGlobalNativeFunction) << "Function pointer should match";
}
