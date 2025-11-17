// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/value.h"
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionCollectionInitializerTest : public VMTest {};

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionEmptyList) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0x00 kb=T kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION for empty list should succeed";
    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionEmptyDict) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x07 C=0x00 kb=T kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION for empty dict should succeed";
    EXPECT_TRUE(IS_DICT(&vm->values[0])) << "Result should be a dict";
    ObjectDict* dict = AS_DICT(&vm->values[0]);
    EXPECT_EQ(dict->len, 0) << "Dict should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListWithCapacity) {
    struct {
        const char* name;
        uint8_t capacity;
    } test_cases[] = {
        {  "capacity_1",   1},
        {  "capacity_5",   5},
        { "capacity_10",  10},
        {"capacity_100", 100},
    };

    for (const auto& test_case : test_cases) {
        SCOPED_TRACE(test_case.name);

        char spec[512];
        snprintf(spec,
                 sizeof(spec),
                 R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0x%02X kb=T kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)",
                 test_case.capacity);

        ErrorId result = InstructionVerifier::BuildAndRunModule(vm, spec);
        EXPECT_EQ(result, 0) << "NEW_COLLECTION with capacity should succeed";

        EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
        ObjectList* list = AS_LIST(&vm->values[0]);
        EXPECT_EQ(list->size, 0) << "List should be empty (size)";
        EXPECT_GE(list->capacity, test_case.capacity) << "List capacity should be at least " << test_case.capacity;
    }
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListTypeFromRegister) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 6

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x01 C=0x05 kb=F kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION with type from register should succeed";
    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionDictTypeFromRegister) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 7

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x01 C=0x00 kb=F kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION with dict type from register should succeed";
    EXPECT_TRUE(IS_DICT(&vm->values[0])) << "Result should be a dict";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionInvalidTypeFromRegister) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Float 3.14

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x01 C=0x00 kb=F kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE) << "NEW_COLLECTION with non-integer type should fail";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionUnsupportedType) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x63 C=0x00 kb=T kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, SEMI_ERROR_UNIMPLEMENTED_FEATURE) << "NEW_COLLECTION with unsupported type should fail";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionMultipleListsInDifferentRegisters) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0x02 kb=T kc=F
1: OP_NEW_COLLECTION A=0x01 B=0x06 C=0x05 kb=T kc=F
2: OP_NEW_COLLECTION A=0x02 B=0x06 C=0x0A kb=T kc=F
3: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "Multiple NEW_COLLECTION operations should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "R[0] should be a list";
    EXPECT_TRUE(IS_LIST(&vm->values[1])) << "R[1] should be a list";
    EXPECT_TRUE(IS_LIST(&vm->values[2])) << "R[2] should be a list";

    EXPECT_GE(AS_LIST(&vm->values[0])->capacity, 2) << "R[0] capacity should be at least 2";
    EXPECT_GE(AS_LIST(&vm->values[1])->capacity, 5) << "R[1] capacity should be at least 5";
    EXPECT_GE(AS_LIST(&vm->values[2])->capacity, 10) << "R[2] capacity should be at least 10";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionMixedTypes) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0x03 kb=T kc=F
1: OP_NEW_COLLECTION A=0x01 B=0x07 C=0x00 kb=T kc=F
2: OP_NEW_COLLECTION A=0x02 B=0x06 C=0x01 kb=T kc=F
3: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "Creating mixed collection types should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "R[0] should be a list";
    EXPECT_TRUE(IS_DICT(&vm->values[1])) << "R[1] should be a dict";
    EXPECT_TRUE(IS_LIST(&vm->values[2])) << "R[2] should be a list";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListMaxCapacity) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0xFE kb=T kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION with max capacity (254) should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_GE(list->capacity, 254) << "List capacity should be at least 254";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListInvalidRegisterCapacity) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0xFF kb=T kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION with INVALID_LOCAL_REGISTER_ID as capacity should use default capacity";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListZeroCapacity) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0x00 kb=T kc=F
1: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION with zero capacity should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionWithSubsequentAppend) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: Int 42
R[2]: Int 100

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x06 C=0x02 kb=T kc=F
1: OP_APPEND_LIST    A=0x00 B=0x01 C=0x02 kb=F kc=F
2: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION followed by APPEND should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 2) << "List should contain 2 elements";
    EXPECT_EQ(AS_INT(&list->values[0]), 42);
    EXPECT_EQ(AS_INT(&list->values[1]), 100);
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionDictWithSubsequentSetItem) {
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm, R"(
[PreDefine:Registers]
R[1]: String "key"
R[2]: Int 123

[ModuleInit]
arity=0 coarity=0 maxStackSize=8

[Instructions]
0: OP_NEW_COLLECTION A=0x00 B=0x07 C=0x00 kb=T kc=F
1: OP_SET_ITEM       A=0x00 B=0x01 C=0x02 kb=F kc=F
2: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
)");

    EXPECT_EQ(result, 0) << "NEW_COLLECTION dict followed by SET_ITEM should succeed";

    EXPECT_TRUE(IS_DICT(&vm->values[0])) << "Result should be a dict";
    ObjectDict* dict = AS_DICT(&vm->values[0]);
    EXPECT_EQ(dict->len, 1) << "Dict should contain 1 element";

    Value key = semiValueStringCreate(&vm->gc, "key", 3);
    EXPECT_TRUE(semiDictHas(dict, key)) << "Dict should contain the key";
}
