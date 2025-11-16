// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

#include "instruction_verifier.hpp"
#include "test_common.hpp"

using namespace InstructionVerifier;

class CompilerModuleAssignmentTest : public CompilerTest {};

TEST_F(CompilerModuleAssignmentTest, ModuleIntegerAssignment) {
    ErrorId result = ParseStatement("x := 42", false);
    ASSERT_EQ(result, 0) << "Parsing 'x := 42' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x002A i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F

[Globals]
G[0]: x
)");
}

TEST_F(CompilerModuleAssignmentTest, ModuleDoubleAssignment) {
    ErrorId result = ParseStatement("y := 3.14", false);
    ASSERT_EQ(result, 0) << "Parsing 'y := 3.14' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Float 3.14

[Globals]
G[0]: y
)");
}

TEST_F(CompilerModuleAssignmentTest, ModuleBooleanAssignment) {
    ErrorId result = ParseStatement("flag := true", false);
    ASSERT_EQ(result, 0) << "Parsing 'flag := true' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F

[Globals]
G[0]: flag
)");
}

TEST_F(CompilerModuleAssignmentTest, ModuleStringAssignment) {
    ErrorId result = ParseStatement("name := \"hello\"", false);
    ASSERT_EQ(result, 0) << "Parsing 'name := \"hello\"' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: String "hello" length=5

[Globals]
G[0]: name
)");
}

TEST_F(CompilerModuleAssignmentTest, ModuleExpressionAssignment) {
    ErrorId result = ParseStatement("result := 10 + 5", false);
    ASSERT_EQ(result, 0) << "Parsing 'result := 10 + 5' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x000F i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F

[Globals]
G[0]: result
)");
}

TEST_F(CompilerModuleAssignmentTest, ModuleVariableToModuleVariableAssignment) {
    InitializeModuleVariable("x");

    ErrorId result = ParseStatement("y := x", false);
    ASSERT_EQ(result, 0) << "Parsing 'y := x' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_GET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0001 i=F s=F

[Globals]
G[0]: x
G[1]: y
)");
}

TEST_F(CompilerModuleAssignmentTest, ModuleVariableReassignment) {
    InitializeModuleVariable("counter");

    ErrorId result = ParseStatement("counter = 100", false);
    ASSERT_EQ(result, 0) << "Assignment should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0064 i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F

[Globals]
G[0]: counter
)");
}

TEST_F(CompilerModuleAssignmentTest, MultipleModuleVariablesUniqueIds) {
    ASSERT_EQ(ParseStatement("a := 1", false), 0) << "Binding a should succeed";
    ASSERT_EQ(ParseStatement("b := 2", false), 0) << "Binding b should succeed";
    ASSERT_EQ(ParseStatement("c := 3", false), 0) << "Binding c should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
3: OP_SET_MODULE_VAR        A=0x00 K=0x0001 i=F s=F
4: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0003 i=T s=T
5: OP_SET_MODULE_VAR        A=0x00 K=0x0002 i=F s=F

[Globals]
G[0]: a
G[1]: b
G[2]: c
)");
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

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x002A i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Exports]
E[0]: x
)");
}

TEST_F(CompilerModuleAssignmentTest, ExportDoubleAssignment) {
    ErrorId result = ParseStatement("export pi := 3.14159", false);
    ASSERT_EQ(result, 0) << "Parsing 'export pi := 3.14159' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Constants]
K[0]: Float 3.14159

[Exports]
E[0]: pi
)");
}

TEST_F(CompilerModuleAssignmentTest, ExportBooleanAssignment) {
    ErrorId result = ParseStatement("export debugMode := true", false);
    ASSERT_EQ(result, 0) << "Parsing 'export debugMode := true' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_BOOL             A=0x00 K=0x0000 i=T s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Exports]
E[0]: debugMode
)");
}

TEST_F(CompilerModuleAssignmentTest, ExportStringAssignment) {
    ErrorId result = ParseStatement("export appName := \"MyApp\"", false);
    ASSERT_EQ(result, 0) << "Parsing 'export appName := \"MyApp\"' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_CONSTANT         A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Constants]
K[0]: String "MyApp" length=5

[Exports]
E[0]: appName
)");
}

TEST_F(CompilerModuleAssignmentTest, ExportExpressionAssignment) {
    ErrorId result = ParseStatement("export maxValue := 100 * 2 + 56", false);
    ASSERT_EQ(result, 0) << "Parsing 'export maxValue := 100 * 2 + 56' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0100 i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Exports]
E[0]: maxValue
)");
}

TEST_F(CompilerModuleAssignmentTest, ExportVariableToExportVariableAssignment) {
    InitializeModuleVariable("baseValue", true);

    ErrorId result = ParseStatement("export derivedValue := baseValue", false);
    ASSERT_EQ(result, 0) << "Parsing 'export derivedValue := baseValue' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_GET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0001 i=F s=T

[Exports]
E[0]: baseValue
E[1]: derivedValue
)");
}

TEST_F(CompilerModuleAssignmentTest, GlobalVariableToExportVariableAssignment) {
    InitializeModuleVariable("globalVar", false);

    ErrorId result = ParseStatement("export publicVar := globalVar", false);
    ASSERT_EQ(result, 0) << "Parsing 'export publicVar := globalVar' should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_GET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Globals]
G[0]: globalVar

[Exports]
E[0]: publicVar
)");
}

TEST_F(CompilerModuleAssignmentTest, ExportVariableReassignment) {
    InitializeModuleVariable("exportCounter", true);

    ErrorId result = ParseStatement("exportCounter = 500", false);
    ASSERT_EQ(result, 0) << "Assignment should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x01F4 i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Exports]
E[0]: exportCounter
)");
}

TEST_F(CompilerModuleAssignmentTest, ExportAndGlobalVariablesUniqueIds) {
    ASSERT_EQ(ParseStatement("globalVar := 1", false), 0) << "First binding should succeed";

    ErrorId result = ParseStatement("export exportVar := 2", false);
    ASSERT_EQ(result, 0) << "Second binding should succeed";

    VerifyCompilation(&compiler, R"(
[Instructions]
0: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0001 i=T s=T
1: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=F
2: OP_LOAD_INLINE_INTEGER   A=0x00 K=0x0002 i=T s=T
3: OP_SET_MODULE_VAR        A=0x00 K=0x0000 i=F s=T

[Globals]
G[0]: globalVar

[Exports]
E[0]: exportVar
)");
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