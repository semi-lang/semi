// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "test_common.hpp"

class CompilerForTest : public CompilerTest {};

// Basic For Loop Tests
TEST_F(CompilerForTest, SimpleForLoopWithRange) {
    const char* source = "for i in 0..10 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Simple for loop with range should parse successfully";

    // Expected instructions:
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (range 0..10 step 1)
    // 1. OP_ITER_NEXT      T      A: 0xFF, B: 0x01, C: 0x00               (index=invalid, item=reg1, iter=reg0)
    // 2. OP_JUMP           J      J: 0x00000002, s: true                  (jump to end if no more)
    // 3. OP_JUMP           J      J: 0x00000002, s: false                 (jump back to iter_next)
    // 4. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00               (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 5) << "Should generate exactly 5 instructions for basic for loop";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should load range constant");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 1, 0, false, false),
                            "Second instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 2, true), "Third instruction should jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(3), makeJInstruction(OP_JUMP, 2, false), "Fourth instruction should jump back to loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(4),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Fifth instruction should close upvalues");
}

TEST_F(CompilerForTest, ForLoopWithExplicitStep) {
    const char* source = "for i in 0..10 step 2 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with explicit step should parse successfully";

    // Expected instructions are the same pattern as simple range, but with step stored in constant table
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (range from constant table)
    // 1. OP_ITER_NEXT      T      A: 0xFF, B: 0x01, C: 0x00               (index=invalid, item=reg1, iter=reg0)
    // 2. OP_JUMP           J      J: 0x00000002, s: true                  (jump to end if no more)
    // 3. OP_JUMP           J      J: 0x00000002, s: false                 (jump back to iter_next)
    // 4. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00               (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 5) << "Should generate exactly 5 instructions for for loop with step";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should create range iterator from constant table");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 1, 0, false, false),
                            "Second instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 2, true), "Third instruction should jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(3), makeJInstruction(OP_JUMP, 2, false), "Fourth instruction should jump back to loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(4),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Fifth instruction should close upvalues");

    ASSERT_EQ(compiler.artifactModule->constantTable.constantMap->len, 1)
        << "Constant table should contain exactly one entry for the range object";
    ASSERT_EQ(VALUE_TYPE(&compiler.artifactModule->constantTable.constantMap->keys[0].key), VALUE_TYPE_OBJECT_RANGE)
        << "Constant table should contain an object range";
    ObjectRange* rangeObj = AS_OBJECT_RANGE(&compiler.artifactModule->constantTable.constantMap->keys[0].key);
    ASSERT_EQ(AS_INT(&rangeObj->start), 0) << "Range start should be 0";
    ASSERT_EQ(AS_INT(&rangeObj->end), 10) << "Range end should be 10";
    ASSERT_EQ(AS_INT(&rangeObj->step), 2) << "Range step should be 2";
}

TEST_F(CompilerForTest, ForLoopWithIndexAndItem) {
    const char* source = "for i, item in 0..5 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with index and item should parse successfully";

    // Expected instructions:
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (range 0..5 step 1)
    // 1. OP_ITER_NEXT      T      A: 0x02, B: 0x01, C: 0x00              (index=reg2, item=reg1, iter=reg0)
    // 2. OP_JUMP           J      J: 0x00000002, s: true                 (jump to end if no more)
    // 3. OP_JUMP           J      J: 0x00000002, s: false                (jump back to iter_next)
    // 4. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00              (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 5) << "Should generate exactly 5 instructions for for loop with index and item";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should load range from constant table");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 2, 1, 0, false, false),
                            "Second instruction should be ITER_NEXT with index and item registers");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 2, true), "Third instruction should jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(3), makeJInstruction(OP_JUMP, 2, false), "Fourth instruction should jump back to loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(4),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Fifth instruction should close upvalues");
}

TEST_F(CompilerForTest, ForLoopWithVariableInRange) {
    InitializeVariable("start");
    InitializeVariable("end");
    const char* source = "for i in start..end { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with variables in range should parse successfully";

    // Expected instructions:
    // 0. OP_MOVE           T      A: 0x02, B: 0x00, C: 0x00              (move start to temp register)
    // 1. OP_MAKE_RANGE     T      A: 0x02, B: 0x01, C: 0x81              (create range from start..end with step 1)
    // 2. OP_ITER_NEXT      T      A: 0xFF, B: 0x03, C: 0x02              (index=invalid, item=reg3, iter=reg2)
    // 3. OP_JUMP           J      J: 0x00000002, s: true                 (jump to end if no more)
    // 4. OP_JUMP           J      J: 0x00000002, s: false                (jump back to iter_next)
    // 5. OP_CLOSE_UPVALUES T      A: 0x02, B: 0x00, C: 0x00              (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 6) << "Should generate exactly 6 instructions for variable range for loop";

    ASSERT_T_INSTRUCTION_EQ(GetInstruction(0),
                            makeTInstruction(OP_MOVE, 2, 0, 0, false, false),
                            "First instruction should move start variable");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_MAKE_RANGE, 2, 1, 0x81, false, true),
                            "Second instruction should create range from variables");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(2),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 3, 2, false, false),
                            "Third instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(3), makeJInstruction(OP_JUMP, 2, true), "Fourth instruction should jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(4), makeJInstruction(OP_JUMP, 2, false), "Fifth instruction should jump back to loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(5),
                            makeTInstruction(OP_CLOSE_UPVALUES, 2, 0, 0, false, false),
                            "Sixth instruction should close upvalues");
}

TEST_F(CompilerForTest, NestedForLoops) {
    const char* source = "for i in 0..3 { for j in 0..2 { } }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Nested for loops should parse successfully";

    // Expected instructions:
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (outer loop iterator)
    // 1. OP_ITER_NEXT      T      A: 0xFF, B: 0x01, C: 0x00               (outer loop iter_next)
    // 2. OP_JUMP           J      J: 0x00000007, s: true                  (jump to outer end)
    // 3. OP_LOAD_CONSTANT  K      A: 0x02, K: 0x0001, i: false, s: false  (inner loop iterator)
    // 4. OP_ITER_NEXT      T      A: 0xFF, B: 0x03, C: 0x02               (inner loop iter_next)
    // 5. OP_JUMP           J      J: 0x00000002, s: true                  (jump to inner end)
    // 6. OP_JUMP           J      J: 0x00000002, s: false                 (jump back to inner iter_next)
    // 7. OP_CLOSE_UPVALUES T      A: 0x02, B: 0x00, C: 0x00               (cleanup inner upvalues)
    // 8. OP_JUMP           J      J: 0x00000007, s: false                 (jump back to outer iter_next)
    // 9. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00               (cleanup outer upvalues)

    ASSERT_EQ(GetCodeSize(), 10) << "Should generate exactly 10 instructions for nested for loops";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should load outer loop constant");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 1, 0, false, false),
                            "Second instruction should be outer ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 7, true), "Third instruction should jump to outer end");
    ASSERT_K_INSTRUCTION_EQ(GetInstruction(3),
                            makeKInstruction(OP_LOAD_CONSTANT, 2, 1, false, false),
                            "Fourth instruction should load inner loop constant");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(4),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 3, 2, false, false),
                            "Fifth instruction should be inner ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(5), makeJInstruction(OP_JUMP, 2, true), "Sixth instruction should jump to inner end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(6), makeJInstruction(OP_JUMP, 2, false), "Seventh instruction should jump back to inner loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(7),
                            makeTInstruction(OP_CLOSE_UPVALUES, 2, 0, 0, false, false),
                            "Eighth instruction should close inner upvalues");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(8), makeJInstruction(OP_JUMP, 7, false), "Ninth instruction should jump back to outer loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(9),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Tenth instruction should close outer upvalues");
}

// Range Expression Tests
TEST_F(CompilerForTest, ConstantRangeOptimization) {
    const char* source = "for i in 1..5 step 1 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Constant range should be optimized";

    // Expected instructions:
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (range 1..5 step 1)
    // 1. OP_ITER_NEXT      T      A: 0xFF, B: 0x01, C: 0x00               (index=invalid, item=reg1, iter=reg0)
    // 2. OP_JUMP           J      J: 0x00000002, s: true                  (jump to end if no more)
    // 3. OP_JUMP           J      J: 0x00000002, s: false                 (jump back to iter_next)
    // 4. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00               (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 5) << "Should generate exactly 5 instructions for constant range optimization";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should load range constant");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 1, 0, false, false),
                            "Second instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 2, true), "Third instruction should jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(3), makeJInstruction(OP_JUMP, 2, false), "Fourth instruction should jump back to loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(4),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Fifth instruction should close upvalues");
}

TEST_F(CompilerForTest, NegativeRangeStep) {
    const char* source = "for i in 10..0 step -1 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Range with negative step should parse successfully";

    // Expected instructions:
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (range from constant table)
    // 1. OP_ITER_NEXT      T      A: 0xFF, B: 0x01, C: 0x00               (index=invalid, item=reg1, iter=reg0)
    // 2. OP_JUMP           J      J: 0x00000002, s: true                  (jump to end if no more)
    // 3. OP_JUMP           J      J: 0x00000002, s: false                 (jump back to iter_next)
    // 4. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00               (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 5) << "Should generate exactly 5 instructions for negative step range";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should load range constant");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 1, 0, false, false),
                            "Second instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 2, true), "Third instruction should jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(3), makeJInstruction(OP_JUMP, 2, false), "Fourth instruction should jump back to loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(4),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Fifth instruction should close upvalues");

    ASSERT_EQ(compiler.artifactModule->constantTable.constantMap->len, 1)
        << "Constant table should contain exactly one entry for the range object";
    ASSERT_EQ(VALUE_TYPE(&compiler.artifactModule->constantTable.constantMap->keys[0].key), VALUE_TYPE_OBJECT_RANGE)
        << "Constant table should contain an object range";
    ObjectRange* rangeObj = AS_OBJECT_RANGE(&compiler.artifactModule->constantTable.constantMap->keys[0].key);
    ASSERT_EQ(AS_INT(&rangeObj->start), 10) << "Range start should be 10";
    ASSERT_EQ(AS_INT(&rangeObj->end), 0) << "Range end should be 0";
    ASSERT_EQ(AS_INT(&rangeObj->step), -1) << "Range step should be -1";
}

TEST_F(CompilerForTest, ExpressionInRange) {
    InitializeVariable("x");
    const char* source = "for i in x-1..x+1 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Range with expressions should parse successfully";

    // Expected instructions (based on actual disassembly):
    // 0. OP_SUBTRACT       T      A: 0x01, B: 0x00, C: 0x81              (x - 1 → reg1)
    // 1. OP_ADD            T      A: 0x02, B: 0x00, C: 0x81              (x + 1 → reg2)
    // 2. OP_MAKE_RANGE     T      A: 0x01, B: 0x02, C: 0x81              (create range from reg1..reg2 with step 1)
    // 3. OP_ITER_NEXT      T      A: 0xFF, B: 0x02, C: 0x01              (index=invalid, item=reg2, iter=reg1)
    // 4. OP_JUMP           J      J: 0x00000002, s: true                 (jump to end if no more)
    // 5. OP_JUMP           J      J: 0x00000002, s: false                (jump back to iter_next)
    // 6. OP_CLOSE_UPVALUES T      A: 0x01, B: 0x00, C: 0x00              (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 7) << "Should generate exactly 7 instructions for expression range";

    ASSERT_T_INSTRUCTION_EQ(GetInstruction(0),
                            makeTInstruction(OP_SUBTRACT, 1, 0, 0x81, false, true),
                            "First instruction should compute x - 1");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ADD, 2, 0, 0x81, false, true),
                            "Second instruction should compute x + 1");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(2),
                            makeTInstruction(OP_MAKE_RANGE, 1, 2, 0x81, false, true),
                            "Third instruction should create range from expressions");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(3),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 2, 1, false, false),
                            "Fourth instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(4), makeJInstruction(OP_JUMP, 2, true), "Fifth instruction should jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(5), makeJInstruction(OP_JUMP, 2, false), "Sixth instruction should jump back to loop");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(6),
                            makeTInstruction(OP_CLOSE_UPVALUES, 1, 0, 0, false, false),
                            "Seventh instruction should close upvalues");

    VariableDescription* xVar = FindVariable("x");
    ASSERT_NE(xVar, nullptr) << "Variable 'x' should be bound";
}

// Break and Continue Tests
TEST_F(CompilerForTest, ForLoopWithBreak) {
    const char* source = "for i in 0..10 { break; }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with break should parse successfully";

    // Expected instructions:
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (range 0..10 step 1)
    // 1. OP_ITER_NEXT      T      A: 0xFF, B: 0x01, C: 0x00               (index=invalid, item=reg1, iter=reg0)
    // 2. OP_JUMP           J      J: 0x00000003, s: true                  (jump to end if no more)
    // 3. OP_JUMP           J      J: 0x00000002, s: true                  (break - jump to end)
    // 4. OP_JUMP           J      J: 0x00000003, s: false                 (jump back to iter_next)
    // 5. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00               (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 6) << "Should generate exactly 6 instructions for for loop with break";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should load range constant");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 1, 0, false, false),
                            "Second instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 3, true), "Third instruction should jump to end if no more");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(3), makeJInstruction(OP_JUMP, 2, true), "Fourth instruction should be break - jump to end");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(4), makeJInstruction(OP_JUMP, 3, false), "Fifth instruction should jump back to iter_next");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(5),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Sixth instruction should close upvalues");
}

TEST_F(CompilerForTest, ForLoopWithContinue) {
    const char* source = "for i in 0..10 { continue; }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with continue should parse successfully";

    // Expected instructions:
    // 0. OP_LOAD_CONSTANT  K      A: 0x00, K: 0x0000, i: false, s: false  (range 0..10 step 1)
    // 1. OP_ITER_NEXT      T      A: 0xFF, B: 0x01, C: 0x00               (index=invalid, item=reg1, iter=reg0)
    // 2. OP_JUMP           J      J: 0x00000003, s: true                  (jump to end if no more)
    // 3. OP_JUMP           J      J: 0x00000002, s: false                 (continue - jump back to iter_next)
    // 4. OP_JUMP           J      J: 0x00000003, s: false                 (jump back to iter_next)
    // 5. OP_CLOSE_UPVALUES T      A: 0x00, B: 0x00, C: 0x00               (cleanup upvalues)

    ASSERT_EQ(GetCodeSize(), 6) << "Should generate exactly 6 instructions for for loop with continue";

    ASSERT_K_INSTRUCTION_EQ(GetInstruction(0),
                            makeKInstruction(OP_LOAD_CONSTANT, 0, 0, false, false),
                            "First instruction should load range constant");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(1),
                            makeTInstruction(OP_ITER_NEXT, 0xFF, 1, 0, false, false),
                            "Second instruction should be ITER_NEXT");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(2), makeJInstruction(OP_JUMP, 3, true), "Third instruction should jump to end if no more");
    ASSERT_J_INSTRUCTION_EQ(GetInstruction(3),
                            makeJInstruction(OP_JUMP, 2, false),
                            "Fourth instruction should be continue - jump back to iter_next");
    ASSERT_J_INSTRUCTION_EQ(
        GetInstruction(4), makeJInstruction(OP_JUMP, 3, false), "Fifth instruction should jump back to iter_next");
    ASSERT_T_INSTRUCTION_EQ(GetInstruction(5),
                            makeTInstruction(OP_CLOSE_UPVALUES, 0, 0, 0, false, false),
                            "Sixth instruction should close upvalues");
}

TEST_F(CompilerForTest, ForLoopWithBreakAndContinue) {
    const char* source = "for i in 0..5 { if i == 2 { continue; } if i == 4 { break; } }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "For loop with both break and continue should parse successfully";

    // The exact instruction count and offsets will vary due to the conditional logic,
    // but we should see continue jumping back to iter_next and break jumping to end
    ASSERT_GT(GetCodeSize(), 6) << "Should generate more than 6 instructions for complex for loop";

    // Verify that we have OP_JUMP instructions (break and continue will generate these)
    bool foundContinueJump = false;
    bool foundBreakJump    = false;

    for (size_t i = 0; i < GetCodeSize(); i++) {
        Instruction instr = GetInstruction(i);
        if (GET_OPCODE(instr) == OP_JUMP) {
            JInstruction jumpInstr = decodeJInstruction(instr);
            // Continue jumps should be backward (s=false) and break jumps should be forward (s=true)
            if (!jumpInstr.signFlag) {
                foundContinueJump = true;
            } else {
                foundBreakJump = true;
            }
        }
    }

    EXPECT_TRUE(foundContinueJump) << "Should find at least one backward jump (for continue or loop continuation)";
    EXPECT_TRUE(foundBreakJump) << "Should find at least one forward jump (for break or conditional jumps)";
}

TEST_F(CompilerForTest, NestedForLoopsWithBreakAndContinue) {
    const char* source = "for i in 0..3 { for j in 0..2 { if j == 1 { continue; } if i == 2 { break; } } }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_EQ(result, 0) << "Nested for loops with break and continue should parse successfully";

    // Nested loops should generate multiple jump instructions
    ASSERT_GT(GetCodeSize(), 10) << "Should generate more than 10 instructions for nested loops with control flow";

    // Count the number of jump instructions
    size_t jumpCount = 0;
    for (size_t i = 0; i < GetCodeSize(); i++) {
        Instruction instr = GetInstruction(i);
        if (GET_OPCODE(instr) == OP_JUMP) {
            jumpCount++;
        }
    }

    // Should have multiple jumps: outer loop control, inner loop control, continue, break, conditionals
    EXPECT_GE(jumpCount, 6) << "Should have at least 6 jump instructions for nested loops with control flow";
}

// Error Cases
TEST_F(CompilerForTest, MissingInKeyword) {
    const char* source = "for i 0..10 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Missing 'in' keyword should cause parse error";
}

TEST_F(CompilerForTest, MissingOpeningBrace) {
    const char* source = "for i in 0..10 }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Missing opening brace should cause parse error";
}

TEST_F(CompilerForTest, MissingIterable) {
    const char* source = "for i in { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Missing iterable expression should cause parse error";
}

TEST_F(CompilerForTest, DuplicateVariableNames) {
    const char* source = "for i, i in 0..5 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Duplicate variable names should cause parse error";
}

TEST_F(CompilerForTest, TooManyVariables) {
    const char* source = "for i, j, k in 0..5 { }";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Too many variables should cause parse error";
}

// Error Cases for Break and Continue
TEST_F(CompilerForTest, BreakOutsideLoop) {
    const char* source = "break;";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Break outside of loop should cause parse error";
}

TEST_F(CompilerForTest, ContinueOutsideLoop) {
    const char* source = "continue;";

    ErrorId(result) = ParseStatement(source, false);
    EXPECT_NE(result, 0) << "Continue outside of loop should cause parse error";
}
