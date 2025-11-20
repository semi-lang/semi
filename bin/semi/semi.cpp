// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <iomanip>
#include <iostream>
#include <string>

extern "C" {
#include "../../include/semi/error.h"
#include "../../src/compiler.h"
#include "../../src/const_table.h"
#include "../../src/primitives.h"
#include "../../src/symbol_table.h"
#include "../../src/vm.h"
}

#include "../../tests/debug.hpp"

static const char* moduleSourceName = "repl_module";

bool readFileToString(const char* filename, std::string& content) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    content.resize(length);
    size_t readLength = fread(&content[0], 1, length, file);
    content.resize(readLength);

    fclose(file);
    return true;
}

bool readStdinToString(std::string& content) {
    std::string line;
    while (std::getline(std::cin, line)) {
        content += line + '\n';
    }
    return true;
}

ErrorId compileAndRun(SemiVM* vm, const char* source, unsigned int length, bool isRepl, bool disassemble) {
    const char* scriptModuleName   = "<script>";
    uint8_t scriptModuleNameLength = (uint8_t)strlen(scriptModuleName);

    SemiModuleSource moduleSource = {
        .source     = source,
        .length     = length,
        .name       = scriptModuleName,
        .nameLength = scriptModuleNameLength,
    };

    SemiModule* module = semiVMCompileModule(vm, &moduleSource);
    if (module == NULL) {
        ErrorId errorId = vm->error;
        uint32_t line   = vm->errorDetails.compileError.line;
        size_t column   = vm->errorDetails.compileError.column;

#if defined(SEMI_DEBUG_MSG)
        const char* message = vm->errorMessage != NULL ? vm->errorMessage : "Unknown error";
        std::cerr << "Error " << errorId << " at line " << line << ", column " << column << ": " << message
                  << std::endl;
#else
        std::cerr << "Error " << errorId << " at line " << line << ", column " << column << std::endl;
#endif  // defined(SEMI_DEBUG_MSG)
        return errorId;
    }

    if (disassemble) {
        // Print disassembly
        std::cout << "=== DISASSEMBLY ===" << std::endl;
        std::cout << "Constants:" << std::endl;
        printConstantsInfo(&module->constantTable);

        std::cout << "Instructions:" << std::endl;
        FunctionProto* func = module->moduleInit;
        disassembleCode(func->chunk.data, func->chunk.size);

        std::cout << "=== EXECUTION ===" << std::endl;
    }

    return semiRunModule(vm, scriptModuleName, scriptModuleNameLength);
}

static const char* printFunctionName = "print";

ErrorId printFunction(SemiVM* vm, uint8_t argCount, Value* args, Value* ret) {
    (void)vm;
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

static const char* nowFunctionName = "now";

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

int executeSource(const char* source, bool isRepl, bool disassemble) {
    SemiVMConfig config;
    semiInitConfig(&config);
    SemiVM* vm = semiCreateVM(&config);

    semiVMAddGlobalVariable(vm,
                            printFunctionName,
                            (IdentifierLength)strlen(printFunctionName),
                            semiValueNativeFunctionCreate(printFunction));
    semiVMAddGlobalVariable(
        vm, nowFunctionName, (IdentifierLength)strlen(nowFunctionName), semiValueNativeFunctionCreate(nowFunction));

    ErrorId errId = compileAndRun(vm, source, (unsigned int)strlen(source), isRepl, disassemble);
    if (errId != 0) {
        return 1;
    } else if (vm->returnedValue != NULL && !IS_INVALID(vm->returnedValue)) {
        std::cout << "=> ";
        printValue(*vm->returnedValue);
        std::cout << std::endl;
    }
    semiDestroyVM(vm);
    return 0;
}

void runRepl(bool disassemble) {
    std::cout << "Semi REPL - Type your code and press Shift+Enter to execute" << std::endl;
    std::cout << "Type 'exit' or Ctrl-D to quit" << std::endl;

    std::string input;
    std::string line;

    SemiVMConfig config;
    semiInitConfig(&config);
    SemiVM* vm = semiCreateVM(&config);

    semiVMAddGlobalVariable(vm,
                            printFunctionName,
                            (IdentifierLength)strlen(printFunctionName),
                            semiValueNativeFunctionCreate(printFunction));

    while (true) {
        std::cout << ">>> ";
        if (!std::getline(std::cin, line)) {
            // EOF (Ctrl-D) was pressed
            std::cout << std::endl;
            break;
        }

        if (line == "exit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        input += line + "\n";

        std::cout << "... ";
        while (std::getline(std::cin, line)) {
            if (line.empty()) {
                break;
            }
            input += line + "\n";
            std::cout << "... ";
        }

        if (!input.empty()) {
            const char* source = input.c_str();
            ErrorId errId      = compileAndRun(vm, source, (unsigned int)strlen(source), true, disassemble);
            if (errId == 0 && vm->returnedValue != NULL && !IS_INVALID(vm->returnedValue)) {
                std::cout << "=> ";
                printValue(*vm->returnedValue);
                std::cout << std::endl;
            }
            input.clear();
        }
    }

    semiDestroyVM(vm);
}

int main(int argc, char* argv[]) {
    std::string source;
    bool useInline      = false;
    bool disassemble    = false;
    int positionalStart = argc;  // Position where positional args start

    // Parse arguments to find flags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--disassemble") == 0) {
            disassemble = true;
        } else if (strcmp(argv[i], "-in") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Error: -in requires source content" << std::endl;
                return 1;
            }
            source    = argv[i + 1];
            useInline = true;
            i++;  // Skip the source content argument
        } else {
            // This is a positional argument (filename or -)
            positionalStart = i;
            break;
        }
    }

    // If no positional arguments and no inline content, start REPL
    if (!useInline && positionalStart >= argc) {
        runRepl(disassemble);
        return 0;
    }

    // Handle inline content
    if (useInline) {
        // Check for unexpected positional arguments after -in
        if (positionalStart < argc) {
            std::cerr << "Error: Unexpected arguments after -in" << std::endl;
            return 1;
        }
        return executeSource(source.c_str(), false, disassemble);
    }

    // Handle positional argument (filename or -)
    if (positionalStart < argc) {
        if (positionalStart + 1 < argc) {
            std::cerr << "Error: Too many positional arguments" << std::endl;
            return 1;
        }

        const char* filename = argv[positionalStart];

        if (strcmp(filename, "-") == 0) {
            if (!readStdinToString(source)) {
                std::cerr << "Failed to read from stdin" << std::endl;
                return 1;
            }
        } else {
            if (!readFileToString(filename, source)) {
                std::cerr << "Failed to read file: " << filename << std::endl;
                return 1;
            }
        }

        return executeSource(source.c_str(), false, disassemble);
    }

    // If we reach here, there were no valid arguments
    std::cerr << "Usage: " << argv[0] << " [--disassemble] [-in \"<source>\" | <filename> | -]" << std::endl;
    std::cerr << "  --disassemble: Print disassembly before execution" << std::endl;
    std::cerr << "  No positional arguments: Start REPL mode" << std::endl;
    std::cerr << "  -in \"<source>\": Execute inline source code" << std::endl;
    std::cerr << "  <filename>: Execute the specified file" << std::endl;
    std::cerr << "  -: Read and execute from stdin" << std::endl;
    return 1;
}