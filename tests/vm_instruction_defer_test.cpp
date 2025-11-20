// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/value.h"
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionDeferTest : public VMTest {
   public:
    static int64_t execution_order[10];
    static int execution_count;

    static void RecordExecution(int64_t value) {
        if (execution_count < 10) {
            execution_order[execution_count++] = value;
        }
    }

    static void ResetExecution() {
        execution_count = 0;
        for (int i = 0; i < 10; i++) {
            execution_order[i] = 0;
        }
    }
    // Native function to track execution order
    static ErrorId trackExecution(SemiVM* vm, uint8_t argCount, Value* args, Value* returnValue) {
        (void)vm;

        if (argCount != 1 || !IS_INT(&args[0])) {
            return SEMI_ERROR_ARGS_COUNT_MISMATCH;
        }

        RecordExecution(AS_INT(&args[0]));
        return 0;
    }

   protected:
    void SetUp() override {
        VMTest::SetUp();

        // Reset execution tracking
        ResetExecution();

        // Add the tracking function as a global constant
        Value trackFn = semiValueNativeFunctionCreate(trackExecution);

        ASSERT_EQ(semiVMAddGlobalVariable(vm, "track", 5, trackFn), 0);
    }

    void TearDown() override {
        // Reset execution tracking
        ResetExecution();

        VMTest::TearDown();
    }

    void VerifyExecutionOrder(const std::vector<int64_t>& expected) {
        ASSERT_EQ(execution_count, expected.size()) << "Execution count mismatch";
        for (size_t i = 0; i < expected.size(); i++) {
            ASSERT_EQ(execution_order[i], expected[i]) << "Execution order mismatch at position " << i;
        }
    }
};

// Define static members
int64_t VMInstructionDeferTest::execution_order[10] = {0};
int VMInstructionDeferTest::execution_count         = 0;

// Tests that defer block executes after function body but before return
//
// Equivalent Semi source code:
//   fn testFunction() {
//       track(1)           // Function body start
//       defer { track(2) } // Defer block
//       track(3)           // Function body end
//   }
//   testFunction()
//
TEST_F(VMInstructionDeferTest, SingleDeferBlock) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0001 i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
4: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
5: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0003 i=T s=T
6: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
7: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @deferFunc

[Instructions:deferFunc]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0002 i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";

    // Expected execution order: track(1), track(3), track(2)
    // Function body executes first, then defer block
    VerifyExecutionOrder({1, 3, 2});
}

// Tests that multiple defer blocks execute in LIFO order (last defer first)
//
// Equivalent Semi source code:
//   fn testFunction() {
//       track(1)            // Function body start
//       defer { track(10) } // First defer (executes last)
//       track(2)            // Function body middle
//       defer { track(20) } // Second defer (executes second)
//       defer { track(30) } // Third defer (executes first)
//       track(3)            // Function body end
//   }
//   testFunction()
//
TEST_F(VMInstructionDeferTest, MultipleDeferBlocks) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1:  OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0001 i=T s=T
2:  OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3:  OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
4:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
5:  OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0002 i=T s=T
6:  OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
7:  OP_DEFER_CALL            A=0x00 K=0x0001 i=F s=F
8:  OP_DEFER_CALL            A=0x00 K=0x0002 i=F s=F
9:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
10: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0003 i=T s=T
11: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
12: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @defer1
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @defer2
K[2]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @defer3

[Instructions:defer1]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x000A i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:defer2]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0014 i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:defer3]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x001E i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";

    // Expected execution order: track(1), track(2), track(3), track(30), track(20), track(10)
    // Function body executes first, then defer blocks in LIFO order (last deferred first)
    VerifyExecutionOrder({1, 2, 3, 30, 20, 10});
}

// Tests that defer executes even when function has early return statements
//
// Equivalent Semi source code:
//   fn testFunction() {
//       track(1)            // Function body start
//       defer { track(100) } // Defer block
//       track(2)            // Function body middle
//       if true {           // Conditional early return
//           return          // Early exit, but defer should still execute
//       }
//       track(999)          // This should NOT execute
//   }
//   testFunction()
//
TEST_F(VMInstructionDeferTest, DeferWithEarlyReturn) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=4

[Instructions]
0:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1:  OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0001 i=T s=T
2:  OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3:  OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
4:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
5:  OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0002 i=T s=T
6:  OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
7:  OP_LOAD_BOOL             A=0x03 B=0x00 C=0x00 kb=T kc=F
8:  OP_C_JUMP                A=0x03 K=0x0004 i=T s=T
9:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
10: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x03E7 i=T s=T
11: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
12: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
13: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @deferFunc

[Instructions:deferFunc]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0064 i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";

    // Expected execution order: track(1), track(2), track(100)
    // Function body executes with early return, but defer still executes
    // track(999) should NOT be called due to early return
    VerifyExecutionOrder({1, 2, 100});
}

// Tests that defer behavior works correctly across function call boundaries
//
// Equivalent Semi source code:
//   fn innerFunction() {
//       track(10)            // Inner function body start
//       defer { track(200) } // Inner defer block
//       track(20)            // Inner function body end
//   }
//
//   fn outerFunction() {
//       track(1)             // Outer function body start
//       defer { track(400) } // Outer defer block
//       track(2)             // Outer function body middle
//       innerFunction()      // Call inner function
//       track(3)             // Outer function body end
//   }
//   outerFunction()
//
// Expected execution: outer body → inner body → inner defer → outer body → outer defer
//
TEST_F(VMInstructionDeferTest, NestedFunctionCallsWithDefer) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1:  OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0001 i=T s=T
2:  OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3:  OP_DEFER_CALL            A=0x00 K=0x0000 i=F s=F
4:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
5:  OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0002 i=T s=T
6:  OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
7:  OP_LOAD_CONSTANT         A=0x01 K=0x0002 i=F s=F
8:  OP_CALL                  A=0x01 B=0x00 C=0x00 kb=F kc=F
9:  OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
10: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0003 i=T s=T
11: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
12: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @outerDefer
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @innerDefer
K[2]: FunctionProto arity=0 coarity=0 maxStackSize=8 -> @innerFunc

[Instructions:outerDefer]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0190 i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:innerDefer]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x00C8 i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F

[Instructions:innerFunc]
0: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
1: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x000A i=T s=T
2: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
3: OP_DEFER_CALL            A=0x00 K=0x0001 i=F s=F
4: OP_LOAD_CONSTANT         A=0x01 K=0x0000 i=F s=T
5: OP_LOAD_INLINE_INTEGER   A=0x02 K=0x0014 i=T s=T
6: OP_CALL                  A=0x01 B=0x01 C=0x00 kb=F kc=F
7: OP_RETURN                A=0xFF B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";

    // Expected execution order:
    // Outer: track(1), track(2)
    // Inner: track(10), track(20)
    // Inner defer: track(200)
    // Outer: track(3)
    // Outer defer: track(400)
    VerifyExecutionOrder({1, 2, 10, 20, 200, 3, 400});
}