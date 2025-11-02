// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <string>

#include "test_common.hpp"

class CompilerDeferTest : public CompilerTest {};

// Test Case 1: Basic defer block compilation - exact instruction verification
TEST_F(CompilerDeferTest, BasicDeferBlockExactInstructions) {
    const char* source = "defer { a := 1 }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Basic defer block should compile successfully";

    // Expected instructions in main module:
    // 0: OP_DEFER_CALL K A: 0x00, K: 0x0000, i: false, s: false
    // 1: OP_RETURN     T A: 0xFF, B: 0x00, C: 0x00, kb: false, kc: false

    EXPECT_EQ(GetCodeSize(), 2) << "Should have exactly 2 instructions in main module";

    // Verify first instruction: OP_DEFER_CALL
    Instruction instr0 = GetInstruction(0);
    EXPECT_EQ(GET_OPCODE(instr0), OP_DEFER_CALL) << "First instruction should be OP_DEFER_CALL";

    KInstruction deferCall = decodeKInstruction(instr0);
    EXPECT_EQ(deferCall.destReg, 0x00) << "DEFER_CALL A operand should be 0x00";
    EXPECT_EQ(deferCall.constant, 0x0000) << "DEFER_CALL K operand should be 0x0000 (first constant)";
    EXPECT_FALSE(deferCall.inlineFlag) << "DEFER_CALL inline flag should be false";
    EXPECT_FALSE(deferCall.signFlag) << "DEFER_CALL sign flag should be false";

    // Verify second instruction: OP_RETURN
    Instruction instr1 = GetInstruction(1);
    EXPECT_EQ(GET_OPCODE(instr1), OP_RETURN) << "Second instruction should be OP_RETURN";

    TInstruction returnInstr = decodeTInstruction(instr1);
    EXPECT_EQ(returnInstr.destReg, 0xFF) << "RETURN A operand should be 0xFF";
    EXPECT_EQ(returnInstr.srcReg1, 0x00) << "RETURN B operand should be 0x00";
    EXPECT_EQ(returnInstr.srcReg2, 0x00) << "RETURN C operand should be 0x00";
    EXPECT_FALSE(returnInstr.constFlag1) << "RETURN kb flag should be false";
    EXPECT_FALSE(returnInstr.constFlag2) << "RETURN kc flag should be false";

    // Verify the defer function exists in constant table
    EXPECT_LT(deferCall.constant, semiConstantTableSize(&module->constantTable)) << "Constant index should be valid";
    Value deferFunction = semiConstantTableGet(&module->constantTable, deferCall.constant);
    EXPECT_TRUE(IS_FUNCTION_PROTO(&deferFunction)) << "Defer constant should be a function prototype";

    // Verify defer function properties
    FunctionProto* fnProto = AS_FUNCTION_PROTO(&deferFunction);
    EXPECT_EQ(fnProto->arity, 0) << "Defer function should have 0 parameters";
    EXPECT_EQ(fnProto->coarity, 0) << "Defer function should have 0 return values";

    // Expected instructions in defer function:
    // 0: OP_LOAD_INLINE_INTEGER K A: 0x00, K: 0x0001, i: true, s: true
    // 1: OP_RETURN              T A: 0xFF, B: 0x00, C: 0x00, kb: false, kc: false

    EXPECT_EQ(fnProto->chunk.size, 2) << "Defer function should have exactly 2 instructions";

    Instruction deferInstr0 = fnProto->chunk.data[0];
    EXPECT_EQ(GET_OPCODE(deferInstr0), OP_LOAD_INLINE_INTEGER)
        << "Defer function first instruction should be OP_LOAD_INLINE_INTEGER";

    KInstruction loadInt = decodeKInstruction(deferInstr0);
    EXPECT_EQ(loadInt.destReg, 0x00) << "LOAD_INLINE_INTEGER A operand should be 0x00";
    EXPECT_EQ(loadInt.constant, 0x0001) << "LOAD_INLINE_INTEGER K operand should be 0x0001";
    EXPECT_TRUE(loadInt.inlineFlag) << "LOAD_INLINE_INTEGER inline flag should be true";
    EXPECT_TRUE(loadInt.signFlag) << "LOAD_INLINE_INTEGER sign flag should be true";
}

// Test Case 2: Multiple defer blocks - exact instruction verification
TEST_F(CompilerDeferTest, MultipleDeferBlocksExactInstructions) {
    const char* source = "defer { a := 1 }\ndefer { b := 2 }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Multiple defer blocks should compile successfully";

    // Expected instructions in main module:
    // 0: OP_DEFER_CALL K A: 0x00, K: 0x0000, i: false, s: false
    // 1: OP_DEFER_CALL K A: 0x00, K: 0x0001, i: false, s: false
    // 2: OP_RETURN     T A: 0xFF, B: 0x00, C: 0x00, kb: false, kc: false

    EXPECT_EQ(GetCodeSize(), 3) << "Should have exactly 3 instructions in main module";

    // Verify first DEFER_CALL (K: 0x0000)
    Instruction instr0 = GetInstruction(0);
    EXPECT_EQ(GET_OPCODE(instr0), OP_DEFER_CALL) << "First instruction should be OP_DEFER_CALL";
    KInstruction deferCall0 = decodeKInstruction(instr0);
    EXPECT_EQ(deferCall0.constant, 0x0000) << "First DEFER_CALL should reference constant 0x0000";

    // Verify second DEFER_CALL (K: 0x0001)
    Instruction instr1 = GetInstruction(1);
    EXPECT_EQ(GET_OPCODE(instr1), OP_DEFER_CALL) << "Second instruction should be OP_DEFER_CALL";
    KInstruction deferCall1 = decodeKInstruction(instr1);
    EXPECT_EQ(deferCall1.constant, 0x0001) << "Second DEFER_CALL should reference constant 0x0001";

    // Verify RETURN instruction
    Instruction instr2 = GetInstruction(2);
    EXPECT_EQ(GET_OPCODE(instr2), OP_RETURN) << "Third instruction should be OP_RETURN";

    // Verify both defer functions exist in constant table
    EXPECT_EQ(semiConstantTableSize(&module->constantTable), 2) << "Should have exactly 2 constants";

    Value deferFunction0 = semiConstantTableGet(&module->constantTable, 0);
    Value deferFunction1 = semiConstantTableGet(&module->constantTable, 1);
    EXPECT_TRUE(IS_FUNCTION_PROTO(&deferFunction0)) << "First defer constant should be function prototype";
    EXPECT_TRUE(IS_FUNCTION_PROTO(&deferFunction1)) << "Second defer constant should be function prototype";

    // Verify both defer functions have correct structure
    FunctionProto* fnProto0 = AS_FUNCTION_PROTO(&deferFunction0);
    FunctionProto* fnProto1 = AS_FUNCTION_PROTO(&deferFunction1);

    EXPECT_EQ(fnProto0->arity, 0) << "First defer function should have 0 arity";
    EXPECT_EQ(fnProto0->coarity, 0) << "First defer function should have 0 coarity";
    EXPECT_EQ(fnProto1->arity, 0) << "Second defer function should have 0 arity";
    EXPECT_EQ(fnProto1->coarity, 0) << "Second defer function should have 0 coarity";

    // Both defer functions should have: OP_LOAD_INLINE_INTEGER + OP_RETURN
    EXPECT_EQ(fnProto0->chunk.size, 2) << "First defer function should have 2 instructions";
    EXPECT_EQ(fnProto1->chunk.size, 2) << "Second defer function should have 2 instructions";

    EXPECT_EQ(GET_OPCODE(fnProto0->chunk.data[0]), OP_LOAD_INLINE_INTEGER);
    EXPECT_EQ(GET_OPCODE(fnProto1->chunk.data[0]), OP_LOAD_INLINE_INTEGER);
}

// Test Case 3: Defer in function scope - exact instruction verification
TEST_F(CompilerDeferTest, DeferInFunctionExactInstructions) {
    const char* source = "fn test() { defer { cleanup := true } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Defer in function should compile successfully";

    // Expected instructions in main module:
    // 0: OP_LOAD_CONSTANT  K A: 0x00, K: 0x0001, i: false, s: false
    // 1: OP_SET_MODULE_VAR K A: 0x00, K: 0x0000, i: false, s: false
    // 2: OP_RETURN         T A: 0xFF, B: 0x00, C: 0x00, kb: false, kc: false

    EXPECT_EQ(GetCodeSize(), 3) << "Should have exactly 3 instructions in main module";

    Instruction instr0 = GetInstruction(0);
    EXPECT_EQ(GET_OPCODE(instr0), OP_LOAD_CONSTANT) << "First instruction should load function constant";

    Instruction instr1 = GetInstruction(1);
    EXPECT_EQ(GET_OPCODE(instr1), OP_SET_MODULE_VAR) << "Second instruction should set module variable";

    Instruction instr2 = GetInstruction(2);
    EXPECT_EQ(GET_OPCODE(instr2), OP_RETURN) << "Third instruction should be return";

    // The test function should be constant index 1 (after defer function at index 0)
    KInstruction loadConst = decodeKInstruction(instr0);
    EXPECT_EQ(loadConst.constant, 0x0001) << "Should load function from constant 1";

    // Verify the test function exists and contains DEFER_CALL
    Value testFunctionValue = semiConstantTableGet(&module->constantTable, 1);
    EXPECT_TRUE(IS_FUNCTION_PROTO(&testFunctionValue)) << "Should be function prototype";

    FunctionProto* testFunction = AS_FUNCTION_PROTO(&testFunctionValue);

    // Expected instructions in test function:
    // 0: OP_DEFER_CALL K A: 0x00, K: 0x0000, i: false, s: false
    // 1: OP_RETURN     T A: 0xFF, B: 0x00, C: 0x00, kb: false, kc: false

    EXPECT_EQ(testFunction->chunk.size, 2) << "Test function should have exactly 2 instructions";

    Instruction fnInstr0 = testFunction->chunk.data[0];
    EXPECT_EQ(GET_OPCODE(fnInstr0), OP_DEFER_CALL) << "Function should start with DEFER_CALL";

    KInstruction fnDeferCall = decodeKInstruction(fnInstr0);
    EXPECT_EQ(fnDeferCall.constant, 0x0000) << "Function DEFER_CALL should reference constant 0";

    Instruction fnInstr1 = testFunction->chunk.data[1];
    EXPECT_EQ(GET_OPCODE(fnInstr1), OP_RETURN) << "Function should end with RETURN";
}

// Test Case 4: Nested defer error - verify compilation fails
TEST_F(CompilerDeferTest, NestedDeferError) {
    const char* source = "defer { defer { x := 42 } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_NESTED_DEFER) << "Nested defer should fail with SEMI_ERROR_NESTED_DEFER";
}

// Test Case 5: Return with value in defer block - verify compilation fails
TEST_F(CompilerDeferTest, ReturnValueInDeferError) {
    const char* source = "fn test() { defer { return 42 } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, SEMI_ERROR_RETURN_VALUE_IN_DEFER)
        << "Return with value in defer should fail with SEMI_ERROR_RETURN_VALUE_IN_DEFER";
}

// Test Case 6: Return without value in defer block - verify exact instructions
TEST_F(CompilerDeferTest, ReturnWithoutValueInDeferExactInstructions) {
    const char* source = "fn test() { defer { x := 1; return } }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Return without value in defer should compile successfully";

    // Find the test function
    Value testFunctionValue = semiConstantTableGet(&module->constantTable, 1);
    EXPECT_TRUE(IS_FUNCTION_PROTO(&testFunctionValue));
    FunctionProto* testFunction = AS_FUNCTION_PROTO(&testFunctionValue);

    // Find the defer function (should be at constant 0)
    Value deferFunctionValue = semiConstantTableGet(&module->constantTable, 0);
    EXPECT_TRUE(IS_FUNCTION_PROTO(&deferFunctionValue));
    FunctionProto* deferFunction = AS_FUNCTION_PROTO(&deferFunctionValue);

    // The defer function should have: LOAD_INLINE_INTEGER, RETURN, RETURN
    // (one for assignment, one explicit return, one implicit return at end)
    EXPECT_EQ(deferFunction->chunk.size, 3) << "Defer function should have exactly 3 instructions";

    // Should have assignment instruction
    EXPECT_EQ(GET_OPCODE(deferFunction->chunk.data[0]), OP_LOAD_INLINE_INTEGER);

    // Should end with RETURN
    Instruction lastInstr = deferFunction->chunk.data[deferFunction->chunk.size - 1];
    EXPECT_EQ(GET_OPCODE(lastInstr), OP_RETURN);
}

// Test Case 7: Empty defer block - verify minimal instructions
TEST_F(CompilerDeferTest, EmptyDeferBlockExactInstructions) {
    const char* source = "defer { }";

    ErrorId result = ParseModule(source);
    EXPECT_EQ(result, 0) << "Empty defer block should compile successfully";

    // Should have same main module structure as basic defer
    EXPECT_EQ(GetCodeSize(), 2) << "Should have exactly 2 instructions in main module";

    Instruction instr0 = GetInstruction(0);
    EXPECT_EQ(GET_OPCODE(instr0), OP_DEFER_CALL) << "Should have DEFER_CALL";

    // Verify empty defer function
    Value deferFunctionValue = semiConstantTableGet(&module->constantTable, 0);
    EXPECT_TRUE(IS_FUNCTION_PROTO(&deferFunctionValue));
    FunctionProto* deferFunction = AS_FUNCTION_PROTO(&deferFunctionValue);

    // Empty defer function should only have RETURN
    EXPECT_EQ(deferFunction->chunk.size, 1) << "Empty defer function should have only 1 instruction";
    EXPECT_EQ(GET_OPCODE(deferFunction->chunk.data[0]), OP_RETURN) << "Empty defer should only have RETURN";
}
