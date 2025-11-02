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

static const char* nowFunctionName = "now";

ErrorId nowFunction(GC* gc, uint8_t argCount, Value* args, Value* ret) {
    (void)gc;
    (void)argCount;
    (void)args;

    struct timespec ts;
    if (!timespec_get(&ts, TIME_UTC)) {
        return 1;
    }
    *ret = semiValueNewInt((IntValue)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    return 0;
}

static const char* printFunctionName = "print";

ErrorId printFunction(GC* gc, uint8_t argCount, Value* args, Value* ret) {
    (void)gc;
    (void)ret;

    if (argCount == 0) {
        return SEMI_ERROR_INVALID_VALUE;
    }

    for (uint8_t i = 0; i < argCount; i++) {
        Value* value = &args[i];
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
            default:
                printf("<unprintable value type %d>", (int)VALUE_TYPE(value));
                break;
        }
        if (i < argCount - 1) {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

ErrorId compileAndRunInternal(SemiVM* vm, const char* source, size_t length) {
    const char* scriptMainModuleName  = "<script>";
    size_t scriptMainModuleNameLength = strlen(scriptMainModuleName);

    SemiModuleSource moduleSource = {
        .source     = source,
        .length     = (unsigned int)length,
        .name       = scriptMainModuleName,
        .nameLength = (unsigned int)scriptMainModuleNameLength,
    };

    Compiler compiler;
    semiCompilerInit(&vm->gc, &compiler);

    SemiModule* module = NULL;

    if (vm->modules.size > 0) {
        if (!semiCompilerInheritMainModule(&compiler, vm)) {
            goto handle_error;
        }
    }

    module = semiCompilerCompileModule(&compiler, vm, &moduleSource);
    if (module == NULL) {
        goto handle_error;
    }

    semiCompilerCleanup(&compiler);
    return semiVMRunMainModule(vm, module);

handle_error: {
    ErrorId errorId = compiler.errorJmpBuf.errorId;

#ifdef SEMI_DEBUG_MSG
    if (compiler.errorJmpBuf.message != NULL) {
        fprintf(stderr, "Error %u: %s\n", errorId, compiler.errorJmpBuf.message);
    }
#endif  // SEMI_DEBUG_MSG

    semiCompilerCleanup(&compiler);
    return errorId;
}
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

    semiVMAddGlobalVariable(
        vm, printFunctionName, (IdentifierLength)strlen(printFunctionName), semiValueNewNativeFunction(printFunction));
    semiVMAddGlobalVariable(
        vm, nowFunctionName, (IdentifierLength)strlen(nowFunctionName), semiValueNewNativeFunction(nowFunction));

    ErrorId errId = compileAndRunInternal(vm, str, strlen(str));

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
