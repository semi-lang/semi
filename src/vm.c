// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./vm.h"

#include <stdlib.h>
#include <string.h>

#include "./gc.h"
#include "./primitives.h"
#include "./symbol_table.h"
#include "./value.h"
#include "semi/config.h"
#include "semi/error.h"

SemiModule* semiVMModuleCreate(GC* gc, ModuleId moduleId) {
    SemiModule* module = semiMalloc(gc, sizeof(SemiModule));
    if (module == NULL) {
        return NULL;
    }

    module->moduleId = moduleId;
    semiObjectStackDictInit(&module->exports);
    semiObjectStackDictInit(&module->globals);
    semiObjectStackDictInit(&module->types);
    semiConstantTableInit(gc, &module->constantTable);
    module->moduleInit = NULL;

    return module;
}

void semiVMModuleDestroy(GC* gc, SemiModule* module) {
    semiObjectStackDictCleanup(gc, &module->types);
    semiObjectStackDictCleanup(gc, &module->exports);
    semiObjectStackDictCleanup(gc, &module->globals);
    semiConstantTableCleanup(&module->constantTable);
    if (module->moduleInit != NULL) {
        semiFunctionProtoDestroy(gc, module->moduleInit);
    }

    semiFree(gc, module, sizeof(SemiModule));
}

static inline void defaultNoopPrintFn(const char* text, size_t length, void* printData) {
    (void)text;
    (void)length;
    (void)printData;
}

ErrorId semiVMAddGlobalVariable(SemiVM* vm, const char* identifier, IdentifierLength identifierLength, Value value) {
    if (vm->globalIdentifiers.size == UINT16_MAX) {
        vm->error = SEMI_ERROR_TOO_MANY_GLOBAL_VARS;
        return vm->error;
    }

    InternedChar* interned    = semiSymbolTableInsert(&vm->symbolTable, identifier, identifierLength);
    IdentifierId identifierId = semiSymbolTableGetId(interned);

    ModuleVariableId oldCapacity = vm->globalIdentifiers.capacity;
    if (GlobalIdentifierListAppend(&vm->gc, &vm->globalIdentifiers, identifierId) != 0) {
        // Failed to add the identifier
        vm->error = SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
        return vm->error;
    }
    if (vm->globalIdentifiers.capacity != oldCapacity) {
        // The capacity has changed, we need to reallocate the global constants array
        Value* newConstants = semiRealloc(
            &vm->gc, vm->globalConstants, sizeof(Value) * oldCapacity, sizeof(Value) * vm->globalIdentifiers.capacity);
        if (newConstants == NULL) {
            // Failed to allocate memory for the new constant
            vm->error = SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
            semiFree(&vm->gc, vm->globalConstants, sizeof(Value) * oldCapacity);
            vm->globalConstants = NULL;
            return vm->error;
        }
        vm->globalConstants = newConstants;
    }

    vm->globalConstants[vm->globalIdentifiers.size - 1] = value;
    return 0;
}

#ifndef SEMI_VM_NO_DEFAULT_ALLOCATOR

static inline void* defaultReallocFn(void* ptr, size_t newSize, void* reallocData) {
    (void)reallocData;
    if (newSize == 0) {
        free(ptr);
        return NULL;  // Return NULL to indicate the memory has been freed
    }
    return realloc(ptr, newSize);
}

#endif

DEFINE_DARRAY(GlobalIdentifierList, IdentifierId, ModuleVariableId, UINT16_MAX)

static ErrorId captureUpvalues(SemiVM* vm, Value* currentBase, ObjectFunction* function) {
    FunctionProto* fnProto = function->proto;
    if (fnProto->upvalueCount == 0) {
        return 0;  // No upvalues to capture
    } else if (vm->frameCount == 0) {
        return SEMI_ERROR_INTERNAL_ERROR;  // No current frame to capture upvalues from
    }

    ObjectUpvalue** currentUpvalues = vm->frames[vm->frameCount - 1].function->upvalues;
    for (uint8_t i = 0; i < function->upvalueCount; i++) {
        uint8_t index = fnProto->upvalues[i].index;
        bool isLocal  = fnProto->upvalues[i].isLocal;
        if (!isLocal) {
            function->upvalues[i] = currentUpvalues[index];
            continue;
        }

        ObjectUpvalue** upvalue = &vm->openUpvalues;
        Value* local            = currentBase + index;
        while (*upvalue != NULL) {
            if ((*upvalue)->value == local) {
                goto found_upvalue;
            }
            upvalue = &(*upvalue)->payload.next;
        }

        // Create a new upvalue
        ObjectUpvalue* newUpvalue = semiObjectUpvalueCreate(&vm->gc, local);
        if (newUpvalue == NULL) {
            return SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
        }

        newUpvalue->payload.next = *upvalue;
        *upvalue                 = newUpvalue;

    found_upvalue:
        function->upvalues[i] = *upvalue;
    }

    return 0;
}

static void closeUpvalues(SemiVM* vm, Value* last) {
    ObjectUpvalue** upvalue = &vm->openUpvalues;
    while (*upvalue != NULL && (*upvalue)->value >= last) {
        ObjectUpvalue* closedUpvalue  = *upvalue;
        *upvalue                      = closedUpvalue->payload.next;
        closedUpvalue->payload.closed = *closedUpvalue->value;
        closedUpvalue->value          = &closedUpvalue->payload.closed;
    }
}

SEMI_EXPORT void semiInitConfig(SemiVMConfig* config) {
#ifndef SEMI_VM_NO_DEFAULT_ALLOCATOR
    config->reallocateFn = defaultReallocFn;
#else
    config->reallocateFn = NULL;
#endif
    config->reallocateUserData = NULL;
}

SEMI_EXPORT SemiVM* semiCreateVM(SemiVMConfig* inputConfig) {
    SemiVMConfig config;

#ifndef SEMI_VM_NO_DEFAULT_ALLOCATOR
    if (inputConfig == NULL) {
        semiInitConfig(&config);
    } else {
        config = *inputConfig;
    }
#else
    config = *config;
#endif

    SemiVM* vm = config.reallocateFn(NULL, sizeof(*vm), config.reallocateUserData);
    if (vm == NULL) {
        return NULL;
    }
    memset(vm, 0, sizeof(SemiVM));
    vm->error        = 0;
    vm->errorMessage = NULL;
    Value* newValues;
    if ((newValues = config.reallocateFn(vm->values, SEMI_MIN_STACK_SIZE * sizeof(Value), config.reallocateUserData)) ==
        NULL) {
        config.reallocateFn(vm, 0, config.reallocateUserData);
        return NULL;
    };
    vm->values        = newValues;
    vm->valueCount    = 0;
    vm->valueCapacity = SEMI_MIN_STACK_SIZE;

    Frame* newFrames;
    if ((newFrames = config.reallocateFn(vm->frames, SEMI_MIN_FRAME_SIZE * sizeof(Frame), config.reallocateUserData)) ==
        NULL) {
        config.reallocateFn(vm->values, 0, config.reallocateUserData);
        config.reallocateFn(vm, 0, config.reallocateUserData);
        return NULL;
    };
    vm->frames        = newFrames;
    vm->frameCount    = 0;
    vm->frameCapacity = SEMI_MIN_FRAME_SIZE;

    vm->openUpvalues = NULL;

    semiObjectStackDictInit(&vm->modules);
    semiGCInit(&vm->gc, config.reallocateFn, config.reallocateUserData);
    semiSymbolTableInit(&vm->gc, &vm->symbolTable);
    semiPrimitivesIntializeBuiltInPrimitives(&vm->gc, &vm->classes, &vm->symbolTable);

    vm->globalConstants = NULL;
    GlobalIdentifierListInit(&vm->globalIdentifiers);

    return vm;
}

SEMI_EXPORT void semiDestroyVM(SemiVM* vm) {
    if (vm == NULL) {
        return;
    }

    if (vm->globalIdentifiers.capacity > 0) {
        semiFree(&vm->gc, vm->globalIdentifiers.data, sizeof(IdentifierId) * vm->globalIdentifiers.capacity);

        if (vm->globalConstants != NULL) {
            semiFree(&vm->gc, vm->globalConstants, sizeof(Value) * vm->globalIdentifiers.size);
        }
    }

    for (uint16_t i = 0; i < vm->modules.len; i++) {
        SemiModule* module = AS_PTR(&vm->modules.values[i], SemiModule);
        semiVMModuleDestroy(&vm->gc, module);
    }
    semiObjectStackDictCleanup(&vm->gc, &vm->modules);

    for (uint16_t i = MIN_CUSTOM_BASE_VALUE_TYPE; i < vm->classes.classCount; i++) {
        semiFree(&vm->gc, &vm->classes.classMethods[i], sizeof(MagicMethodsTable));
    }
    semiPrimitivesCleanupClassTable(&vm->gc, &vm->classes);
    semiSymbolTableCleanup(&vm->symbolTable);

    SemiReallocateFn reallocateFn = vm->gc.reallocateFn;
    void* reallocateUserData      = vm->gc.reallocateUserData;
    semiGCCleanup(&vm->gc);

    if (vm->values != NULL) {
        reallocateFn(vm->values, 0, reallocateUserData);
    }
    if (vm->frames != NULL) {
        reallocateFn(vm->frames, 0, reallocateUserData);
    }

    reallocateFn(vm, 0, reallocateUserData);
}

#define load_value_abc(vm, instruction, ra, rb, rc) \
    Value valueTempC, valueTempB;                   \
    do {                                            \
        uint8_t a = OPERAND_T_A(instruction);       \
        uint8_t b = OPERAND_T_B(instruction);       \
        uint8_t c = OPERAND_T_C(instruction);       \
        bool kb   = OPERAND_T_KB(instruction);      \
        bool kc   = OPERAND_T_KC(instruction);      \
        ra        = &stack[a];                      \
        if (kb) {                                   \
            valueTempB.header = VALUE_TYPE_INT;     \
            valueTempB.as.i   = b + INT8_MIN;       \
            rb                = &valueTempB;        \
        } else {                                    \
            rb = &stack[b];                         \
        }                                           \
        if (kc) {                                   \
            valueTempC.header = VALUE_TYPE_INT;     \
            valueTempC.as.i   = c + INT8_MIN;       \
            rc                = &valueTempC;        \
        } else {                                    \
            rc = &stack[c];                         \
        }                                           \
    } while (0)

#define load_value_ab(vm, instruction, ra, rb) \
    do {                                       \
        uint8_t a = OPERAND_T_A(instruction);  \
        uint8_t b = OPERAND_T_B(instruction);  \
        ra        = &stack[a];                 \
        rb        = &stack[b];                 \
    } while (0)

static ErrorId growVMStackSize(SemiVM* vm, uint32_t requiredSize) {
    if (requiredSize > SEMI_MAX_STACK_SIZE) {
        return SEMI_ERROR_STACK_OVERFLOW;
    }

    uint32_t newCapacity;
    if (requiredSize < SEMI_MAX_STACK_SIZE / 2) {
        newCapacity = nextPowerOfTwoCapacity(requiredSize);
    } else {
        newCapacity = SEMI_MAX_STACK_SIZE;
    }

    ErrorId error    = 0;
    Value* newValues = semiRealloc(&vm->gc, vm->values, vm->valueCapacity * sizeof(Value), newCapacity * sizeof(Value));
    if (newValues != NULL) {
        vm->values        = newValues;
        vm->valueCapacity = newCapacity;
    } else {
        error = SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
    }

    return 0;
}

static ErrorId growVMFrameSize(SemiVM* vm, uint32_t requiredSize) {
    if (requiredSize > SEMI_MAX_FRAME_SIZE) {
        return SEMI_ERROR_STACK_OVERFLOW;
    }

    uint32_t newCapacity;
    if (requiredSize < SEMI_MAX_FRAME_SIZE / 2) {
        newCapacity = nextPowerOfTwoCapacity(requiredSize);
    } else {
        newCapacity = SEMI_MAX_FRAME_SIZE;
    }

    Frame* newFrames = semiRealloc(&vm->gc, vm->frames, vm->frameCapacity * sizeof(Frame), newCapacity * sizeof(Frame));
    if (newFrames != NULL) {
        vm->frames        = newFrames;
        vm->frameCapacity = newCapacity;
        return 0;
    } else {
        return SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
    }
}

#ifdef SEMI_DEBUG_MSG
#define TRAP_ON_ERROR(vm, errorId, message) \
    do {                                    \
        SemiVM* _vm    = (vm);              \
        ErrorId _errId = (errorId);         \
        if (_errId != 0) {                  \
            _vm->error        = _errId;     \
            _vm->errorMessage = (message);  \
            return;                         \
        }                                   \
    } while (0)
#else
#define TRAP_ON_ERROR(vm, errorId, message) \
    do {                                    \
        SemiVM* _vm    = (vm);              \
        ErrorId _errId = (errorId);         \
        if (_errId != 0) {                  \
            _vm->error = _errId;            \
            return;                         \
        }                                   \
    } while (0)
#endif
static void appendFrame(SemiVM* vm, ObjectFunction* func, Value* newStack) {
    FunctionProto* fnProto  = func->proto;
    uint64_t newStackOffset = (uint64_t)(newStack - vm->values);
    if (newStackOffset > SEMI_MAX_STACK_SIZE - fnProto->maxStackSize) {
        TRAP_ON_ERROR(vm, SEMI_ERROR_STACK_OVERFLOW, "Stack overflow on function call");
    }

    // Since newStackOffset + fnProto->maxStackSize <= SEMI_MAX_STACK_SIZE <= UINT32_MAX,
    // newStackOffset + fnProto->maxStackSize is guaranteed to fit in uint32_t
    uint32_t newMaxStackNeeded = (uint32_t)(newStackOffset + fnProto->maxStackSize);
    if (newMaxStackNeeded > vm->valueCapacity) {
        TRAP_ON_ERROR(vm, growVMStackSize(vm, newMaxStackNeeded), "Failed to grow VM stack for function call");
    }
    if (vm->frameCount >= vm->frameCapacity) {
        TRAP_ON_ERROR(vm, growVMFrameSize(vm, vm->frameCount + 1), "Failed to grow VM frame stack for function call");
    }

    // Set up the new frame
    vm->frames[vm->frameCount++] = (Frame){
        .stackOffset = (uint32_t)newStackOffset,
        .returnIP    = fnProto->chunk.data,
        .function    = func,
        .deferredFn  = NULL,
        .moduleId    = fnProto->moduleId,
    };
}

static inline bool verifyChunk(Chunk* chunk) {
    Instruction* chunkStart = chunk->data;
    Instruction* chunkEnd   = chunkStart + chunk->size;
    if (chunkStart == NULL || chunkEnd == NULL || chunkStart >= chunkEnd) {
        return false;
    }

    if (GET_OPCODE(*(chunkEnd - 1)) != OP_TRAP && GET_OPCODE(*(chunkEnd - 1)) != OP_RETURN) {
        return false;
    }

    return true;
}

static void runMainLoop(SemiVM* vm) {
    register Frame* frame;
    register Value* stack;
    register Instruction* ip;
    register SemiModule* module;

    Instruction* chunkStart;
    Instruction* chunkEnd;

#define MOVE_FORWARD(steps)                                                            \
    do {                                                                               \
        uint32_t _steps = (steps);                                                     \
        if ((uint32_t)(chunkEnd - ip) <= _steps) {                                     \
            TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Program counter out of bounds"); \
            return;                                                                    \
        }                                                                              \
        ip += _steps;                                                                  \
    } while (0)

#define MOVE_BACKWARD(steps)                                                           \
    do {                                                                               \
        uint32_t _steps = (steps);                                                     \
        if ((uint32_t)(ip - chunkStart) < _steps) {                                    \
            TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Program counter out of bounds"); \
            return;                                                                    \
        }                                                                              \
        ip -= _steps;                                                                  \
    } while (0)

#define RECONCILE_STATE()                                                      \
    do {                                                                       \
        frame      = &vm->frames[vm->frameCount - 1];                          \
        stack      = vm->values + frame->stackOffset;                          \
        ip         = frame->returnIP;                                          \
        module     = AS_PTR(&vm->modules.values[frame->moduleId], SemiModule); \
        chunkStart = frame->function->proto->chunk.data;                       \
        chunkEnd   = chunkStart + frame->function->proto->chunk.size;          \
    } while (0)

    // The first frame is already set up before calling this function
    verifyChunk(&vm->frames[vm->frameCount - 1].function->proto->chunk);
    RECONCILE_STATE();

    Instruction instruction;
    for (;;) {
    start_of_vm_loop:
        instruction = *ip;
        switch (GET_OPCODE(instruction)) {
            /* Null Instructions --------------------------------------------------- */
            case OP_NOOP:
                break;

            /* J Type Instructions --------------------------------------------------- */
            case OP_JUMP: {
                uint32_t j = OPERAND_J_J(instruction);
                bool s     = OPERAND_J_S(instruction);

                if (j != 0) {
                    if (s) {
                        MOVE_FORWARD(j);
                    } else {
                        MOVE_BACKWARD(j);
                    }
                    goto start_of_vm_loop;
                }
                break;
            }

            case OP_EXTRA_ARG: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "OP_EXTRA_ARG is not implemented yet");
                break;
            }

            /* K Type Instructions --------------------------------------------------- */
            case OP_TRAP: {
                Instruction operand = OPERAND_K_K(instruction);
                vm->error           = (ErrorId)operand;
                return;
            }

            case OP_C_JUMP: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);
                bool i     = OPERAND_K_I(instruction);
                bool s     = OPERAND_K_S(instruction);

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, &stack[a]);

                Value boolValue;
                ErrorId errorId = table->conversionMethods->toBool(&vm->gc, &boolValue, &stack[a]);
                TRAP_ON_ERROR(vm, errorId, "Failed to convert value to bool for conditional jump");

                if (AS_BOOL(&boolValue) == i && k != 0) {
                    if (s) {
                        MOVE_FORWARD(k);
                    } else {
                        MOVE_BACKWARD(k);
                    }
                    goto start_of_vm_loop;
                }
                break;
            }

            case OP_LOAD_CONSTANT: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);
                bool s     = OPERAND_K_S(instruction);

                Value v;
                if (s) {
                    v = vm->globalConstants[k];
                } else {
                    v = semiConstantTableGet(&module->constantTable, k);
                }

                if (IS_FUNCTION_PROTO(&v)) {
                    stack[a]                 = semiValueFunctionCreate(&vm->gc, AS_FUNCTION_PROTO(&v));
                    ObjectFunction* function = AS_COMPILED_FUNCTION(&stack[a]);
                    TRAP_ON_ERROR(vm, captureUpvalues(vm, stack, function), "Failed to capture upvalues for function");
                } else if (IS_OBJECT_RANGE(&v)) {
                    stack[a] = semiValueRangeCreate(
                        &vm->gc, AS_OBJECT_RANGE(&v)->start, AS_OBJECT_RANGE(&v)->end, AS_OBJECT_RANGE(&v)->step);
                } else {
                    stack[a] = v;
                }

                break;
            }

            case OP_LOAD_BOOL: {
                uint8_t a = OPERAND_K_A(instruction);
                bool i    = OPERAND_K_I(instruction);
                bool s    = OPERAND_K_S(instruction);

                stack[a].header = VALUE_TYPE_BOOL;
                stack[a].as.i   = i;
                break;
            }

            case OP_LOAD_INLINE_INTEGER: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);
                bool s     = OPERAND_K_S(instruction);

                stack[a].header = VALUE_TYPE_INT;
                stack[a].as.i   = s ? (IntValue)k : -(IntValue)k;

                break;
            }

            case OP_LOAD_INLINE_STRING: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);

                stack[a].header     = VALUE_TYPE_INLINE_STRING;
                stack[a].as.is.c[0] = (char)(k & 0xFF);
                stack[a].as.is.c[1] = (char)((k >> 8) & 0xFF);
                if (stack[a].as.is.c[0] == '\0' && stack[a].as.is.c[1] == '\0') {
                    stack[a].as.is.length = 0;
                } else if (stack[a].as.is.c[1] == '\0') {
                    stack[a].as.is.length = 1;
                } else {
                    stack[a].as.is.length = 2;
                }

                break;
            }

            case OP_GET_MODULE_VAR: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);
                bool s     = OPERAND_K_S(instruction);

                ObjectDict* targetDict = s ? &module->exports : &module->globals;
                if (k >= semiDictLen(targetDict)) {
                    TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid module variable index");
                }

                stack[a] = targetDict->values[k];
                break;
            }
            case OP_SET_MODULE_VAR: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);
                bool s     = OPERAND_K_S(instruction);

                ObjectDict* targetDict = s ? &module->exports : &module->globals;
                if (k >= semiDictLen(targetDict)) {
                    TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid module variable index");
                }
                targetDict->values[k] = stack[a];
                break;
            }
            case OP_DEFER_CALL: {
                uint16_t k = OPERAND_K_K(instruction);

                Value v = semiConstantTableGet(&module->constantTable, k);
                if (!IS_FUNCTION_PROTO(&v)) {
                    vm->error = SEMI_ERROR_INVALID_INSTRUCTION;
                    return;
                }

                ObjectFunction* deferFn = semiObjectFunctionCreate(&vm->gc, AS_FUNCTION_PROTO(&v));
                TRAP_ON_ERROR(
                    vm, captureUpvalues(vm, stack, deferFn), "Failed to capture upvalues for deferred function");

                deferFn->prevDeferredFn = frame->deferredFn;
                frame->deferredFn       = deferFn;
                break;
            }

            /* T Type Instructions --------------------------------------------------- */
            case OP_MOVE: {
                uint8_t a = OPERAND_T_A(instruction);
                uint8_t b = OPERAND_T_B(instruction);
                stack[a]  = stack[b];
                break;
            }

            case OP_GET_UPVALUE: {
                uint8_t a = OPERAND_T_A(instruction);
                uint8_t b = OPERAND_T_B(instruction);
                stack[a]  = *frame->function->upvalues[b]->value;
                break;
            }

            case OP_SET_UPVALUE: {
                uint8_t a                            = OPERAND_T_A(instruction);
                uint8_t b                            = OPERAND_T_B(instruction);
                *frame->function->upvalues[a]->value = stack[b];
                break;
            }

            case OP_CLOSE_UPVALUES: {
                uint8_t a = OPERAND_T_A(instruction);
                closeUpvalues(vm, &stack[a]);
                break;
            }

            case OP_ADD: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->add(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_SUBTRACT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->subtract(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_MULTIPLY: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->multiply(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_DIVIDE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->divide(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_FLOOR_DIVIDE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->floorDivide(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_MODULO: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->modulo(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_POWER: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->power(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_NEGATE: {
                Value *ra, *rb;
                load_value_ab(vm, instruction, ra, rb);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->negate(&vm->gc, ra, rb), "Arithmetic failed");
                break;
            }
            case OP_GT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->gt(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_GE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->gte(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_EQ: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->eq(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_NEQ: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->neq(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_AND: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseAnd(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_OR: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseOr(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_XOR: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseXor(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_L_SHIFT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseShiftLeft(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_R_SHIFT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseShiftRight(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_INVERT: {
                Value *ra, *rb;
                load_value_ab(vm, instruction, ra, rb);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseInvert(&vm->gc, ra, rb), "Arithmetic failed");
                break;
            }
            case OP_MAKE_RANGE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                if (!IS_NUMBER(ra) || !IS_NUMBER(rb) || !IS_NUMBER(rc)) {
                    TRAP_ON_ERROR(vm, SEMI_ERROR_UNEXPECTED_TYPE, "Range bounds must be numeric values");
                } else {
                    uint8_t a = OPERAND_T_A(instruction);
                    stack[a]  = semiValueRangeCreate(&vm->gc, *ra, *rb, *rc);
                }
                break;
            }
            case OP_ITER_NEXT: {
                Value *ra, *rb, *rc;

                uint8_t a = OPERAND_T_A(instruction);
                uint8_t b = OPERAND_T_B(instruction);
                uint8_t c = OPERAND_T_C(instruction);

                ra = a == INVALID_LOCAL_REGISTER_ID ? NULL : &stack[a];
                rb = &stack[b];
                rc = &stack[c];

                closeUpvalues(vm, rb);

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, &stack[c]);
                TRAP_ON_ERROR(vm, table->next(&vm->gc, rb, rc), "Failed to get next iterator value");
                bool hasNext = IS_VALID(rb);
                if (hasNext && ra != NULL) {
                    Value one = semiValueNewInt(1);
                    table     = semiVMGetMagicMethodsTable(vm, &stack[a]);
                    TRAP_ON_ERROR(
                        vm, table->numericMethods->add(&vm->gc, ra, ra, &one), "Failed to increment iterator index");
                }
                if (hasNext) {
                    MOVE_FORWARD(2);
                    goto start_of_vm_loop;
                }
                break;
            }
            case OP_BOOL_NOT: {
                Value *ra, *rb;
                load_value_ab(vm, instruction, ra, rb);
                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->conversionMethods->inverse(&vm->gc, ra, rb), "Arithmetic failed");
                break;
            }
            case OP_GET_ATTR: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "OP_GET_ATTR is not implemented yet");
                break;
            }
            case OP_NEW_COLLECTION: {
                Value* ra  = stack + OPERAND_T_A(instruction);
                TypeId b   = (TypeId)OPERAND_T_B(instruction);
                uint8_t c  = OPERAND_T_C(instruction);
                uint8_t kb = OPERAND_T_KB(instruction);
                if (!kb) {
                    Value rb = stack[(uint8_t)b];
                    if (!IS_INT(&rb)) {
                        TRAP_ON_ERROR(vm, SEMI_ERROR_UNEXPECTED_TYPE, "Expected integer for NEW_COLLECTION size");
                    }
                    b = (TypeId)AS_INT(&rb);
                }

                switch (b) {
                    case BASE_VALUE_TYPE_LIST: {
                        uint32_t capacity = c == INVALID_LOCAL_REGISTER_ID ? 0 : (uint32_t)c;
                        *ra               = semiValueListCreate(&vm->gc, capacity);
                        break;
                    }
                    case BASE_VALUE_TYPE_DICT: {
                        *ra = semiValueDictCreate(&vm->gc);
                        break;
                    }
                    default: {
                        TRAP_ON_ERROR(
                            vm, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "Unsupported collection type for NEW_COLLECTION");
                    }
                }
                break;
            }
            case OP_SET_ATTR: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "OP_SET_ATTR is not implemented yet");
                break;
            }
            case OP_GET_ITEM: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->collectionMethods->getItem(&vm->gc, ra, rb, rc), "GetItem failed");
                break;
            }
            case OP_SET_ITEM: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, ra);
                TRAP_ON_ERROR(vm, table->collectionMethods->setItem(&vm->gc, ra, rb, rc), "SetItem failed");
                break;
            }
            case OP_DEL_ITEM: {
                Value *ra, *rb, *rc;
                uint8_t c = OPERAND_T_C(instruction);
                bool kc   = OPERAND_T_KC(instruction);
                uint8_t a = OPERAND_T_A(instruction);
                uint8_t b = OPERAND_T_B(instruction);
                Value valueTempA, valueTempC;
                if (a == INVALID_LOCAL_REGISTER_ID) {
                    ra = &valueTempA;
                } else {
                    ra = &stack[a];
                }
                rb = &stack[b];
                if (kc) {
                    valueTempC.header = VALUE_TYPE_INT;
                    valueTempC.as.i   = c + INT8_MIN;
                    rc                = &valueTempC;
                } else {
                    rc = &stack[c];
                }

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, ra);
                TRAP_ON_ERROR(vm, table->collectionMethods->delItem(&vm->gc, ra, rb, rc), "DeleteItem failed");
                break;
            }
            case OP_CONTAIN: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, rc);
                TRAP_ON_ERROR(vm, table->collectionMethods->contain(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_APPEND_LIST: {
                Value* ra        = stack + OPERAND_T_A(instruction);
                uint8_t startReg = OPERAND_T_B(instruction);
                uint8_t count    = OPERAND_T_C(instruction);

                ObjectList stackList;
                stackList.values = stack + startReg;
                stackList.size   = count;

                Value temp;
                temp.header = VALUE_TYPE_LIST;
                temp.as.obj = &stackList.obj;

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, ra);
                TRAP_ON_ERROR(vm, table->collectionMethods->extend(&vm->gc, ra, &temp), "List append failed");
                break;
            }
            case OP_APPEND_MAP: {
                Value* ra          = stack + OPERAND_T_A(instruction);
                uint8_t startReg   = OPERAND_T_B(instruction);
                uint8_t count      = OPERAND_T_C(instruction);
                Value* startValues = stack + startReg;

                MagicMethodsTable* table = semiVMGetMagicMethodsTable(vm, ra);

                for (uint32_t i = 0; i < count; i++) {
                    Value key   = startValues[i * 2];
                    Value value = startValues[i * 2 + 1];

                    TRAP_ON_ERROR(
                        vm, table->collectionMethods->setItem(&vm->gc, ra, &key, &value), "Map insert failed");
                }
                break;
            }
            case OP_CALL: {
                uint8_t a   = OPERAND_T_A(instruction);
                uint8_t b   = OPERAND_T_B(instruction);
                Value* args = &stack[a + 1];

                switch (VALUE_TYPE(&stack[a])) {
                    case VALUE_TYPE_NATIVE_FUNCTION: {
                        NativeFunction* nativeFunc = AS_NATIVE_FUNCTION(&stack[a]);
                        TRAP_ON_ERROR(vm, (*nativeFunc)(vm, b, args, &stack[a]), "Native function call failed");
                        break;
                    }
                    case VALUE_TYPE_COMPILED_FUNCTION: {
                        // We can safely set the PC to the next instruction because we ensure
                        // every chunk ends with an OP_RETURN or OP_TRAP. Since this instruction
                        // is OP_CALL, there must be at least one instruction after it.
                        frame->returnIP = ip + 1;

                        ObjectFunction* func   = AS_COMPILED_FUNCTION(&stack[a]);
                        FunctionProto* fnProto = func->proto;
                        if (fnProto->arity != b) {
                            TRAP_ON_ERROR(vm, SEMI_ERROR_ARGS_COUNT_MISMATCH, "Function arguments mismatch");
                        }
                        if (!verifyChunk(&fnProto->chunk)) {
                            TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_FUNCTION_PROTO, "Invalid function prototype");
                        }

                        appendFrame(vm, func, args);
                        if (vm->error != 0) {
                            return;
                        }

                        RECONCILE_STATE();
                        goto start_of_vm_loop;
                    }
                    default:
                        TRAP_ON_ERROR(vm, SEMI_ERROR_UNEXPECTED_TYPE, "Attempted to call a non-function value");
                }
                break;
            }
            case OP_RETURN: {
                uint8_t a = OPERAND_T_A(instruction);

                // Execute deferred functions if any exist
                if (frame->deferredFn != NULL) {
                    ObjectFunction* deferFn = frame->deferredFn;
                    frame->deferredFn       = deferFn->prevDeferredFn;

                    // Set up the current frame to return to the same OP_RETURN instruction
                    frame->returnIP = ip;

                    // Push a new frame for deferred function execution starting from a+1
                    Value* newStackStart = a != INVALID_LOCAL_REGISTER_ID ? stack + a + 1 : stack;
                    closeUpvalues(vm, newStackStart);
                    appendFrame(vm, deferFn, newStackStart);
                    if (vm->error != 0) {
                        return;
                    }

                    RECONCILE_STATE();
                    goto start_of_vm_loop;
                }

                if (vm->frameCount <= 1) {
                    // This is only when the last statement of the module is an expression.
                    // We use this in REPL to print the result of the expression.
                    if (a != INVALID_LOCAL_REGISTER_ID) {
                        vm->returnedValue = stack + a;
                    }
                    vm->frameCount = 0;
                    return;
                }
                if (a != INVALID_LOCAL_REGISTER_ID) {
                    // TODO: Make the stack of the main frame starts at vm->values[1] so that
                    //       we can always do stack[-1] to get the caller's return value slot.
                    if (stack > vm->values) {
                        Value* caller = stack - 1;
                        *caller       = stack[a];
                    } else {
                        TRAP_ON_ERROR(vm, SEMI_ERROR_INTERNAL_ERROR, "Stack underflow on function return");
                    }
                } else if (frame->function->proto->coarity > 0) {
                    // If the function has coarity, we need to return a value.
                    // This only happens when the last statement of the function doesn't
                    // have return statement.
                    TRAP_ON_ERROR(vm, SEMI_ERROR_MISSING_RETURN_VALUE, "Missing return value for function");
                }

                closeUpvalues(vm, stack);
                vm->frameCount--;

                RECONCILE_STATE();
                goto start_of_vm_loop;
            }
            case OP_CHECK_TYPE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);

                TypeId targetypeId    = (TypeId)BASE_TYPE(rb);
                TypeId expectedTypeId = (TypeId)AS_INT(rc);
                *ra                   = semiValueNewBool(targetypeId == expectedTypeId);
                break;
            }
            default: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid opcode encountered in VM");
                break;
            }
        }

        ip++;
    }
}

ErrorId semiVMRunMainModule(SemiVM* vm, ModuleId moduleId) {
    vm->error         = 0;
    vm->returnedValue = NULL;

    if (moduleId >= vm->modules.len) {
        return SEMI_ERROR_MODULE_NOT_FOUND;
    }

    SemiModule* module = AS_PTR(&vm->modules.values[moduleId], SemiModule);

    ObjectFunction mainFunction;
    mainFunction.obj.header   = VALUE_TYPE_COMPILED_FUNCTION;
    mainFunction.proto        = module->moduleInit;
    mainFunction.upvalueCount = 0;

    appendFrame(vm, &mainFunction, vm->values);
    runMainLoop(vm);
    return vm->error;
}

ErrorId semiRunModule(SemiVM* vm, const char* moduleName, uint8_t moduleNameLength) {
    InternedChar* internedModuleName = semiSymbolTableGet(&vm->symbolTable, moduleName, moduleNameLength);
    if (internedModuleName == NULL) {
        return SEMI_ERROR_MODULE_NOT_FOUND;
    }
    IdentifierId moduleNameIdentifierId = semiSymbolTableGetId(internedModuleName);
    Value moduleValue                   = semiDictGet(&vm->modules, semiValueNewInt(moduleNameIdentifierId));
    if (IS_INVALID(&moduleValue)) {
        return SEMI_ERROR_MODULE_NOT_FOUND;
    }

    SemiModule* module = AS_PTR(&moduleValue, SemiModule);
    ObjectFunction mainFunction;
    mainFunction.obj.header   = VALUE_TYPE_COMPILED_FUNCTION;
    mainFunction.proto        = module->moduleInit;
    mainFunction.upvalueCount = 0;
    appendFrame(vm, &mainFunction, vm->values);

    vm->error         = 0;
    vm->returnedValue = NULL;
    runMainLoop(vm);

    if (module->moduleInit != NULL) {
        semiFunctionProtoDestroy(&vm->gc, module->moduleInit);
    }
    module->moduleInit = NULL;
    return vm->error;
}