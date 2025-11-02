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

const char* opcodeNames[MAX_OPCODE + 1];

void initOpcodeNames() {
#define SET_OPCODE_NAME(op) opcodeNames[op] = #op

    SET_OPCODE_NAME(OP_NOOP);

    SET_OPCODE_NAME(OP_JUMP);
    SET_OPCODE_NAME(OP_EXTRA_ARG);

    SET_OPCODE_NAME(OP_TRAP);
    SET_OPCODE_NAME(OP_C_JUMP);
    SET_OPCODE_NAME(OP_LOAD_CONSTANT);
    SET_OPCODE_NAME(OP_LOAD_BOOL);
    SET_OPCODE_NAME(OP_LOAD_INLINE_INTEGER);
    SET_OPCODE_NAME(OP_LOAD_INLINE_STRING);
    SET_OPCODE_NAME(OP_GET_MODULE_VAR);
    SET_OPCODE_NAME(OP_SET_MODULE_VAR);

    SET_OPCODE_NAME(OP_MOVE);
    SET_OPCODE_NAME(OP_GET_UPVALUE);
    SET_OPCODE_NAME(OP_SET_UPVALUE);
    SET_OPCODE_NAME(OP_CLOSE_UPVALUES);
    SET_OPCODE_NAME(OP_ADD);
    SET_OPCODE_NAME(OP_SUBTRACT);
    SET_OPCODE_NAME(OP_MULTIPLY);
    SET_OPCODE_NAME(OP_DIVIDE);
    SET_OPCODE_NAME(OP_FLOOR_DIVIDE);
    SET_OPCODE_NAME(OP_MODULO);
    SET_OPCODE_NAME(OP_POWER);
    SET_OPCODE_NAME(OP_NEGATE);
    SET_OPCODE_NAME(OP_GT);
    SET_OPCODE_NAME(OP_GE);
    SET_OPCODE_NAME(OP_EQ);
    SET_OPCODE_NAME(OP_NEQ);
    SET_OPCODE_NAME(OP_BITWISE_AND);
    SET_OPCODE_NAME(OP_BITWISE_OR);
    SET_OPCODE_NAME(OP_BITWISE_XOR);
    SET_OPCODE_NAME(OP_BITWISE_L_SHIFT);
    SET_OPCODE_NAME(OP_BITWISE_R_SHIFT);
    SET_OPCODE_NAME(OP_BITWISE_INVERT);
    SET_OPCODE_NAME(OP_MAKE_RANGE);
    SET_OPCODE_NAME(OP_ITER_NEXT);
    SET_OPCODE_NAME(OP_BOOL_NOT);
    SET_OPCODE_NAME(OP_GET_ATTR);
    SET_OPCODE_NAME(OP_SET_ATTR);
    SET_OPCODE_NAME(OP_GET_ITEM);
    SET_OPCODE_NAME(OP_SET_ITEM);
    SET_OPCODE_NAME(OP_CONTAIN);
    SET_OPCODE_NAME(OP_CALL);
    SET_OPCODE_NAME(OP_RETURN);
    SET_OPCODE_NAME(OP_CHECK_TYPE);

#undef SET_OPCODE_NAME
}

const char* getInstructionType(Opcode opcode) {
    switch (opcode) {
        case OP_NOOP:
            return "N";
        case OP_JUMP:
        case OP_EXTRA_ARG:
            return "J";
        case OP_TRAP:
        case OP_C_JUMP:
        case OP_LOAD_CONSTANT:
        case OP_LOAD_BOOL:
        case OP_LOAD_INLINE_INTEGER:
        case OP_LOAD_INLINE_STRING:
        case OP_GET_MODULE_VAR:
        case OP_SET_MODULE_VAR:
            return "K";
        default:
            return "T";
    }
}

void printInstruction(Instruction instruction, PCLocation pc) {
    Opcode opcode          = (Opcode)GET_OPCODE(instruction);
    const char* opcodeName = (opcode < sizeof(opcodeNames) / sizeof(opcodeNames[0])) ? opcodeNames[opcode] : "UNKNOWN";
    const char* type       = getInstructionType(opcode);

    // Print hex location every 4 lines, otherwise print spaces
    if (pc % 4 == 0) {
        std::cout << std::hex << std::uppercase << std::setw(4) << std::left << pc << std::dec;
    } else {
        std::cout << std::setw(4) << "";
    }

    std::cout << std::left << std::setw(25) << opcodeName << std::setw(7) << type;

    if (strcmp(type, "T") == 0) {
        uint8_t A = OPERAND_T_A(instruction);
        uint8_t B = OPERAND_T_B(instruction);
        uint8_t C = OPERAND_T_C(instruction);
        bool kb   = OPERAND_T_KB(instruction);
        bool kc   = OPERAND_T_KC(instruction);
        std::cout << "A: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << std::right << (int)A
                  << ", B: 0x" << std::setw(2) << (int)B << ", C: 0x" << std::setw(2) << (int)C << std::dec
                  << std::setfill(' ') << ", kb: " << (kb ? "true" : "false") << ", kc: " << (kc ? "true" : "false");
    } else if (strcmp(type, "K") == 0) {
        uint8_t A  = OPERAND_K_A(instruction);
        uint16_t K = OPERAND_K_K(instruction);
        bool i     = OPERAND_K_I(instruction);
        bool s     = OPERAND_K_S(instruction);
        std::cout << "A: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << std::right << (int)A
                  << ", K: 0x" << std::setw(4) << K << std::dec << std::setfill(' ')
                  << ", i: " << (i ? "true" : "false") << ", s: " << (s ? "true" : "false");
    } else if (strcmp(type, "J") == 0) {
        uint32_t J = OPERAND_J_J(instruction);
        bool s     = OPERAND_J_S(instruction);
        std::cout << "J: 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << std::right << J
                  << std::dec << std::setfill(' ') << ", s: " << (s ? "true" : "false");
    }

    std::cout << std::endl;
}

void disassembleCode(Instruction* instructions, size_t count) {
    std::cout << std::left << std::setw(4) << "Loc" << std::setw(25) << "Opcode" << std::setw(7) << "Type"
              << "Operands" << std::endl;
    std::cout << "-----------------------------------------------------------------------" << std::endl;

    for (PCLocation pc = 0; pc < count; pc++) {
        printInstruction(instructions[pc], pc);
    }

    std::cout << std::endl;
}

void printValue(Value v) {
    Value* value = &v;
    switch (VALUE_TYPE(value)) {
        case VALUE_TYPE_BOOL:
            std::cout << (AS_BOOL(value) ? "true" : "false");
            break;
        case VALUE_TYPE_INT:
            std::cout << AS_INT(value);
            break;
        case VALUE_TYPE_FLOAT:
            std::cout << AS_FLOAT(value);
            break;
        case VALUE_TYPE_INLINE_STRING: {
            std::cout << "\"";
            InlineString inlineStr = AS_INLINE_STRING(value);
            for (uint8_t j = 0; j < inlineStr.length; j++) {
                std::cout << inlineStr.c[j];
            }
            std::cout << "\"";
            break;
        }
        case VALUE_TYPE_OBJECT_STRING: {
            std::cout << "\"";
            ObjectString* str = AS_OBJECT_STRING(value);
            for (size_t j = 0; j < str->length; j++) {
                std::cout << str->str[j];
            }
            std::cout << "\"";
            break;
        }
        case VALUE_TYPE_INLINE_RANGE: {
            InlineRange ir = AS_INLINE_RANGE(value);
            std::cout << "range(" << ir.start << ", " << ir.end << ", 1)";
            break;
        }
        case VALUE_TYPE_OBJECT_RANGE: {
            ObjectRange* range = AS_OBJECT_RANGE(value);
            std::cout << "range(";
            printValue(range->start);
            std::cout << ", ";
            printValue(range->end);
            std::cout << ", ";
            printValue(range->step);
            std::cout << ")";
            break;
        }
        case VALUE_TYPE_FUNCTION_PROTO: {
            FunctionProto* func = AS_FUNCTION_PROTO(value);
            std::cout << "<fnProto at " << func << ">";
            break;
        }
        default:
            std::cout << "<unprintable value type " << (int)VALUE_TYPE(value) << ">";
            break;
    }
}

void printConstantsInfo(ConstantTable* constTable) {
    std::cout << std::left << std::setw(8) << "Index" << "Content" << std::endl;
    std::cout << "--------------------" << std::endl;

    size_t tableSize = semiConstantTableSize(constTable);
    for (size_t i = 0; i < tableSize; i++) {
        Value v = semiConstantTableGet(constTable, (ConstantIndex)i);
        if (IS_INVALID(&v)) {
            std::cout << std::left << std::setw(8) << i << "UNINITIALIZED" << std::endl;
            continue;
        }

        std::cout << std::left << std::setw(8) << i;
        printValue(v);
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

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

void printError(ErrorId errorId, const char* message) {
    if (errorId != 0) {
#ifdef SEMI_DEBUG_MSG
        std::cerr << "Error " << errorId << ": " << message << std::endl;
#else
        std::cerr << "Error " << errorId << std::endl;
#endif  // SEMI_DEBUG_MSG
    }
}

ErrorId compileAndRun(SemiVM* vm, const char* source, size_t length, bool isRepl, bool disassemble) {
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

    semiCompilerCleanup(&compiler);
    return semiVMRunMainModule(vm, module);

handle_error:
#ifdef SEMI_DEBUG_MSG
    printError(compiler.errorJmpBuf.errorId, compiler.errorJmpBuf.message);
#else
    printError(compiler.errorJmpBuf.errorId, "An error occurred during compilation.");
#endif  // SEMI_DEBUG_MSG

    ErrorId errorId = compiler.errorJmpBuf.errorId;
    semiCompilerCleanup(&compiler);
    return errorId;
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

int executeSource(const char* source, bool isRepl, bool disassemble) {
    SemiVMConfig config;
    semiInitConfig(&config);
    SemiVM* vm = semiCreateVM(&config);

    semiVMAddGlobalVariable(
        vm, printFunctionName, (IdentifierLength)strlen(printFunctionName), semiValueNewNativeFunction(printFunction));
    semiVMAddGlobalVariable(
        vm, nowFunctionName, (IdentifierLength)strlen(nowFunctionName), semiValueNewNativeFunction(nowFunction));

    ErrorId errId = compileAndRun(vm, source, strlen(source), isRepl, disassemble);
    if (errId != 0) {
#ifdef SEMI_DEBUG_MSG
        printError(errId, vm->errorMessage);
#else
        printError(errId, "An error occurred during execution.");
#endif  // SEMI_DEBUG_MSG
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

    semiVMAddGlobalVariable(
        vm, printFunctionName, (IdentifierLength)strlen(printFunctionName), semiValueNewNativeFunction(printFunction));

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
            ErrorId errId      = compileAndRun(vm, source, strlen(source), true, disassemble);
            if (errId != 0) {
                printError(errId, "An error occurred during execution.");
            } else if (vm->returnedValue != NULL && !IS_INVALID(vm->returnedValue)) {
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
    initOpcodeNames();

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