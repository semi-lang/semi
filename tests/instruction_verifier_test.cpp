// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "instruction_verifier.hpp"

#include <gtest/gtest.h>

#include "test_common.hpp"

using namespace InstructionVerifier;

class InstructionVerifierTest : public VMTest {};

TEST_F(InstructionVerifierTest, BasicDeferBlock) {
    Instruction code[2];
    code[0] = INSTRUCTION_DEFER_CALL(0, 0, false, false);
    code[1] = INSTRUCTION_RETURN(0xFF, 0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    VerifyModule(module, R"(
[Instructions]
0: OP_DEFER_CALL    A=0x00 K=0x0000 i=F s=F
1: OP_RETURN        A=0xFF B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(InstructionVerifierTest, SimpleIfStatement) {
    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_BOOL(0, 0, true, false);
    code[1] = INSTRUCTION_C_JUMP(0, 1, false, true);
    code[2] = INSTRUCTION_CLOSE_UPVALUES(0, 0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 3, 8, 0, 0);
    module->moduleInit  = func;

    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_BOOL         A=0x00 K=0x0000 i=T s=F
1: OP_C_JUMP            A=0x00 K=0x0001 i=F s=T
2: OP_CLOSE_UPVALUES    A=0x00 B=0x00 C=0x00 kb=F kc=F
)");
}

TEST_F(InstructionVerifierTest, LocalIntegerAssignment) {
    Instruction code[1];
    code[0] = INSTRUCTION_LOAD_INLINE_INTEGER(0, 0x2A, true, true);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 1, 8, 0, 0);
    module->moduleInit  = func;

    VerifyModule(module, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x002A i=T s=T
)");
}

TEST_F(InstructionVerifierTest, IgnoredSection) {
    // Create a module with some instructions
    Instruction code[3];
    code[0] = INSTRUCTION_LOAD_INLINE_INTEGER(0, 0x2A, true, true);
    code[1] = INSTRUCTION_ADD(1, 0, 0, false, false);
    code[2] = INSTRUCTION_RETURN(0xFF, 0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 3, 8, 0, 0);
    module->moduleInit  = func;

    // Verify that we can mark the main instructions section as ignored
    // This should pass even though we're not specifying any instructions
    VerifyModule(module, R"(
[Instructions] (ignored)
)");

    // Create a nested function for testing ignored named sections
    Instruction nestedCode[2];
    nestedCode[0] = INSTRUCTION_LOAD_INLINE_INTEGER(0, 5, true, true);
    nestedCode[1] = INSTRUCTION_RETURN(0, 0, 0, false, false);

    FunctionProto* nestedFunc = CreateFunctionObject(0, nestedCode, 2, 5, 0, 1);
    Value funcProtoValue      = semiValueNewPtr(nestedFunc, VALUE_TYPE_FUNCTION_PROTO);
    semiConstantTableInsert(&module->constantTable, funcProtoValue);

    // Verify that we can ignore the nested function's instructions
    VerifyModule(module, R"(
[Instructions] (ignored)

[Constants]
K[0]: FunctionProto arity=0 coarity=1 maxStackSize=5 -> @testFunc

[Instructions:testFunc] (ignored)
)");
}
