// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "../../include/semi/error.h"
#include "../../src/compiler.h"
#include "../../src/const_table.h"
#include "../../src/primitives.h"
#include "../../src/symbol_table.h"
#include "../../src/vm.h"

typedef struct builtInFunctions {
    const char* name;
    ErrorId (*function)(SemiVM* vm, uint8_t argCount, Value* args, Value* ret);
} builtInFunctions;

ErrorId nowFunction(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    (void)vm;
    (void)argCount;
    (void)args;

    struct timespec ts;
    if (!timespec_get(&ts, TIME_UTC)) {
        return 1;
    }
    *ret = semiValueIntCreate((IntValue)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    return 0;
}

static void printValue(Value* value) {
    switch (VALUE_TYPE(value)) {
        case VALUE_TYPE_BOOL:
            printf("%s", AS_BOOL(value) ? "true" : "false");
            break;
        case VALUE_TYPE_INVALID:
            printf("invalid");
            break;
        case VALUE_TYPE_INT:
            printf("%lld", AS_INT(value));
            break;
        case VALUE_TYPE_FLOAT:
            printf("%g", AS_FLOAT(value));
            break;
        case VALUE_TYPE_INLINE_STRING: {
            InlineString inlineStr = AS_INLINE_STRING(value);
            for (uint8_t j = 0; j < inlineStr.length; j++) {
                printf("%c", inlineStr.c[j]);
            }
            break;
        }
        case VALUE_TYPE_OBJECT_STRING: {
            ObjectString* str = AS_OBJECT_STRING(value);
            printf("%.*s", (int)str->length, str->str);
            break;
        }
        case VALUE_TYPE_LIST: {
            ObjectList* list = AS_LIST(value);
            if (list->size == 0) {
                printf("List[]");
                break;
            }

            printf("List[ ");
            for (uint32_t j = 0; j < list->size; j++) {
                if (j > 0) {
                    printf(", ");
                }
                printValue(&list->values[j]);
            }
            printf(" ]");
            break;
        }
        case VALUE_TYPE_DICT: {
            ObjectDict* dict = AS_DICT(value);
            if (dict->len == 0) {
                printf("Dict[]");
                break;
            }
            printf("Dict[ ");
            uint32_t j = 0;
            for (;;) {
                if (IS_VALID(&dict->keys[j].key)) {
                    printValue(&dict->keys[j].key);
                    printf(": ");
                    printValue(&dict->values[j]);
                }
                j++;
                if (j < dict->len) {
                    printf(", ");
                } else {
                    break;
                }
            }
            printf(" ]");
            break;
        }
        default:
            printf("<unprintable value type %d>", (int)VALUE_TYPE(value));
            break;
    }
    return;
}

ErrorId printFunction(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    (void)vm;
    (void)ret;

    if (argCount == 0) {
        return SEMI_ERROR_INVALID_VALUE;
    }

    for (uint8_t i = 0; i < argCount; i++) {
        printValue(&args[i]);
        if (i < argCount - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

ErrorId minFunction(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    if (argCount == 0) {
        return SEMI_ERROR_INVALID_VALUE;
    }
    ErrorId err;
    Value* minItem             = &args[0];
    MagicMethodsTable* methods = semiVMGetMagicMethodsTable(vm, minItem);
    Value cmpResult;

    for (uint8_t i = 1; i < argCount; i++) {
        Value* item = &args[i];
        if ((err = methods->comparisonMethods->lt(&vm->gc, &cmpResult, item, minItem)) != 0) {
            return err;
        }

        if (AS_BOOL(&cmpResult)) {
            minItem = item;
            methods = semiVMGetMagicMethodsTable(vm, minItem);
        }
    }
    *ret = *minItem;
    return 0;
}

ErrorId maxFunction(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    if (argCount == 0) {
        return SEMI_ERROR_INVALID_VALUE;
    }
    ErrorId err;
    Value* minItem             = &args[0];
    MagicMethodsTable* methods = semiVMGetMagicMethodsTable(vm, minItem);
    Value cmpResult;

    for (uint8_t i = 1; i < argCount; i++) {
        Value* item = &args[i];
        if ((err = methods->comparisonMethods->gt(&vm->gc, &cmpResult, item, minItem)) != 0) {
            return err;
        }

        if (AS_BOOL(&cmpResult)) {
            minItem = item;
            methods = semiVMGetMagicMethodsTable(vm, minItem);
        }
    }
    *ret = *minItem;
    return 0;
}

ErrorId appendFunction(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    (void)ret;
    if (argCount < 2) {
        return SEMI_ERROR_INVALID_VALUE;
    }

    Value* listValue = &args[0];

    MagicMethodsTable* methods = semiVMGetMagicMethodsTable(vm, listValue);

    ObjectList stackList;
    stackList.values = &args[1];
    stackList.size   = argCount - 1;

    Value temp;
    temp.header = VALUE_TYPE_LIST;
    temp.as.obj = &stackList.obj;

    return methods->collectionMethods->extend(&vm->gc, listValue, &temp);
}

ErrorId lenFunction(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    if (argCount != 1) {
        return SEMI_ERROR_INVALID_VALUE;
    }

    Value* collectionValue     = &args[0];
    MagicMethodsTable* methods = semiVMGetMagicMethodsTable(vm, collectionValue);
    return methods->collectionMethods->len(&vm->gc, ret, collectionValue);
}

static const builtInFunctions builtInFunctionList[] = {
    { "print",  printFunction},
    {   "now",    nowFunction},
    {   "min",    minFunction},
    {   "max",    maxFunction},
    {"append", appendFunction},
    {   "len",    lenFunction},
};

ErrorId compileAndRunInternal(SemiVM* vm, const char* source, unsigned int length) {
    const char* scriptMainModuleName   = "<script>";
    uint8_t scriptMainModuleNameLength = (uint8_t)strlen(scriptMainModuleName);

    SemiModuleSource moduleSource = {
        .source     = source,
        .length     = length,
        .name       = scriptMainModuleName,
        .nameLength = scriptMainModuleNameLength,
    };

    SemiModule* module = semiVMCompileModule(vm, &moduleSource);
    if (module == NULL) {
        ErrorId errorId = vm->error;
        uint32_t line   = vm->errorDetails.compileError.line;
        size_t column   = vm->errorDetails.compileError.column;

#if defined(SEMI_DEBUG_MSG)
        const char* message = vm->errorMessage != NULL ? vm->errorMessage : "Unknown error";
        fprintf(stderr, "Error %u at line %d, column %zu: %s\n", errorId, line, column, vm->errorMessage);
#else
        fprintf(stderr, "Error %u at line %d, column %zu\n", errorId, line, column);
#endif  // defined(SEMI_DEBUG_MSG)
        return errorId;
    }

    return semiRunModule(vm, scriptMainModuleName, scriptMainModuleNameLength);
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
int compileAndRun(const char* str) {
    if (str == NULL) {
        return SEMI_ERROR_INVALID_VALUE;
    }

    SemiVMConfig config;
    semiInitConfig(&config);
    SemiVM* vm = semiCreateVM(&config);

    if (vm == NULL) {
        return SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
    }

    for (size_t i = 0; i < sizeof(builtInFunctionList) / sizeof(builtInFunctions); i++) {
        semiVMAddGlobalVariable(vm,
                                builtInFunctionList[i].name,
                                (IdentifierLength)strlen(builtInFunctionList[i].name),
                                semiValueNativeFunctionCreate(builtInFunctionList[i].function));
    }

    ErrorId errId = compileAndRunInternal(vm, str, (unsigned int)strlen(str));

    semiDestroyVM(vm);

    return (int)errId;
}

#ifndef __EMSCRIPTEN__
int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_code>\n", argv[0]);
        return 1;
    }

    return compileAndRun(argv[1]);
}
#endif
