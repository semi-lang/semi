// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../tests/debug.hpp"

extern "C" {
#include "../../include/semi/error.h"
#include "../../src/compiler.h"
#include "../../src/const_table.h"
#include "../../src/primitives.h"
#include "../../src/symbol_table.h"
#include "../../src/vm.h"
}

static const char* printFunctionName = "print";

ErrorId printFunction(GC* gc, uint8_t argCount, Value* args, Value* ret) {
    (void)gc;
    (void)ret;

    if (argCount == 0) {
        return SEMI_ERROR_INVALID_VALUE;
    }

    for (uint8_t i = 0; i < argCount; i++) {
        printValue(args[i]);
        if (i < argCount - 1) {
            std::cout << " ";
        }
    }

    std::cout << std::endl;
    return 0;
}

static const char* moduleSourceName = "dis_script";

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [-var <var>]... -in \"<source_code>\" | " << argv[0]
                  << " [-var <var>]... -in -" << std::endl;
        return 1;
    }

    initOpcodeNames();

    std::string sourceBuffer;
    const char* source;
    std::vector<std::string> predefinedVars;
    int inFlagIndex = -1;

    // Parse arguments to find -var flags and -in flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-var") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: -var requires a variable name" << std::endl;
                return 1;
            }
            predefinedVars.push_back(argv[i + 1]);
            i++;  // Skip the variable name argument
        } else if (strcmp(argv[i], "-in") == 0) {
            inFlagIndex = i;
            break;
        } else {
            std::cerr << "Error: Unknown argument '" << argv[i] << "'" << std::endl;
            return 1;
        }
    }

    if (inFlagIndex == -1 || inFlagIndex + 1 >= argc) {
        std::cerr << "Usage: " << argv[0] << " [-var <var>]... -in \"<source_code>\" | " << argv[0]
                  << " [-var <var>]... -in -" << std::endl;
        return 1;
    }

    // Check for extra arguments after the source
    if (inFlagIndex + 2 < argc) {
        std::cerr << "Error: Unexpected arguments after source" << std::endl;
        return 1;
    }

    const char* sourceArg = argv[inFlagIndex + 1];
    if (strcmp(sourceArg, "-") == 0) {
        std::string line;
        while (std::getline(std::cin, line)) {
            sourceBuffer += line + '\n';
        }
        source = sourceBuffer.c_str();
    } else {
        source = sourceArg;
    }

    SemiVMConfig config;
    semiInitConfig(&config);

    SemiVM* vm = semiCreateVM(&config);
    if (vm == NULL) {
        std::cerr << "Failed to create VM" << std::endl;
        return 1;
    }

    semiVMAddGlobalVariable(
        vm, printFunctionName, (IdentifierLength)strlen(printFunctionName), semiValueNewNativeFunction(printFunction));

    Compiler compiler;
    semiCompilerInit(&vm->gc, &compiler);

    // Initialize predefined variables
    for (const std::string& varName : predefinedVars) {
        const char* varNameCStr = varName.c_str();
        semiVMAddGlobalVariable(vm, varNameCStr, (IdentifierLength)strlen(varNameCStr), semiValueNewInt(0));
    }

    SemiModuleSource moduleSource = {
        .source     = source,
        .length     = (unsigned int)strlen(source),
        .name       = moduleSourceName,
        .nameLength = (unsigned int)strlen(moduleSourceName),
    };

    SemiModule* module = semiCompilerCompileModule(&compiler, vm, &moduleSource);
    if (module == NULL) {
        goto cleanup;
    }

    printConstantsInfo(&module->constantTable);

    std::cout << "<main>" << std::endl;
    disassembleCode(module->moduleInit->chunk.data, module->moduleInit->chunk.size);

    for (ConstantIndex i = 0; i < semiConstantTableSize(&module->constantTable); i++) {
        Value v = semiConstantTableGet(&module->constantTable, i);
        if (!IS_FUNCTION_PROTO(&v)) {
            continue;
        }
        FunctionProto* func = AS_FUNCTION_PROTO(&v);

        std::cout << "<fnProto at " << func << ">" << std::endl;
        disassembleCode(func->chunk.data, func->chunk.size);
    }

cleanup:
    ErrorId errorId = compiler.errorJmpBuf.errorId;
    uint32_t line   = compiler.lexer.line + 1;
    size_t column   = compiler.lexer.curr - compiler.lexer.lineStart + 1;
    if (errorId != 0) {
#if defined(SEMI_DEBUG_MSG)
        const char* message = compiler.errorJmpBuf.message;
        std::cerr << "Error " << errorId << " at line " << line << ", column " << column << ": " << message
                  << std::endl;
#else
        std::cerr << "Error " << errorId << " at line " << line << ", column " << column << std::endl;
#endif  // defined(SEMI_DEBUG_MSG)
    }

    semiCompilerCleanup(&compiler);
    semiDestroyVM(vm);

    return errorId == 0 ? 0 : 1;
}
