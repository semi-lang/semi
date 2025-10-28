// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <gtest/gtest.h>

extern "C" {
#include "../src/value.h"
#include "../src/vm.h"
#include "semi/error.h"
}

#include "test_common.hpp"

class VMInstructionFunctionUpvalueTest : public VMTest {};

TEST_F(VMInstructionFunctionUpvalueTest, BasicUpvalueCreation) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    Instruction fnCode[2];
    fnCode[0] = INSTRUCTION_GET_UPVALUE(0, 0, 0, false, false);  // R[0] := Upvalue[0] (captured value 42)
    fnCode[1] = INSTRUCTION_RETURN(0, 0, 0, false, false);       // Function returns

    // Create a function that captures one local variable as an upvalue
    FunctionTemplate* func    = createFunctionObject(0, fnCode, 2, 2, 1, 0);  // 1 upvalue
    func->upvalues[0].index   = 1;                                            // Capture local at index 1
    func->upvalues[0].isLocal = true;

    ConstantIndex index = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    Instruction code[5];
    code[0] = INSTRUCTION_LOAD_INLINE_INTEGER(1, 42, true, true);  // Set local[1] = 42
    code[1] = INSTRUCTION_LOAD_CONSTANT(0, index, false, false);   // Create function, captures local[1]
    code[2] = INSTRUCTION_CALL(0, 2, 0, false, false);             // Call function
    code[3] = INSTRUCTION_TRAP(0, 0, false, false);                // Exit
    code[4] = INSTRUCTION_TRAP(0, 1, false, false);                // Should not reach

    module->moduleInit = createFunctionObject(0, code, 5, 3, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";
    ASSERT_NE(vm->openUpvalues, nullptr) << "Open upvalues should not be closed after function returns";
    ASSERT_EQ(AS_INT(&vm->values[0]), 42) << "Function should return captured upvalue (42)";
}

TEST_F(VMInstructionFunctionUpvalueTest, MultipleUpvalueCreation) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Step 1: Create function
    Instruction fnCode[6];
    fnCode[0] = INSTRUCTION_GET_UPVALUE(0, 0, 0, false, false);  // R[0] := Upvalue[0] (10)
    fnCode[1] = INSTRUCTION_GET_UPVALUE(1, 1, 0, false, false);  // R[1] := Upvalue[1] (20)
    fnCode[2] = INSTRUCTION_GET_UPVALUE(2, 2, 0, false, false);  // R[2] := Upvalue[2] (30)
    fnCode[3] = INSTRUCTION_ADD(0, 0, 1, false, false);          // R[0] += R[1] (30)
    fnCode[4] = INSTRUCTION_ADD(0, 0, 2, false, false);          // R[0] += R[2] (60)
    fnCode[5] = INSTRUCTION_RETURN(0, 0, 0, false, false);       // Function returns

    // Step 2: Create function object and store in constant table
    FunctionTemplate* func    = createFunctionObject(0, fnCode, 6, 3, 3, 0);  // 3 upvalues
    func->upvalues[0].index   = 0;
    func->upvalues[0].isLocal = true;
    func->upvalues[1].index   = 1;
    func->upvalues[1].isLocal = true;
    func->upvalues[2].index   = 2;
    func->upvalues[2].isLocal = true;

    ConstantIndex funcIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(func));

    // Step 3: Create main code using the actual constant table index
    Instruction code[7];
    code[0] = INSTRUCTION_LOAD_INLINE_INTEGER(0, 10, true, true);     // local[0] = 10
    code[1] = INSTRUCTION_LOAD_INLINE_INTEGER(1, 20, true, true);     // local[1] = 20
    code[2] = INSTRUCTION_LOAD_INLINE_INTEGER(2, 30, true, true);     // local[2] = 30
    code[3] = INSTRUCTION_LOAD_CONSTANT(3, funcIndex, false, false);  // Create function
    code[4] = INSTRUCTION_CALL(3, 4, 0, false, false);                // Call function
    code[5] = INSTRUCTION_TRAP(0, 0, false, false);                   // Exit
    code[6] = INSTRUCTION_TRAP(0, 1, false, false);                   // Should not reach

    module->moduleInit = createFunctionObject(0, code, 7, 5, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";
    ASSERT_NE(vm->openUpvalues, nullptr) << "Open upvalues should not be closed after function returns";
    ASSERT_EQ(AS_INT(&vm->values[3]), 60) << "Function should return the sum of the captured upvalues (60)";
}

TEST_F(VMInstructionFunctionUpvalueTest, NestedFunctionsWithUpvalues) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Test upvalue behavior with nested function calls. Here's the pseudo-code for the test:
    /*
        fn outer() {
            x := 1
            fn middle() {
                fn inner() {
                    return x + 3
                }
                x += 2
                return inner
            }
            return middle
        }
        result := outer()()();  // result should be 6
    */

    // Step 1: Create inner function first (no dependencies)
    Instruction innerCode[3];
    innerCode[0] = INSTRUCTION_GET_UPVALUE(0, 0, 0, false, false);  // R[0] := Upvalue[0]
    innerCode[1] = INSTRUCTION_ADD(0, 0, 3 + 128, false, true);     // R[0] += 3
    innerCode[2] = INSTRUCTION_RETURN(0, 0, 0, false, false);       // Inner function returns R[0]

    FunctionTemplate* innerFunc    = createFunctionObject(0, innerCode, 3, 2, 1, 0);  // 1 upvalue
    innerFunc->upvalues[0].index   = 0;
    innerFunc->upvalues[0].isLocal = false;

    ConstantIndex innerIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(innerFunc));

    // Step 2: Create middle function (depends on inner)
    Instruction middleCode[5];
    middleCode[0] = INSTRUCTION_LOAD_CONSTANT(0, innerIndex, false, false);  // R[0] := inner function
    middleCode[1] = INSTRUCTION_GET_UPVALUE(1, 0, 0, false, false);          // R[1] := Upvalue[0]
    middleCode[2] = INSTRUCTION_ADD(1, 1, 2 + 128, false, true);             // R[1] += 2
    middleCode[3] = INSTRUCTION_SET_UPVALUE(0, 1, 0, false, false);          // Upvalue[0] := R[1]
    middleCode[4] = INSTRUCTION_RETURN(0, 0, 0, false, false);               // Middle function returns R[0]

    FunctionTemplate* middleFunc    = createFunctionObject(0, middleCode, 5, 2, 1, 0);  // 1 upvalue
    middleFunc->upvalues[0].index   = 0;
    middleFunc->upvalues[0].isLocal = true;

    ConstantIndex middleIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(middleFunc));

    // Step 3: Create outer function (depends on middle)
    Instruction outerCode[3];
    outerCode[0] = INSTRUCTION_LOAD_INLINE_INTEGER(0, 1, true, true);        // outer R[0] = 1
    outerCode[1] = INSTRUCTION_LOAD_CONSTANT(1, middleIndex, false, false);  // R[1] := middle function
    outerCode[2] = INSTRUCTION_RETURN(1, 0, 0, false, false);                // Outer function returns R[1]

    FunctionTemplate* outerFunc = createFunctionObject(0, outerCode, 3, 2, 0, 0);  // 0 upvalue

    ConstantIndex outerIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(outerFunc));

    // Step 4: Create main code using the actual constant table indices
    Instruction code[6];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, outerIndex, false, false);  // Create outer function
    code[1] = INSTRUCTION_CALL(0, 1, 0, false, false);                 // Call outer function
    code[2] = INSTRUCTION_CALL(0, 1, 0, false, false);                 // Call middle function
    code[3] = INSTRUCTION_CALL(0, 1, 0, false, false);                 // Call inner function
    code[4] = INSTRUCTION_TRAP(0, 0, false, false);                    // Exit
    code[5] = INSTRUCTION_TRAP(0, 1, false, false);                    // Should not reach

    module->moduleInit = createFunctionObject(0, code, 6, 2, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should execute nested calls successfully";
    ASSERT_EQ(vm->openUpvalues, nullptr) << "All upvalues should be closed after nested functions return";
    // The original local variable should be modified by the SET_UPVALUE instruction
    ASSERT_EQ(AS_INT(&vm->values[0]), 6) << "The closed captured variable should be 6 (1 + 2 + 3)";
}

TEST_F(VMInstructionFunctionUpvalueTest, UpvalueReuse) {
    SemiModule* module = semiVMModuleCreate(&vm->gc, SEMI_REPL_MODULE_ID);

    // Test that existing upvalues are reused when multiple functions capture the same local. Here's
    // the pseudo-code for the test:
    /*
        fn outer() {
            x := 1
            fn first() {
                x += 2
            }
            fn second() {
                return x + 4
            }

            first()
            return second
        }
        result := outer()();  // result should be 7
    */

    // Step 1: Create first function
    Instruction firstCode[4];
    firstCode[0] = INSTRUCTION_GET_UPVALUE(0, 0, 0, false, false);  // R[0] := Upvalue[0]
    firstCode[1] = INSTRUCTION_ADD(0, 0, 2 + 128, false, true);     // R[0] += 2
    firstCode[2] = INSTRUCTION_SET_UPVALUE(0, 0, 0, false, false);  // Upvalue[0] := R[0]
    firstCode[3] = INSTRUCTION_RETURN(255, 0, 0, false, false);     // First function returns

    FunctionTemplate* first    = createFunctionObject(0, firstCode, 4, 3, 1, 0);  // 1 upvalue
    first->upvalues[0].index   = 0;
    first->upvalues[0].isLocal = true;

    ConstantIndex firstIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(first));

    // Step 2: Create second function
    Instruction secondCode[3];
    secondCode[0] = INSTRUCTION_GET_UPVALUE(0, 0, 0, false, false);  // R[0] := Upvalue[0]
    secondCode[1] = INSTRUCTION_ADD(0, 0, 4 + 128, false, true);     // R[0] += 4
    secondCode[2] = INSTRUCTION_RETURN(0, 0, 0, false, false);       // Second function returns R[0]

    FunctionTemplate* second    = createFunctionObject(0, secondCode, 3, 2, 1, 0);  // 1 upvalue
    second->upvalues[0].index   = 0;
    second->upvalues[0].isLocal = true;

    ConstantIndex secondIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(second));

    // Step 3: Create outer function (depends on first and second)
    Instruction outerCode[5];
    outerCode[0] = INSTRUCTION_LOAD_INLINE_INTEGER(0, 1, true, true);        // local[0] = 1
    outerCode[1] = INSTRUCTION_LOAD_CONSTANT(1, firstIndex, false, false);   // Create function first
    outerCode[2] = INSTRUCTION_LOAD_CONSTANT(2, secondIndex, false, false);  // Create function second
    outerCode[3] = INSTRUCTION_CALL(1, 3, 0, false, false);                  // Call function first
    outerCode[4] = INSTRUCTION_RETURN(2, 0, 0, false, false);                // Outer function returns R[2]

    FunctionTemplate* outerFunc = createFunctionObject(0, outerCode, 5, 4, 0, 0);  // 0 upvalue

    ConstantIndex outerIndex = semiConstantTableInsert(&module->constantTable, FUNCTION_VALUE(outerFunc));

    // Step 4: Create main code using the actual constant table indices
    Instruction code[5];
    code[0] = INSTRUCTION_LOAD_CONSTANT(0, outerIndex, false, false);  // Create outer function
    code[1] = INSTRUCTION_CALL(0, 1, 0, false, false);                 // Call outer function
    code[2] = INSTRUCTION_CALL(0, 1, 0, false, false);                 // Call second function
    code[3] = INSTRUCTION_TRAP(0, 0, false, false);                    // Exit
    code[4] = INSTRUCTION_TRAP(0, 1, false, false);                    // Should not reach

    module->moduleInit = createFunctionObject(0, code, 5, 2, 0, 0);

    ErrorId result = semiVMRunMainModule(vm, module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";
    ASSERT_EQ(vm->openUpvalues, nullptr) << "All upvalues should be closed after functions return";

    // The shared upvalue should have been modified by function 1 and accessed by function 2
    ASSERT_EQ(AS_INT(&vm->values[0]), 7) << "Shared upvalue should reflect the modification";
}
