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

        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: String "%s"
R[2]: Int %d

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GET_ITEM A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP     A=0x00 K=0x0000 i=F s=F
)",
                 test_case.str,
                 test_case.index);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);

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
        const char* value_spec;
        ErrorId expected_error;
    } test_cases[] = {
        {"integer",     "Int 42", SEMI_ERROR_UNEXPECTED_TYPE},
        {  "float", "Float 3.14", SEMI_ERROR_UNEXPECTED_TYPE},
        {"boolean",  "Bool true", SEMI_ERROR_UNEXPECTED_TYPE},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: Int 0

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GET_ITEM A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP     A=0x00 K=0x0000 i=F s=F
)",
                 test_case.value_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
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
    // Create a dictionary and set key-value: R[1]["key"] = 42
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x01 B=0x07 C=0x00 kb=T kc=F
1: OP_LOAD_CONSTANT   A=0x02 K=0x0000 i=F s=F
2: OP_LOAD_CONSTANT   A=0x03 K=0x0001 i=F s=F
3: OP_SET_ITEM        A=0x01 B=0x02 C=0x03 kb=F kc=F
4: OP_TRAP            A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: String "key" length=3
K[1]: Int 42
)");

    EXPECT_EQ(result, 0) << "SET_ITEM on dict should succeed";

    // Verify the value was set by using dict's internal functions
    ObjectDict* dict = AS_DICT(&vm->values[1]);
    Value key        = semiValueStringCreate(&vm->gc, "key", 3);
    EXPECT_TRUE(semiDictHas(dict, key)) << "Dictionary should contain the key";
}

TEST_F(VMInstructionCollectionTest, OpSetItemList) {
    struct {
        const char* name;
        int32_t index;
        int32_t expected_final_value;
    } test_cases[] = {
        {"valid_positive_index",  1, 99},
        {"valid_negative_index", -1, 88},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[1024];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[4]: Int 10
R[5]: Int 20
R[6]: Int 30

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x01 B=0x06 C=0x03 kb=T kc=F
1: OP_APPEND_LIST    A=0x01 B=0x04 C=0x03 kb=F kc=F
2: OP_LOAD_CONSTANT  A=0x02 K=0x0000 i=F s=F
3: OP_LOAD_CONSTANT  A=0x03 K=0x0001 i=F s=F
4: OP_SET_ITEM       A=0x01 B=0x02 C=0x03 kb=F kc=F
5: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Int %d
K[1]: Int %d
)",
                 test_case.index,
                 test_case.expected_final_value);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);

        EXPECT_EQ(result, 0) << "SET_ITEM should succeed";

        // Verify the value was set correctly
        ObjectList* list      = AS_LIST(&vm->values[1]);
        uint32_t actual_index = (test_case.index < 0) ? (list->size + test_case.index) : test_case.index;
        EXPECT_EQ(AS_INT(&list->values[actual_index]), test_case.expected_final_value);
    }
}

TEST_F(VMInstructionCollectionTest, OpSetItemListErrors) {
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

        char spec[1024];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[4]: Int 10
R[5]: Int 20

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x01 B=0x06 C=0x02 kb=T kc=F
1: OP_APPEND_LIST    A=0x01 B=0x04 C=0x02 kb=F kc=F
2: OP_LOAD_CONSTANT  A=0x02 K=0x0000 i=F s=F
3: OP_LOAD_CONSTANT  A=0x03 K=0x0001 i=F s=F
4: OP_SET_ITEM       A=0x01 B=0x02 C=0x03 kb=F kc=F
5: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: Int %d
K[1]: Int 999
)",
                 error_case.index);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        EXPECT_EQ(result, error_case.expected_error);
    }
}

TEST_F(VMInstructionCollectionTest, OpSetItemUnsupportedTypes) {
    struct {
        const char* name;
        const char* container_spec;
        ErrorId expected_error;
    } test_cases[] = {
        { "string", "String \"test\"", SEMI_ERROR_UNEXPECTED_TYPE},
        {"integer",          "Int 42", SEMI_ERROR_UNEXPECTED_TYPE},
        {  "float",      "Float 3.14", SEMI_ERROR_UNEXPECTED_TYPE},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[2]: Int 0
R[3]: Int 123

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_SET_ITEM A=0x01 B=0x02 C=0x03 kb=F kc=F
1: OP_TRAP     A=0x00 K=0x0000 i=F s=F
)",
                 test_case.container_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        EXPECT_EQ(result, test_case.expected_error);
    }
}

// Test cases for OP_CONTAIN
TEST_F(VMInstructionCollectionTest, OpContainDict) {
    struct {
        const char* name;
        const char* search_key_spec;
        bool expected_result;
    } test_cases[] = {
        {"existing_string_key",    "String \"key1\"",  true},
        {   "existing_int_key",             "Int 42",  true},
        {    "nonexistent_key", "String \"missing\"", false},
        {     "wrong_type_key",         "Float 42.0", false},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[1024];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x02 B=0x07 C=0x00 kb=T kc=F
1: OP_LOAD_CONSTANT  A=0x03 K=0x0000 i=F s=F
2: OP_LOAD_CONSTANT  A=0x04 K=0x0001 i=F s=F
3: OP_SET_ITEM       A=0x02 B=0x03 C=0x04 kb=F kc=F
4: OP_LOAD_CONSTANT  A=0x03 K=0x0002 i=F s=F
5: OP_LOAD_CONSTANT  A=0x04 K=0x0003 i=F s=F
6: OP_SET_ITEM       A=0x02 B=0x03 C=0x04 kb=F kc=F
7: OP_CONTAIN        A=0x00 B=0x01 C=0x02 kb=F kc=F
8: OP_TRAP           A=0x00 K=0x0000 i=F s=F

[Constants]
K[0]: String "key1" length=4
K[1]: Int 100
K[2]: Int 42
K[3]: Bool true
)",
                 test_case.search_key_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        EXPECT_EQ(result, 0) << "CONTAIN should succeed";
        EXPECT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Result should be boolean";
        EXPECT_EQ(AS_BOOL(&vm->values[0]), test_case.expected_result);
    }
}

TEST_F(VMInstructionCollectionTest, OpContainList) {
    struct {
        const char* name;
        const char* search_value_spec;
        bool expected_result;
    } test_cases[] = {
        {      "existing_int",           "Int 10",  true},
        {   "existing_string", "String \"hello\"",  true},
        {     "existing_bool",        "Bool true",  true},
        {    "existing_float",       "Float 3.14",  true},
        {   "nonexistent_int",           "Int 99", false},
        {"nonexistent_string", "String \"world\"", false},
        {  "nonexistent_bool",       "Bool false", false},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[1024];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: %s
R[4]: Int 10
R[5]: String "hello"
R[6]: Bool true
R[7]: Float 3.14

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x02 B=0x06 C=0x04 kb=T kc=F
1: OP_APPEND_LIST    A=0x02 B=0x04 C=0x04 kb=F kc=F
2: OP_CONTAIN        A=0x00 B=0x01 C=0x02 kb=F kc=F
3: OP_TRAP           A=0x00 K=0x0000 i=F s=F
)",
                 test_case.search_value_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        EXPECT_EQ(result, 0) << "CONTAIN should succeed";
        EXPECT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Result should be boolean";
        EXPECT_EQ(AS_BOOL(&vm->values[0]), test_case.expected_result);
    }
}

TEST_F(VMInstructionCollectionTest, OpContainString) {
    struct {
        const char* name;
        const char* str;
        char search_char;
        bool expected_result;
    } test_cases[] = {
        { "inline_string_char_exists", "hello", 'e',  true},
        {"inline_string_char_missing", "hello", 'x', false},
        { "object_string_char_exists", "world", 'r',  true},
        {"object_string_char_missing", "world", 'z', false},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: String "%c"
R[2]: String "%s"

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_CONTAIN A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP    A=0x00 K=0x0000 i=F s=F
)",
                 test_case.search_char,
                 test_case.str);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
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

        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: String "%s"
R[2]: String "%s"

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_CONTAIN A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP    A=0x00 K=0x0000 i=F s=F
)",
                 test_case.search_str,
                 test_case.str);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        EXPECT_EQ(result, 0) << "CONTAIN should succeed for multi-character search";
        EXPECT_EQ(VALUE_TYPE(&vm->values[0]), VALUE_TYPE_BOOL) << "Result should be boolean";
        EXPECT_EQ(AS_BOOL(&vm->values[0]), test_case.expected_result);
    }
}

TEST_F(VMInstructionCollectionTest, OpContainUnsupportedTypes) {
    struct {
        const char* name;
        const char* container_spec;
        ErrorId expected_error;
    } test_cases[] = {
        {"integer",     "Int 42", SEMI_ERROR_UNEXPECTED_TYPE},
        {  "float", "Float 3.14", SEMI_ERROR_UNEXPECTED_TYPE},
        {"boolean",  "Bool true", SEMI_ERROR_UNEXPECTED_TYPE},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[PreDefine:Registers]
R[1]: Int 123
R[2]: %s

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_CONTAIN A=0x00 B=0x01 C=0x02 kb=F kc=F
1: OP_TRAP    A=0x00 K=0x0000 i=F s=F
)",
                 test_case.container_spec);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        EXPECT_EQ(result, test_case.expected_error);
    }
}

// Test using constants vs registers (kb, kc flags)
TEST_F(VMInstructionCollectionTest, OpCollectionInstructionsWithConstants) {
    // Test GET_ITEM with constant index using kc=true
    // Note: Constant value 1 needs to be encoded as 1 - INT8_MIN = 129 to handle signed->unsigned conversion
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: String "hi"

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_GET_ITEM A=0x00 B=0x01 C=0x81 kb=F kc=T
1: OP_TRAP     A=0x00 K=0x0000 i=F s=F
)");

    EXPECT_EQ(result, 0) << "GET_ITEM with constant should succeed";
    EXPECT_TRUE(IS_INLINE_STRING(&vm->values[0])) << "Result should be inline string";
    EXPECT_EQ(AS_INLINE_STRING(&vm->values[0]).c[0], 'i') << "Should get second character";
}
