// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cmath>

extern "C" {
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionArithmeticTest : public VMTest {};

TEST_F(VMInstructionArithmeticTest, OpAdd) {
    struct TestCase {
        const char* name;
        const char* lhs_spec;
        const char* rhs_spec;
        bool expect_float_result;
        int expected_int;
        float expected_float;
    } test_cases[] = {
        {    "int_plus_int",     "Int 5",     "Int 3", false,  8, 0.0f},
        {  "int_plus_float",     "Int 5", "Float 3.5",  true,  0, 8.5f},
        {  "float_plus_int", "Float 5.5",     "Int 3",  true,  0, 8.5f},
        {"float_plus_float", "Float 5.5", "Float 3.5",  true,  0, 9.0f},
        {"negative_numbers",    "Int -5",    "Int -3", false, -8, 0.0f},
        {   "zero_addition",     "Int 0",     "Int 0", false,  0, 0.0f}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_ADD  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs_spec,
                 tc.rhs_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;

        if (tc.expect_float_result) {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << tc.name;
            ASSERT_FLOAT_EQ(vm->values[0].as.f, tc.expected_float) << "Test case: " << tc.name;
        } else {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << tc.name;
            ASSERT_EQ(vm->values[0].as.i, tc.expected_int) << "Test case: " << tc.name;
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpSubtract) {
    struct TestCase {
        const char* name;
        const char* lhs_spec;
        const char* rhs_spec;
        bool expect_float_result;
        int expected_int;
        float expected_float;
    } test_cases[] = {
        {    "int_minus_int",     "Int 10",     "Int 3", false,  7, 0.0f},
        {  "int_minus_float",     "Int 10", "Float 3.5",  true,  0, 6.5f},
        {  "float_minus_int", "Float 10.5",     "Int 3",  true,  0, 7.5f},
        {"float_minus_float", "Float 10.5", "Float 3.5",  true,  0, 7.0f},
        {  "negative_result",      "Int 3",    "Int 10", false, -7, 0.0f},
        { "zero_subtraction",      "Int 5",     "Int 0", false,  5, 0.0f}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_SUBTRACT A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP     A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs_spec,
                 tc.rhs_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;

        if (tc.expect_float_result) {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << tc.name;
            ASSERT_FLOAT_EQ(vm->values[0].as.f, tc.expected_float) << "Test case: " << tc.name;
        } else {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << tc.name;
            ASSERT_EQ(vm->values[0].as.i, tc.expected_int) << "Test case: " << tc.name;
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpMultiply) {
    struct TestCase {
        const char* name;
        const char* lhs_spec;
        const char* rhs_spec;
        bool expect_float_result;
        int expected_int;
        float expected_float;
    } test_cases[] = {
        {    "int_times_int",     "Int 5",     "Int 3", false,  15,  0.0f},
        {  "int_times_float",     "Int 5", "Float 2.5",  true,   0, 12.5f},
        {  "float_times_int", "Float 2.5",     "Int 4",  true,   0, 10.0f},
        {"float_times_float", "Float 2.5", "Float 2.0",  true,   0,  5.0f},
        { "multiply_by_zero",     "Int 5",     "Int 0", false,   0,  0.0f},
        {"negative_multiply",    "Int -3",     "Int 4", false, -12,  0.0f}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_MULTIPLY A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP     A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs_spec,
                 tc.rhs_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;

        if (tc.expect_float_result) {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << tc.name;
            ASSERT_FLOAT_EQ(vm->values[0].as.f, tc.expected_float) << "Test case: " << tc.name;
        } else {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << tc.name;
            ASSERT_EQ(vm->values[0].as.i, tc.expected_int) << "Test case: " << tc.name;
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpDivide) {
    struct TestCase {
        const char* name;
        const char* lhs_spec;
        const char* rhs_spec;
        bool expect_float_result;
        int expected_int;
        float expected_float;
    } test_cases[] = {
        {          "int_divide_int",     "Int 15",     "Int 3", false, 5, 0.0f},
        {"int_divide_int_truncated",      "Int 7",     "Int 2", false, 3, 0.0f},
        {        "int_divide_float",     "Int 10", "Float 2.0",  true, 0, 5.0f},
        {        "float_divide_int", "Float 15.0",     "Int 3",  true, 0, 5.0f},
        {      "float_divide_float", "Float 15.0", "Float 3.0",  true, 0, 5.0f}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_DIVIDE A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP   A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs_spec,
                 tc.rhs_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;

        if (tc.expect_float_result) {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << tc.name;
            ASSERT_FLOAT_EQ(vm->values[0].as.f, tc.expected_float) << "Test case: " << tc.name;
        } else {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << tc.name;
            ASSERT_EQ(vm->values[0].as.i, tc.expected_int) << "Test case: " << tc.name;
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpDivideByZero) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 10
R[2]: Int 0

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_DIVIDE A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP   A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    ASSERT_EQ(result, SEMI_ERROR_DIVIDE_BY_ZERO);
    ASSERT_EQ(vm->error, SEMI_ERROR_DIVIDE_BY_ZERO);
}

TEST_F(VMInstructionArithmeticTest, OpFloorDivide) {
    struct TestCase {
        const char* name;
        const char* lhs_spec;
        const char* rhs_spec;
        int expected_int;
    } test_cases[] = {
        {     "int_floor_divide_int",     "Int 15",  "Int 4",  3},
        {"int_floor_divide_negative",    "Int -15",  "Int 4", -3},
        {     "negative_by_negative",    "Int -15", "Int -4",  3},
        {       "float_floor_divide", "Float 15.7",  "Int 4",  3}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_FLOOR_DIVIDE A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP         A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs_spec,
                 tc.rhs_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;
        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Floor divide should return int for " << tc.name;
        ASSERT_EQ(vm->values[0].as.i, tc.expected_int) << "Test case: " << tc.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpModulo) {
    struct TestCase {
        const char* name;
        int lhs;
        int rhs;
        int expected;
    } test_cases[] = {
        {  "positive_modulo",  15,  4,  3},
        {"negative_dividend", -15,  4, -3},
        { "negative_divisor",  15, -4,  3},
        {    "both_negative", -15, -4, -3},
        {    "zero_dividend",   0,  4,  0}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: Int %d
R[2]: Int %d

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_MODULO A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP   A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs,
                 tc.rhs);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;
        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Modulo should return int for " << tc.name;
        ASSERT_EQ(vm->values[0].as.i, tc.expected) << "Test case: " << tc.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpPower) {
    struct TestCase {
        const char* name;
        const char* lhs_spec;
        const char* rhs_spec;
        bool expect_float_result;
        int expected_int;
        float expected_float;
    } test_cases[] = {
        {    "int_power_int",     "Int 2",     "Int 3", false, 8,       0.0f},
        {  "int_power_float",     "Int 2", "Float 3.5",  true, 0, 11.313708f},
        {  "float_power_int", "Float 2.5",     "Int 2",  true, 0,      6.25f},
        {"float_power_float", "Float 2.0", "Float 0.5",  true, 0,  1.414214f},
        {    "power_of_zero",     "Int 5",     "Int 0", false, 1,       0.0f},
        {    "zero_to_power",     "Int 0",     "Int 3", false, 0,       0.0f}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_POWER A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP  A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs_spec,
                 tc.rhs_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;

        if (tc.expect_float_result) {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Power should return float for " << tc.name;
            ASSERT_NEAR(vm->values[0].as.f, tc.expected_float, 0.001f) << "Test case: " << tc.name;
        } else {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Power should return int for " << tc.name;
            ASSERT_EQ(vm->values[0].as.i, tc.expected_int) << "Test case: " << tc.name;
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpNegate) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 42

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_NEGATE A=0x00 B=0x01 C=0x00 kb=F kc=F
1: OP_TRAP   A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    ASSERT_EQ(result, 0);
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT);
    ASSERT_EQ(vm->values[0].as.i, -42);
}

TEST_F(VMInstructionArithmeticTest, OpNegateFloat) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Float 3.14

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_NEGATE A=0x00 B=0x01 C=0x00 kb=F kc=F
1: OP_TRAP   A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    ASSERT_EQ(result, 0);
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT);
    ASSERT_FLOAT_EQ(vm->values[0].as.f, -3.14f);
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseAnd) {
    struct TestCase {
        const char* name;
        int lhs;
        int rhs;
        int expected;
    } test_cases[] = {
        {    "and_basic", 0b1010, 0b1100, 0b1000},
        { "and_all_ones",   0xFF,   0xFF,   0xFF},
        {"and_with_zero",   0xFF,   0x00,   0x00},
        { "and_negative",     -1, 0b1010, 0b1010}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: Int %d
R[2]: Int %d

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_BITWISE_AND A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP        A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs,
                 tc.rhs);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;
        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << tc.name;
        ASSERT_EQ(vm->values[0].as.i, tc.expected) << "Test case: " << tc.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseOr) {
    struct TestCase {
        const char* name;
        int lhs;
        int rhs;
        int expected;
    } test_cases[] = {
        {    "or_basic", 0b1010, 0b1100, 0b1110},
        {"or_with_zero", 0b1010,   0x00, 0b1010},
        { "or_all_ones",   0xFF,   0x00,   0xFF},
        { "or_negative",     -1, 0b1010,     -1}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: Int %d
R[2]: Int %d

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_BITWISE_OR A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP       A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs,
                 tc.rhs);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;
        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << tc.name;
        ASSERT_EQ(vm->values[0].as.i, tc.expected) << "Test case: " << tc.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseXor) {
    struct TestCase {
        const char* name;
        int lhs;
        int rhs;
        int expected;
    } test_cases[] = {
        {    "xor_basic", 0b1010, 0b1100, 0b0110},
        {     "xor_same", 0b1010, 0b1010,   0x00},
        {"xor_with_zero", 0b1010,   0x00, 0b1010},
        { "xor_all_ones",   0xFF,   0xFF,   0x00}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: Int %d
R[2]: Int %d

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_BITWISE_XOR A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP        A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs,
                 tc.rhs);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;
        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << tc.name;
        ASSERT_EQ(vm->values[0].as.i, tc.expected) << "Test case: " << tc.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseShift) {
    struct TestCase {
        const char* name;
        int lhs;
        int rhs;
        int expected;
    } test_cases[] = {
        { "shift_left", 0b1010, 2, 0b101000},
        { "shift_zero", 0b1010, 0,   0b1010},
        {"shift_large",      1, 8,      256}
    };

    for (const auto& tc : test_cases) {
        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: Int %d
R[2]: Int %d

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_BITWISE_L_SHIFT A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP            A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 tc.lhs,
                 tc.rhs);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        ASSERT_EQ(result, 0) << "Test case: " << tc.name;
        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << tc.name;
        ASSERT_EQ(vm->values[0].as.i, tc.expected) << "Test case: " << tc.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseInvert) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 10

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_BITWISE_INVERT A=0x00 B=0x01 C=0x00 kb=F kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    ASSERT_EQ(result, 0);
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT);
    ASSERT_EQ(vm->values[0].as.i, ~0b1010);
}

TEST_F(VMInstructionArithmeticTest, OpArithmeticWithConstants) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 10

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_ADD  A=0x00 B=0x01 C=0x85 kb=F kc=T
1: OP_TRAP A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    ASSERT_EQ(result, 0);
    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT);
    ASSERT_EQ(vm->values[0].as.i, 15);
}

TEST_F(VMInstructionArithmeticTest, OpArithmeticTypeError) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 10
R[2]: Bool true

[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_ADD  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE);
    ASSERT_EQ(vm->error, SEMI_ERROR_UNEXPECTED_TYPE);
}
