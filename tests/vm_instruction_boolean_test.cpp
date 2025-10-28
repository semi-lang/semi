// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/vm.h"
#include "semi/error.h"
}

class VMInstructionBooleanTest : public ::testing::Test {
   protected:
    SemiVM* vm;

    void SetUp() override {
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to create VM";
    }

    void TearDown() override {
        if (vm) {
            semiDestroyVM(vm);
            vm = nullptr;
        }
    }

    FunctionTemplate* createFunctionObject(Instruction* code, size_t codeSize) {
        FunctionTemplate* func = semiFunctionTemplateCreate(&vm->gc, 0);
        Instruction* codeCopy  = (Instruction*)semiMalloc(&vm->gc, sizeof(Instruction) * codeSize);
        memcpy(codeCopy, code, sizeof(Instruction) * codeSize);
        func->arity          = 0;
        func->chunk.data     = codeCopy;
        func->chunk.size     = codeSize;
        func->chunk.capacity = codeSize;
        func->maxStackSize   = 0;

        return func;
    }
};