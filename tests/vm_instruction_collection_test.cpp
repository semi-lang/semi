// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "../src/value.h"
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionCollectionTest : public VMTest {};

// Test cases for OP_GET_ITEM string indexing
struct GetItemStringTestCase {
    const char* name;
    const char* str;
    bool use_object_string;
    int32_t index;
    bool expect_error;
    ErrorId expected_error;
    char expected_char;  // Only valid if !expect_error
};

TEST_F(VMInstructionCollectionTest, OpGetItemStringIndexing) {
    GetItemStringTestCase test_cases[] = {
        {       "inline_string_positive_index",    "hi", false,   1, false,                    0,  'i'},
        {       "inline_string_negative_index",    "hi", false,  -1, false,                    0,  'i'},
        {"inline_string_negative_index_middle",    "ab", false,  -2, false,                    0,  'a'},
        {   "inline_string_index_oob_positive",    "hi", false,  10,  true, SEMI_ERROR_INDEX_OOB, '\0'},
        {   "inline_string_index_oob_negative",    "hi", false, -10,  true, SEMI_ERROR_INDEX_OOB, '\0'},
        {       "object_string_positive_index", "world",  true,   2, false,                    0,  'r'},
        {       "object_string_negative_index", "world",  true,  -2, false,                    0,  'l'},
        {            "object_string_index_oob", "world",  true,  20,  true, SEMI_ERROR_INDEX_OOB, '\0'},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        // Reset VM state

        vm->error = 0;

        // Setup: R[1] = string, R[2] = index, R[0] = result
        if (test_case.use_object_string) {
            vm->values[1] = semiValueStringCreate(&vm->gc, test_case.str, strlen(test_case.str));
        } else {
            // Create inline string
            Value str_val        = INLINE_STRING_VALUE_0();
            str_val.as.is.length = (uint8_t)strlen(test_case.str);
            for (size_t i = 0; i < strlen(test_case.str); i++) {
                str_val.as.is.c[i] = test_case.str[i];
            }
            vm->values[1] = str_val;
        }

        vm->values[2] = semiValueNewInt(test_case.index);

        // Create instruction: GET_ITEM R[0], R[1], R[2]
        Instruction code[2];
        code[0] = INSTRUCTION_GET_ITEM(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);

        if (test_case.expect_error) {
            EXPECT_EQ(result, test_case.expected_error);
        } else {
            EXPECT_EQ(result, 0) << "Expected success but got error: " << result;
            EXPECT_TRUE(IS_INLINE_STRING(&vm->values[0])) << "Result should be an inline string";
            EXPECT_EQ(AS_INLINE_STRING(&vm->values[0]).length, 1) << "Result should be single character";
            EXPECT_EQ(AS_INLINE_STRING(&vm->values[0]).c[0], test_case.expected_char);
        }
    }
}

TEST_F(VMInstructionCollectionTest, OpGetItemUnsupportedTypes) {
    // Test GET_ITEM on unsupported types
    struct {
        const char* name;
        Value test_value;
        ErrorId expected_error;
    } test_cases[] = {
        {"integer",      semiValueNewInt(42), SEMI_ERROR_UNEXPECTED_TYPE},
        {  "float", semiValueNewFloat(3.14f), SEMI_ERROR_UNEXPECTED_TYPE},
        {"boolean",   semiValueNewBool(true), SEMI_ERROR_UNEXPECTED_TYPE},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        vm->values[1] = test_case.test_value;
        vm->values[2] = semiValueNewInt(0);  // index

        Instruction code[2];
        code[0] = INSTRUCTION_GET_ITEM(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, test_case.expected_error);
    }
}

// Test cases for OP_SET_ITEM
struct SetItemTestCase {
    const char* name;
    bool use_dict;  // true for dict, false for list
    bool expect_error;
    ErrorId expected_error;
};

TEST_F(VMInstructionCollectionTest, OpSetItemDict) {
    vm->error = 0;

    // Create a dictionary: R[1] = {}
    vm->values[1] = semiValueDictCreate(&vm->gc);

    // Set key-value: R[1]["key"] = 42
    vm->values[2] = semiValueStringCreate(&vm->gc, "key", 3);  // key
    vm->values[3] = semiValueNewInt(42);                       // value

    Instruction code[2];
    code[0] = INSTRUCTION_SET_ITEM(1, 2, 3, false, false);  // R[1][R[2]] = R[3]
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "SET_ITEM on dict should succeed";

    // Verify the value was set by using dict's internal functions
    ObjectDict* dict = AS_DICT(&vm->values[1]);
    Value key        = semiValueStringCreate(&vm->gc, "key", 3);
    EXPECT_TRUE(semiDictHas(dict, key)) << "Dictionary should contain the key";
}

TEST_F(VMInstructionCollectionTest, OpSetItemList) {
    SetItemTestCase test_cases[] = {
        {"valid_positive_index", false, false, 0},
        {"valid_negative_index", false, false, 0},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        // Create a list with some elements: R[1] = [10, 20, 30]
        vm->values[1]    = semiValueListCreate(&vm->gc, 3);
        ObjectList* list = AS_LIST(&vm->values[1]);
        semiListAppend(&vm->gc, list, semiValueNewInt(10));
        semiListAppend(&vm->gc, list, semiValueNewInt(20));
        semiListAppend(&vm->gc, list, semiValueNewInt(30));

        // Test different indices
        int32_t index;
        int32_t expected_final_value;
        if (strcmp(test_case.name, "valid_positive_index") == 0) {
            index                = 1;
            expected_final_value = 99;
        } else {  // valid_negative_index
            index                = -1;
            expected_final_value = 88;
        }

        vm->values[2] = semiValueNewInt(index);                 // index
        vm->values[3] = semiValueNewInt(expected_final_value);  // new value

        Instruction code[2];
        code[0] = INSTRUCTION_SET_ITEM(1, 2, 3, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);

        if (test_case.expect_error) {
            EXPECT_EQ(result, test_case.expected_error);
        } else {
            EXPECT_EQ(result, 0) << "SET_ITEM should succeed";

            // Verify the value was set correctly
            uint32_t actual_index = (index < 0) ? (list->size + index) : index;
            EXPECT_EQ(AS_INT(&list->values[actual_index]), expected_final_value);
        }
    }
}

TEST_F(VMInstructionCollectionTest, OpSetItemListErrors) {
    vm->error = 0;

    // Create a small list: R[1] = [10, 20]
    vm->values[1]    = semiValueListCreate(&vm->gc, 2);
    ObjectList* list = AS_LIST(&vm->values[1]);
    semiListAppend(&vm->gc, list, semiValueNewInt(10));
    semiListAppend(&vm->gc, list, semiValueNewInt(20));

    struct {
        const char* name;
        int32_t index;
        ErrorId expected_error;
    } error_cases[] = {
        {"index_oob_positive",  10, SEMI_ERROR_INDEX_OOB},
        {"index_oob_negative", -10, SEMI_ERROR_INDEX_OOB},
    };

    for (const auto& error_case : error_cases) {
        SCOPED_TRACE(error_case.name);

        vm->error = 0;

        vm->values[2] = semiValueNewInt(error_case.index);
        vm->values[3] = semiValueNewInt(999);

        Instruction code[2];
        code[0] = INSTRUCTION_SET_ITEM(1, 2, 3, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, error_case.expected_error);
    }
}

TEST_F(VMInstructionCollectionTest, OpSetItemUnsupportedTypes) {
    struct {
        const char* name;
        Value container;
        ErrorId expected_error;
    } test_cases[] = {
        {"string", semiValueStringCreate(&vm->gc, "test", 4), SEMI_ERROR_UNEXPECTED_TYPE},
        {"integer", semiValueNewInt(42), SEMI_ERROR_UNEXPECTED_TYPE},
        {"float", semiValueNewFloat(3.14f), SEMI_ERROR_UNEXPECTED_TYPE},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        vm->values[1] = test_case.container;
        vm->values[2] = semiValueNewInt(0);    // index/key
        vm->values[3] = semiValueNewInt(123);  // value

        Instruction code[2];
        code[0] = INSTRUCTION_SET_ITEM(1, 2, 3, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, test_case.expected_error);
    }
}

// Test cases for OP_CONTAIN
TEST_F(VMInstructionCollectionTest, OpContainDict) {
    vm->error = 0;

    // Create dictionary and add some values
    vm->values[2]    = semiValueDictCreate(&vm->gc);
    ObjectDict* dict = AS_DICT(&vm->values[2]);

    Value key1 = semiValueStringCreate(&vm->gc, "key1", 4);
    Value key2 = semiValueNewInt(42);
    semiDictSet(&vm->gc, dict, key1, semiValueNewInt(100));
    semiDictSet(&vm->gc, dict, key2, semiValueNewBool(true));

    struct {
        const char* name;
        Value search_key;
        bool expected_result;
    } test_cases[] = {
        {"existing_string_key", semiValueStringCreate(&vm->gc, "key1", 4), true},
        {"existing_int_key", semiValueNewInt(42), true},
        {"nonexistent_key", semiValueStringCreate(&vm->gc, "missing", 7), false},
        {"wrong_type_key", semiValueNewFloat(42.0f), false},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        vm->values[1] = test_case.search_key;

        Instruction code[2];
        code[0] = INSTRUCTION_CONTAIN(0, 1, 2, false, false);  // R[0] = R[1] in R[2]
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, 0) << "CONTAIN should succeed";
        EXPECT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Result should be boolean";
        EXPECT_EQ(AS_BOOL(&vm->values[0]), test_case.expected_result);
    }
}

TEST_F(VMInstructionCollectionTest, OpContainList) {
    vm->error = 0;

    // Create list with various values
    vm->values[2]    = semiValueListCreate(&vm->gc, 4);
    ObjectList* list = AS_LIST(&vm->values[2]);
    semiListAppend(&vm->gc, list, semiValueNewInt(10));
    semiListAppend(&vm->gc, list, semiValueStringCreate(&vm->gc, "hello", 5));
    semiListAppend(&vm->gc, list, semiValueNewBool(true));
    semiListAppend(&vm->gc, list, semiValueNewFloat(3.14f));

    struct {
        const char* name;
        Value search_value;
        bool expected_result;
    } test_cases[] = {
        {"existing_int", semiValueNewInt(10), true},
        {"existing_string", semiValueStringCreate(&vm->gc, "hello", 5), true},
        {"existing_bool", semiValueNewBool(true), true},
        {"existing_float", semiValueNewFloat(3.14f), true},
        {"nonexistent_int", semiValueNewInt(99), false},
        {"nonexistent_string", semiValueStringCreate(&vm->gc, "world", 5), false},
        {"nonexistent_bool", semiValueNewBool(false), false},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        vm->values[1] = test_case.search_value;

        Instruction code[2];
        code[0] = INSTRUCTION_CONTAIN(0, 1, 2, false, false);  // R[0] = R[1] in R[2]
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, 0) << "CONTAIN should succeed";
        EXPECT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Result should be boolean";
        EXPECT_EQ(AS_BOOL(&vm->values[0]), test_case.expected_result);
    }
}

TEST_F(VMInstructionCollectionTest, OpContainString) {
    struct {
        const char* name;
        const char* str;
        bool use_object_string;
        char search_char;
        bool expected_result;
    } test_cases[] = {
        { "inline_string_char_exists", "hello", false, 'e',  true},
        {"inline_string_char_missing", "hello", false, 'x', false},
        { "object_string_char_exists", "world",  true, 'r',  true},
        {"object_string_char_missing", "world",  true, 'z', false},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        // Setup string
        if (test_case.use_object_string) {
            vm->values[2] = semiValueStringCreate(&vm->gc, test_case.str, strlen(test_case.str));
        } else {
            Value str_val        = INLINE_STRING_VALUE_0();
            str_val.as.is.length = (uint8_t)strlen(test_case.str);
            for (size_t i = 0; i < strlen(test_case.str) && i < 7; i++) {
                str_val.as.is.c[i] = test_case.str[i];
            }
            vm->values[2] = str_val;
        }

        // Search for single character
        vm->values[1] = INLINE_STRING_VALUE_1(test_case.search_char);

        Instruction code[2];
        code[0] = INSTRUCTION_CONTAIN(0, 1, 2, false, false);  // R[0] = R[1] in R[2]
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, 0) << "CONTAIN should succeed";
        EXPECT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Result should be boolean";
        EXPECT_EQ(AS_BOOL(&vm->values[0]), test_case.expected_result);
    }
}

TEST_F(VMInstructionCollectionTest, OpContainStringMultiChar) {
    struct {
        const char* name;
        const char* str;
        const char* search_str;
        bool expected_result;
    } test_cases[] = {
        {          "substring_found", "hello",    "lo",  true},
        {      "substring_not_found", "hello",    "xy", false},
        {       "substring_at_start", "hello",    "he",  true},
        {         "substring_at_end", "hello",    "lo",  true},
        {        "full_string_match", "hello", "hello",  true},
        {      "empty_search_string", "hello",      "",  true},
        {"search_longer_than_string",    "hi", "hello", false},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        vm->values[2] = semiValueStringCreate(&vm->gc, test_case.str, strlen(test_case.str));
        vm->values[1] = semiValueStringCreate(&vm->gc, test_case.search_str, strlen(test_case.search_str));

        Instruction code[2];
        code[0] = INSTRUCTION_CONTAIN(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, 0) << "CONTAIN should succeed for multi-character search";
        EXPECT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Result should be boolean";
        EXPECT_EQ(AS_BOOL(&vm->values[0]), test_case.expected_result);
    }
}

TEST_F(VMInstructionCollectionTest, OpContainUnsupportedTypes) {
    struct {
        const char* name;
        Value container;
        ErrorId expected_error;
    } test_cases[] = {
        {"integer",      semiValueNewInt(42), SEMI_ERROR_UNEXPECTED_TYPE},
        {  "float", semiValueNewFloat(3.14f), SEMI_ERROR_UNEXPECTED_TYPE},
        {"boolean",   semiValueNewBool(true), SEMI_ERROR_UNEXPECTED_TYPE},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        vm->error = 0;

        vm->values[2] = test_case.container;
        vm->values[1] = semiValueNewInt(123);  // search value

        Instruction code[2];
        code[0] = INSTRUCTION_CONTAIN(0, 1, 2, false, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, test_case.expected_error);
    }
}

// Test using constants vs registers (kb, kc flags)
TEST_F(VMInstructionCollectionTest, OpCollectionInstructionsWithConstants) {
    vm->error = 0;

    // Test GET_ITEM with constant index
    vm->values[1] = INLINE_STRING_VALUE_2('h', 'i');  // "hi"

    // GET_ITEM R[0], R[1], constant(1) - using kb flag for constant index
    Instruction code[2];
    code[0] = INSTRUCTION_GET_ITEM(0, 1, 1 - INT8_MIN, false, true);  // kc=true for constant index
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "GET_ITEM with constant should succeed";
    EXPECT_TRUE(IS_INLINE_STRING(&vm->values[0])) << "Result should be inline string";
    EXPECT_EQ(AS_INLINE_STRING(&vm->values[0]).c[0], 'i') << "Should get second character";
}
