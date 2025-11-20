// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "../src/gc.h"
#include "../src/primitives.h"
#include "../src/value.h"
}

#include "test_common.hpp"

class ObjectValueDictTest : public ::testing::Test {
   protected:
    GC gc;

    void SetUp() override {
        semiGCInit(&gc, defaultReallocFn, NULL);
    }

    void TearDown() override {
        semiGCCleanup(&gc);
    }

    // Helper to create values for testing
    Value createStringValue(const char* str) {
        return semiValueStringCreate(&gc, str, strlen(str));
    }

    Value createUnsetValue() {
        return (Value){.header = VALUE_TYPE_UNSET};
    }
};

// Test dictionary creation
TEST_F(ObjectValueDictTest, CreateDict) {
    Value dict_val = semiValueDictCreate(&gc);

    ASSERT_FALSE(IS_INVALID(&dict_val));
    ASSERT_TRUE(IS_DICT(&dict_val));

    ObjectDict* dict = AS_DICT(&dict_val);
    ASSERT_EQ(semiDictLen(dict), 0);
    ASSERT_EQ(dict->keys, nullptr);
    ASSERT_EQ(dict->tids, nullptr);
    ASSERT_EQ(dict->values, nullptr);
    ASSERT_EQ(dict->indexSize, 0);
    ASSERT_EQ(dict->used, 0);
}

// Test basic set/get operations with different value types
struct BasicSetGetTestCase {
    const char* name;
    Value key;
    Value value;
};

TEST_F(ObjectValueDictTest, BasicSetGet) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    BasicSetGetTestCase test_cases[] = {
        {"int_key_int_value", semiValueIntCreate(42), semiValueIntCreate(100)},
        {"int_key_bool_value", semiValueIntCreate(1), semiValueBoolCreate(true)},
        {"int_key_float_value", semiValueIntCreate(2), semiValueFloatCreate(3.14)},
        {"bool_key_int_value", semiValueBoolCreate(false), semiValueIntCreate(200)},
        {"bool_key_bool_value", semiValueBoolCreate(true), semiValueBoolCreate(false)},
        {"float_key_int_value", semiValueFloatCreate(1.5), semiValueIntCreate(300)},
        {"inline_str_key", INLINE_STRING_VALUE_1('a'), semiValueIntCreate(400)},
        {"inline_str2_key", INLINE_STRING_VALUE_2('x', 'y'), semiValueIntCreate(500)}
    };

    for (const auto& test_case : test_cases) {
        ASSERT_TRUE(semiDictSet(&gc, dict, test_case.key, test_case.value)) << "Set failed for " << test_case.name;

        ASSERT_TRUE(semiDictHas(dict, test_case.key)) << "Has failed for " << test_case.name;

        Value retrieved = semiDictGet(dict, test_case.key);
        ASSERT_FALSE(IS_INVALID(&retrieved)) << "Get returned uninit for " << test_case.name;

        ASSERT_TRUE(semiBuiltInEquals(retrieved, test_case.value)) << "Value mismatch for " << test_case.name;
    }

    ASSERT_EQ(semiDictLen(dict), 8);
}

// Test object string keys
TEST_F(ObjectValueDictTest, ObjectStringKeys) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    Value str1 = createStringValue("hello");
    Value str2 = createStringValue("world");
    Value str3 = createStringValue("test");

    Value val1 = semiValueIntCreate(10);
    Value val2 = semiValueIntCreate(20);
    Value val3 = semiValueIntCreate(30);

    ASSERT_TRUE(semiDictSet(&gc, dict, str1, val1));
    ASSERT_TRUE(semiDictSet(&gc, dict, str2, val2));
    ASSERT_TRUE(semiDictSet(&gc, dict, str3, val3));

    ASSERT_TRUE(semiDictHas(dict, str1));
    ASSERT_TRUE(semiDictHas(dict, str2));
    ASSERT_TRUE(semiDictHas(dict, str3));

    Value retrieved1 = semiDictGet(dict, str1);
    Value retrieved2 = semiDictGet(dict, str2);
    Value retrieved3 = semiDictGet(dict, str3);

    ASSERT_TRUE(semiBuiltInEquals(retrieved1, val1));
    ASSERT_TRUE(semiBuiltInEquals(retrieved2, val2));
    ASSERT_TRUE(semiBuiltInEquals(retrieved3, val3));

    ASSERT_EQ(semiDictLen(dict), 3);
}

// Test updating existing keys
TEST_F(ObjectValueDictTest, UpdateExistingKey) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    Value key          = semiValueIntCreate(42);
    Value original_val = semiValueIntCreate(100);
    Value updated_val  = semiValueIntCreate(200);

    // Set original value
    ASSERT_TRUE(semiDictSet(&gc, dict, key, original_val));
    ASSERT_EQ(semiDictLen(dict), 1);

    Value retrieved = semiDictGet(dict, key);
    ASSERT_TRUE(semiBuiltInEquals(retrieved, original_val));

    // Update with new value
    ASSERT_TRUE(semiDictSet(&gc, dict, key, updated_val));
    ASSERT_EQ(semiDictLen(dict), 1);  // Length should not change

    retrieved = semiDictGet(dict, key);
    ASSERT_TRUE(semiBuiltInEquals(retrieved, updated_val));
}

// Test deletion
struct DeletionTestCase {
    const char* name;
    Value key;
    Value value;
};

TEST_F(ObjectValueDictTest, Deletion) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    DeletionTestCase test_cases[] = {
        {       "int_key",      semiValueIntCreate(1), semiValueIntCreate(10)},
        {      "bool_key",  semiValueBoolCreate(true), semiValueIntCreate(20)},
        {     "float_key", semiValueFloatCreate(3.14), semiValueIntCreate(30)},
        {"inline_str_key", INLINE_STRING_VALUE_1('z'), semiValueIntCreate(40)}
    };

    // Set all values
    for (const auto& test_case : test_cases) {
        ASSERT_TRUE(semiDictSet(&gc, dict, test_case.key, test_case.value));
    }
    ASSERT_EQ(semiDictLen(dict), 4);

    // Delete each value and verify
    for (const auto& test_case : test_cases) {
        Value deleted = semiDictDelete(&gc, dict, test_case.key);
        ASSERT_FALSE(IS_INVALID(&deleted)) << "Delete failed for " << test_case.name;

        ASSERT_FALSE(semiDictHas(dict, test_case.key)) << "Key still exists after delete for " << test_case.name;

        Value retrieved = semiDictGet(dict, test_case.key);
        ASSERT_TRUE(IS_INVALID(&retrieved)) << "Get should return uninit after delete for " << test_case.name;
    }

    ASSERT_EQ(semiDictLen(dict), 0);
}

// Test operations on empty dictionary
TEST_F(ObjectValueDictTest, EmptyDictOperations) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    Value test_key = semiValueIntCreate(42);

    // Test operations on empty dict
    ASSERT_FALSE(semiDictHas(dict, test_key));

    Value retrieved = semiDictGet(dict, test_key);
    ASSERT_TRUE(IS_INVALID(&retrieved));

    Value deleted = semiDictDelete(&gc, dict, test_key);
    ASSERT_TRUE(IS_INVALID(&deleted));

    ASSERT_EQ(semiDictLen(dict), 0);
}

// Test non-existent key operations
TEST_F(ObjectValueDictTest, NonExistentKeyOperations) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    // Add one item
    Value existing_key = semiValueIntCreate(1);
    Value existing_val = semiValueIntCreate(100);
    ASSERT_TRUE(semiDictSet(&gc, dict, existing_key, existing_val));

    // Test operations on non-existent key
    Value nonexistent_key = semiValueIntCreate(999);

    ASSERT_FALSE(semiDictHas(dict, nonexistent_key));

    Value retrieved = semiDictGet(dict, nonexistent_key);
    ASSERT_TRUE(IS_INVALID(&retrieved));

    Value deleted = semiDictDelete(&gc, dict, nonexistent_key);
    ASSERT_TRUE(IS_INVALID(&deleted));

    // Original item should still exist
    ASSERT_TRUE(semiDictHas(dict, existing_key));
    ASSERT_EQ(semiDictLen(dict), 1);
}

// Test hash collision scenario with inline string and integer
TEST_F(ObjectValueDictTest, HashCollision) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    // Create inline string 'ab'
    Value inline_str = INLINE_STRING_VALUE_2('a', 'b');

    // Create integer with same hash: a=97, b=98, VALUE_TYPE_INLINE_STRING=4
    // Hash = (97 << 0) | (98 << 8) | (4 << 16) = 97 + 25088 + 262144 = 287329
    int64_t collision_int   = 97 + (98 << 8) + (4 << 16);  // Should be 287329
    Value collision_int_val = semiValueIntCreate(collision_int);

    Value str_value = semiValueIntCreate(1000);
    Value int_value = semiValueIntCreate(2000);

    // Set both values
    ASSERT_TRUE(semiDictSet(&gc, dict, inline_str, str_value));
    ASSERT_TRUE(semiDictSet(&gc, dict, collision_int_val, int_value));

    // Both should exist independently
    ASSERT_TRUE(semiDictHas(dict, inline_str));
    ASSERT_TRUE(semiDictHas(dict, collision_int_val));

    Value retrieved_str = semiDictGet(dict, inline_str);
    Value retrieved_int = semiDictGet(dict, collision_int_val);

    ASSERT_TRUE(semiBuiltInEquals(retrieved_str, str_value));
    ASSERT_TRUE(semiBuiltInEquals(retrieved_int, int_value));

    ASSERT_EQ(semiDictLen(dict), 2);
}

// Test dictionary growth/resizing
TEST_F(ObjectValueDictTest, DictionaryGrowth) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    const int num_items = 50;  // Enough to trigger multiple resizes

    // Insert many items
    for (int i = 0; i < num_items; i++) {
        Value key   = semiValueIntCreate(i);
        Value value = semiValueIntCreate(i * 10);
        ASSERT_TRUE(semiDictSet(&gc, dict, key, value)) << "Set failed for key " << i;
    }

    ASSERT_EQ(semiDictLen(dict), num_items);
    ASSERT_GT(dict->indexSize, 0);  // Should have allocated index table
    ASSERT_NE(dict->keys, nullptr);
    ASSERT_NE(dict->tids, nullptr);
    ASSERT_NE(dict->values, nullptr);

    // Verify all items are still accessible
    for (int i = 0; i < num_items; i++) {
        Value key            = semiValueIntCreate(i);
        Value expected_value = semiValueIntCreate(i * 10);

        ASSERT_TRUE(semiDictHas(dict, key)) << "Has failed for key " << i;

        Value retrieved = semiDictGet(dict, key);
        ASSERT_TRUE(semiBuiltInEquals(retrieved, expected_value)) << "Value mismatch for key " << i;
    }
}

// Test setting unset values
TEST_F(ObjectValueDictTest, UnsetValues) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    Value key         = semiValueIntCreate(42);
    Value unset_value = createUnsetValue();

    // Should be able to set uninit value
    ASSERT_TRUE(semiDictSet(&gc, dict, key, unset_value));
    ASSERT_TRUE(semiDictHas(dict, key));
    ASSERT_EQ(semiDictLen(dict), 1);

    Value retrieved = semiDictGet(dict, key);
    ASSERT_TRUE(IS_VALID(&retrieved));
}

// Test mixed operations scenario
TEST_F(ObjectValueDictTest, MixedOperationsScenario) {
    ObjectDict* dict = semiObjectDictCreate(&gc);

    // Add several items
    Value k1 = semiValueIntCreate(1);
    Value k2 = semiValueBoolCreate(true);
    Value k3 = semiValueFloatCreate(2.5);
    Value k4 = INLINE_STRING_VALUE_1('x');

    Value v1 = semiValueIntCreate(10);
    Value v2 = semiValueIntCreate(20);
    Value v3 = semiValueIntCreate(30);
    Value v4 = semiValueIntCreate(40);

    ASSERT_TRUE(semiDictSet(&gc, dict, k1, v1));
    ASSERT_TRUE(semiDictSet(&gc, dict, k2, v2));
    ASSERT_TRUE(semiDictSet(&gc, dict, k3, v3));
    ASSERT_TRUE(semiDictSet(&gc, dict, k4, v4));
    ASSERT_EQ(semiDictLen(dict), 4);

    // Delete middle items
    Value deleted2 = semiDictDelete(&gc, dict, k2);
    ASSERT_FALSE(IS_INVALID(&deleted2));
    Value deleted3 = semiDictDelete(&gc, dict, k3);
    ASSERT_FALSE(IS_INVALID(&deleted3));
    ASSERT_EQ(semiDictLen(dict), 2);

    // Verify remaining items
    ASSERT_TRUE(semiDictHas(dict, k1));
    ASSERT_TRUE(semiDictHas(dict, k4));
    ASSERT_FALSE(semiDictHas(dict, k2));
    ASSERT_FALSE(semiDictHas(dict, k3));

    // Add new items to potentially reuse slots
    Value k5 = semiValueIntCreate(5);
    Value k6 = semiValueBoolCreate(false);
    Value v5 = semiValueIntCreate(50);
    Value v6 = semiValueIntCreate(60);

    ASSERT_TRUE(semiDictSet(&gc, dict, k5, v5));
    ASSERT_TRUE(semiDictSet(&gc, dict, k6, v6));
    ASSERT_EQ(semiDictLen(dict), 4);

    // Update existing item
    Value new_v1 = semiValueIntCreate(11);
    ASSERT_TRUE(semiDictSet(&gc, dict, k1, new_v1));
    ASSERT_EQ(semiDictLen(dict), 4);  // Should not change

    Value retrieved = semiDictGet(dict, k1);
    ASSERT_TRUE(semiBuiltInEquals(retrieved, new_v1));
}
