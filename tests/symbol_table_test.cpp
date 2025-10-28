// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>

extern "C" {
#include "../src/symbol_table.h"
}

#include "test_common.hpp"

class SymbolTableTest : public ::testing::Test {
   protected:
    GC gc;
    SymbolTable table;

    void SetUp() override {
        semiGCInit(&gc, defaultReallocFn, nullptr);
        semiSymbolTableInit(&gc, &table);
    }

    void TearDown() override {
        semiSymbolTableCleanup(&table);
        semiGCCleanup(&gc);
    }
};

TEST_F(SymbolTableTest, InsertSingleString) {
    const char* test_str = "hello";
    size_t len           = strlen(test_str);

    char* result = semiSymbolTableInsert(&table, test_str, len);
    ASSERT_NE(result, nullptr) << "String insertion failed";
    ASSERT_EQ(memcmp(result, test_str, len), 0) << "Inserted string content mismatch";
}

TEST_F(SymbolTableTest, InsertMultipleStrings) {
    const char* str1 = "first";
    const char* str2 = "second";
    const char* str3 = "third";

    char* result1 = semiSymbolTableInsert(&table, str1, strlen(str1));
    char* result2 = semiSymbolTableInsert(&table, str2, strlen(str2));
    char* result3 = semiSymbolTableInsert(&table, str3, strlen(str3));

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_NE(result3, nullptr);

    ASSERT_EQ(memcmp(result1, str1, strlen(str1)), 0);
    ASSERT_EQ(memcmp(result2, str2, strlen(str2)), 0);
    ASSERT_EQ(memcmp(result3, str3, strlen(str3)), 0);
}

TEST_F(SymbolTableTest, StringDeduplication) {
    const char* test_str = "duplicate_me";
    size_t len           = strlen(test_str);

    char* result1 = semiSymbolTableInsert(&table, test_str, len);
    char* result2 = semiSymbolTableInsert(&table, test_str, len);

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_EQ(result1, result2) << "Identical strings should return same pointer for deduplication";
}

TEST_F(SymbolTableTest, DifferentStringsReturnDifferentPointers) {
    const char* str1 = "string_one";
    const char* str2 = "string_two";

    char* result1 = semiSymbolTableInsert(&table, str1, strlen(str1));
    char* result2 = semiSymbolTableInsert(&table, str2, strlen(str2));

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_NE(result1, result2) << "Different strings should return different pointers";
}

TEST_F(SymbolTableTest, EmptyString) {
    const char* empty_str = "";
    size_t len            = 0;

    char* result = semiSymbolTableInsert(&table, empty_str, len);
    ASSERT_EQ(result, nullptr) << "Empty string insertion should fail";
}

TEST_F(SymbolTableTest, StringsWithSameLength) {
    const char* str1 = "abcde";
    const char* str2 = "fghij";
    const char* str3 = "klmno";

    char* result1 = semiSymbolTableInsert(&table, str1, strlen(str1));
    char* result2 = semiSymbolTableInsert(&table, str2, strlen(str2));
    char* result3 = semiSymbolTableInsert(&table, str3, strlen(str3));

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_NE(result3, nullptr);

    ASSERT_NE(result1, result2);
    ASSERT_NE(result2, result3);
    ASSERT_NE(result1, result3);
}

TEST_F(SymbolTableTest, HashCollisionHandling) {
    const char* str1 = "collision_test_1";
    const char* str2 = "collision_test_2";
    const char* str3 = "collision_test_3";

    char* result1 = semiSymbolTableInsert(&table, str1, strlen(str1));
    char* result2 = semiSymbolTableInsert(&table, str2, strlen(str2));
    char* result3 = semiSymbolTableInsert(&table, str3, strlen(str3));

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_NE(result3, nullptr);

    ASSERT_EQ(memcmp(result1, str1, strlen(str1)), 0);
    ASSERT_EQ(memcmp(result2, str2, strlen(str2)), 0);
    ASSERT_EQ(memcmp(result3, str3, strlen(str3)), 0);
}

TEST_F(SymbolTableTest, StringLength) {
    const char* test_str = "length_test";
    size_t expected_len  = strlen(test_str);

    char* result = semiSymbolTableInsert(&table, test_str, expected_len);
    ASSERT_NE(result, nullptr);

    size_t actual_len = semiSymbolTableLength(result);
    ASSERT_EQ(actual_len, expected_len) << "String length mismatch";
}

TEST_F(SymbolTableTest, MultipleInsertsWithDeduplication) {
    const char* common_str  = "common";
    const char* unique_str1 = "unique1";
    const char* unique_str2 = "unique2";

    char* common1 = semiSymbolTableInsert(&table, common_str, strlen(common_str));
    char* unique1 = semiSymbolTableInsert(&table, unique_str1, strlen(unique_str1));
    char* common2 = semiSymbolTableInsert(&table, common_str, strlen(common_str));
    char* unique2 = semiSymbolTableInsert(&table, unique_str2, strlen(unique_str2));
    char* common3 = semiSymbolTableInsert(&table, common_str, strlen(common_str));

    ASSERT_NE(common1, nullptr);
    ASSERT_NE(unique1, nullptr);
    ASSERT_NE(common2, nullptr);
    ASSERT_NE(unique2, nullptr);
    ASSERT_NE(common3, nullptr);

    ASSERT_EQ(common1, common2) << "First and second insertion of same string should be identical";
    ASSERT_EQ(common2, common3) << "Second and third insertion of same string should be identical";
    ASSERT_NE(unique1, unique2) << "Different strings should have different pointers";
    ASSERT_NE(common1, unique1) << "Common and unique strings should have different pointers";
}

TEST_F(SymbolTableTest, BoundaryLength) {
    size_t boundary_size = 255;
    char* boundary_str   = (char*)malloc(boundary_size + 1);
    ASSERT_NE(boundary_str, nullptr);

    memset(boundary_str, 'B', boundary_size);
    boundary_str[boundary_size] = '\0';

    char* result = semiSymbolTableInsert(&table, boundary_str, boundary_size);
    ASSERT_NE(result, nullptr) << "Boundary length string insertion failed";
    ASSERT_EQ(memcmp(result, boundary_str, boundary_size), 0) << "Boundary length string content mismatch";

    free(boundary_str);
}

TEST_F(SymbolTableTest, SingleCharacterStrings) {
    char single_chars[] = {'a', 'b', 'c', 'd', 'e'};
    char* results[5];

    for (int i = 0; i < 5; i++) {
        results[i] = semiSymbolTableInsert(&table, &single_chars[i], 1);
        ASSERT_NE(results[i], nullptr) << "Single character string insertion failed for char: " << single_chars[i];
        ASSERT_EQ(results[i][0], single_chars[i]) << "Single character string content mismatch";
    }

    for (int i = 0; i < 5; i++) {
        for (int j = i + 1; j < 5; j++) {
            ASSERT_NE(results[i], results[j]) << "Different single character strings should have different pointers";
        }
    }
}

TEST_F(SymbolTableTest, IdentifierIdAssignment) {
    const char* str1 = "first_id";
    const char* str2 = "second_id";
    const char* str3 = "third_id";

    char* result1 = semiSymbolTableInsert(&table, str1, strlen(str1));
    char* result2 = semiSymbolTableInsert(&table, str2, strlen(str2));
    char* result3 = semiSymbolTableInsert(&table, str3, strlen(str3));

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_NE(result3, nullptr);

    IdentifierId id1 = semiSymbolTableGetId(result1);
    IdentifierId id2 = semiSymbolTableGetId(result2);
    IdentifierId id3 = semiSymbolTableGetId(result3);

    ASSERT_EQ(id1, MAX_RESERVED_IDENTIFIER_ID + 1)
        << "First identifier should get ID " << (MAX_RESERVED_IDENTIFIER_ID + 1);
    ASSERT_EQ(id2, MAX_RESERVED_IDENTIFIER_ID + 2)
        << "Second identifier should get ID " << (MAX_RESERVED_IDENTIFIER_ID + 2);
    ASSERT_EQ(id3, MAX_RESERVED_IDENTIFIER_ID + 3)
        << "Third identifier should get ID " << (MAX_RESERVED_IDENTIFIER_ID + 3);
}

TEST_F(SymbolTableTest, DuplicateStringsSameId) {
    const char* test_str = "duplicate_for_id";
    size_t len           = strlen(test_str);

    char* result1 = semiSymbolTableInsert(&table, test_str, len);
    char* result2 = semiSymbolTableInsert(&table, test_str, len);

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_EQ(result1, result2) << "Duplicate strings should return same pointer";

    IdentifierId id1 = semiSymbolTableGetId(result1);
    IdentifierId id2 = semiSymbolTableGetId(result2);

    ASSERT_EQ(id1, id2) << "Duplicate strings should have the same identifier ID";
    ASSERT_EQ(id1, MAX_RESERVED_IDENTIFIER_ID + 1)
        << "First unique string should get ID " << (MAX_RESERVED_IDENTIFIER_ID + 1);
}
