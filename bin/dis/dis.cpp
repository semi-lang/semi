// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

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
    SET_OPCODE_NAME(OP_DEFER_CALL);

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
        case OP_DEFER_CALL:
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
        case VALUE_TYPE_FUNCTION_TEMPLATE: {
            FunctionTemplate* func = AS_FUNCTION_TEMPLATE(value);
            std::cout << "<fnTemplate at " << func << ">";
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

void initializeVariable(Compiler* compiler, const char* varName) {
    InternedChar* identifier = semiSymbolTableInsert(compiler->symbolTable, varName, strlen(varName));
    if (!identifier) {
        std::cerr << "Failed to insert identifier '" << varName << "' into symbol table" << std::endl;
        return;
    }
    IdentifierId identifierId = semiSymbolTableGetId(identifier);

    LocalRegisterId registerId = compiler->currentFunction->nextRegisterId;
    compiler->currentFunction->nextRegisterId++;

    // Check if we need to expand the variables array
    VariableListAppend(compiler->gc,
                       &compiler->variables,
                       (VariableDescription){
                           .identifierId = identifierId,
                           .registerId   = registerId,
                       });

    compiler->currentFunction->currentBlock->variableStackEnd = compiler->variables.size;
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
        initializeVariable(&compiler, varName.c_str());
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
        if (!IS_FUNCTION_TEMPLATE(&v)) {
            continue;
        }
        FunctionTemplate* func = AS_FUNCTION_TEMPLATE(&v);

        std::cout << "<fnTemplate at " << func << ">" << std::endl;
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
