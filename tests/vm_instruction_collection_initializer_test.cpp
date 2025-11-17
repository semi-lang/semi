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
    vm->error = 0;

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, 0, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION for empty list should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionEmptyDict) {
    vm->error = 0;

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_DICT, 0, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
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

        vm->error = 0;

        Instruction code[2];
        code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, test_case.capacity, true, false);
        code[1] = INSTRUCTION_TRAP(0, 0, false, false);

        SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
        FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
        module->moduleInit  = func;

        ErrorId result = RunModule(module);
        EXPECT_EQ(result, 0) << "NEW_COLLECTION with capacity should succeed";

        EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
        ObjectList* list = AS_LIST(&vm->values[0]);
        EXPECT_EQ(list->size, 0) << "List should be empty (size)";
        EXPECT_GE(list->capacity, test_case.capacity) << "List capacity should be at least " << test_case.capacity;
    }
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListTypeFromRegister) {
    vm->error = 0;

    vm->values[1] = semiValueNewInt(BASE_VALUE_TYPE_LIST);

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, 1, 5, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION with type from register should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionDictTypeFromRegister) {
    vm->error = 0;

    vm->values[1] = semiValueNewInt(BASE_VALUE_TYPE_DICT);

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, 1, 0, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION with dict type from register should succeed";

    EXPECT_TRUE(IS_DICT(&vm->values[0])) << "Result should be a dict";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionInvalidTypeFromRegister) {
    vm->error = 0;

    vm->values[1] = semiValueNewFloat(3.14f);

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, 1, 0, false, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, SEMI_ERROR_UNEXPECTED_TYPE) << "NEW_COLLECTION with non-integer type should fail";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionUnsupportedType) {
    vm->error = 0;

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, 99, 0, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, SEMI_ERROR_UNIMPLEMENTED_FEATURE) << "NEW_COLLECTION with unsupported type should fail";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionMultipleListsInDifferentRegisters) {
    vm->error = 0;

    Instruction code[4];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, 2, true, false);
    code[1] = INSTRUCTION_NEW_COLLECTION(1, BASE_VALUE_TYPE_LIST, 5, true, false);
    code[2] = INSTRUCTION_NEW_COLLECTION(2, BASE_VALUE_TYPE_LIST, 10, true, false);
    code[3] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 4, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "Multiple NEW_COLLECTION operations should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "R[0] should be a list";
    EXPECT_TRUE(IS_LIST(&vm->values[1])) << "R[1] should be a list";
    EXPECT_TRUE(IS_LIST(&vm->values[2])) << "R[2] should be a list";

    EXPECT_GE(AS_LIST(&vm->values[0])->capacity, 2) << "R[0] capacity should be at least 2";
    EXPECT_GE(AS_LIST(&vm->values[1])->capacity, 5) << "R[1] capacity should be at least 5";
    EXPECT_GE(AS_LIST(&vm->values[2])->capacity, 10) << "R[2] capacity should be at least 10";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionMixedTypes) {
    vm->error = 0;

    Instruction code[4];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, 3, true, false);
    code[1] = INSTRUCTION_NEW_COLLECTION(1, BASE_VALUE_TYPE_DICT, 0, true, false);
    code[2] = INSTRUCTION_NEW_COLLECTION(2, BASE_VALUE_TYPE_LIST, 1, true, false);
    code[3] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 4, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "Creating mixed collection types should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "R[0] should be a list";
    EXPECT_TRUE(IS_DICT(&vm->values[1])) << "R[1] should be a dict";
    EXPECT_TRUE(IS_LIST(&vm->values[2])) << "R[2] should be a list";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListMaxCapacity) {
    vm->error = 0;

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, 254, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION with max capacity (254) should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_GE(list->capacity, 254) << "List capacity should be at least 254";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListInvalidRegisterCapacity) {
    vm->error = 0;

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, 255, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION with INVALID_LOCAL_REGISTER_ID as capacity should use default capacity";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionListZeroCapacity) {
    vm->error = 0;

    Instruction code[2];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, 0, true, false);
    code[1] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 2, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION with zero capacity should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 0) << "List should be empty";
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionWithSubsequentAppend) {
    vm->error = 0;

    vm->values[1] = semiValueNewInt(42);
    vm->values[2] = semiValueNewInt(100);

    Instruction code[3];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_LIST, 2, true, false);
    code[1] = INSTRUCTION_APPEND_LIST(0, 1, 2, false, false);
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 3, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION followed by APPEND should succeed";

    EXPECT_TRUE(IS_LIST(&vm->values[0])) << "Result should be a list";
    ObjectList* list = AS_LIST(&vm->values[0]);
    EXPECT_EQ(list->size, 2) << "List should contain 2 elements";
    EXPECT_EQ(AS_INT(&list->values[0]), 42);
    EXPECT_EQ(AS_INT(&list->values[1]), 100);
}

TEST_F(VMInstructionCollectionInitializerTest, OpNewCollectionDictWithSubsequentSetItem) {
    vm->error = 0;

    vm->values[1] = semiValueStringCreate(&vm->gc, "key", 3);
    vm->values[2] = semiValueNewInt(123);

    Instruction code[3];
    code[0] = INSTRUCTION_NEW_COLLECTION(0, BASE_VALUE_TYPE_DICT, 0, true, false);
    code[1] = INSTRUCTION_SET_ITEM(0, 1, 2, false, false);
    code[2] = INSTRUCTION_TRAP(0, 0, false, false);

    SemiModule* module  = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);
    FunctionProto* func = CreateFunctionObject(0, code, 3, 8, 0, 0);
    module->moduleInit  = func;

    ErrorId result = RunModule(module);
    EXPECT_EQ(result, 0) << "NEW_COLLECTION dict followed by SET_ITEM should succeed";

    EXPECT_TRUE(IS_DICT(&vm->values[0])) << "Result should be a dict";
    ObjectDict* dict = AS_DICT(&vm->values[0]);
    EXPECT_EQ(dict->len, 1) << "Dict should contain 1 element";

    Value key = semiValueStringCreate(&vm->gc, "key", 3);
    EXPECT_TRUE(semiDictHas(dict, key)) << "Dict should contain the key";
}
