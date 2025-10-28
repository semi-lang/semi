// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "test_common.hpp"

class CompilerIfTest : public CompilerTest {};

// Basic If Statement Variations
TEST_F(CompilerIfTest, SimpleIfStatement) {
    const char* source = "if true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Simple if statement should parse successfully";

    // Verify CLOSE_UPVALUES instruction is emitted at the end
    ASSERT_EQ(GetCodeSize(), 3) << "Should generate 3 instruction";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "First instruction should load 'true' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 1, false, true),
                            "Second instruction should be conditional jump based on register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(2),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Third instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, IfElseStatement) {
    const char* source = "if true { } else { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "If-else statement should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 4) << "Should generate 4 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "First instruction should load 'true' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 1, true), "Third instruction should be unconditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Fourth instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, IfElifStatement) {
    const char* source = "if false { } elif true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "If-elif statement should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 6) << "Should generate 6 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, false, false),
                            "First instruction should load 'false' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 3, true), "Third instruction should be unconditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "Fourth instruction should load 'true' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_C_JUMP, 0, 1, false, true),
                            "Fifth instruction should be conditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(5),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Sixth instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, IfElifElseStatement) {
    const char* source = "if false { } elif false { } else { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "If-elif-else statement should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 7) << "Should generate 7 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, false, false),
                            "First instruction should load 'false' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 4, true), "Third instruction should be unconditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, false, false),
                            "Fourth instruction should load 'false' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Fifth instruction should be conditional jump");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(5), makeJInstruction(OP_JUMP, 1, true), "Sixth instruction should be unconditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(6),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Seventh instruction should be CLOSE_UPVALUES");
}

// Condition Expression Types
TEST_F(CompilerIfTest, ConstantTrueCondition) {
    const char* source = "if true { x := 5 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Constant true condition should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 4) << "Should generate 4 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "First instruction should load 'true' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(2),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 5, true, true),
                            "Third instruction should load integer 5 into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Fourth instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, ConstantFalseCondition) {
    const char* source = "if false { } else { y := 10 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Constant false condition should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 5) << "Should generate 5 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, false, false),
                            "First instruction should load 'false' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 2, true), "Third instruction should be unconditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 10, true, true),
                            "Fourth instruction should load integer 10 into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Fifth instruction should be CLOSE_UPVALUES");
}
TEST_F(CompilerIfTest, VariableCondition) {
    InitializeVariable("x");
    const char* source = "if x { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable condition should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 2) << "Should generate 2 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_C_JUMP, 0, 1, false, true),
                            "First instruction should be conditional jump using variable x");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_CLOSE_UPVALUES, 1, 0, false, false),
                            "Second instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, ComplexExpressionCondition) {
    InitializeVariable("x");
    const char* source = "if x > 5 { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Complex expression condition should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 3) << "Should generate 3 instructions";
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(0),
                            makeTInstruction(OP_GT, 1, 0, 0x85, false, true),
                            "First instruction should perform GT comparison between x and constant 5");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 1, 1, false, true),
                            "Second instruction should be conditional jump using comparison result");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(2),
                            makeKInstruction(OP_CLOSE_UPVALUES, 1, 0, false, false),
                            "Third instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, VariableBindingInElifBlock) {
    const char* source = "if false { } elif true { z := 15 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable binding in elif block should succeed";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 7) << "Should generate 7 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, false, false),
                            "First instruction should load 'false' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 4, true), "Third instruction should be unconditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "Fourth instruction should load 'true' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Fifth instruction should be conditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(5),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 15, true, true),
                            "Sixth instruction should load integer 15 into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(6),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Seventh instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, VariableShadowingPrevention) {
    const char* source = "{ x := 1\nif true { x := 2 } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_VARIABLE_ALREADY_DEFINED) << "Variable shadowing should be prevented";
}
TEST_F(CompilerIfTest, SiblingScopeIsolation) {
    InitializeVariable("someCondition");
    const char* source = "if someCondition { x := 5 } else { x := 10 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Sibling scope isolation should work";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 5) << "Should generate 5 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_C_JUMP, 0, 3, false, true),
                            "First instruction should be conditional jump using someCondition");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 1, 5, true, true),
                            "Second instruction should load integer 5 for x in if block");
    ASSERT_J_INSTRUCTION_EQ(GetInstruction(2),
                            makeJInstruction(OP_JUMP, 2, true),
                            "Third instruction should be unconditional jump to skip else block");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 1, 10, true, true),
                            "Fourth instruction should load integer 10 for x in else block");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_CLOSE_UPVALUES, 1, 0, false, false),
                            "Fifth instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, VariableAccessFromParentScope) {
    const char* source = "{ x := 5\nif true { y := x } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable access from parent scope should work";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 5) << "Should generate 5 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 5, true, true),
                            "First instruction should load integer 5 into register 0 for x");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_LOAD_BOOL, 1, 0, true, false),
                            "Second instruction should load 'true' into register 1");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(2),
                            makeKInstruction(OP_C_JUMP, 1, 2, false, true),
                            "Third instruction should be conditional jump");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(3),
                            makeTInstruction(OP_MOVE, 1, 0, 0, false, false),
                            "Fourth instruction should move x from register 0 to register 1 for y");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_CLOSE_UPVALUES, 1, 0, false, false),
                            "Fifth instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, VariableAssignmentInBlocks) {
    const char* source = "{ x := 5\nif true { x = 10 } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable assignment in blocks should work";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 5) << "Should generate 5 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 5, true, true),
                            "First instruction should load integer 5 into register 0 for x");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_LOAD_BOOL, 1, 0, true, false),
                            "Second instruction should load 'true' into register 1");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(2),
                            makeKInstruction(OP_C_JUMP, 1, 2, false, true),
                            "Third instruction should be conditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 10, true, true),
                            "Fourth instruction should load integer 10 into register 0 (assignment to x)");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_CLOSE_UPVALUES, 1, 0, false, false),
                            "Fifth instruction should be CLOSE_UPVALUES");
}

TEST_F(CompilerIfTest, VariableOutOfScopeAssignment) {
    const char* source = "{ if true { x := 5 }\nx = 5 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_BINDING_ERROR) << "Variable out of scope assignment should fail";
}

TEST_F(CompilerIfTest, UnbindVariableAfterScope) {
    const char* source = "{ if true { x := 2 }\nx := 3 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable binding after scope should work (variables in different scopes)";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 5) << "Should generate 5 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "First instruction should load 'true' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(2),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 2, true, true),
                            "Third instruction should load integer 2 into register 0 for x in if block");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Fourth instruction should be CLOSE_UPVALUES (end of if block)");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 3, true, true),
                            "Fifth instruction should load integer 3 into register 0 for x in outer scope");
}

TEST_F(CompilerIfTest, UnbindVariableAfterElseScope) {
    const char* source = "{ if false { } else { y := 10 }\ny := 20 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Variable binding after else scope should work (variables in different scopes)";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 6) << "Should generate 6 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, false, false),
                            "First instruction should load 'false' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Second instruction should be conditional jump");
    ASSERT_J_INSTRUCTION_EQ(GetInstruction(2),
                            makeJInstruction(OP_JUMP, 2, true),
                            "Third instruction should be unconditional jump to skip else block");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 10, true, true),
                            "Fourth instruction should load integer 10 into register 0 for y in else block");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Fifth instruction should be CLOSE_UPVALUES (end of if-else block)");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(5),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 20, true, true),
                            "Sixth instruction should load integer 20 into register 0 for y in outer scope");
}

// Negative Test Cases
TEST_F(CompilerIfTest, MissingOpeningBrace) {
    const char* source = "if true x := 5";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_UNEXPECTED_TOKEN) << "Missing opening brace should fail";
}

TEST_F(CompilerIfTest, MissingCondition) {
    const char* source = "if { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_UNEXPECTED_TOKEN) << "Missing condition should fail";
}

TEST_F(CompilerIfTest, UnclosedBraces) {
    const char* source = "if true { x := 5";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_UNEXPECTED_TOKEN) << "Unclosed braces should fail";
}

TEST_F(CompilerIfTest, VariableAlreadyDefinedInSameScope) {
    const char* source = "if true { x := 5\nx := 10 }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Compilation should fail";
    EXPECT_EQ(GetCompilerError(), SEMI_ERROR_VARIABLE_ALREADY_DEFINED)
        << "Variable already defined in same scope should fail";
}

// Instruction Generation Verification
TEST_F(CompilerIfTest, JumpInstructionVerification) {
    const char* source = "if false { } else { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Should parse successfully";

    // Check for C_JUMP instruction (conditional jump)
    bool foundCJump = false;
    for (size_t i = 0; i < GetCodeSize(); i++) {
        Instruction inst = GetInstruction(i);
        if (get_opcode(inst) == OP_C_JUMP) {
            foundCJump = true;
            break;
        }
    }
    EXPECT_TRUE(foundCJump) << "Should generate conditional jump instruction";
}

TEST_F(CompilerIfTest, CloseUpvaluesInstruction) {
    const char* source = "if true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Should parse successfully";

    // Verify CLOSE_UPVALUES instruction is emitted at the end
    EXPECT_GT(GetCodeSize(), 0) << "Should generate at least one instruction";

    // The last instruction should be CLOSE_UPVALUES
    Instruction lastInst = GetInstruction(GetCodeSize() - 1);
    EXPECT_EQ(get_opcode(lastInst), OP_CLOSE_UPVALUES) << "Last instruction should be CLOSE_UPVALUES";
}

// Complex Nested Cases
TEST_F(CompilerIfTest, NestedIfStatements) {
    const char* source = "if true { if true { x := 5 } }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Nested if statements should parse successfully";

    // Verify instruction generation
    ASSERT_EQ(GetCodeSize(), 7) << "Should generate 7 instructions";
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "First instruction should load 'true' into register 0");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(1),
                            makeKInstruction(OP_C_JUMP, 0, 5, false, true),
                            "Second instruction should be conditional jump (outer if)");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(2),
                            makeKInstruction(OP_LOAD_BOOL, 0, 0, true, false),
                            "Third instruction should load 'true' into register 1");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                            "Fourth instruction should be conditional jump (inner if)");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(4),
                            makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 5, true, true),
                            "Fifth instruction should load integer 5 into register 0 for x");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(5),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Sixth instruction should be CLOSE_UPVALUES (inner block)");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(6),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            "Seventh instruction should be CLOSE_UPVALUES (outer block)");
}

// Edge Cases
TEST_F(CompilerIfTest, EmptyBlocks) {
    const char* source = "if true { }";

    ErrorId result = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Empty blocks should work";

    // This is the same as SimpleIfStatement test case - already verified above
}

TEST_F(CompilerIfTest, LongElifChainsWithinLimits) {
    // Create a source with 10 elif statements (within limits)
    std::string source = "if false { }";
    for (int i = 0; i < 10; i++) {
        source += " elif false { }";
    }
    source += " else { x := 1 }";

    ErrorId result = ParseStatement(source.c_str(), false);
    EXPECT_EQ(result, 0) << "Long elif chains within limits should succeed";

    // Verify instruction generation - should generate many instructions for the chain
    ASSERT_EQ(GetCodeSize(), 35) << "Should generate 35 instructions for long elif chain";

    // Verify the pattern: each if/elif generates 3 instructions (LOAD_BOOL, C_JUMP, JUMP)
    // Total: 11 conditions (1 if + 10 elif) * 3 instructions = 33 instructions
    // Plus: 1 LOAD_INLINE_INTEGER for else block + 1 CLOSE_UPVALUES = 35 total

    size_t instructionIndex = 0;
    // Verify pattern for each if/elif condition (11 total: 1 if + 10 elif)
    for (int condition = 0; condition < 11; condition++) {
        // Each condition block generates 3 instructions

        // 1. LOAD_BOOL instruction
        ASSERT_K_INSTRUCTION_EQ(GetInstruction(instructionIndex),
                                makeKInstruction(OP_LOAD_BOOL, 0, 0, false, false),
                                ("Instruction " + std::to_string(instructionIndex) +
                                 " should load 'false' for condition " + std::to_string(condition))
                                    .c_str());
        instructionIndex++;

        // 2. C_JUMP instruction
        ASSERT_K_INSTRUCTION_EQ(GetInstruction(instructionIndex),
                                makeKInstruction(OP_C_JUMP, 0, 2, false, true),
                                ("Instruction " + std::to_string(instructionIndex) +
                                 " should be conditional jump for condition " + std::to_string(condition))
                                    .c_str());
        instructionIndex++;

        // 3. JUMP instruction (skip distance decreases by 3 each time)
        // Last condition (condition 10) has jump offset 2, second-to-last has 5, etc.
        uint32_t expectedJumpOffset = (11 - condition - 1) * 3 + 2;
        ASSERT_J_INSTRUCTION_EQ(GetInstruction(instructionIndex),
                                makeJInstruction(OP_JUMP, expectedJumpOffset, true),
                                ("Instruction " + std::to_string(instructionIndex) +
                                 " should be unconditional jump with offset " + std::to_string(expectedJumpOffset))
                                    .c_str());
        instructionIndex++;
    }

    // Verify else block instruction (LOAD_INLINE_INTEGER)
    ASSERT_K_INSTRUCTION_EQ(
        GetInstruction(instructionIndex),
        makeKInstruction(OP_LOAD_INLINE_INTEGER, 0, 1, true, true),
        ("Instruction " + std::to_string(instructionIndex) + " should load integer 1 for else block").c_str());
    instructionIndex++;

    // Verify final CLOSE_UPVALUES instruction
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(instructionIndex),
                            makeKInstruction(OP_CLOSE_UPVALUES, 0, 0, false, false),
                            ("Instruction " + std::to_string(instructionIndex) + " should be CLOSE_UPVALUES").c_str());
}
