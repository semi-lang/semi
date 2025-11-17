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
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=3

[Instructions]
0: OP_LOAD_INLINE_INTEGER  A=0x01 K=0x002A i=T s=T
1: OP_LOAD_CONSTANT        A=0x02 K=0x0000 i=F s=F
2: OP_CALL                 A=0x02 B=0x00 C=0x00 kb=F kc=F
3: OP_TRAP                 A=0x00 B=0x00 C=0x00 kb=F kc=F
4: OP_TRAP                 A=0x00 B=0x01 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=2 -> @testFunc

[Instructions:testFunc]
0: OP_GET_UPVALUE  A=0x00 B=0x00 C=0x00 kb=F kc=F
1: OP_RETURN       A=0x00 B=0x00 C=0x00 kb=F kc=F

[UpvalueDescription:testFunc]
U[0]: index=1 isLocal=T
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";
    ASSERT_NE(vm->openUpvalues, nullptr) << "Open upvalues should not be closed after function returns";
    ASSERT_EQ(AS_INT(&vm->values[2]), 42) << "Function should return captured upvalue (42)";
}

TEST_F(VMInstructionFunctionUpvalueTest, MultipleUpvalueCreation) {
    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=5

[Instructions]
0: OP_LOAD_INLINE_INTEGER  A=0x00 K=0x000A i=T s=T
1: OP_LOAD_INLINE_INTEGER  A=0x01 K=0x0014 i=T s=T
2: OP_LOAD_INLINE_INTEGER  A=0x02 K=0x001E i=T s=T
3: OP_LOAD_CONSTANT        A=0x03 K=0x0000 i=F s=F
4: OP_CALL                 A=0x03 B=0x00 C=0x00 kb=F kc=F
5: OP_TRAP                 A=0x00 B=0x00 C=0x00 kb=F kc=F
6: OP_TRAP                 A=0x00 B=0x01 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=3 -> @testFunc

[Instructions:testFunc]
0: OP_GET_UPVALUE  A=0x00 B=0x00 C=0x00 kb=F kc=F
1: OP_GET_UPVALUE  A=0x01 B=0x01 C=0x00 kb=F kc=F
2: OP_GET_UPVALUE  A=0x02 B=0x02 C=0x00 kb=F kc=F
3: OP_ADD          A=0x00 B=0x00 C=0x01 kb=F kc=F
4: OP_ADD          A=0x00 B=0x00 C=0x02 kb=F kc=F
5: OP_RETURN       A=0x00 B=0x00 C=0x00 kb=F kc=F

[UpvalueDescription:testFunc]
U[0]: index=0 isLocal=T
U[1]: index=1 isLocal=T
U[2]: index=2 isLocal=T
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";
    ASSERT_NE(vm->openUpvalues, nullptr) << "Open upvalues should not be closed after function returns";
    ASSERT_EQ(AS_INT(&vm->values[3]), 60) << "Function should return the sum of the captured upvalues (60)";
}

TEST_F(VMInstructionFunctionUpvalueTest, NestedFunctionsWithUpvalues) {
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

    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=2

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0002 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
3: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
4: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
5: OP_TRAP           A=0x00 B=0x01 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=2 -> @innerFunc
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=2 -> @middleFunc
K[2]: FunctionProto arity=0 coarity=0 maxStackSize=2 -> @outerFunc

[Instructions:innerFunc]
0: OP_GET_UPVALUE  A=0x00 B=0x00 C=0x00 kb=F kc=F
1: OP_ADD          A=0x00 B=0x00 C=0x83 kb=F kc=T
2: OP_RETURN       A=0x00 B=0x00 C=0x00 kb=F kc=F

[UpvalueDescription:innerFunc]
U[0]: index=0 isLocal=F

[Instructions:middleFunc]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0000 i=F s=F
1: OP_GET_UPVALUE    A=0x01 B=0x00 C=0x00 kb=F kc=F
2: OP_ADD            A=0x01 B=0x01 C=0x82 kb=F kc=T
3: OP_SET_UPVALUE    A=0x00 B=0x01 C=0x00 kb=F kc=F
4: OP_RETURN         A=0x00 B=0x00 C=0x00 kb=F kc=F

[UpvalueDescription:middleFunc]
U[0]: index=0 isLocal=T

[Instructions:outerFunc]
0: OP_LOAD_INLINE_INTEGER  A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_CONSTANT        A=0x01 K=0x0001 i=F s=F
2: OP_RETURN               A=0x01 B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute nested calls successfully";
    ASSERT_EQ(vm->openUpvalues, nullptr) << "All upvalues should be closed after nested functions return";
    ASSERT_EQ(AS_INT(&vm->values[0]), 6) << "The closed captured variable should be 6 (1 + 2 + 3)";
}

TEST_F(VMInstructionFunctionUpvalueTest, UpvalueReuse) {
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

    SemiModule* module;
    ErrorId result = InstructionVerifier::BuildAndRunModule(vm,
                                                            R"(
[ModuleInit]
arity=0 coarity=0 maxStackSize=2

[Instructions]
0: OP_LOAD_CONSTANT  A=0x00 K=0x0002 i=F s=F
1: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
2: OP_CALL           A=0x00 B=0x00 C=0x00 kb=F kc=F
3: OP_TRAP           A=0x00 B=0x00 C=0x00 kb=F kc=F
4: OP_TRAP           A=0x00 B=0x01 C=0x00 kb=F kc=F

[Constants]
K[0]: FunctionProto arity=0 coarity=0 maxStackSize=3 -> @firstFunc
K[1]: FunctionProto arity=0 coarity=0 maxStackSize=2 -> @secondFunc
K[2]: FunctionProto arity=0 coarity=0 maxStackSize=4 -> @outerFunc

[Instructions:firstFunc]
0: OP_GET_UPVALUE  A=0x00 B=0x00 C=0x00 kb=F kc=F
1: OP_ADD          A=0x00 B=0x00 C=0x82 kb=F kc=T
2: OP_SET_UPVALUE  A=0x00 B=0x00 C=0x00 kb=F kc=F
3: OP_RETURN       A=0xFF B=0x00 C=0x00 kb=F kc=F

[UpvalueDescription:firstFunc]
U[0]: index=0 isLocal=T

[Instructions:secondFunc]
0: OP_GET_UPVALUE  A=0x00 B=0x00 C=0x00 kb=F kc=F
1: OP_ADD          A=0x00 B=0x00 C=0x84 kb=F kc=T
2: OP_RETURN       A=0x00 B=0x00 C=0x00 kb=F kc=F

[UpvalueDescription:secondFunc]
U[0]: index=0 isLocal=T

[Instructions:outerFunc]
0: OP_LOAD_INLINE_INTEGER  A=0x00 K=0x0001 i=T s=T
1: OP_LOAD_CONSTANT        A=0x01 K=0x0000 i=F s=F
2: OP_LOAD_CONSTANT        A=0x02 K=0x0001 i=F s=F
3: OP_MOVE                 A=0x03 B=0x01 C=0x00 kb=F kc=F
4: OP_CALL                 A=0x03 B=0x00 C=0x00 kb=F kc=F
5: OP_RETURN               A=0x02 B=0x00 C=0x00 kb=F kc=F
)",
                                                            &module);

    ASSERT_EQ(result, 0) << "VM should execute successfully";
    ASSERT_EQ(vm->openUpvalues, nullptr) << "All upvalues should be closed after functions return";
    ASSERT_EQ(AS_INT(&vm->values[0]), 7) << "Shared upvalue should reflect the modification";
}
