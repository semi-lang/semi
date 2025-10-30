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
};