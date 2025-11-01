// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/value.h"
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionFunctionCallTest : public VMTest {};

TEST_F(VMInstructionFunctionCallTest, CallZeroArgumentFunction) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[1];
    fnCode[0] = INSTRUCTION_RETURN(255, 0, 0, false, false);  // Function body: just return

    FunctionProto* func = CreateFunctionObject(0, fnCode, 1, 1, 0, 0);  // no args, stack size 1
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Create function from K[0]
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);            // Call function at R[0] with 0 args
    code[2] = INSTRUCTION_TRAP(0, 1, false, false);               // Exit here

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 1) << "VM should complete successfully";

    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, CallFunctionWithArguments) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[1];
    fnCode[0] = INSTRUCTION_RETURN(255, 0, 0, false, false);  // Function body: just return

    FunctionProto* func = CreateFunctionObject(2, fnCode, 1, 3, 0, 0);  // 2 args, stack size 3
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Create function from K[0]
    code[1] = INSTRUCTION_CALL(0, 2, 0, false, false);            // Call function at R[0] with 2 args
    code[2] = INSTRUCTION_TRAP(0, 1, false, false);               // Exit here

    vm->values[2] = semiValueNewInt(42);  // First argument
    vm->values[3] = semiValueNewInt(84);  // Second argument

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 1) << "VM should complete successfully";

    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, NestedFunctionCalls) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Create inner function
    Instruction innerCode[1];
    innerCode[0] = INSTRUCTION_RETURN(255, 0, 0, false, false);  // Inner function returns

    FunctionProto* innerFunc = CreateFunctionObject(0, innerCode, 1, 1, 0, 0);
    ConstantIndex innerIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(innerFunc));

    // Create outer function
    Instruction outerCode[3];
    outerCode[0] = INSTRUCTION_LOAD_CONSTANT(1, innerIndex, false, false);  // Load inner function
    outerCode[1] = INSTRUCTION_CALL(1, 0, 0, false, false);                 // Outer function calls inner
    outerCode[2] = INSTRUCTION_RETURN(255, 0, 0, false, false);             // Outer function returns

    FunctionProto* outerFunc = CreateFunctionObject(0, outerCode, 3, 2, 0, 0);
    ConstantIndex outerIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(outerFunc));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, outerIndex, false, false);  // Load outer function
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);                 // Call outer function
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);                    // Success

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully with nested calls";

    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, ReturnValue) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction code[2];
    code[0] = INSTRUCTION_LOAD_INLINE_INTEGER(0, 42, false, true);  // Load integer 42 into R[0]
    code[1] = INSTRUCTION_RETURN(1, 0, 0, false, false);            // Return R[1] without any call

    module->moduleInit = CreateFunctionObject(0, code, 2, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "Should not return invalid error";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Returned value should be an integer";
    ASSERT_EQ(AS_INT(&vm->values[0]), 42) << "Returned value should be 42";
}

// Error Condition Tests for OP_CALL

TEST_F(VMInstructionFunctionCallTest, CallNonFunctionObject) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    ConstantIndex index = semiConstantTableInsert(&module->constantTable, semiValueNewInt(42));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load non-function value
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE) << "Should return type error";
    ASSERT_EQ(vm->error, SEMI_ERROR_UNEXPECTED_TYPE) << "VM error should be set";
}

TEST_F(VMInstructionFunctionCallTest, CallArityMismatch) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[1];
    fnCode[0] = INSTRUCTION_RETURN(255, 0, 0, false, false);

    FunctionProto* func = CreateFunctionObject(1, fnCode, 1, 2, 0, 0);  // Function expects 1 arg, not 2
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Create function from K[0]
    code[1] = INSTRUCTION_CALL(0, 2, 0, false, false);            // Call with 2 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, SEMI_ERROR_ARGS_COUNT_MISMATCH) << "Should return arity mismatch error";
    ASSERT_EQ(vm->error, SEMI_ERROR_ARGS_COUNT_MISMATCH) << "VM error should be set";
}

// Stack Management Tests

TEST_F(VMInstructionFunctionCallTest, CallStackGrowth) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Create functions with large stack requirements
    Instruction fnCode4[1];
    fnCode4[0]           = INSTRUCTION_RETURN(255, 0, 0, false, false);
    FunctionProto* func4 = CreateFunctionObject(0, fnCode4, 1, 254, 0, 0);  // Large stack requirement
    ConstantIndex index4 = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func4));

    Instruction fnCode3[3];
    fnCode3[0]           = INSTRUCTION_LOAD_CONSTANT(0, index4, false, false);
    fnCode3[1]           = INSTRUCTION_CALL(0, 0, 0, false, false);
    fnCode3[2]           = INSTRUCTION_RETURN(255, 0, 0, false, false);
    FunctionProto* func3 = CreateFunctionObject(0, fnCode3, 3, 254, 0, 0);  // Large stack requirement
    ConstantIndex index3 = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func3));

    Instruction fnCode2[3];
    fnCode2[0]           = INSTRUCTION_LOAD_CONSTANT(0, index3, false, false);
    fnCode2[1]           = INSTRUCTION_CALL(0, 0, 0, false, false);
    fnCode2[2]           = INSTRUCTION_RETURN(255, 0, 0, false, false);
    FunctionProto* func2 = CreateFunctionObject(0, fnCode2, 3, 254, 0, 0);  // Large stack requirement
    ConstantIndex index2 = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func2));

    Instruction fnCode1[3];
    fnCode1[0]           = INSTRUCTION_LOAD_CONSTANT(0, index2, false, false);
    fnCode1[1]           = INSTRUCTION_CALL(0, 0, 0, false, false);
    fnCode1[2]           = INSTRUCTION_RETURN(255, 0, 0, false, false);
    FunctionProto* func1 = CreateFunctionObject(0, fnCode1, 3, 254, 0, 0);  // Large stack requirement
    ConstantIndex index1 = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func1));

    Instruction fnCode0[3];
    fnCode0[0]           = INSTRUCTION_LOAD_CONSTANT(0, index1, false, false);
    fnCode0[1]           = INSTRUCTION_CALL(0, 0, 0, false, false);
    fnCode0[2]           = INSTRUCTION_RETURN(255, 0, 0, false, false);
    FunctionProto* func0 = CreateFunctionObject(0, fnCode0, 3, 254, 0, 0);  // Large stack requirement
    ConstantIndex index0 = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func0));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index0, false, false);
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should handle stack growth successfully";

    //        base + maxStackSize
    // root:  0 + 254 (specified in RunInstructions)
    // func0: 1 + 254
    // func1: 2 + 254
    // func2: 3 + 254
    // func3: 4 + 254
    // func4: 5 + 254
    ASSERT_EQ(vm->valueCapacity, 512) << "Stack should have grown to accommodate function";
}

TEST_F(VMInstructionFunctionCallTest, ArgumentPositioning) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[2];
    fnCode[0] = INSTRUCTION_MOVE(0, 1, 0, false, false);    // Move R[7] to R[6] (arg2 to arg1)
    fnCode[1] = INSTRUCTION_RETURN(0, 0, 0, false, false);  // Return R[6] (which is arg1)

    FunctionProto* func = CreateFunctionObject(3, fnCode, 2, 4, 0, 0);  // 3 args, stack size 4
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[5];
    code[0] = INSTRUCTION_LOAD_CONSTANT(5, index, false, false);  // Load function into R[5]
    code[1] = INSTRUCTION_CALL(5, 3, 0, false, false);            // Call function at R[5] with 3 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);               // Success
    code[3] = INSTRUCTION_MOVE(0, 1, 0, false, false);            // Move arg2 to result
    code[4] = INSTRUCTION_RETURN(0, 0, 0, false, false);

    vm->values[6] = semiValueNewInt(10);  // First argument (at R[0] inside the function)
    vm->values[7] = semiValueNewInt(20);  // Second argument
    vm->values[8] = semiValueNewInt(30);  // Third argument

    module->moduleInit = CreateFunctionObject(0, code, 5, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[5].as.i, 20) << "Returned value should be copied to the function register";
}

// Program Counter Management Tests

TEST_F(VMInstructionFunctionCallTest, PCCorrectlyRestored) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[2];
    fnCode[0] =
        INSTRUCTION_MOVE(0, 1, 0, false, false);  // Function body: move from R[1] to R[0] (stack[5] to stack[4])
    fnCode[1] = INSTRUCTION_RETURN(255, 0, 0, false, false);  // Return

    FunctionProto* func = CreateFunctionObject(0, fnCode, 2, 2, 0, 0);  // Function starts at PC=4
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[4];
    code[0] = INSTRUCTION_LOAD_CONSTANT(3, index, false, false);  // Load function
    code[1] = INSTRUCTION_CALL(3, 0, 0, false, false);            // Call function, the new stack starts at R[4]
    code[2] = INSTRUCTION_MOVE(1, 2, 0, false, false);            // Should execute after return
    code[3] = INSTRUCTION_TRAP(0, 0, false, false);               // Success

    vm->values[2] = semiValueNewInt(42);  // For the instruction after return
    vm->values[5] = semiValueNewInt(84);  // This will be available in the function's register space

    module->moduleInit = CreateFunctionObject(0, code, 4, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(vm->values[1].as.i, 42) << "Instruction after return should execute";
    ASSERT_EQ(vm->values[4].as.i, 84) << "Function body should execute and return value should be in function register";
}

// Edge Cases

TEST_F(VMInstructionFunctionCallTest, FunctionWithMaxArity) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[1];
    fnCode[0] = INSTRUCTION_RETURN(255, 0, 0, false, false);

    FunctionProto* func = CreateFunctionObject(253, fnCode, 1, 253, 0, 0);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[4];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load function
    code[1] = INSTRUCTION_CALL(0, 253, 0, false, false);
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);
    code[3] = INSTRUCTION_RETURN(255, 0, 0, false, false);

    // Set up 15 arguments
    for (int i = 0; i < 15; i++) {
        vm->values[2 + i] = semiValueNewInt(i);
    }

    module->moduleInit = CreateFunctionObject(0, code, 4, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should handle maximum arity successfully";
}

TEST_F(VMInstructionFunctionCallTest, ImmediateReturn) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[1];
    fnCode[0] = INSTRUCTION_RETURN(255, 0, 0, false, false);  // Function immediately returns

    FunctionProto* func = CreateFunctionObject(0, fnCode, 1, 1, 0, 0);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load function
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should handle immediate return";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

// Native Function Call Tests

static ErrorId testNativeFunctionNoArgs(GC* gc, uint8_t argCount, Value* args, Value* ret) {
    *ret = semiValueNewInt(42);
    return 0;
}

static ErrorId testNativeFunctionWithArgs(GC* gc, uint8_t argCount, Value* args, Value* ret) {
    if (argCount != 2) {
        return SEMI_ERROR_ARGS_COUNT_MISMATCH;
    }

    if (!IS_INT(&args[0]) || !IS_INT(&args[1])) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    IntValue sum = AS_INT(&args[0]) + AS_INT(&args[1]);
    *ret         = semiValueNewInt(sum);
    return 0;
}

static ErrorId testNativeFunctionWithError(GC* gc, uint8_t argCount, Value* args, Value* ret) {
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId testNativeFunctionMaxArgs(GC* gc, uint8_t argCount, Value* args, Value* ret) {
    IntValue sum = 0;
    for (uint8_t i = 0; i < argCount; i++) {
        if (!IS_INT(&args[i])) {
            return SEMI_ERROR_UNEXPECTED_TYPE;
        }
        sum += AS_INT(&args[i]);
    }
    *ret = semiValueNewInt(sum);
    return 0;
}

TEST_F(VMInstructionFunctionCallTest, CallNativeFunctionNoArgs) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Value nativeFunc    = semiValueNewNativeFunction(testNativeFunctionNoArgs);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, nativeFunc);

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load native function
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);            // Call with 0 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Return value should be an integer";
    ASSERT_EQ(AS_INT(&vm->values[0]), 42) << "Return value should be 42";
}

TEST_F(VMInstructionFunctionCallTest, CallNativeFunctionWithArgs) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Value nativeFunc    = semiValueNewNativeFunction(testNativeFunctionWithArgs);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, nativeFunc);

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load native function
    code[1] = INSTRUCTION_CALL(0, 2, 0, false, false);            // Call with 2 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1] = semiValueNewInt(10);  // First argument
    vm->values[2] = semiValueNewInt(32);  // Second argument

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Return value should be an integer";
    ASSERT_EQ(AS_INT(&vm->values[0]), 42) << "Return value should be 10 + 32 = 42";
}

TEST_F(VMInstructionFunctionCallTest, CallNativeFunctionWithError) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Value nativeFunc    = semiValueNewNativeFunction(testNativeFunctionWithError);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, nativeFunc);

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load native function
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);            // Call with 0 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE) << "VM should return error from native function";
    ASSERT_EQ(vm->error, SEMI_ERROR_UNEXPECTED_TYPE) << "VM error should be set by native function";
}

TEST_F(VMInstructionFunctionCallTest, CallNativeFunctionMaxArguments) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Value nativeFunc    = semiValueNewNativeFunction(testNativeFunctionMaxArgs);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, nativeFunc);

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load native function
    code[1] = INSTRUCTION_CALL(0, 253, 0, false, false);          // Call with 255 args (max)
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    // Set up 255 arguments, each with value equal to its index
    for (int i = 0; i < 253; i++) {
        vm->values[1 + i] = semiValueNewInt(i);
    }

    module->moduleInit = CreateFunctionObject(0, code, 3, 255, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Return value should be an integer";

    IntValue expectedSum = 0;
    for (int i = 0; i < 253; i++) {
        expectedSum += i;
    }
    ASSERT_EQ(AS_INT(&vm->values[0]), expectedSum) << "Return value should be sum of all arguments";
}

TEST_F(VMInstructionFunctionCallTest, CallNativeFunctionArgumentPositioning) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Value nativeFunc    = semiValueNewNativeFunction(testNativeFunctionWithArgs);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, nativeFunc);

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(5, index, false, false);  // Load native function into R[5]
    code[1] = INSTRUCTION_CALL(5, 2, 0, false, false);            // Call with 2 args starting at R[6]
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[6] = semiValueNewInt(15);  // First argument
    vm->values[7] = semiValueNewInt(27);  // Second argument

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
    ASSERT_EQ(vm->values[5].header, VALUE_TYPE_INT) << "Return value should be stored in function register";
    ASSERT_EQ(AS_INT(&vm->values[5]), 42) << "Return value should be 15 + 27 = 42";
}

TEST_F(VMInstructionFunctionCallTest, CallNativeFunctionZeroReturnValue) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    auto testNativeFunctionZeroReturn = [](GC* gc, uint8_t argCount, Value* args, Value* ret) -> ErrorId {
        *ret = semiValueNewInt(0);
        return 0;
    };

    Value nativeFunc    = semiValueNewNativeFunction(testNativeFunctionZeroReturn);
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, nativeFunc);

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load native function
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);            // Call with 0 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Return value should be an integer";
    ASSERT_EQ(AS_INT(&vm->values[0]), 0) << "Return value should be 0";
}

TEST_F(VMInstructionFunctionCallTest, FunctionReachesEndWithMatchingCoarity) {
    // Test case: Function with coarity=0 reaches end without explicit return
    // This should succeed as the implicit return matches the expected coarity
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Function that just ends without explicit return (coarity=0)
    Instruction fnCode[1];
    fnCode[0] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);  // Implicit return added by compiler

    FunctionProto* func = CreateFunctionObject(0, fnCode, 1, 1, 0, 0);  // coarity=0
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load function
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);            // Call function with 0 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);               // Success

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully when function reaches end with matching coarity";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, FunctionReachesEndWithMismatchedCoarity) {
    // Test case: Function with coarity=1 reaches end without explicit return
    // This should fail with SEMI_ERROR_MISSING_RETURN_VALUE
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Function that just ends without explicit return but expects to return a value (coarity=1)
    Instruction fnCode[1];
    fnCode[0] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);  // Implicit return added by compiler

    FunctionProto* func = CreateFunctionObject(0, fnCode, 1, 1, 0, 1);  // coarity=1
    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);  // Load function
    code[1] = INSTRUCTION_CALL(0, 0, 0, false, false);            // Call function with 0 args
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);               // Should not reach here

    module->moduleInit = CreateFunctionObject(0, code, 3, 254, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, SEMI_ERROR_MISSING_RETURN_VALUE) << "VM should return missing return value error";
    ASSERT_EQ(vm->error, SEMI_ERROR_MISSING_RETURN_VALUE) << "VM error should be set to missing return value";
}
