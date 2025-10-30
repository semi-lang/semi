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

struct ArithmeticTestCase {
    const char* name;
    int lhs_int;
    float lhs_float;
    int rhs_int;
    float rhs_float;
    bool use_float_lhs;
    bool use_float_rhs;
    int expected_int;
    float expected_float;
    bool expect_float_result;
    bool expect_error;
    int expected_error_code;
};

TEST_F(VMInstructionArithmeticTest, OpAdd) {
    ArithmeticTestCase test_cases[] = {
        {    "int_plus_int",  5, 0.0f,  3, 0.0f, false, false,  8, 0.0f, false, false, 0},
        {  "int_plus_float",  5, 0.0f,  0, 3.5f, false,  true,  0, 8.5f,  true, false, 0},
        {  "float_plus_int",  0, 5.5f,  3, 0.0f,  true, false,  0, 8.5f,  true, false, 0},
        {"float_plus_float",  0, 5.5f,  0, 3.5f,  true,  true,  0, 9.0f,  true, false, 0},
        {"negative_numbers", -5, 0.0f, -3, 0.0f, false, false, -8, 0.0f, false, false, 0},
        {   "zero_addition",  0, 0.0f,  0, 0.0f, false, false,  0, 0.0f, false, false, 0}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_ADD(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error = 0;

        if (test_case.use_float_lhs) {
            vm->values[1] = semiValueNewFloat(test_case.lhs_float);

        } else {
            vm->values[1] = semiValueNewInt(test_case.lhs_int);
        }

        if (test_case.use_float_rhs) {
            vm->values[2] = semiValueNewFloat(test_case.rhs_float);
        } else {
            vm->values[2] = semiValueNewInt(test_case.rhs_int);
        }

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        if (test_case.expect_error) {
            ASSERT_EQ(result, test_case.expected_error_code) << "Test case: " << test_case.name;
            ASSERT_EQ(vm->error, test_case.expected_error_code) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

            if (test_case.expect_float_result) {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << test_case.name;
                ASSERT_FLOAT_EQ(vm->values[0].as.f, test_case.expected_float) << "Test case: " << test_case.name;
            } else {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << test_case.name;
                ASSERT_EQ(vm->values[0].as.i, test_case.expected_int) << "Test case: " << test_case.name;
            }
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpSubtract) {
    ArithmeticTestCase test_cases[] = {
        {    "int_minus_int", 10,  0.0f,  3, 0.0f, false, false,  7, 0.0f, false, false, 0},
        {  "int_minus_float", 10,  0.0f,  0, 3.5f, false,  true,  0, 6.5f,  true, false, 0},
        {  "float_minus_int",  0, 10.5f,  3, 0.0f,  true, false,  0, 7.5f,  true, false, 0},
        {"float_minus_float",  0, 10.5f,  0, 3.5f,  true,  true,  0, 7.0f,  true, false, 0},
        {  "negative_result",  3,  0.0f, 10, 0.0f, false, false, -7, 0.0f, false, false, 0},
        { "zero_subtraction",  5,  0.0f,  0, 0.0f, false, false,  5, 0.0f, false, false, 0}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_SUBTRACT(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error = 0;

        if (test_case.use_float_lhs) {
            vm->values[1].header = VALUE_TYPE_FLOAT;
            vm->values[1].as.f   = test_case.lhs_float;
        } else {
            vm->values[1].header = VALUE_TYPE_INT;
            vm->values[1].as.i   = test_case.lhs_int;
        }

        if (test_case.use_float_rhs) {
            vm->values[2].header = VALUE_TYPE_FLOAT;
            vm->values[2].as.f   = test_case.rhs_float;
        } else {
            vm->values[2].header = VALUE_TYPE_INT;
            vm->values[2].as.i   = test_case.rhs_int;
        }

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        if (test_case.expect_error) {
            ASSERT_EQ(result, test_case.expected_error_code) << "Test case: " << test_case.name;
            ASSERT_EQ(vm->error, test_case.expected_error_code) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

            if (test_case.expect_float_result) {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << test_case.name;
                ASSERT_FLOAT_EQ(vm->values[0].as.f, test_case.expected_float) << "Test case: " << test_case.name;
            } else {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << test_case.name;
                ASSERT_EQ(vm->values[0].as.i, test_case.expected_int) << "Test case: " << test_case.name;
            }
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpMultiply) {
    ArithmeticTestCase test_cases[] = {
        {    "int_times_int",  5, 0.0f, 3, 0.0f, false, false,  15,  0.0f, false, false, 0},
        {  "int_times_float",  5, 0.0f, 0, 2.5f, false,  true,   0, 12.5f,  true, false, 0},
        {  "float_times_int",  0, 2.5f, 4, 0.0f,  true, false,   0, 10.0f,  true, false, 0},
        {"float_times_float",  0, 2.5f, 0, 2.0f,  true,  true,   0,  5.0f,  true, false, 0},
        { "multiply_by_zero",  5, 0.0f, 0, 0.0f, false, false,   0,  0.0f, false, false, 0},
        {"negative_multiply", -3, 0.0f, 4, 0.0f, false, false, -12,  0.0f, false, false, 0}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_MULTIPLY(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error = 0;

        if (test_case.use_float_lhs) {
            vm->values[1].header = VALUE_TYPE_FLOAT;
            vm->values[1].as.f   = test_case.lhs_float;
        } else {
            vm->values[1].header = VALUE_TYPE_INT;
            vm->values[1].as.i   = test_case.lhs_int;
        }

        if (test_case.use_float_rhs) {
            vm->values[2].header = VALUE_TYPE_FLOAT;
            vm->values[2].as.f   = test_case.rhs_float;
        } else {
            vm->values[2].header = VALUE_TYPE_INT;
            vm->values[2].as.i   = test_case.rhs_int;
        }

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        if (test_case.expect_error) {
            ASSERT_EQ(result, test_case.expected_error_code) << "Test case: " << test_case.name;
            ASSERT_EQ(vm->error, test_case.expected_error_code) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

            if (test_case.expect_float_result) {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << test_case.name;
                ASSERT_FLOAT_EQ(vm->values[0].as.f, test_case.expected_float) << "Test case: " << test_case.name;
            } else {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << test_case.name;
                ASSERT_EQ(vm->values[0].as.i, test_case.expected_int) << "Test case: " << test_case.name;
            }
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpDivide) {
    ArithmeticTestCase test_cases[] = {
        {          "int_divide_int", 15,  0.0f, 3, 0.0f, false, false, 5, 0.0f, false, false, 0},
        {"int_divide_int_truncated",  7,  0.0f, 2, 0.0f, false, false, 3, 0.0f, false, false, 0},
        {        "int_divide_float", 10,  0.0f, 0, 2.0f, false,  true, 0, 5.0f,  true, false, 0},
        {        "float_divide_int",  0, 15.0f, 3, 0.0f,  true, false, 0, 5.0f,  true, false, 0},
        {      "float_divide_float",  0, 15.0f, 0, 3.0f,  true,  true, 0, 5.0f,  true, false, 0}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_DIVIDE(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error = 0;

        if (test_case.use_float_lhs) {
            vm->values[1].header = VALUE_TYPE_FLOAT;
            vm->values[1].as.f   = test_case.lhs_float;
        } else {
            vm->values[1].header = VALUE_TYPE_INT;
            vm->values[1].as.i   = test_case.lhs_int;
        }

        if (test_case.use_float_rhs) {
            vm->values[2].header = VALUE_TYPE_FLOAT;
            vm->values[2].as.f   = test_case.rhs_float;
        } else {
            vm->values[2].header = VALUE_TYPE_INT;
            vm->values[2].as.i   = test_case.rhs_int;
        }

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        if (test_case.expect_error) {
            ASSERT_EQ(result, test_case.expected_error_code) << "Test case: " << test_case.name;
            ASSERT_EQ(vm->error, test_case.expected_error_code) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

            if (test_case.expect_float_result) {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << test_case.name;
                ASSERT_FLOAT_EQ(vm->values[0].as.f, test_case.expected_float) << "Test case: " << test_case.name;
            } else {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Test case: " << test_case.name;
                ASSERT_EQ(vm->values[0].as.i, test_case.expected_int) << "Test case: " << test_case.name;
            }
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpDivideByZero) {
    ASSERT_NE(vm, nullptr) << "Failed to create VM";

    Instruction code[2];
    code[0] = INSTRUCTION_DIVIDE(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 10;
    vm->values[2].header = VALUE_TYPE_INT;
    vm->values[2].as.i   = 0;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, SEMI_ERROR_DIVIDE_BY_ZERO) << "Division by zero should return error";
    ASSERT_EQ(vm->error, SEMI_ERROR_DIVIDE_BY_ZERO) << "VM error should be DIVIDE_BY_ZERO";
}

TEST_F(VMInstructionArithmeticTest, OpFloorDivide) {
    ArithmeticTestCase test_cases[] = {
        {     "int_floor_divide_int",  15,  0.0f,  4, 0.0f, false, false,  3, 0.0f, false, false, 0},
        {"int_floor_divide_negative", -15,  0.0f,  4, 0.0f, false, false, -3, 0.0f, false, false, 0},
        {     "negative_by_negative", -15,  0.0f, -4, 0.0f, false, false,  3, 0.0f, false, false, 0},
        {       "float_floor_divide",   0, 15.7f,  4, 0.0f,  true, false,  3, 0.0f, false, false, 0}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_FLOOR_DIVIDE(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error = 0;

        if (test_case.use_float_lhs) {
            vm->values[1].header = VALUE_TYPE_FLOAT;
            vm->values[1].as.f   = test_case.lhs_float;
        } else {
            vm->values[1].header = VALUE_TYPE_INT;
            vm->values[1].as.i   = test_case.lhs_int;
        }

        if (test_case.use_float_rhs) {
            vm->values[2].header = VALUE_TYPE_FLOAT;
            vm->values[2].as.f   = test_case.rhs_float;
        } else {
            vm->values[2].header = VALUE_TYPE_INT;
            vm->values[2].as.i   = test_case.rhs_int;
        }

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        if (test_case.expect_float_result) {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Test case: " << test_case.name;
            ASSERT_FLOAT_EQ(vm->values[0].as.f, test_case.expected_float) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Floor divide should return int for " << test_case.name;
            ASSERT_EQ(vm->values[0].as.i, test_case.expected_int) << "Test case: " << test_case.name;
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpModulo) {
    ArithmeticTestCase test_cases[] = {
        {  "positive_modulo",  15, 0.0f,  4, 0.0f, false, false,  3, 0.0f, false, false, 0},
        {"negative_dividend", -15, 0.0f,  4, 0.0f, false, false, -3, 0.0f, false, false, 0},
        { "negative_divisor",  15, 0.0f, -4, 0.0f, false, false,  3, 0.0f, false, false, 0},
        {    "both_negative", -15, 0.0f, -4, 0.0f, false, false, -3, 0.0f, false, false, 0},
        {    "zero_dividend",   0, 0.0f,  4, 0.0f, false, false,  0, 0.0f, false, false, 0}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_MODULO(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error = 0;

        vm->values[1].header = VALUE_TYPE_INT;
        vm->values[1].as.i   = test_case.lhs_int;
        vm->values[2].header = VALUE_TYPE_INT;
        vm->values[2].as.i   = test_case.rhs_int;

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Modulo should return int for " << test_case.name;
        ASSERT_EQ(vm->values[0].as.i, test_case.expected_int) << "Test case: " << test_case.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpPower) {
    ArithmeticTestCase test_cases[] = {
        {    "int_power_int", 2, 0.0f, 3, 0.0f, false, false, 8,       0.0f, false, false, 0},
        {  "int_power_float", 2, 0.0f, 0, 3.5f, false,  true, 0, 11.313708f,  true, false, 0},
        {  "float_power_int", 0, 2.5f, 2, 0.0f,  true, false, 0,      6.25f,  true, false, 0},
        {"float_power_float", 0, 2.0f, 0, 0.5f,  true,  true, 0,  1.414214f,  true, false, 0},
        {    "power_of_zero", 5, 0.0f, 0, 0.0f, false, false, 1,       0.0f, false, false, 0},
        {    "zero_to_power", 0, 0.0f, 3, 0.0f, false, false, 0,       0.0f, false, false, 0}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_POWER(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error = 0;

        if (test_case.use_float_lhs) {
            vm->values[1].header = VALUE_TYPE_FLOAT;
            vm->values[1].as.f   = test_case.lhs_float;
        } else {
            vm->values[1].header = VALUE_TYPE_INT;
            vm->values[1].as.i   = test_case.lhs_int;
        }

        if (test_case.use_float_rhs) {
            vm->values[2].header = VALUE_TYPE_FLOAT;
            vm->values[2].as.f   = test_case.rhs_float;
        } else {
            vm->values[2].header = VALUE_TYPE_INT;
            vm->values[2].as.i   = test_case.rhs_int;
        }

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        if (test_case.expect_float_result) {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Power should return float for " << test_case.name;
            ASSERT_NEAR(vm->values[0].as.f, test_case.expected_float, 0.001f) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Power should return int for " << test_case.name;
            ASSERT_EQ(vm->values[0].as.i, test_case.expected_int) << "Test case: " << test_case.name;
        }
    }
}

TEST_F(VMInstructionArithmeticTest, OpNegate) {
    ASSERT_NE(vm, nullptr) << "Failed to create VM";

    Instruction code[2];
    code[0] = INSTRUCTION_NEGATE(0, 1, 0, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 42;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int";
    ASSERT_EQ(vm->values[0].as.i, -42) << "Negation should work";
}

TEST_F(VMInstructionArithmeticTest, OpNegateFloat) {
    ASSERT_NE(vm, nullptr) << "Failed to create VM";

    Instruction code[2];
    code[0] = INSTRUCTION_NEGATE(0, 1, 0, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_FLOAT;
    vm->values[1].as.f   = 3.14f;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_FLOAT) << "Result should be float";
    ASSERT_FLOAT_EQ(vm->values[0].as.f, -3.14f) << "Float negation should work";
}

struct BitwiseTestCase {
    const char* name;
    int lhs;
    int rhs;
    int expected_result;
};

TEST_F(VMInstructionArithmeticTest, OpBitwiseAnd) {
    BitwiseTestCase test_cases[] = {
        {    "and_basic", 0b1010, 0b1100, 0b1000},
        { "and_all_ones",   0xFF,   0xFF,   0xFF},
        {"and_with_zero",   0xFF,   0x00,   0x00},
        { "and_negative",     -1, 0b1010, 0b1010}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_BITWISE_AND(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error            = 0;
        vm->values[1].header = VALUE_TYPE_INT;
        vm->values[1].as.i   = test_case.lhs;
        vm->values[2].header = VALUE_TYPE_INT;
        vm->values[2].as.i   = test_case.rhs;

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << test_case.name;
        ASSERT_EQ(vm->values[0].as.i, test_case.expected_result) << "Test case: " << test_case.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseOr) {
    BitwiseTestCase test_cases[] = {
        {    "or_basic", 0b1010, 0b1100, 0b1110},
        {"or_with_zero", 0b1010,   0x00, 0b1010},
        { "or_all_ones",   0xFF,   0x00,   0xFF},
        { "or_negative",     -1, 0b1010,     -1}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_BITWISE_OR(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error            = 0;
        vm->values[1].header = VALUE_TYPE_INT;
        vm->values[1].as.i   = test_case.lhs;
        vm->values[2].header = VALUE_TYPE_INT;
        vm->values[2].as.i   = test_case.rhs;

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << test_case.name;
        ASSERT_EQ(vm->values[0].as.i, test_case.expected_result) << "Test case: " << test_case.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseXor) {
    BitwiseTestCase test_cases[] = {
        {    "xor_basic", 0b1010, 0b1100, 0b0110},
        {     "xor_same", 0b1010, 0b1010,   0x00},
        {"xor_with_zero", 0b1010,   0x00, 0b1010},
        { "xor_all_ones",   0xFF,   0xFF,   0x00}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_BITWISE_XOR(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error            = 0;
        vm->values[1].header = VALUE_TYPE_INT;
        vm->values[1].as.i   = test_case.lhs;
        vm->values[2].header = VALUE_TYPE_INT;
        vm->values[2].as.i   = test_case.rhs;

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << test_case.name;
        ASSERT_EQ(vm->values[0].as.i, test_case.expected_result) << "Test case: " << test_case.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseShift) {
    BitwiseTestCase test_cases[] = {
        { "shift_left", 0b1010, 2, 0b101000},
        { "shift_zero", 0b1010, 0,   0b1010},
        {"shift_large",      1, 8,      256}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_NE(vm, nullptr) << "Failed to create VM for " << test_case.name;

        Instruction code[2];
        code[0] = INSTRUCTION_BITWISE_L_SHIFT(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        vm->error            = 0;
        vm->values[1].header = VALUE_TYPE_INT;
        vm->values[1].as.i   = test_case.lhs;
        vm->values[2].header = VALUE_TYPE_INT;
        vm->values[2].as.i   = test_case.rhs;

        SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
        module->moduleInit     = func;

        int result = semiVMRunMainModule(vm, module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int for " << test_case.name;
        ASSERT_EQ(vm->values[0].as.i, test_case.expected_result) << "Test case: " << test_case.name;
    }
}

TEST_F(VMInstructionArithmeticTest, OpBitwiseInvert) {
    ASSERT_NE(vm, nullptr) << "Failed to create VM";

    Instruction code[2];
    code[0] = INSTRUCTION_BITWISE_INVERT(0, 1, 0, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 0b1010;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int";
    ASSERT_EQ(vm->values[0].as.i, ~0b1010) << "Bitwise invert should work";
}

TEST_F(VMInstructionArithmeticTest, OpArithmeticWithConstants) {
    ASSERT_NE(vm, nullptr) << "Failed to create VM";

    Instruction code[2];
    code[0] = INSTRUCTION_ADD(0, 1, 133, false, true);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 10;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should complete successfully";

    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INT) << "Result should be int";
    ASSERT_EQ(vm->values[0].as.i, 15) << "Addition with immediate constant should work (10 + (133 - 128) = 15)";
}

TEST_F(VMInstructionArithmeticTest, OpArithmeticTypeError) {
    ASSERT_NE(vm, nullptr) << "Failed to create VM";

    Instruction code[2];
    code[0] = INSTRUCTION_ADD(0, 1, 2, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    vm->values[1].header = VALUE_TYPE_INT;
    vm->values[1].as.i   = 10;
    vm->values[2].header = VALUE_TYPE_BOOL;
    vm->values[2].as.b   = true;

    SemiModule* module     = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionTemplate* func = CreateFunctionObject(0, code, 2, 3, 0, 0);
    module->moduleInit     = func;

    int result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE) << "VM should return type error";
    ASSERT_EQ(vm->error, SEMI_ERROR_UNEXPECTED_TYPE) << "VM error should be UNEXPECTED_TYPE";
}
