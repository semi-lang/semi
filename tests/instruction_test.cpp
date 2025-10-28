// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/instruction.h"
}

class InstructionTest : public ::testing::Test {
   protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// GENERAL INSTRUCTION TESTS
// ============================================================================

TEST_F(InstructionTest, OpcodeExtraction) {
    struct {
        Instruction input;
        uint32_t expected_opcode;
        const char* description;
    } test_cases[] = {
        {       0x12345678, 0x12345678 & OPCODE_MASK,  "Random instruction opcode extraction"},
        {0xFFFFFFC0 | 0x22,                     0x22, "Full instruction with specific opcode"},
        {             0x3F,                     0x3F,                  "Maximum opcode value"},
        {             0x00,                     0x00,                  "Minimum opcode value"}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_EQ(get_opcode(test_case.input), test_case.expected_opcode) << test_case.description;
    }
}

TEST_F(InstructionTest, InstructionTypeBoundaryAndMinimumValues) {
    // T-type boundary and minimum values
    struct {
        uint32_t opcode, A, B, C;
        bool kb, kc;
        const char* description;
    } t_test_cases[] = {
        {OP_ADD, 0xFF, 0xFF, 0xFF,  true,  true, "T-type boundary values"},
        {OP_ADD, 0x00, 0x00, 0x00, false, false,  "T-type minimum values"}
    };

    for (const auto& test_case : t_test_cases) {
        Instruction instr = INSTRUCTION_ADD(test_case.A, test_case.B, test_case.C, test_case.kb, test_case.kc);
        ASSERT_EQ(get_opcode(instr), test_case.opcode) << test_case.description;
        ASSERT_EQ(OPERAND_T_A(instr), test_case.A) << test_case.description;
        ASSERT_EQ(OPERAND_T_B(instr), test_case.B) << test_case.description;
        ASSERT_EQ(OPERAND_T_C(instr), test_case.C) << test_case.description;
        ASSERT_EQ(OPERAND_T_KB(instr), test_case.kb) << test_case.description;
        ASSERT_EQ(OPERAND_T_KC(instr), test_case.kc) << test_case.description;
    }

    // K-type boundary and minimum values
    struct {
        uint32_t opcode, A, K;
        bool i, s;
        const char* description;
    } k_test_cases[] = {
        {OP_TRAP, 0xFF, 0xFFFF,  true,  true, "K-type boundary values"},
        {OP_TRAP, 0x00, 0x0000, false, false,  "K-type minimum values"}
    };

    for (const auto& test_case : k_test_cases) {
        Instruction instr = INSTRUCTION_TRAP(test_case.A, test_case.K, test_case.i, test_case.s);
        ASSERT_EQ(get_opcode(instr), test_case.opcode) << test_case.description;
        ASSERT_EQ(OPERAND_K_A(instr), test_case.A) << test_case.description;
        ASSERT_EQ(OPERAND_K_K(instr), test_case.K) << test_case.description;
        ASSERT_EQ(OPERAND_K_I(instr), test_case.i) << test_case.description;
        ASSERT_EQ(OPERAND_K_S(instr), test_case.s) << test_case.description;
    }

    // J-type boundary and minimum values
    struct {
        uint32_t opcode, J;
        bool s;
        const char* description;
    } j_test_cases[] = {
        {OP_JUMP, 0xFFFFFF,  true, "J-type boundary values"},
        {OP_JUMP, 0x000000, false,  "J-type minimum values"}
    };

    for (const auto& test_case : j_test_cases) {
        Instruction instr = INSTRUCTION_JUMP(test_case.J, test_case.s);
        ASSERT_EQ(get_opcode(instr), test_case.opcode) << test_case.description;
        ASSERT_EQ(OPERAND_J_J(instr), test_case.J) << test_case.description;
        ASSERT_EQ(OPERAND_J_S(instr), test_case.s) << test_case.description;
    }
}

// ============================================================================
// T-TYPE INSTRUCTION TESTS
// ============================================================================

TEST_F(InstructionTest, TTypeInstructionCreation) {
    struct {
        uint32_t opcode, A, B, C;
        bool kb, kc;
        const char* description;
    } test_cases[] = {
        {OP_ADD, 0x12, 0x34, 0x56,  true,  true,        "Basic T-type creation"},
        {OP_ADD, 0x78, 0x9A, 0xBC, false,  true,         "T-type with kb=false"},
        {OP_ADD, 0xAA, 0xBB, 0xCC,  true, false,         "T-type with kc=false"},
        {OP_ADD, 0x11, 0x22, 0x33, false, false, "T-type with both flags false"}
    };

    for (const auto& test_case : test_cases) {
        Instruction instr = INSTRUCTION_ADD(test_case.A, test_case.B, test_case.C, test_case.kb, test_case.kc);
        ASSERT_EQ(get_opcode(instr), test_case.opcode) << test_case.description;
        ASSERT_EQ(OPERAND_T_A(instr), test_case.A) << test_case.description;
        ASSERT_EQ(OPERAND_T_B(instr), test_case.B) << test_case.description;
        ASSERT_EQ(OPERAND_T_C(instr), test_case.C) << test_case.description;
        ASSERT_EQ(OPERAND_T_KB(instr), test_case.kb) << test_case.description;
        ASSERT_EQ(OPERAND_T_KC(instr), test_case.kc) << test_case.description;
    }
}

// ============================================================================
// K-TYPE INSTRUCTION TESTS
// ============================================================================

TEST_F(InstructionTest, KTypeInstructionCreation) {
    struct {
        uint32_t opcode, A, K;
        bool i, s;
        const char* description;
    } test_cases[] = {
        {OP_TRAP, 0x87, 0x1234,  true,  true,        "Basic K-type creation"},
        {OP_TRAP, 0x44, 0x5678, false,  true,          "K-type with i=false"},
        {OP_TRAP, 0x99, 0x9ABC,  true, false,          "K-type with s=false"},
        {OP_TRAP, 0x11, 0x2233, false, false, "K-type with both flags false"}
    };

    for (const auto& test_case : test_cases) {
        Instruction instr = INSTRUCTION_TRAP(test_case.A, test_case.K, test_case.i, test_case.s);

        ASSERT_EQ(get_opcode(instr), test_case.opcode) << test_case.description;
        ASSERT_EQ(OPERAND_K_A(instr), test_case.A) << test_case.description;
        ASSERT_EQ(OPERAND_K_K(instr), test_case.K) << test_case.description;
        ASSERT_EQ(OPERAND_K_I(instr), test_case.i) << test_case.description;
        ASSERT_EQ(OPERAND_K_S(instr), test_case.s) << test_case.description;
    }
}

// ============================================================================
// J-TYPE INSTRUCTION TESTS
// ============================================================================

TEST_F(InstructionTest, JTypeInstructionCreation) {
    struct {
        uint32_t opcode, J;
        bool s;
        const char* description;
    } test_cases[] = {
        {OP_JUMP, 0x123456,  true,       "Basic J-type creation"},
        {OP_JUMP, 0x789ABC, false,         "J-type with s=false"},
        {OP_JUMP, 0x555555,  true,          "J-type with s=true"},
        {OP_JUMP, 0x000001, false, "J-type with minimal J value"}
    };

    for (const auto& test_case : test_cases) {
        Instruction instr = INSTRUCTION_JUMP(test_case.J, test_case.s);

        ASSERT_EQ(get_opcode(instr), test_case.opcode) << test_case.description;
        ASSERT_EQ(OPERAND_J_J(instr), test_case.J) << test_case.description;
        ASSERT_EQ(OPERAND_J_S(instr), test_case.s) << test_case.description;
    }
}
