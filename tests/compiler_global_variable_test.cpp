// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

#include "test_common.hpp"

class CompilerGlobalVariableTest : public CompilerTest {};

TEST_F(CompilerGlobalVariableTest, GlobalVariableCorrectlyResolvedDuringCompilation) {
    // Add a global variable to the VM
    Value globalValue = semiValueNewInt(42);
    AddGlobalVariable("globalVar", globalValue);

    // Parse an expression that references the global variable
    PrattExpr expr;
    ErrorId result = ParseExpression("globalVar", &expr);
    ASSERT_EQ(result, 0) << "Parsing reference to global variable should succeed";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr), OP_LOAD_CONSTANT) << "Should be LOAD_CONSTANT instruction";
    ASSERT_FALSE(OPERAND_K_I(instr)) << "Should not be inline constant";
    ASSERT_TRUE(OPERAND_K_S(instr)) << "Should have global flag set";
    ASSERT_EQ(OPERAND_K_K(instr), 0) << "Should reference global constant at index 0";
}

TEST_F(CompilerGlobalVariableTest, MultipleGlobalVariablesResolvedCorrectly) {
    // Add multiple global variables
    Value globalValue1 = semiValueNewInt(10);
    Value globalValue2 = semiValueNewFloat(3.14);
    Value globalValue3 = semiValueNewBool(true);

    AddGlobalVariable("global1", globalValue1);
    AddGlobalVariable("global2", globalValue2);
    AddGlobalVariable("global3", globalValue3);

    // Test accessing the second global variable
    PrattExpr expr;
    ErrorId result = ParseExpression("global2", &expr);
    ASSERT_EQ(result, 0) << "Parsing reference to second global variable should succeed";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr), OP_LOAD_CONSTANT) << "Should be LOAD_CONSTANT instruction";
    ASSERT_FALSE(OPERAND_K_I(instr)) << "Should not be inline constant";
    ASSERT_TRUE(OPERAND_K_S(instr)) << "Should have global flag set";
    ASSERT_EQ(OPERAND_K_K(instr), 1) << "Should reference global constant at index 1";
}

TEST_F(CompilerGlobalVariableTest, LocalVariableCannotBeDefinedWhenGlobalExists) {
    // Add a global variable
    Value globalValue = semiValueNewInt(100);
    AddGlobalVariable("conflictVar", globalValue);

    // Try to define a local variable with the same name
    ErrorId result = ParseStatement("{ conflictVar := 200 }", true);
    ASSERT_NE(result, 0) << "Defining local variable with same name as global should fail";
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Should return variable already defined error";
}

TEST_F(CompilerGlobalVariableTest, ModuleVariableCannotBeDefinedWhenGlobalExists) {
    // Add a global variable
    Value globalValue = semiValueStringCreate(&vm->gc, "global", 6);
    AddGlobalVariable("moduleConflict", globalValue);

    // Try to define a module variable with the same name
    ErrorId result = ParseStatement("export moduleConflict := \"module\"", false);
    ASSERT_NE(result, 0) << "Defining module variable with same name as global should fail";
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Should return variable already defined error";
}

TEST_F(CompilerGlobalVariableTest, GlobalVariableGeneratesCorrectInstruction) {
    // Add a global variable
    Value globalValue = semiValueStringCreate(&vm->gc, "test", 4);
    AddGlobalVariable("testGlobal", globalValue);

    // Parse an expression that references the global variable
    PrattExpr expr;
    ErrorId result = ParseExpression("testGlobal", &expr);
    ASSERT_EQ(result, 0) << "Parsing reference to global variable should succeed";

    ASSERT_EQ(GetCodeSize(), 1) << "Should generate exactly 1 instruction";

    Instruction instr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(instr), OP_LOAD_CONSTANT) << "Should be LOAD_CONSTANT instruction";

    uint8_t targetReg    = OPERAND_K_A(instr);
    uint16_t constantIdx = OPERAND_K_K(instr);
    bool isInline        = OPERAND_K_I(instr);
    bool isGlobal        = OPERAND_K_S(instr);

    ASSERT_GE(targetReg, 0) << "Target register should be valid";
    ASSERT_EQ(constantIdx, 0) << "Should reference global constant at index 0";
    ASSERT_FALSE(isInline) << "Should not be inline constant";
    ASSERT_TRUE(isGlobal) << "Should have global flag set (s=true)";
}

TEST_F(CompilerGlobalVariableTest, GlobalVariableTakesPrecedenceOverModuleVariable) {
    // First add a global variable
    Value globalValue = semiValueNewInt(42);
    AddGlobalVariable("sharedName", globalValue);

    // Add a module variable with the same name (this should be allowed)
    ErrorId moduleResult = ParseStatement("sharedName := 100", false);
    ASSERT_EQ(moduleResult, SEMI_ERROR_VARIABLE_ALREADY_DEFINED)
        << "Defining module variable with same name as global should fail";
}

TEST_F(CompilerGlobalVariableTest, NonExistentGlobalVariableNotResolved) {
    // Try to reference a non-existent global variable
    PrattExpr expr;
    ErrorId result = ParseExpression("nonExistentGlobal", &expr);
    ASSERT_EQ(result, SEMI_ERROR_UNINITIALIZED_VARIABLE) << "Should return uninitialized variable error";
}

TEST_F(CompilerGlobalVariableTest, ForLoopVariableCannotBeDefinedWhenGlobalExists) {
    // Add a global variable
    Value globalValue = semiValueNewInt(42);
    AddGlobalVariable("loopVar", globalValue);

    // Try to define a for-loop with the same variable name
    ErrorId result = ParseStatement("for loopVar in 0..5 { }", true);
    ASSERT_NE(result, 0) << "For-loop with global variable name should fail";
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Should return variable already defined error";
}

TEST_F(CompilerGlobalVariableTest, GlobalVariableAccessInComplexExpression) {
    // Add a global variable
    Value globalValue = semiValueNewInt(10);
    AddGlobalVariable("factor", globalValue);

    // Parse a complex expression using the global variable
    PrattExpr expr;
    ErrorId result = ParseExpression("factor * 2", &expr);
    ASSERT_EQ(result, 0) << "Parsing complex expression with global variable should succeed";

    ASSERT_GE(GetCodeSize(), 2) << "Should generate multiple instructions";

    // The first instruction should load the global variable
    Instruction firstInstr = GetInstruction(0);
    ASSERT_EQ(GET_OPCODE(firstInstr), OP_LOAD_CONSTANT) << "First instruction should be LOAD_CONSTANT";
    ASSERT_TRUE(OPERAND_K_S(firstInstr)) << "Should have global flag set";
    ASSERT_EQ(OPERAND_K_K(firstInstr), 0) << "Should reference global constant at index 0";
}

TEST_F(CompilerGlobalVariableTest, GlobalVariableInAssignmentExpression) {
    // Add a global variable
    Value globalValue = semiValueNewInt(5);
    AddGlobalVariable("base", globalValue);

    // Parse an assignment that uses the global variable in the right-hand side
    ErrorId result = ParseStatement("local := base + 10", true);
    ASSERT_EQ(result, 0) << "Parsing assignment with global variable should succeed";

    ASSERT_GE(GetCodeSize(), 1) << "Should generate at least one instruction";

    // Find the instruction that loads the global variable
    bool foundGlobalLoad = false;
    for (size_t i = 0; i < GetCodeSize(); i++) {
        Instruction instr = GetInstruction(i);
        if (GET_OPCODE(instr) == OP_LOAD_CONSTANT && OPERAND_K_S(instr)) {
            foundGlobalLoad = true;
            ASSERT_EQ(OPERAND_K_K(instr), 0) << "Should reference global constant at index 0";
            break;
        }
    }
    ASSERT_TRUE(foundGlobalLoad) << "Should find global variable load instruction";
}