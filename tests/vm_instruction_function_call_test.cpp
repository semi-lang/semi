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
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0001 i=F s=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @testFunc

[Instructions:testFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 1) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, CallFunctionWithArguments) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[2]: Int 42
R[3]: Int 84

[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x02 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=2 coarity=0 maxStackSize=3 -> @testFunc

[Instructions:testFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, NestedFunctionCalls) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=2 -> @outerFunc
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @innerFunc

[Instructions:outerFunc]
0: OP_LOAD_CONSTANT  A=0x01 K=0x0001 i=F s=F
1: OP_CALL           A=0x01 B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN         A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:innerFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully with nested calls";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, ReturnValue) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_INLINE_INTEGER  A=0x00 K=0x002A i=T s=T
1: OP_RETURN               A=0x01 B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "Should not return invalid error";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Returned value should be an integer";
    ASSERT_EQ(AS_INT(&vm->values[0]), 42) << "Returned value should be 42";
}

// Error Condition Tests for OP_CALL

TEST_F(VMInstructionFunctionCallTest, CallNonFunctionObject) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Int 42
)",
                                                            &module);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE) << "Should return type error";
    ASSERT_EQ(vm->error, SEMI_ERROR_UNEXPECTED_TYPE) << "VM error should be set";
}

TEST_F(VMInstructionFunctionCallTest, CallArityMismatch) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x02 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=1 coarity=0 maxStackSize=2 -> @testFunc

[Instructions:testFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, SEMI_ERROR_ARGS_COUNT_MISMATCH) << "Should return arity mismatch error";
    ASSERT_EQ(vm->error, SEMI_ERROR_ARGS_COUNT_MISMATCH) << "VM error should be set";
}

// Stack Management Tests

TEST_F(VMInstructionFunctionCallTest, CallStackGrowth) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=254 -> @func0
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=254 -> @func1
K[2]: FunctionProto arity=0 coarity=0 maxStackSize=254 -> @func2
K[3]: FunctionProto arity=0 coarity=0 maxStackSize=254 -> @func3
K[4]: FunctionProto arity=0 coarity=0 maxStackSize=254 -> @func4

[Instructions:func0]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0001 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN         A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:func1]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0002 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN         A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:func2]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0003 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN         A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:func3]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0004 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_RETURN         A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:func4]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should handle stack growth successfully";

    //        base + maxStackSize
    // root:  0 + 254 (specified in ModuleInit)
    // func0: 1 + 254
    // func1: 2 + 254
    // func2: 3 + 254
    // func3: 4 + 254
    // func4: 5 + 254
    ASSERT_EQ(vm->valueCapacity, 512) << "Stack should have grown to accommodate function";
}

TEST_F(VMInstructionFunctionCallTest, ArgumentPositioning) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[1]: Int 10
R[2]: Int 20
R[3]: Int 30

[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x03 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=3 coarity=0 maxStackSize=2 -> @testFunc

[Instructions:testFunc]
0: OP_MOVE    A=0x00 B=0x01 C=0x00 kb=F kc=F
1: OP_RETURN  A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Return value should be an integer";
    ASSERT_EQ(AS_INT(&vm->values[0]), 20) << "Function should have received correct arguments at correct positions";
}

// Program Counter Management Tests

TEST_F(VMInstructionFunctionCallTest, PCCorrectlyRestored) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[PreDefine:Registers]
R[2]: Int 42
R[5]: Int 84

[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x03 K=0x0000 i=F s=F
1: OP_CALL           A=0x03 B=0x00 C=0x00 kb=F kc=F
2: OP_MOVE           A=0x01 B=0x02 C=0x00 kb=F kc=F
3: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=2 -> @testFunc

[Instructions:testFunc]
0: OP_MOVE    A=0x00 B=0x01 C=0x00 kb=F kc=F
1: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(vm->values[1].as.i, 42) << "Instruction after return should execute";
    ASSERT_EQ(vm->values[4].as.i, 84) << "Function body should execute and return value should be in function register";
}

// Edge Cases

TEST_F(VMInstructionFunctionCallTest, FunctionWithMaxArity) {
    // Set up 253 arguments
    for (int i = 0; i < 253; i++) {
        vm->values[1 + i] = semiValueNewInt(i);
    }

    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0xFD C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=253 coarity=0 maxStackSize=1 -> @testFunc

[Instructions:testFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should handle maximum arity successfully";
}

TEST_F(VMInstructionFunctionCallTest, ImmediateReturn) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @testFunc

[Instructions:testFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should handle immediate return";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

// Native Function Call Tests

static ErrorId testNativeFunctionNoArgs(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    *ret = semiValueNewInt(42);
    return 0;
}

static ErrorId testNativeFunctionWithArgs(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
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

static ErrorId testNativeFunctionWithError(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId testNativeFunctionMaxArgs(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
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

    ErrorId result = RunModule(module);

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

    ErrorId result = RunModule(module);

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

    ErrorId result = RunModule(module);

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

    ErrorId result = RunModule(module);

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

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
    ASSERT_EQ(vm->values[5].header, VALUE_TYPE_INT) << "Return value should be stored in function register";
    ASSERT_EQ(AS_INT(&vm->values[5]), 42) << "Return value should be 15 + 27 = 42";
}

TEST_F(VMInstructionFunctionCallTest, CallNativeFunctionZeroReturnValue) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    auto testNativeFunctionZeroReturn = [](SemiVM* vm, uint8_t argCount, Value* args, Value* ret) -> ErrorId {
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

    ErrorId result = RunModule(module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Return value should be an integer";
    ASSERT_EQ(AS_INT(&vm->values[0]), 0) << "Return value should be 0";
}

TEST_F(VMInstructionFunctionCallTest, FunctionReachesEndWithMatchingCoarity) {
    // Test case: Function with coarity=0 reaches end without explicit return
    // This should succeed as the implicit return matches the expected coarity
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=1 -> @testFunc

[Instructions:testFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should complete successfully when function reaches end with matching coarity";
    ASSERT_EQ(vm->frameCount, 1) << "Only root frame should remain";
}

TEST_F(VMInstructionFunctionCallTest, FunctionReachesEndWithMismatchedCoarity) {
    // Test case: Function with coarity=1 reaches end without explicit return
    // This should fail with SEMI_ERROR_MISSING_RETURN_VALUE
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=254

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=1 -> @testFunc

[Instructions:testFunc]
0: OP_RETURN  A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, SEMI_ERROR_MISSING_RETURN_VALUE) << "VM should return missing return value error";
    ASSERT_EQ(vm->error, SEMI_ERROR_MISSING_RETURN_VALUE) << "VM error should be set to missing return value";
}
