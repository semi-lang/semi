// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./vm.h"

#include <limits.h>
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
    semiConstantTableInit(gc, &module->constantTable);
    module->moduleInit = NULL;

    return module;
}

SemiModule* semiVMModuleCreateFrom(GC* gc, SemiModule* source) {
    SemiModule* module = semiMalloc(gc, sizeof(SemiModule));
    if (module == NULL) {
        return NULL;
    }

    *module = (SemiModule){
        .moduleId      = source->moduleId,
        .exports       = source->exports,
        .globals       = source->globals,
        .constantTable = source->constantTable,
        .moduleInit    = NULL,
    };

    return module;
}

void semiVMModuleDestroy(GC* gc, SemiModule* module) {
    semiObjectStackDictCleanup(gc, &module->exports);
    semiObjectStackDictCleanup(gc, &module->globals);
    semiConstantTableCleanup(&module->constantTable);
    if (module->moduleInit != NULL) {
        semiFunctionTemplateDestroy(gc, module->moduleInit);
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

DEFINE_DARRAY(ModuleList, SemiModule*, uint16_t, UINT16_MAX)
DEFINE_DARRAY(GlobalIdentifierList, IdentifierId, ModuleVariableId, UINT16_MAX)

static ErrorId captureUpvalues(SemiVM* vm, Value* currentBase, ObjectFunction* function) {
    FunctionTemplate* fnTemplate = function->fnTemplate;
    if (fnTemplate->upvalueCount == 0) {
        return 0;  // No upvalues to capture
    } else if (vm->frameCount == 0) {
        return SEMI_ERROR_INTERNAL_ERROR;  // No current frame to capture upvalues from
    }

    ObjectUpvalue** currentUpvalues = vm->frames[vm->frameCount - 1].function->upvalues;
    for (uint8_t i = 0; i < function->upvalueCount; i++) {
        uint8_t index = fnTemplate->upvalues[i].index;
        bool isLocal  = fnTemplate->upvalues[i].isLocal;
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

    ObjectFunction* rootFunction;
    if ((rootFunction = config.reallocateFn(NULL, sizeof(ObjectFunction), config.reallocateUserData)) == NULL) {
        config.reallocateFn(vm->frames, 0, config.reallocateUserData);
        config.reallocateFn(vm->values, 0, config.reallocateUserData);
        config.reallocateFn(vm, 0, config.reallocateUserData);
        return NULL;
    }
    vm->rootFunction = rootFunction;
    vm->nextModuleId = 0;

    ModuleListInit(&vm->modules);
    semiGCInit(&vm->gc, config.reallocateFn, config.reallocateUserData);
    semiSymbolTableInit(&vm->gc, &vm->symbolTable);
    semiPrimitivesIntializeBuiltInPrimitives(&vm->gc, &vm->classes);

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

    for (uint16_t i = 0; i < vm->modules.size; i++) {
        SemiModule* module = vm->modules.data[i];
        semiVMModuleDestroy(&vm->gc, module);
    }
    ModuleListCleanup(&vm->gc, &vm->modules);

    for (uint16_t i = MIN_CUSTOM_BASE_VALUE_TYPE; i < vm->classes.classCount; i++) {
        semiFree(&vm->gc, &vm->classes.classMethods[i], sizeof(MagicMethodsTable));
    }
    semiPrimitivesCleanupClassTable(&vm->gc, &vm->classes);
    semiSymbolTableCleanup(&vm->symbolTable);

    SemiReallocateFn reallocateFn = vm->gc.reallocateFn;
    void* reallocateUserData      = vm->gc.reallocateUserData;
    semiGCCleanup(&vm->gc);

    if (vm->rootFunction != NULL) {
        reallocateFn(vm->rootFunction, 0, reallocateUserData);
    }
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

static inline uint32_t nextPowerOfTwoCapacity(uint32_t x) {
    if (x <= 8) {
        return 8;
    }

#if __has_builtin(__builtin_clzl)
    // From https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html
    //
    //   * __builtin_clz:  Returns the number of leading 0-bits in x, starting at the most
    //                     significant bit position. If x is 0, the result is undefined.
    //   * __builtin_clzl: Similar to __builtin_clz, except the argument type is unsigned long.
    return (uint32_t)1 << (sizeof(unsigned long) * CHAR_BIT - (size_t)__builtin_clzl(x));
#else

    // From https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
#endif
}

static ErrorId growVMStackSize(SemiVM* vm, uint32_t requiredSize) {
    if (requiredSize > SEMI_MAX_STACK_SIZE) {
        return SEMI_ERROR_STACK_OVERFLOW;
    }

    for (uint32_t i = 0; i < vm->frameCount; i++) {
        vm->frames[i].callerStack.offset = (uint32_t)(vm->frames[i].callerStack.base - vm->values);
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

    for (uint32_t i = 0; i < vm->frameCount; i++) {
        vm->frames[i].callerStack.base = vm->values + vm->frames[i].callerStack.offset;
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

static inline MagicMethodsTable* getMagicMethodsTable(SemiVM* vm, Value* value) {
    BaseValueType type = BASE_TYPE(value);
    return type < vm->classes.classCount ? &vm->classes.classMethods[type] : &vm->classes.classMethods[0];
}

#ifdef SEMI_DEBUG_MSG
#define TRAP_ON_ERROR(vm, errorId, message) \
    do {                                    \
        SemiVM* _vm    = (vm);              \
        ErrorId _errId = (errorId);         \
        if (_errId != 0) {                  \
            _vm->error        = _errId;     \
            _vm->errorMessage = (message);  \
            return _vm->error;              \
        }                                   \
    } while (0)
#else
#define TRAP_ON_ERROR(vm, errorId, message) \
    do {                                    \
        SemiVM* _vm    = (vm);              \
        ErrorId _errId = (errorId);         \
        if (_errId != 0) {                  \
            _vm->error = _errId;            \
            return _vm->error;              \
        }                                   \
    } while (0)
#endif
ErrorId semiVMRunMainModule(SemiVM* vm, SemiModule* module) {
    vm->error                      = 0;
    vm->returnedValue              = NULL;
    vm->rootFunction->obj.header   = VALUE_TYPE_COMPILED_FUNCTION;
    vm->rootFunction->fnTemplate   = module->moduleInit;
    vm->rootFunction->upvalueCount = 0;
    vm->frames[0]                  = (Frame){.callerStack       = {.base = vm->values},
                                             .returnValueOffset = 0,
                                             .callerPC          = 0,
                                             .function          = vm->rootFunction,
                                             .moduleId          = 0};
    vm->frameCount                 = 1;

    ModuleListEnsureCapacity(&vm->gc, &vm->modules, 1);
    if (vm->modules.size < 1) {
        vm->modules.size    = 1;
        vm->modules.data[0] = module;
    } else {
        semiFunctionTemplateDestroy(&vm->gc, vm->modules.data[0]->moduleInit);
        semiFree(&vm->gc, vm->modules.data[0], sizeof(SemiModule));
        vm->modules.data[0] = module;
    }

    Instruction* code = module->moduleInit->chunk.data;
    uint32_t codeSize = module->moduleInit->chunk.size;
    PCLocation pc     = 0;
    Value* stack      = vm->values;

    for (;;) {
        if (pc >= codeSize) {
            TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Program counter out of bounds");
        }
        Instruction instruction = code[pc];

        switch (GET_OPCODE(instruction)) {
            /* Null Instructions --------------------------------------------------- */
            case OP_NOOP:
                break;

            /* J Type Instructions --------------------------------------------------- */
            case OP_JUMP: {
                uint32_t j = OPERAND_J_J(instruction);
                bool s     = OPERAND_J_S(instruction);

                if (j != 0) {
                    if (s && j <= UINT32_MAX - pc) {
                        pc = pc + j;
                    } else if (!s && j <= pc) {
                        pc = pc - j;
                    } else {
                        TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Invalid program counter after jump");
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
                return vm->error;
            }

            case OP_C_JUMP: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);
                bool i     = OPERAND_K_I(instruction);
                bool s     = OPERAND_K_S(instruction);

                MagicMethodsTable* table = getMagicMethodsTable(vm, &stack[a]);

                Value boolValue;
                ErrorId errorId = table->conversionMethods->toBool(&vm->gc, &boolValue, &stack[a]);
                TRAP_ON_ERROR(vm, errorId, "Failed to convert value to bool for conditional jump");

                if (AS_BOOL(&boolValue) == i && k != 0) {
                    if (s && k <= UINT32_MAX - pc) {
                        pc = pc + k;
                    } else if (!s && pc >= k) {
                        pc = pc - k;
                    } else {
                        TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Invalid program counter after conditional jump");
                    }

                    goto start_of_vm_loop;
                }
                break;
            }

            case OP_LOAD_CONSTANT: {
                uint8_t a  = OPERAND_K_A(instruction);
                uint16_t k = OPERAND_K_K(instruction);
                bool s     = OPERAND_K_S(instruction);

                ModuleId moduleId = vm->frames[vm->frameCount - 1].moduleId;
                SemiModule* mod   = vm->modules.data[moduleId];
                Value v;
                if (s) {
                    if (k >= vm->globalIdentifiers.size) {
                        // TODO: This should be encoded in the module and checked before running it.
                        TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid global constant index");
                    }
                    v = vm->globalConstants[k];
                } else {
                    v = semiConstantTableGet(&mod->constantTable, k);
                }

                if (IS_FUNCTION_TEMPLATE(&v)) {
                    stack[a]                 = semiValueFunctionCreate(&vm->gc, AS_FUNCTION_TEMPLATE(&v));
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

                ModuleId moduleId = vm->frames[vm->frameCount - 1].moduleId;
                if (moduleId >= vm->modules.size) {
                    TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid module ID");
                }
                SemiModule* mod        = vm->modules.data[moduleId];
                ObjectDict* targetDict = s ? &mod->exports : &mod->globals;
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

                ModuleId moduleId = vm->frames[vm->frameCount - 1].moduleId;
                if (moduleId >= vm->modules.size) {
                    TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid module ID");
                }
                SemiModule* mod        = vm->modules.data[moduleId];
                ObjectDict* targetDict = s ? &mod->exports : &mod->globals;
                if (k >= semiDictLen(targetDict)) {
                    TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid module variable index");
                }
                targetDict->values[k] = stack[a];
                break;
            }
            case OP_DEFER_CALL: {
                // TODO: Implement defer functionality and remove early return.
                vm->error = SEMI_ERROR_UNIMPLEMENTED_FEATURE;
                return vm->error;

                uint16_t k = OPERAND_K_K(instruction);
                bool s     = OPERAND_K_S(instruction);

                Frame* currentFrame = &vm->frames[vm->frameCount - 1];
                ModuleId moduleId   = currentFrame->moduleId;
                SemiModule* mod     = vm->modules.data[moduleId];

                Value v = semiConstantTableGet(&mod->constantTable, k);
                if (!IS_FUNCTION_TEMPLATE(&v)) {
                    vm->error = SEMI_ERROR_INVALID_INSTRUCTION;
                    return vm->error;
                }

                ObjectFunction* deferFn = semiObjectFunctionCreate(&vm->gc, AS_FUNCTION_TEMPLATE(&v));
                TRAP_ON_ERROR(
                    vm, captureUpvalues(vm, stack, deferFn), "Failed to capture upvalues for deferred function");

                deferFn->prevDeferredFn  = currentFrame->deferredFn;
                currentFrame->deferredFn = deferFn;
                break;
            }

            /* T Type Instructions --------------------------------------------------- */
            case OP_MOVE: {
                uint8_t a = OPERAND_T_A(instruction);
                uint8_t b = OPERAND_T_B(instruction);
                uint8_t c = OPERAND_T_C(instruction);
                bool kc   = OPERAND_T_KC(instruction);

                stack[a] = stack[b];
                if (c != 0) {
                    if (c != 0) {
                        if (kc && c <= UINT32_MAX - pc) {
                            pc = pc + c;
                        } else if (!kc && pc >= c) {
                            pc = pc - c;
                        } else {
                            TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Invalid program counter after move instruction");
                        }

                        goto start_of_vm_loop;
                    }
                }
                break;
            }

            case OP_GET_UPVALUE: {
                uint8_t a = OPERAND_T_A(instruction);
                uint8_t b = OPERAND_T_B(instruction);
                stack[a]  = *vm->frames[vm->frameCount - 1].function->upvalues[b]->value;
                break;
            }

            case OP_SET_UPVALUE: {
                uint8_t a                                                    = OPERAND_T_A(instruction);
                uint8_t b                                                    = OPERAND_T_B(instruction);
                *vm->frames[vm->frameCount - 1].function->upvalues[a]->value = stack[b];
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
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->add(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_SUBTRACT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->subtract(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_MULTIPLY: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->multiply(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_DIVIDE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->divide(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_FLOOR_DIVIDE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->floorDivide(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_MODULO: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->modulo(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_POWER: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->power(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_NEGATE: {
                Value *ra, *rb;
                load_value_ab(vm, instruction, ra, rb);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->negate(&vm->gc, ra, rb), "Arithmetic failed");
                break;
            }
            case OP_GT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->gt(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_GE: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->gte(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_EQ: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->eq(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_NEQ: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->comparisonMethods->neq(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_AND: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseAnd(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_OR: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseOr(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_XOR: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseXor(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_L_SHIFT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseShiftLeft(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_R_SHIFT: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->numericMethods->bitwiseShiftRight(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_BITWISE_INVERT: {
                Value *ra, *rb;
                load_value_ab(vm, instruction, ra, rb);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
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

                MagicMethodsTable* table = getMagicMethodsTable(vm, &stack[c]);
                TRAP_ON_ERROR(vm, table->next(&vm->gc, rb, rc), "Failed to get next iterator value");
                bool hasNext = IS_VALID(rb);
                if (hasNext && ra != NULL) {
                    Value one = semiValueNewInt(1);
                    table     = getMagicMethodsTable(vm, &stack[a]);
                    TRAP_ON_ERROR(
                        vm, table->numericMethods->add(&vm->gc, ra, ra, &one), "Failed to increment iterator index");
                }
                if (hasNext) {
                    pc += 2;
                    goto start_of_vm_loop;
                }
                break;
            }
            case OP_BOOL_NOT: {
                Value *ra, *rb;
                load_value_ab(vm, instruction, ra, rb);
                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->conversionMethods->inverse(&vm->gc, ra, rb), "Arithmetic failed");
                break;
            }
            case OP_GET_ATTR: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "OP_GET_ATTR is not implemented yet");
                break;
            }
            case OP_SET_ATTR: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "OP_SET_ATTR is not implemented yet");
                break;
            }
            case OP_GET_ITEM: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);

                MagicMethodsTable* table = getMagicMethodsTable(vm, rb);
                TRAP_ON_ERROR(vm, table->collectionMethods->getItem(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_SET_ITEM: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);

                MagicMethodsTable* table = getMagicMethodsTable(vm, ra);
                TRAP_ON_ERROR(vm, table->collectionMethods->setItem(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_CONTAIN: {
                Value *ra, *rb, *rc;
                load_value_abc(vm, instruction, ra, rb, rc);

                MagicMethodsTable* table = getMagicMethodsTable(vm, rc);
                TRAP_ON_ERROR(vm, table->collectionMethods->contain(&vm->gc, ra, rb, rc), "Arithmetic failed");
                break;
            }
            case OP_CALL: {
                uint8_t a   = OPERAND_T_A(instruction);
                uint8_t b   = OPERAND_T_B(instruction);
                Value* args = &stack[a + 1];

                switch (VALUE_TYPE(&stack[a])) {
                    case VALUE_TYPE_NATIVE_FUNCTION: {
                        NativeFunction* nativeFunc = AS_NATIVE_FUNCTION(&stack[a]);
                        TRAP_ON_ERROR(vm, (*nativeFunc)(&vm->gc, b, args, &stack[a]), "Native function call failed");
                        break;
                    }
                    case VALUE_TYPE_COMPILED_FUNCTION: {
                        ObjectFunction* func         = AS_COMPILED_FUNCTION(&stack[a]);
                        FunctionTemplate* fnTemplate = func->fnTemplate;
                        if (fnTemplate->arity != b) {
                            TRAP_ON_ERROR(vm, SEMI_ERROR_ARGS_COUNT_MISMATCH, "Function arguments mismatch");
                        }

                        uint32_t stackOffset = (uint32_t)(stack - vm->values);
                        if (stackOffset > SEMI_MAX_STACK_SIZE - (a + 1 + fnTemplate->maxStackSize)) {
                            TRAP_ON_ERROR(vm, SEMI_ERROR_STACK_OVERFLOW, "Stack overflow on function call");
                        }
                        if ((vm->values + vm->valueCapacity) - args < fnTemplate->maxStackSize) {
                            TRAP_ON_ERROR(vm,
                                          growVMStackSize(vm, stackOffset + a + 1 + fnTemplate->maxStackSize),
                                          "Failed to grow VM stack for function call");
                            stack = vm->values + stackOffset;
                        }
                        if (vm->frameCount >= vm->frameCapacity) {
                            TRAP_ON_ERROR(vm,
                                          growVMFrameSize(vm, vm->frameCount + 1),
                                          "Failed to grow VM frame stack for function call");
                        }

                        PCLocation nextPC = pc + 1;
                        if (nextPC == UINT32_MAX) {
                            TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Invalid program counter after function call");
                        }
                        vm->frames[vm->frameCount] = (Frame){
                            .callerStack       = {.base = stack},
                            .returnValueOffset = (uint32_t)(stackOffset + a),
                            .callerPC          = nextPC,
                            .function          = func,
                            .deferredFn        = NULL,
                            .moduleId          = fnTemplate->moduleId,
                            .coarity           = fnTemplate->coarity,
                        };
                        vm->frameCount++;

                        code     = fnTemplate->chunk.data;
                        codeSize = fnTemplate->chunk.size;
                        pc       = 0;
                        stack    = args;

                        goto start_of_vm_loop;
                    }
                    default:
                        TRAP_ON_ERROR(vm, SEMI_ERROR_UNEXPECTED_TYPE, "Attempted to call a non-function value");
                }
                break;
            }
            case OP_RETURN: {
                uint8_t a = OPERAND_T_A(instruction);
                if (vm->frameCount <= 1) {
                    // This is only when the last statement of the module is an expression.
                    // We use this in REPL to print the result of the expression.
                    if (a != INVALID_LOCAL_REGISTER_ID) {
                        vm->returnedValue = stack + a;
                    }
                    vm->frameCount = 0;
                    return 0;
                }
                Frame* frame         = &vm->frames[vm->frameCount - 1];
                Frame* previousFrame = &vm->frames[vm->frameCount - 2];

                // TODO: Implement deferred functions execution here.

                if (a != INVALID_LOCAL_REGISTER_ID && frame->returnValueOffset != UINT32_MAX) {
                    Value* caller = vm->values + frame->returnValueOffset;
                    *caller       = stack[a];
                } else if (frame->function->fnTemplate->coarity > 0) {
                    // If the function has coarity, we need to return a value.
                    // This only happens when the last statement of the function doesn't
                    // have return statement.
                    TRAP_ON_ERROR(vm, SEMI_ERROR_MISSING_RETURN_VALUE, "Missing return value for function");
                }

                closeUpvalues(vm, stack);
                pc       = frame->callerPC;
                stack    = frame->callerStack.base;
                code     = previousFrame->function->fnTemplate->chunk.data;
                codeSize = previousFrame->function->fnTemplate->chunk.size;
                vm->frameCount--;

                goto start_of_vm_loop;
            }
            case OP_CHECK_TYPE: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_UNIMPLEMENTED_FEATURE, "OP_CHECK_TYPE is not implemented yet");
                break;
            }
            default: {
                TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_INSTRUCTION, "Invalid opcode encountered in VM");
                break;
            }
        }

        if (pc < UINT32_MAX) {
            pc += 1;
        } else {
            TRAP_ON_ERROR(vm, SEMI_ERROR_INVALID_PC, "Invalid program counter after instruction execution");
        }

    start_of_vm_loop:
        // label at end of compound statement is a C23 extension
        (void)0;
    }

    return vm->error;
}
