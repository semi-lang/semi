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
    static ErrorId trackExecution(GC* gc, uint8_t argCount, Value* args, Value* returnValue) {
        (void)gc;

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
        Value trackFn = semiValueNewNativeFunction(trackExecution);

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
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Create defer function that calls track(2)
    Instruction deferCode[4];
    deferCode[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);        // Load track function (global constant 0)
    deferCode[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 2, false, true);  // Load argument 2 (positive)
    deferCode[2] = INSTRUCTION_CALL(1, 1, 0, false, false);             // Call track(2)
    deferCode[3] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);   // Return

    FunctionProto* deferFn = CreateFunctionObject(0, deferCode, 4, 3, 0, 0);

    // Add defer function to module's constant table
    ConstantIndex deferIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(deferFn));

    // Create main function
    Instruction mainCode[8];
    mainCode[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);         // Load track function (global constant 0)
    mainCode[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 1, false, true);   // Load argument 1 (positive)
    mainCode[2] = INSTRUCTION_CALL(1, 1, 0, false, false);              // Call track(1) - function body start
    mainCode[3] = INSTRUCTION_DEFER_CALL(0, deferIndex, false, false);  // Register defer block
    mainCode[4] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);         // Load track function again
    mainCode[5] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 3, false, true);   // Load argument 3 (positive)
    mainCode[6] = INSTRUCTION_CALL(1, 1, 0, false, false);              // Call track(3) - function body end
    mainCode[7] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);    // Return (triggers defer execution)

    module->moduleInit = CreateFunctionObject(0, mainCode, 8, 3, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

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
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Create first defer function that calls track(10)
    Instruction defer1Code[4];
    defer1Code[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);         // Load track function (global constant 0)
    defer1Code[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 10, false, true);  // Load argument 10 (positive)
    defer1Code[2] = INSTRUCTION_CALL(1, 1, 0, false, false);              // Call track(10)
    defer1Code[3] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);    // Return

    FunctionProto* defer1Fn   = CreateFunctionObject(0, defer1Code, 4, 3, 0, 0);
    ConstantIndex defer1Index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(defer1Fn));

    // Create second defer function that calls track(20)
    Instruction defer2Code[4];
    defer2Code[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);         // Load track function (global constant 0)
    defer2Code[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 20, false, true);  // Load argument 20 (positive)
    defer2Code[2] = INSTRUCTION_CALL(1, 1, 0, false, false);              // Call track(20)
    defer2Code[3] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);    // Return

    FunctionProto* defer2Fn   = CreateFunctionObject(0, defer2Code, 4, 3, 0, 0);
    ConstantIndex defer2Index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(defer2Fn));

    // Create third defer function that calls track(30)
    Instruction defer3Code[4];
    defer3Code[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);         // Load track function (global constant 0)
    defer3Code[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 30, false, true);  // Load argument 30 (positive)
    defer3Code[2] = INSTRUCTION_CALL(1, 1, 0, false, false);              // Call track(30)
    defer3Code[3] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);    // Return

    FunctionProto* defer3Fn   = CreateFunctionObject(0, defer3Code, 4, 3, 0, 0);
    ConstantIndex defer3Index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(defer3Fn));

    // Create main function
    // track(3) }
    Instruction mainCode[13];
    mainCode[0]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);          // Load track function (global constant 0)
    mainCode[1]  = INSTRUCTION_LOAD_INLINE_INTEGER(2, 1, false, true);    // Load argument 1 (positive)
    mainCode[2]  = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(1) - function body start
    mainCode[3]  = INSTRUCTION_DEFER_CALL(0, defer1Index, false, false);  // Register first defer block
    mainCode[4]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);          // Load track function again
    mainCode[5]  = INSTRUCTION_LOAD_INLINE_INTEGER(2, 2, false, true);    // Load argument 2 (positive)
    mainCode[6]  = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(2) - function body middle
    mainCode[7]  = INSTRUCTION_DEFER_CALL(0, defer2Index, false, false);  // Register second defer block
    mainCode[8]  = INSTRUCTION_DEFER_CALL(0, defer3Index, false, false);  // Register third defer block
    mainCode[9]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);          // Load track function again
    mainCode[10] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 3, false, true);    // Load argument 3 (positive)
    mainCode[11] = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(3) - function body end
    mainCode[12] =
        INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);  // Return (triggers defer execution in LIFO order)

    module->moduleInit = CreateFunctionObject(0, mainCode, 13, 3, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

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
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Create defer function that calls track(100)
    Instruction deferCode[4];
    deferCode[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);          // Load track function (global constant 0)
    deferCode[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 100, false, true);  // Load argument 100 (positive)
    deferCode[2] = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(100)
    deferCode[3] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);     // Return

    FunctionProto* deferFn   = CreateFunctionObject(0, deferCode, 4, 3, 0, 0);
    ConstantIndex deferIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(deferFn));

    // Create main function with conditional early return
    Instruction mainCode[14];
    mainCode[0]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);          // Load track function (global constant 0)
    mainCode[1]  = INSTRUCTION_LOAD_INLINE_INTEGER(2, 1, false, true);    // Load argument 1 (positive)
    mainCode[2]  = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(1) - function body start
    mainCode[3]  = INSTRUCTION_DEFER_CALL(0, deferIndex, false, false);   // Register defer block
    mainCode[4]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);          // Load track function again
    mainCode[5]  = INSTRUCTION_LOAD_INLINE_INTEGER(2, 2, false, true);    // Load argument 2 (positive)
    mainCode[6]  = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(2) - before condition
    mainCode[7]  = INSTRUCTION_LOAD_BOOL(3, 0, true, false);              // Load true for condition
    mainCode[8]  = INSTRUCTION_C_JUMP(3, 4, true, true);                  // Jump if true (skip next 4 instructions)
    mainCode[9]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);          // This should be skipped
    mainCode[10] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 999, false, true);  // This should be skipped (positive)
    mainCode[11] = INSTRUCTION_CALL(1, 1, 0, false, false);               // This should be skipped: track(999)
    mainCode[12] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);     // This should be skipped
    mainCode[13] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);     // Early return (triggers defer execution)

    module->moduleInit = CreateFunctionObject(0, mainCode, 14, 4, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

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
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Create defer function for inner function that calls track(200)
    Instruction innerDeferCode[4];
    innerDeferCode[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);  // Load track function (global constant 0)
    innerDeferCode[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 200, false, true);  // Load argument 200 (positive)
    innerDeferCode[2] = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(200)
    innerDeferCode[3] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);     // Return

    FunctionProto* innerDeferFn   = CreateFunctionObject(0, innerDeferCode, 4, 3, 0, 0);
    ConstantIndex innerDeferIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(innerDeferFn));

    // Create defer function for outer function that calls track(400)
    Instruction outerDeferCode[4];
    outerDeferCode[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);  // Load track function (global constant 0)
    outerDeferCode[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 400, false, true);  // Load argument 400 (positive)
    outerDeferCode[2] = INSTRUCTION_CALL(1, 1, 0, false, false);               // Call track(400)
    outerDeferCode[3] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);     // Return

    FunctionProto* outerDeferFn   = CreateFunctionObject(0, outerDeferCode, 4, 3, 0, 0);
    ConstantIndex outerDeferIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(outerDeferFn));

    // Create inner function (called by outer function)
    Instruction innerCode[8];
    innerCode[0] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);              // Load track function (global constant 0)
    innerCode[1] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 10, false, true);       // Load argument 10 (positive)
    innerCode[2] = INSTRUCTION_CALL(1, 1, 0, false, false);                   // Call track(10) - inner function start
    innerCode[3] = INSTRUCTION_DEFER_CALL(0, innerDeferIndex, false, false);  // Register inner defer block
    innerCode[4] = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);              // Load track function again
    innerCode[5] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 20, false, true);       // Load argument 20 (positive)
    innerCode[6] = INSTRUCTION_CALL(1, 1, 0, false, false);                   // Call track(20) - inner function end
    innerCode[7] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);         // Return (triggers inner defer)

    FunctionProto* innerFn     = CreateFunctionObject(0, innerCode, 8, 3, 0, 0);
    ConstantIndex innerFnIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(innerFn));

    // Create outer function (main function)
    Instruction outerCode[13];
    outerCode[0]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);        // Load track function (global constant 0)
    outerCode[1]  = INSTRUCTION_LOAD_INLINE_INTEGER(2, 1, false, true);  // Load argument 1 (positive)
    outerCode[2]  = INSTRUCTION_CALL(1, 1, 0, false, false);             // Call track(1) - outer function start
    outerCode[3]  = INSTRUCTION_DEFER_CALL(0, outerDeferIndex, false, false);  // Register outer defer block
    outerCode[4]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);              // Load track function again
    outerCode[5]  = INSTRUCTION_LOAD_INLINE_INTEGER(2, 2, false, true);        // Load argument 2 (positive)
    outerCode[6]  = INSTRUCTION_CALL(1, 1, 0, false, false);                   // Call track(2) - before inner call
    outerCode[7]  = INSTRUCTION_LOAD_CONSTANT(1, innerFnIndex, false, false);  // Load inner function
    outerCode[8]  = INSTRUCTION_CALL(1, 0, 0, false, false);                   // Call inner function
    outerCode[9]  = INSTRUCTION_LOAD_CONSTANT(1, 0, false, true);              // Load track function again
    outerCode[10] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 3, false, true);        // Load argument 3 (positive)
    outerCode[11] = INSTRUCTION_CALL(1, 1, 0, false, false);                   // Call track(3) - outer function end
    outerCode[12] = INSTRUCTION_RETURN(UINT8_MAX, 0, 0, false, false);         // Return (triggers outer defer)

    module->moduleInit = CreateFunctionObject(0, outerCode, 13, 3, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";

    // Expected execution order:
    // Outer: track(1), track(2)
    // Inner: track(10), track(20)
    // Inner defer: track(200)
    // Outer: track(3)
    // Outer defer: track(400)
    VerifyExecutionOrder({1, 2, 10, 20, 200, 3, 400});
}