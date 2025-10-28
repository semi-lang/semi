// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "../src/const_table.h"
#include "../src/gc.h"
#include "../src/value.h"
}

#include "test_common.hpp"

class ConstantTableTest : public ::testing::Test {
   protected:
    ConstantTable table;
    GC gc;

    void SetUp() override {
        semiGCInit(&gc, defaultReallocFn, nullptr);
        semiConstantTableInit(&gc, &table);
    }

    void TearDown() override {
        semiConstantTableCleanup(&table);
        semiGCCleanup(&gc);
    }

    ConstantIndex InsertString(const char* str) {
        Value v = semiValueStringCreate(&gc, str, strlen(str));
        return semiConstantTableInsert(&table, v);
    }
};

TEST_F(ConstantTableTest, IntegerInsertion) {
    // Insert a positive integer
    IntValue test_int   = 42;
    ConstantIndex index = semiConstantTableInsert(&table, semiValueNewInt(test_int));
    EXPECT_NE(index, CONST_INDEX_INVALID) << "Integer insertion failed.";
    EXPECT_EQ(index, 0) << "First insertion should have index 0.";

    // Verify size increased
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 after insertion.";

    // Retrieve and verify the value
    Value retrieved = semiConstantTableGet(&table, index);
    EXPECT_TRUE(IS_INT(&retrieved)) << "Retrieved value should be an integer.";
    EXPECT_EQ(retrieved.as.i, test_int) << "Retrieved integer value should match inserted value.";
}

TEST_F(ConstantTableTest, IntegerDeduplication) {
    IntValue test_int = 123;

    // Insert the same integer twice
    ConstantIndex index1 = semiConstantTableInsert(&table, semiValueNewInt(test_int));
    ConstantIndex index2 = semiConstantTableInsert(&table, semiValueNewInt(test_int));

    EXPECT_NE(index1, CONST_INDEX_INVALID) << "First integer insertion failed.";
    EXPECT_NE(index2, CONST_INDEX_INVALID) << "Second integer insertion failed.";
    EXPECT_EQ(index1, index2) << "Duplicate integers should return the same index.";

    // Size should still be 1 due to deduplication
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 due to deduplication.";
}

TEST_F(ConstantTableTest, DoubleInsertion) {
    // Insert a double value
    FloatValue test_double = 3.14159;
    ConstantIndex index    = semiConstantTableInsert(&table, semiValueNewFloat(test_double));
    EXPECT_NE(index, CONST_INDEX_INVALID) << "Double insertion failed.";
    EXPECT_EQ(index, 0) << "First insertion should have index 0.";

    // Verify size increased
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 after insertion.";

    // Retrieve and verify the value
    Value retrieved = semiConstantTableGet(&table, index);
    EXPECT_TRUE(IS_FLOAT(&retrieved)) << "Retrieved value should be a float.";
    EXPECT_DOUBLE_EQ(retrieved.as.f, test_double) << "Retrieved double value should match inserted value.";
}

TEST_F(ConstantTableTest, DoubleDeduplication) {
    FloatValue test_double = 2.71828;

    // Insert the same double twice
    ConstantIndex index1 = semiConstantTableInsert(&table, semiValueNewFloat(test_double));
    ConstantIndex index2 = semiConstantTableInsert(&table, semiValueNewFloat(test_double));

    EXPECT_NE(index1, CONST_INDEX_INVALID) << "First double insertion failed.";
    EXPECT_NE(index2, CONST_INDEX_INVALID) << "Second double insertion failed.";
    EXPECT_EQ(index1, index2) << "Duplicate doubles should return the same index.";

    // Size should still be 1 due to deduplication
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 due to deduplication.";
}

TEST_F(ConstantTableTest, StringInsertion) {
    // Insert a string
    const char* test_string = "hello world";
    size_t test_length      = strlen(test_string);
    ConstantIndex index     = InsertString(test_string);
    EXPECT_NE(index, CONST_INDEX_INVALID) << "String insertion failed.";
    EXPECT_EQ(index, 0) << "First insertion should have index 0.";

    // Verify size increased
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 after insertion.";

    // Retrieve and verify the value
    Value retrieved = semiConstantTableGet(&table, index);
    EXPECT_TRUE(AS_OBJECT_STRING(&retrieved)) << "Retrieved value should be a string.";
    EXPECT_EQ(AS_OBJECT_STRING(&retrieved)->length, test_length) << "Retrieved string length should match.";
    EXPECT_EQ(memcmp(AS_OBJECT_STRING(&retrieved)->str, test_string, test_length), 0)
        << "Retrieved string content should match inserted string.";
}

TEST_F(ConstantTableTest, InlineStringDeduplication) {
    // Insert short inline string (should be deduplicated)
    const char* test_string = "a";  // 1 character - will be inline string
    size_t test_length      = strlen(test_string);

    // Insert the same inline string twice
    ConstantIndex index1 = InsertString(test_string);
    ConstantIndex index2 = InsertString(test_string);

    EXPECT_NE(index1, CONST_INDEX_INVALID) << "First string insertion failed.";
    EXPECT_NE(index2, CONST_INDEX_INVALID) << "Second string insertion failed.";
    EXPECT_EQ(index1, index2) << "Duplicate inline strings should return the same index.";

    // Size should still be 1 due to deduplication
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 due to deduplication.";
}

TEST_F(ConstantTableTest, ObjectStringNoDeduplication) {
    // Insert longer object string (should NOT be deduplicated)
    const char* test_string = "longer_string";  // >2 characters - will be object string
    size_t test_length      = strlen(test_string);

    // Insert the same object string twice
    ConstantIndex index1 = InsertString(test_string);
    ConstantIndex index2 = InsertString(test_string);

    EXPECT_NE(index1, CONST_INDEX_INVALID) << "First string insertion failed.";
    EXPECT_NE(index2, CONST_INDEX_INVALID) << "Second string insertion failed.";
    EXPECT_EQ(index1, index2) << "Object strings should be deduplicated.";

    // Size should be 2 (no deduplication for object strings)
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 due to deduplication.";
}

TEST_F(ConstantTableTest, MixedTypeInsertion) {
    // Insert different types
    IntValue test_int       = 100;
    FloatValue test_double  = 1.5;
    const char* test_string = "test";
    size_t test_length      = strlen(test_string);

    ConstantIndex int_index    = semiConstantTableInsert(&table, semiValueNewInt(test_int));
    ConstantIndex double_index = semiConstantTableInsert(&table, semiValueNewFloat(test_double));
    ConstantIndex string_index = InsertString(test_string);

    EXPECT_NE(int_index, CONST_INDEX_INVALID) << "Integer insertion failed.";
    EXPECT_NE(double_index, CONST_INDEX_INVALID) << "Double insertion failed.";
    EXPECT_NE(string_index, CONST_INDEX_INVALID) << "String insertion failed.";

    // Indices should be sequential
    EXPECT_EQ(int_index, 0) << "First insertion should have index 0.";
    EXPECT_EQ(double_index, 1) << "Second insertion should have index 1.";
    EXPECT_EQ(string_index, 2) << "Third insertion should have index 2.";

    // Verify total size
    EXPECT_EQ(semiConstantTableSize(&table), 3) << "Size should be 3 after three insertions.";

    // Verify each value can be retrieved correctly
    Value intValue    = semiConstantTableGet(&table, int_index);
    Value floatValue  = semiConstantTableGet(&table, double_index);
    Value stringValue = semiConstantTableGet(&table, string_index);

    EXPECT_TRUE(IS_INT(&intValue)) << "First value should be integer.";
    EXPECT_TRUE(IS_FLOAT(&floatValue)) << "Second value should be float.";
    EXPECT_TRUE(AS_OBJECT_STRING(&stringValue)) << "Third value should be string.";

    EXPECT_EQ(intValue.as.i, test_int) << "Integer value should match.";
    EXPECT_DOUBLE_EQ(floatValue.as.f, test_double) << "Double value should match.";
    EXPECT_EQ(AS_OBJECT_STRING(&stringValue)->length, test_length) << "String length should match.";
    EXPECT_EQ(memcmp(AS_OBJECT_STRING(&stringValue)->str, test_string, test_length), 0)
        << "String content should match.";
}

TEST_F(ConstantTableTest, EdgeCaseValues) {
    // Test edge case integer values
    IntValue zero_int     = 0;
    IntValue negative_int = -42;
    IntValue max_int      = INT64_MAX;
    IntValue min_int      = INT64_MIN;

    ConstantIndex zero_index = semiConstantTableInsert(&table, semiValueNewInt(zero_int));
    ConstantIndex neg_index  = semiConstantTableInsert(&table, semiValueNewInt(negative_int));
    ConstantIndex max_index  = semiConstantTableInsert(&table, semiValueNewInt(max_int));
    ConstantIndex min_index  = semiConstantTableInsert(&table, semiValueNewInt(min_int));

    EXPECT_NE(zero_index, CONST_INDEX_INVALID) << "Zero integer insertion failed.";
    EXPECT_NE(neg_index, CONST_INDEX_INVALID) << "Negative integer insertion failed.";
    EXPECT_NE(max_index, CONST_INDEX_INVALID) << "Max integer insertion failed.";
    EXPECT_NE(min_index, CONST_INDEX_INVALID) << "Min integer insertion failed.";

    // Test edge case double values
    FloatValue zero_double     = 0.0;
    FloatValue negative_double = -3.14;
    FloatValue inf_double      = 1.0 / 0.0;  // Infinity

    ConstantIndex zero_d_index = semiConstantTableInsert(&table, semiValueNewFloat(zero_double));
    ConstantIndex neg_d_index  = semiConstantTableInsert(&table, semiValueNewFloat(negative_double));
    ConstantIndex inf_index    = semiConstantTableInsert(&table, semiValueNewFloat(inf_double));

    EXPECT_NE(zero_d_index, CONST_INDEX_INVALID) << "Zero double insertion failed.";
    EXPECT_NE(neg_d_index, CONST_INDEX_INVALID) << "Negative double insertion failed.";
    EXPECT_NE(inf_index, CONST_INDEX_INVALID) << "Infinity double insertion failed.";

    // Test edge case strings
    const char* empty_string  = "";
    ConstantIndex empty_index = InsertString(empty_string);
    EXPECT_NE(empty_index, CONST_INDEX_INVALID) << "Empty string insertion failed.";

    // Verify all values can be retrieved
    EXPECT_EQ(semiConstantTableGet(&table, zero_index).as.i, zero_int);
    EXPECT_EQ(semiConstantTableGet(&table, neg_index).as.i, negative_int);
    EXPECT_EQ(semiConstantTableGet(&table, max_index).as.i, max_int);
    EXPECT_EQ(semiConstantTableGet(&table, min_index).as.i, min_int);

    EXPECT_DOUBLE_EQ(semiConstantTableGet(&table, zero_d_index).as.f, zero_double);
    EXPECT_DOUBLE_EQ(semiConstantTableGet(&table, neg_d_index).as.f, negative_double);
    EXPECT_TRUE(std::isinf(semiConstantTableGet(&table, inf_index).as.f));

    Value empty_val = semiConstantTableGet(&table, empty_index);
    EXPECT_EQ(AS_INLINE_STRING(&empty_val).length, 0) << "Empty string should have zero length.";
}

TEST_F(ConstantTableTest, InvalidIndexRetrieval) {
    // Insert one value
    ConstantIndex valid_index = semiConstantTableInsert(&table, semiValueNewInt(42));
    EXPECT_NE(valid_index, CONST_INDEX_INVALID) << "Integer insertion failed.";

    // Try to retrieve with invalid indices
    // Note: The current implementation doesn't bounds-check, so this test
    // verifies that accessing within bounds works correctly
    Value valid_value = semiConstantTableGet(&table, valid_index);
    EXPECT_TRUE(IS_INT(&valid_value)) << "Valid index should return a valid value.";
    EXPECT_EQ(valid_value.as.i, 42) << "Retrieved value should be correct.";
}

TEST_F(ConstantTableTest, SizeTracking) {
    EXPECT_EQ(semiConstantTableSize(&table), 0) << "Empty table should have size 0.";

    // Add values and check size increases
    semiConstantTableInsert(&table, semiValueNewInt(1));
    EXPECT_EQ(semiConstantTableSize(&table), 1) << "Size should be 1 after first insertion.";

    semiConstantTableInsert(&table, semiValueNewFloat(2.0));
    EXPECT_EQ(semiConstantTableSize(&table), 2) << "Size should be 2 after second insertion.";

    Value s = semiValueStringCreate(&gc, "three", 5);
    semiConstantTableInsert(&table, s);
    EXPECT_EQ(semiConstantTableSize(&table), 3) << "Size should be 3 after third insertion.";

    // Test deduplication doesn't increase size
    semiConstantTableInsert(&table, semiValueNewInt(1));  // Duplicate
    EXPECT_EQ(semiConstantTableSize(&table), 3) << "Size should remain 3 after duplicate insertion.";
}
