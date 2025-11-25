// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionIterTest : public VMTest {};

// Test data structures for different scenarios
struct IntegerRangeTestCase {
    const char* name;
    int start;
    int end;
    int step;
    bool expect_error;
    int expected_error_code;
    bool expect_inline_range;
};

struct FloatRangeTestCase {
    const char* name;
    double start;
    double end;
    double step;
    bool expect_error;
    int expected_error_code;
};

// OP_MAKE_RANGE Tests
TEST_F(VMInstructionIterTest, OpMakeRangeBasicInteger) {
    IntegerRangeTestCase test_cases[] = {
        {  "simple_range_1_to_10",   1, 10, 1, false, 0,  true},
        {   "simple_range_0_to_5",   0,  5, 1, false, 0,  true},
        {     "range_with_step_2",   1, 10, 2, false, 0, false},
        {        "negative_start",  -5,  5, 1, false, 0,  true},
        {        "negative_range", -10, -1, 1, false, 0,  true},
        {             "zero_step",   1, 10, 0, false, 0, false},
        {      "start_equals_end",   5,  5, 1, false, 0,  true},
        {"start_greater_than_end",  10,  1, 1, false, 0,  true},
    };

    for (const auto& test_case : test_cases) {
        // Set up initial values according to MAKE_RANGE semantics:
        // R[A] := range(R[A], RK(B, kb), RK(C, kc))
        // A=0: start (becomes range result)
        // B=1: end
        // C=2: step
        vm->values[0] = semiValueIntCreate(test_case.start);
        vm->values[1] = semiValueIntCreate(test_case.end);
        vm->values[2] = semiValueIntCreate(test_case.step);

        SemiModule* module;
        ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                                R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_MAKE_RANGE  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP        A=0x00 K=0x0000 i=F s=F
)",
                                                                &module);

        if (test_case.expect_error) {
            ASSERT_EQ(result, test_case.expected_error_code) << "Test case: " << test_case.name;
            ASSERT_EQ(vm->error, test_case.expected_error_code) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

            // Check if the correct range type was created
            if (test_case.expect_inline_range) {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_INLINE_RANGE)
                    << "Should create InlineRange for " << test_case.name;
                ASSERT_EQ(vm->values[0].as.ir.start, test_case.start)
                    << "InlineRange start mismatch for " << test_case.name;
                ASSERT_EQ(vm->values[0].as.ir.end, test_case.end) << "InlineRange end mismatch for " << test_case.name;
            } else {
                ASSERT_EQ(vm->values[0].header, VALUE_TYPE_OBJECT_INT_RANGE)
                    << "Should create ObjectRange for " << test_case.name;
                // Additional ObjectRange validation could be added here
            }
        }

        // Reset VM for next test case
        semiDestroyVM(vm);
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to recreate VM for next test case";
    }
}

TEST_F(VMInstructionIterTest, OpMakeRangeFloatValues) {
    FloatRangeTestCase test_cases[] = {
        {        "float_start",  1.5, 10.0,  1.0, false, 0},
        {          "float_end",  1.0, 10.5,  1.0, false, 0},
        {         "float_step",  1.0, 10.0,  1.5, false, 0},
        {          "all_float",  1.5, 10.5,  1.5, false, 0},
        {"negative_float_step", 10.0,  1.0, -1.0, false, 0},
    };

    for (const auto& test_case : test_cases) {
        // Set up initial values according to MAKE_RANGE semantics:
        // R[A] := range(R[A], RK(B, kb), RK(C, kc))
        // A=0: start (becomes range result)
        // B=1: end
        // C=2: step
        vm->values[0] = semiValueFloatCreate(test_case.start);
        vm->values[1] = semiValueFloatCreate(test_case.end);
        vm->values[2] = semiValueFloatCreate(test_case.step);

        SemiModule* module;
        ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                                R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_MAKE_RANGE  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP        A=0x00 K=0x0000 i=F s=F
)",
                                                                &module);

        if (test_case.expect_error) {
            ASSERT_EQ(result, test_case.expected_error_code) << "Test case: " << test_case.name;
            ASSERT_EQ(vm->error, test_case.expected_error_code) << "Test case: " << test_case.name;
        } else {
            ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

            // Float ranges should always create ObjectRange
            ASSERT_EQ(vm->values[0].header, VALUE_TYPE_OBJECT_FLOAT_RANGE)
                << "Float ranges should create ObjectRange for " << test_case.name;
        }

        // Reset VM for next test case
        semiDestroyVM(vm);
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to recreate VM for next test case";
    }
}

TEST_F(VMInstructionIterTest, OpMakeRangeMixedTypes) {
    // Test mixing integer and float values - should always create ObjectRange
    struct {
        const char* name;
        Value start;  // Register A (becomes result)
        Value end;    // Register B
        Value step;   // Register C
    } test_cases[] = {
        { "int_start_float_end",     semiValueIntCreate(1), semiValueFloatCreate(5.5),     semiValueIntCreate(1)},
        { "float_start_int_end", semiValueFloatCreate(1.5),     semiValueIntCreate(5),     semiValueIntCreate(1)},
        {"int_start_float_step",     semiValueIntCreate(1),    semiValueIntCreate(10), semiValueFloatCreate(1.5)},
    };

    for (const auto& test_case : test_cases) {
        // Set up according to MAKE_RANGE: R[A] := range(R[A], RK(B, kb), RK(C, kc))
        // A=0: start->result, B=1: end, C=2: step
        vm->values[0] = test_case.start;
        vm->values[1] = test_case.end;
        vm->values[2] = test_case.step;

        SemiModule* module;
        ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                                R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_MAKE_RANGE  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP        A=0x00 K=0x0000 i=F s=F
)",
                                                                &module);

        ASSERT_EQ(result, 0) << "Test case: " << test_case.name;

        // Mixed types should always create ObjectRange
        ASSERT_EQ(vm->values[0].header, VALUE_TYPE_OBJECT_FLOAT_RANGE)
            << "Mixed types should create ObjectRange for " << test_case.name;

        // Reset VM for next test case
        semiDestroyVM(vm);
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to recreate VM for next test case";
    }
}

TEST_F(VMInstructionIterTest, OpMakeRangeWithConstants) {
    // Test using inline constants for B and C operands
    // R[A] := range(R[A], RK(B, kb), RK(C, kc))
    // A=0: start->result, B=129: end (1 after adding INT8_MIN), C=133: step (5 after adding INT8_MIN)
    vm->values[0] = semiValueIntCreate(1);  // start

    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_MAKE_RANGE  A=0x00 B=0x81 C=0x85 kb=T kc=T
1: OP_TRAP        A=0x00 K=0x0000 i=F s=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "Should handle inline constants";

    ASSERT_EQ(vm->values[0].header, VALUE_TYPE_OBJECT_INT_RANGE) << "Should create ObjectRange since step=5 (not 1)";
}

TEST_F(VMInstructionIterTest, OpMakeRangeTypeErrors) {
    struct {
        const char* name;
        Value start;  // Register A
        Value end;    // Register B
        Value step;   // Register C
    } error_cases[] = {
        {"bool_start", semiValueBoolCreate(true),     semiValueIntCreate(10),     semiValueIntCreate(1)},
        {  "bool_end",     semiValueIntCreate(1), semiValueBoolCreate(false),     semiValueIntCreate(1)},
        { "bool_step",     semiValueIntCreate(1),     semiValueIntCreate(10), semiValueBoolCreate(true)},
    };

    for (const auto& test_case : error_cases) {
        vm->values[0] = test_case.start;
        vm->values[1] = test_case.end;
        vm->values[2] = test_case.step;

        SemiModule* module;
        ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                                R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_MAKE_RANGE  A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP        A=0x00 K=0x0000 i=F s=F
)",
                                                                &module);

        ASSERT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE) << "Should error for " << test_case.name;
        ASSERT_EQ(vm->error, SEMI_ERROR_UNEXPECTED_TYPE) << "VM error should be UNEXPECTED_TYPE for " << test_case.name;

        // Reset VM for next test case
        semiDestroyVM(vm);
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to recreate VM for next test case";
    }
}

// OP_RANGE_NEXT Tests
struct InlineRangeIterTestCase {
    const char* name;
    int range_start;
    int range_end;
    int expected_iterations;
    bool has_counter;
    int expected_final_result;  // Expected trap value when iteration ends
};

TEST_F(VMInstructionIterTest, OpRangeNextInlineRange) {
    InlineRangeIterTestCase test_cases[] = {
        {  "range_1_to_5",  1, 5, 4,  true, 153},
        {  "range_0_to_3",  0, 3, 3,  true, 153},
        {   "empty_range",  5, 5, 0,  true, 153},
        { "reverse_range", 10, 1, 0,  true, 153},
        {"single_element",  1, 2, 1,  true, 153},
        {  "with_counter",  1, 3, 2, false, 153},
    };

    for (const auto& test_case : test_cases) {
        vm->values[0] = semiValueIntCreate(0);  // iteration counter
        vm->values[1] = semiValueInlineRangeCreate((int32_t)test_case.range_start, (int32_t)test_case.range_end);
        if (test_case.has_counter) {
            vm->values[2] = semiValueIntCreate(0);  // Counter register
        }

        char dsl_spec[512];
        snprintf(dsl_spec,
                 sizeof(dsl_spec),
                 R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_RANGE_NEXT  A=0x01 K=0x0003 i=%c s=F
1: OP_ADD         A=0x00 B=0x00 C=0x81 kb=F kc=T
1: OP_JUMP        J=0x000002 s=F
2: OP_TRAP        A=0x00 K=0x0099 i=F s=F
3: OP_TRAP        A=0x00 K=0x0058 i=F s=F
)",
                 test_case.has_counter ? 'T' : 'F');

        int iterations = 0;
        int result;

        SemiModule* module;
        result = InstructionVerifier::BuildAndRunModule(vm, dsl_spec, &module);

        ASSERT_EQ(result, test_case.expected_final_result) << "Final result mismatch for " << test_case.name;
        ASSERT_EQ(AS_INT(&vm->values[0]), test_case.expected_iterations)
            << "Iteration count mismatch for " << test_case.name;

        // For non-no-assign cases, check that internal counter was incremented
        if (test_case.has_counter) {
            ASSERT_EQ(AS_INT(&vm->values[2]), test_case.expected_iterations)
                << "Counter should be incremented for " << test_case.name;
        }

        // Reset VM for next test case
        semiDestroyVM(vm);
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to recreate VM for next test case";
    }
}

struct ObjectRangeIterIntTestCase {
    const char* name;
    int range_start;
    int range_end;
    int range_step;
    int expected_iterations;
    bool has_counter;
    int expected_final_result;  // Expected trap value when iteration ends
};

TEST_F(VMInstructionIterTest, OpRangeNextObjectRangeInt) {
    ObjectRangeIterIntTestCase test_cases[] = {
        { "range_1_to_5",  1,  5,  1, 4,  true, 153},
        { "range_0_to_3",  0,  3,  1, 3,  true, 153},
        {  "empty_range",  5,  5,  1, 0,  true, 153},
        {"reverse_range", 10,  1,  1, 0,  true, 153},
        {       "step_2",  1, 10,  2, 5,  true, 153},
        {      "step_-2", 10,  0, -2, 5,  true, 153},
        { "with_counter",  1,  3,  1, 2, false, 153},
    };

    for (const auto& test_case : test_cases) {
        vm->values[0] = semiValueIntCreate(0);  // iteration counter
        vm->values[1] = OBJECT_VALUE(
            semiObjectIntRangeCreate(
                &vm->gc, (int64_t)test_case.range_start, (int64_t)test_case.range_end, (int64_t)test_case.range_step),
            VALUE_TYPE_OBJECT_INT_RANGE);
        if (test_case.has_counter) {
            vm->values[2] = semiValueIntCreate(0);  // Counter register
        }

        char dsl_spec[512];
        snprintf(dsl_spec,
                 sizeof(dsl_spec),
                 R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_RANGE_NEXT  A=0x01 K=0x0003 i=%c s=F
1: OP_ADD         A=0x00 B=0x00 C=0x81 kb=F kc=T
1: OP_JUMP        J=0x000002 s=F
2: OP_TRAP        A=0x00 K=0x0099 i=F s=F
3: OP_TRAP        A=0x00 K=0x0058 i=F s=F
)",
                 test_case.has_counter ? 'T' : 'F');

        int iterations = 0;
        int result;

        SemiModule* module;
        result = InstructionVerifier::BuildAndRunModule(vm, dsl_spec, &module);

        ASSERT_EQ(result, test_case.expected_final_result) << "Final result mismatch for " << test_case.name;
        ASSERT_EQ(AS_INT(&vm->values[0]), test_case.expected_iterations)
            << "Iteration count mismatch for " << test_case.name;

        // For non-no-assign cases, check that internal counter was incremented
        if (test_case.has_counter) {
            ASSERT_EQ(AS_INT(&vm->values[2]), test_case.expected_iterations)
                << "Counter should be incremented for " << test_case.name;
        }

        // Reset VM for next test case
        semiDestroyVM(vm);
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to recreate VM for next test case";
    }
}
struct ObjectRangeIterFloatTestCase {
    const char* name;
    double range_start;
    double range_end;
    double range_step;
    int expected_iterations;
    bool has_counter;
    int expected_final_result;  // Expected trap value when iteration ends
};
TEST_F(VMInstructionIterTest, OpRangeNextObjectRangeFloat) {
    ObjectRangeIterFloatTestCase test_cases[] = {
        { "range_1_to_5",   1,   5,  1, 4,  true, 153},
        { "range_0_to_3",   0,   3,  1, 3,  true, 153},
        {  "empty_range",   5,   5,  1, 0,  true, 153},
        {"reverse_range",  10,   1,  1, 0,  true, 153},
        {       "step_2", 1.5,  10,  2, 5,  true, 153},
        {      "step_-2",  10, 0.5, -2, 5,  true, 153},
        { "with_counter",   1,   3,  1, 2, false, 153},
    };

    for (const auto& test_case : test_cases) {
        vm->values[0] = semiValueIntCreate(0);  // iteration counter
        vm->values[1] = OBJECT_VALUE(
            semiObjectFloatRangeCreate(&vm->gc, test_case.range_start, test_case.range_end, test_case.range_step),
            VALUE_TYPE_OBJECT_FLOAT_RANGE);
        if (test_case.has_counter) {
            vm->values[2] = semiValueIntCreate(0);  // Counter register
        }

        char dsl_spec[512];
        snprintf(dsl_spec,
                 sizeof(dsl_spec),
                 R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_RANGE_NEXT  A=0x01 K=0x0003 i=%c s=F
1: OP_ADD         A=0x00 B=0x00 C=0x81 kb=F kc=T
1: OP_JUMP        J=0x000002 s=F
2: OP_TRAP        A=0x00 K=0x0099 i=F s=F
3: OP_TRAP        A=0x00 K=0x0058 i=F s=F
)",
                 test_case.has_counter ? 'T' : 'F');

        int iterations = 0;
        int result;

        SemiModule* module;
        result = InstructionVerifier::BuildAndRunModule(vm, dsl_spec, &module);

        ASSERT_EQ(result, test_case.expected_final_result) << "Final result mismatch for " << test_case.name;
        ASSERT_EQ(AS_INT(&vm->values[0]), test_case.expected_iterations)
            << "Iteration count mismatch for " << test_case.name;

        // For non-no-assign cases, check that internal counter was incremented
        if (test_case.has_counter) {
            ASSERT_EQ(AS_INT(&vm->values[2]), test_case.expected_iterations)
                << "Counter should be incremented for " << test_case.name;
        }

        // Reset VM for next test case
        semiDestroyVM(vm);
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to recreate VM for next test case";
    }
}