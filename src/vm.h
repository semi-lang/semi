// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_VM_H
#define SEMI_VM_H

#include "./const_table.h"
#include "./gc.h"
#include "./instruction.h"
#include "./primitives.h"
#include "./semi_common.h"
#include "./symbol_table.h"
#include "./value.h"
#include "semi/error.h"
#include "semi/semi.h"

// A SemiModule represents a compiled self-contained executable unit.
typedef struct SemiModule {
    ModuleId moduleId;
    ObjectDict exports;
    ObjectDict globals;
    ConstantTable constantTable;

    // The function proto used to initialize this module. For main module, this is the main function.
    FunctionProto* moduleInit;
} SemiModule;

SemiModule* semiVMModuleCreate(GC* gc, ModuleId moduleId);
SemiModule* semiVMModuleCreateFrom(GC* gc, SemiModule* source);
void semiVMModuleDestroy(GC* gc, SemiModule* module);

#define MAX_FRAME_DEPTH (1 << 16)

/*
 Calling Convention

             ▲      │                   │
             │      │                   │
             │      │                   │
       Frame │   ┌─ │    (more args)    │
             │   │  │       arg 1       │
             └─  │  │       arg 0       │ <── frame.callerStack.base  ▲
 previous <──────┤  │                   │                             │
  frame          └─ │                   │                             │ frame.stack.offset
                    │                   │                             │
      VM stack      └───────────────────┘ <── vm.values               ▼


 `OP_RETURN` copies the returned value (indexed by the current frame) to the caller's register.
 */

// Frame keeps track of the call stack of the VM.
typedef struct Frame {
    // The next instruction to be executed in the caller function.
    Instruction* returnIP;
    // The function being executed in this frame.
    ObjectFunction* function;
    // The chain of deferred functions to be executed when this frame is exited.
    ObjectFunction* deferredFn;
    // The stack of the function as an offset to the VM's stack.
    uint32_t stackOffset;
    ModuleId moduleId;
} Frame;

DECLARE_DARRAY(ModuleList, SemiModule*, uint16_t)
DECLARE_DARRAY(GlobalIdentifierList, IdentifierId, ModuleVariableId)

typedef struct SemiVM {
    // The garbage collector for this VM instance. Must be the first field of the struct.
    GC gc;

    ErrorId error;
    // Error message associated with the error ID. Currently we only assign static strings to this field,
    // so there is no need to free it.
    const char* errorMessage;
    // A weak reference to the result of the last expression statement (if there is one).
    Value* returnedValue;

    Value* values;
    uint32_t valueCount;
    uint32_t valueCapacity;

    Frame* frames;
    uint32_t frameCount;
    uint32_t frameCapacity;

    ObjectUpvalue* openUpvalues;

    SymbolTable symbolTable;

    ClassTable classes;
    ModuleId nextModuleId;

    ModuleList modules;

    // Global constants shared across all modules. The length of this array is the same as
    // `globalIdentifiers.capacity`.
    Value* globalConstants;

    // IdentifierId to Constant index for global variables shared across all modules.
    GlobalIdentifierList globalIdentifiers;
} SemiVM;

ErrorId semiVMRunMainModule(SemiVM* vm, SemiModule* module);
ErrorId semiVMAddGlobalVariable(SemiVM* vm, const char* identifier, IdentifierLength identifierLength, Value value);

#endif  // SEMI_VM_H
