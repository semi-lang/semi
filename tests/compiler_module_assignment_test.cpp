// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "test_common.hpp"

class CompilerModuleAssignmentTest : public CompilerTest {};

TEST_F(CompilerModuleAssignmentTest, ModuleIntegerAssignment) {
    ErrorId result = ParseStatement("x := 42", false);
    ASSERT_EQ(result, 0) << "Parsing 'x := 42' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("x", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'x' should exist as module variable";
    ASSERT_FALSE(isExport) << "Variable 'x' should be in globals, not exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_INLINE_INTEGER) << "First instruction should be LOAD_INLINE_INTEGER";
    ASSERT_EQ(OPERAND_K_K(instr1), 42) << "Should load constant 42";
    ASSERT_TRUE(OPERAND_K_I(instr1)) << "Should be inline constant";
    ASSERT_TRUE(OPERAND_K_S(instr1)) << "Should be positive";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr2)) << "Should target globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, ModuleDoubleAssignment) {
    ErrorId result = ParseStatement("y := 3.14", false);
    ASSERT_EQ(result, 0) << "Parsing 'y := 3.14' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("y", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'y' should exist as module variable";
    ASSERT_FALSE(isExport) << "Variable 'y' should be in globals, not exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_CONSTANT) << "First instruction should be LOAD_CONSTANT";
    ASSERT_FALSE(OPERAND_K_I(instr1)) << "Floats always use constant table";

    // The value should be stored in the constant table
    uint16_t constIdx = OPERAND_K_K(instr1);
    ASSERT_LT(constIdx, semiConstantTableSize(&compiler.artifactModule->constantTable))
        << "Constant index should be valid";
    Value constValue = semiConstantTableGet(&compiler.artifactModule->constantTable, constIdx);
    ASSERT_DOUBLE_EQ(AS_FLOAT(&constValue), 3.14) << "Constant value should be 3.14";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr2)) << "Should target globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, ModuleBooleanAssignment) {
    ErrorId result = ParseStatement("flag := true", false);
    ASSERT_EQ(result, 0) << "Parsing 'flag := true' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("flag", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'flag' should exist as module variable";
    ASSERT_FALSE(isExport) << "Variable 'flag' should be in globals, not exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_BOOL) << "First instruction should be LOAD_BOOL";
    ASSERT_EQ(OPERAND_K_I(instr1), 1) << "Should load true";
    ASSERT_FALSE(OPERAND_K_S(instr1)) << "Should not jump (K=0)";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr2)) << "Should target globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, ModuleStringAssignment) {
    ErrorId result = ParseStatement("name := \"hello\"", false);
    ASSERT_EQ(result, 0) << "Parsing 'name := \"hello\"' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("name", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'name' should exist as module variable";
    ASSERT_FALSE(isExport) << "Variable 'name' should be in globals, not exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_CONSTANT) << "First instruction should be LOAD_CONSTANT";

    // The value should be stored in the constant table
    uint16_t constIdx = OPERAND_K_K(instr1);
    ASSERT_LT(constIdx, semiConstantTableSize(&compiler.artifactModule->constantTable))
        << "Constant index should be valid";

    Value constValue = semiConstantTableGet(&compiler.artifactModule->constantTable, constIdx);
    ASSERT_EQ(VALUE_TYPE(&constValue), VALUE_TYPE_OBJECT_STRING) << "Should be string constant";
    ASSERT_EQ(AS_OBJECT_STRING(&constValue)->length, 5) << "String should have correct size";
    ASSERT_EQ(strncmp((const char*)AS_OBJECT_STRING(&constValue)->str, "hello", 5), 0) << "String content should match";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr2)) << "Should target globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, ModuleExpressionAssignment) {
    ErrorId result = ParseStatement("result := 10 + 5", false);
    ASSERT_EQ(result, 0) << "Parsing 'result := 10 + 5' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("result", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'result' should exist as module variable";
    ASSERT_FALSE(isExport) << "Variable 'result' should be in globals, not exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_INLINE_INTEGER) << "First instruction should be LOAD_INLINE_INTEGER";
    ASSERT_EQ(OPERAND_K_K(instr1), 15) << "Should load constant 15";
    ASSERT_TRUE(OPERAND_K_I(instr1)) << "Should be inline constant";
    ASSERT_TRUE(OPERAND_K_S(instr1)) << "Should be positive";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr2)) << "Should target globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, ModuleVariableToModuleVariableAssignment) {
    // Initialize first module variable
    InitializeModuleVariable("x");

    ErrorId result = ParseStatement("y := x", false);
    ASSERT_EQ(result, 0) << "Parsing 'y := x' should succeed";

    bool isExportX, isExportY;
    ModuleVariableId moduleVarIdX = GetModuleVariableId("x", &isExportX);
    ModuleVariableId moduleVarIdY = GetModuleVariableId("y", &isExportY);
    ASSERT_NE(moduleVarIdX, INVALID_MODULE_VARIABLE_ID) << "Variable 'x' should exist as module variable";
    ASSERT_NE(moduleVarIdY, INVALID_MODULE_VARIABLE_ID) << "Variable 'y' should exist as module variable";
    ASSERT_FALSE(isExportX) << "Variable 'x' should be in globals, not exports";
    ASSERT_FALSE(isExportY) << "Variable 'y' should be in globals, not exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load x into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_GET_MODULE_VAR) << "First instruction should be GET_MODULE_VAR";

    ASSERT_EQ(OPERAND_K_K(instr1), moduleVarIdX) << "Should read from x's module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr1)) << "Should read from globals, not exports";

    // Second instruction: Store register to y's module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";

    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarIdY) << "Should write to y's module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr2)) << "Should write to globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, ModuleVariableReassignment) {
    InitializeModuleVariable("counter");

    ErrorId result = ParseStatement("counter = 100", false);
    ASSERT_EQ(result, 0) << "Assignment should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("counter", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'counter' should exist as module variable";
    ASSERT_FALSE(isExport) << "Variable 'counter' should be in globals, not exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_INLINE_INTEGER) << "First instruction should be LOAD_INLINE_INTEGER";
    ASSERT_EQ(OPERAND_K_K(instr1), 100) << "Should load 100";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use same module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr2)) << "Should target globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, MultipleModuleVariablesUniqueIds) {
    ASSERT_EQ(ParseStatement("a := 1", false), 0) << "Binding a should succeed";
    ASSERT_EQ(ParseStatement("b := 2", false), 0) << "Binding b should succeed";
    ASSERT_EQ(ParseStatement("c := 3", false), 0) << "Binding c should succeed";

    ModuleVariableId varId_a = GetModuleVariableId("a");
    ModuleVariableId varId_b = GetModuleVariableId("b");
    ModuleVariableId varId_c = GetModuleVariableId("c");

    ASSERT_NE(varId_a, INVALID_MODULE_VARIABLE_ID) << "Variable 'a' should have valid module variable ID";
    ASSERT_NE(varId_b, INVALID_MODULE_VARIABLE_ID) << "Variable 'b' should have valid module variable ID";
    ASSERT_NE(varId_c, INVALID_MODULE_VARIABLE_ID) << "Variable 'c' should have valid module variable ID";

    ASSERT_NE(varId_a, varId_b) << "Variables should have different module variable IDs";
    ASSERT_NE(varId_b, varId_c) << "Variables should have different module variable IDs";
    ASSERT_NE(varId_a, varId_c) << "Variables should have different module variable IDs";

    bool isExportA, isExportB, isExportC;
    GetModuleVariableId("a", &isExportA);
    GetModuleVariableId("b", &isExportB);
    GetModuleVariableId("c", &isExportC);
    ASSERT_FALSE(isExportA) << "Variable 'a' should be in globals, not exports";
    ASSERT_FALSE(isExportB) << "Variable 'b' should be in globals, not exports";
    ASSERT_FALSE(isExportC) << "Variable 'c' should be in globals, not exports";
}

TEST_F(CompilerModuleAssignmentTest, AssignmentToConstant) {
    ErrorId result = ParseStatement("42 := x", false);
    ASSERT_EQ(result, SEMI_ERROR_EXPECT_LVALUE) << "Assignment to constant should fail";
}

TEST_F(CompilerModuleAssignmentTest, AssignmentToLiteral) {
    ErrorId result = ParseStatement("42 := 10", false);
    ASSERT_EQ(result, SEMI_ERROR_EXPECT_LVALUE) << "Assignment to literal should fail";
}

TEST_F(CompilerModuleAssignmentTest, AssignmentToExpression) {
    // Initialize module variables x and y
    InitializeModuleVariable("x");
    InitializeModuleVariable("y");

    // Try to assign to an arithmetic expression - should fail
    ErrorId result = ParseStatement("x + y := 10", false);
    ASSERT_EQ(result, SEMI_ERROR_EXPECT_LVALUE) << "Assignment to expression should fail";
}

TEST_F(CompilerModuleAssignmentTest, ModuleVariableRedefinitionInSameScope) {
    ASSERT_EQ(ParseStatement("x := 42", false), 0) << "First binding should succeed";

    ErrorId result = ParseStatement("x := 100", false);
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Module variable redefinition should fail";
}

TEST_F(CompilerModuleAssignmentTest, ExportIntegerAssignment) {
    ErrorId result = ParseStatement("export x := 42", false);
    ASSERT_EQ(result, 0) << "Parsing 'export x := 42' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("x", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'x' should exist as module variable";
    ASSERT_TRUE(isExport) << "Variable 'x' should be in exports, not globals";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_INLINE_INTEGER) << "First instruction should be LOAD_INLINE_INTEGER";
    ASSERT_EQ(OPERAND_K_K(instr1), 42) << "Should load constant 42";
    ASSERT_TRUE(OPERAND_K_I(instr1)) << "Should be inline constant";
    ASSERT_TRUE(OPERAND_K_S(instr1)) << "Should be positive";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should target exports, not globals";
}

TEST_F(CompilerModuleAssignmentTest, ExportDoubleAssignment) {
    ErrorId result = ParseStatement("export pi := 3.14159", false);
    ASSERT_EQ(result, 0) << "Parsing 'export pi := 3.14159' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("pi", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'pi' should exist as module variable";
    ASSERT_TRUE(isExport) << "Variable 'pi' should be in exports, not globals";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_CONSTANT) << "First instruction should be LOAD_CONSTANT";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should target exports, not globals";
}

TEST_F(CompilerModuleAssignmentTest, ExportBooleanAssignment) {
    ErrorId result = ParseStatement("export debugMode := true", false);
    ASSERT_EQ(result, 0) << "Parsing 'export debugMode := true' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("debugMode", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'debugMode' should exist as module variable";
    ASSERT_TRUE(isExport) << "Variable 'debugMode' should be in exports, not globals";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_BOOL) << "First instruction should be LOAD_BOOL";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should target exports, not globals";
}

TEST_F(CompilerModuleAssignmentTest, ExportStringAssignment) {
    ErrorId result = ParseStatement("export appName := \"MyApp\"", false);
    ASSERT_EQ(result, 0) << "Parsing 'export appName := \"MyApp\"' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("appName", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'appName' should exist as module variable";
    ASSERT_TRUE(isExport) << "Variable 'appName' should be in exports, not globals";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_CONSTANT) << "First instruction should be LOAD_CONSTANT";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should target exports, not globals";
}

TEST_F(CompilerModuleAssignmentTest, ExportExpressionAssignment) {
    ErrorId result = ParseStatement("export maxValue := 100 * 2 + 56", false);
    ASSERT_EQ(result, 0) << "Parsing 'export maxValue := 100 * 2 + 56' should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("maxValue", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'maxValue' should exist as module variable";
    ASSERT_TRUE(isExport) << "Variable 'maxValue' should be in exports, not globals";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_INLINE_INTEGER) << "First instruction should be LOAD_INLINE_INTEGER";
    ASSERT_EQ(OPERAND_K_K(instr1), 256) << "Should load constant 256";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use correct module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should target exports, not globals";
}

TEST_F(CompilerModuleAssignmentTest, ExportVariableToExportVariableAssignment) {
    // Initialize first export variable
    InitializeModuleVariable("baseValue", true);

    ErrorId result = ParseStatement("export derivedValue := baseValue", false);
    ASSERT_EQ(result, 0) << "Parsing 'export derivedValue := baseValue' should succeed";

    bool isExportBase, isExportDerived;
    ModuleVariableId moduleVarIdBase    = GetModuleVariableId("baseValue", &isExportBase);
    ModuleVariableId moduleVarIdDerived = GetModuleVariableId("derivedValue", &isExportDerived);
    ASSERT_NE(moduleVarIdBase, INVALID_MODULE_VARIABLE_ID) << "Variable 'baseValue' should exist as module variable";
    ASSERT_NE(moduleVarIdDerived, INVALID_MODULE_VARIABLE_ID)
        << "Variable 'derivedValue' should exist as module variable";
    ASSERT_TRUE(isExportBase) << "Variable 'baseValue' should be in exports";
    ASSERT_TRUE(isExportDerived) << "Variable 'derivedValue' should be in exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load baseValue into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_GET_MODULE_VAR) << "First instruction should be GET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr1), moduleVarIdBase) << "Should read from baseValue's module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr1)) << "Should read from exports";

    // Second instruction: Store register to derivedValue's module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarIdDerived) << "Should write to derivedValue's module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should write to exports";
}

TEST_F(CompilerModuleAssignmentTest, GlobalVariableToExportVariableAssignment) {
    // Initialize a global variable
    InitializeModuleVariable("globalVar", false);

    ErrorId result = ParseStatement("export publicVar := globalVar", false);
    ASSERT_EQ(result, 0) << "Parsing 'export publicVar := globalVar' should succeed";

    bool isExportGlobal, isExportPublic;
    ModuleVariableId moduleVarIdGlobal = GetModuleVariableId("globalVar", &isExportGlobal);
    ModuleVariableId moduleVarIdPublic = GetModuleVariableId("publicVar", &isExportPublic);
    ASSERT_NE(moduleVarIdGlobal, INVALID_MODULE_VARIABLE_ID) << "Variable 'globalVar' should exist as module variable";
    ASSERT_NE(moduleVarIdPublic, INVALID_MODULE_VARIABLE_ID) << "Variable 'publicVar' should exist as module variable";
    ASSERT_FALSE(isExportGlobal) << "Variable 'globalVar' should be in globals";
    ASSERT_TRUE(isExportPublic) << "Variable 'publicVar' should be in exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load globalVar into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_GET_MODULE_VAR) << "First instruction should be GET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr1), moduleVarIdGlobal) << "Should read from globalVar's module variable ID";
    ASSERT_FALSE(OPERAND_K_S(instr1)) << "Should read from globals";

    // Second instruction: Store register to publicVar's module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarIdPublic) << "Should write to publicVar's module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should write to exports";
}

TEST_F(CompilerModuleAssignmentTest, ExportVariableReassignment) {
    InitializeModuleVariable("exportCounter", true);

    ErrorId result = ParseStatement("exportCounter = 500", false);
    ASSERT_EQ(result, 0) << "Assignment should succeed";

    bool isExport;
    ModuleVariableId moduleVarId = GetModuleVariableId("exportCounter", &isExport);
    ASSERT_NE(moduleVarId, INVALID_MODULE_VARIABLE_ID) << "Variable 'exportCounter' should exist as module variable";
    ASSERT_TRUE(isExport) << "Variable 'exportCounter' should be in exports";

    ASSERT_EQ(GetCodeSize(), 2) << "Should generate exactly 2 instructions";

    // First instruction: Load value into register
    Instruction instr1 = GetInstruction(0);
    ASSERT_EQ(get_opcode(instr1), OP_LOAD_INLINE_INTEGER) << "First instruction should be LOAD_INLINE_INTEGER";
    ASSERT_EQ(OPERAND_K_K(instr1), 500) << "Should load 500";

    // Second instruction: Store register to module variable
    Instruction instr2 = GetInstruction(1);
    ASSERT_EQ(get_opcode(instr2), OP_SET_MODULE_VAR) << "Second instruction should be SET_MODULE_VAR";
    ASSERT_EQ(OPERAND_K_K(instr2), moduleVarId) << "Should use same module variable ID";
    ASSERT_TRUE(OPERAND_K_S(instr2)) << "Should target exports";
}

TEST_F(CompilerModuleAssignmentTest, ExportAndGlobalVariablesUniqueIds) {
    ASSERT_EQ(ParseStatement("globalVar := 1", false), 0) << "First binding should succeed";

    ErrorId result = ParseStatement("export exportVar := 2", false);
    ASSERT_EQ(result, 0) << "Second binding should succeed";

    bool isExportGlobal, isExportExport;
    ModuleVariableId globalVarId = GetModuleVariableId("globalVar", &isExportGlobal);
    ModuleVariableId exportVarId = GetModuleVariableId("exportVar", &isExportExport);

    ASSERT_NE(globalVarId, INVALID_MODULE_VARIABLE_ID) << "Global variable should have valid module variable ID";
    ASSERT_NE(exportVarId, INVALID_MODULE_VARIABLE_ID) << "Export variable should have valid module variable ID";

    ASSERT_FALSE(isExportGlobal) << "Global variable should be in globals";
    ASSERT_TRUE(isExportExport) << "Export variable should be in exports";
}

TEST_F(CompilerModuleAssignmentTest, ExportVariableRedefinitionError) {
    ASSERT_EQ(ParseStatement("export version := \"1.0\"", false), 0) << "First binding should succeed";

    ErrorId result = ParseStatement("export version := \"2.0\"", false);
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Export variable redefinition should fail";
}

TEST_F(CompilerModuleAssignmentTest, GlobalAndExportNameCollision) {
    ASSERT_EQ(ParseStatement("config := \"local\"", false), 0) << "First binding should succeed";

    ErrorId result = ParseStatement("export config := \"public\"", false);
    ASSERT_EQ(result, SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Name collision between global and export should fail";
}