// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_TEST_COMMON_HPP
#define SEMI_TEST_COMMON_HPP

#include <gtest/gtest.h>

#include <cstdint>

#define SEMI_TEST 1

extern "C" {
#include <stdlib.h>

#include "../src/compiler.h"
#include "../src/const_table.h"
#include "../src/instruction.h"
#include "../src/primitives.h"
#include "../src/symbol_table.h"
#include "../src/value.h"
#include "../src/vm.h"
#include "semi/error.h"

static inline void* defaultReallocFn(void* ptr, size_t newSize, void* reallocData) {
    (void)reallocData;
    if (newSize == 0) {
        free(ptr);
        return NULL;  // Return NULL to indicate the memory has been freed
    }
    return realloc(ptr, newSize);
}
}

#include "./debug.hpp"
#include "./instruction_verifier.hpp"

/*
 │ Expression Test Helpers
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

static inline void enterTestBlock(Compiler* compiler, BlockScope* newBlock) {
    FunctionScope* currentFunction = compiler->currentFunction;
    BlockScope* currentBlock       = currentFunction->currentBlock;

    newBlock->parent              = currentBlock;
    currentFunction->currentBlock = newBlock;
    newBlock->variableStackStart  = currentBlock->variableStackEnd;
    newBlock->variableStackEnd    = currentBlock->variableStackEnd;
}

/*
 │ Instruction Helpers
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

/**
 * Semantic instruction types that provide meaningful field names
 * for the different instruction formats defined in instruction.h
 */

// T-type (Ternary) instruction format
typedef struct TInstruction {
    uint8_t opcode;
    uint8_t destReg;  // A operand - destination register
    uint8_t srcReg1;  // B operand - first source register
    uint8_t srcReg2;  // C operand - second source register
    bool constFlag1;  // kb flag - B operand is constant
    bool constFlag2;  // kc flag - C operand is constant
} TInstruction;

// K-type (Constant) instruction format
typedef struct KInstruction {
    uint8_t opcode;
    uint8_t destReg;    // A operand - destination register
    uint16_t constant;  // K operand - constant value or jump distance
    bool inlineFlag;    // i flag - inline constant or jump condition
    bool signFlag;      // s flag - sign or jump direction
} KInstruction;

// J-type (Jump) instruction format
typedef struct JInstruction {
    uint8_t opcode;
    uint32_t jumpOffset;  // J operand - jump offset
    bool signFlag;        // s flag - jump direction
} JInstruction;

static TInstruction makeTInstruction(
    uint8_t opcode, uint8_t destReg, uint8_t srcReg1, uint8_t srcReg2, bool constFlag1, bool constFlag2) {
    return (TInstruction){.opcode     = opcode,
                          .destReg    = destReg,
                          .srcReg1    = srcReg1,
                          .srcReg2    = srcReg2,
                          .constFlag1 = constFlag1,
                          .constFlag2 = constFlag2};
}

static KInstruction makeKInstruction(
    uint8_t opcode, uint8_t destReg, uint16_t constant, bool inlineFlag, bool signFlag) {
    return (KInstruction){
        .opcode = opcode, .destReg = destReg, .constant = constant, .inlineFlag = inlineFlag, .signFlag = signFlag};
}

static JInstruction makeJInstruction(uint8_t opcode, uint32_t jumpOffset, bool signFlag) {
    return (JInstruction){.opcode = opcode, .jumpOffset = jumpOffset, .signFlag = signFlag};
}

// Helper functions to extract instruction fields into semantic structures
static TInstruction decodeTInstruction(Instruction instr) {
    return (TInstruction){.opcode     = (uint8_t)GET_OPCODE(instr),
                          .destReg    = OPERAND_T_A(instr),
                          .srcReg1    = OPERAND_T_B(instr),
                          .srcReg2    = OPERAND_T_C(instr),
                          .constFlag1 = OPERAND_T_KB(instr),
                          .constFlag2 = OPERAND_T_KC(instr)};
}

static KInstruction decodeKInstruction(Instruction instr) {
    return (KInstruction){.opcode     = (uint8_t)GET_OPCODE(instr),
                          .destReg    = OPERAND_K_A(instr),
                          .constant   = OPERAND_K_K(instr),
                          .inlineFlag = OPERAND_K_I(instr),
                          .signFlag   = OPERAND_K_S(instr)};
}

static JInstruction decodeJInstruction(Instruction instr) {
    return (JInstruction){
        .opcode = (uint8_t)GET_OPCODE(instr), .jumpOffset = OPERAND_J_J(instr), .signFlag = OPERAND_J_S(instr)};
}

// Helper functions to create instructions from semantic structures
static Instruction encodeTInstruction(TInstruction tInst) {
    return ((((Instruction)(tInst.opcode)) & OPCODE_MASK) | (((Instruction)(tInst.destReg)) << 24) |
            (((Instruction)(tInst.srcReg1) & 0xFF) << 16) | (((Instruction)(tInst.srcReg2) & 0xFF) << 8) |
            (((Instruction)(bool)(tInst.constFlag1)) << 7) | (((Instruction)(bool)(tInst.constFlag2)) << 6));
}

static Instruction encodeKInstruction(KInstruction kInst) {
    return ((((Instruction)(kInst.opcode)) & OPCODE_MASK) | (((Instruction)(kInst.destReg)) << 24) |
            (((Instruction)(kInst.constant) & 0xFFFF) << 8) | (((Instruction)(bool)(kInst.inlineFlag)) << 7) |
            (((Instruction)(bool)(kInst.signFlag)) << 6));
}

static Instruction encodeJInstruction(JInstruction jInst) {
    return ((((Instruction)(jInst.opcode)) & OPCODE_MASK) | (((Instruction)(jInst.jumpOffset)) << 8) |
            (((Instruction)(bool)(jInst.signFlag)) << 7));
}

// Google Test assertion macros for instruction comparison
#define ASSERT_T_INSTRUCTION_EQ(actualInst, expectedInst, msg)                                \
    do {                                                                                      \
        TInstruction actual         = decodeTInstruction(actualInst);                         \
        const TInstruction expected = (expectedInst);                                         \
        ASSERT_EQ(actual.opcode, expected.opcode) << msg << " - opcode mismatch";             \
        ASSERT_EQ(actual.destReg, expected.destReg) << msg << " - destReg mismatch";          \
        ASSERT_EQ(actual.srcReg1, expected.srcReg1) << msg << " - srcReg1 mismatch";          \
        ASSERT_EQ(actual.srcReg2, expected.srcReg2) << msg << " - srcReg2 mismatch";          \
        ASSERT_EQ(actual.constFlag1, expected.constFlag1) << msg << " - constFlag1 mismatch"; \
        ASSERT_EQ(actual.constFlag2, expected.constFlag2) << msg << " - constFlag2 mismatch"; \
    } while (0)

#define ASSERT_K_INSTRUCTION_EQ(actualInst, expectedInst, msg)                                \
    do {                                                                                      \
        KInstruction actual         = decodeKInstruction(actualInst);                         \
        const KInstruction expected = (expectedInst);                                         \
        ASSERT_EQ(actual.opcode, expected.opcode) << msg << " - opcode mismatch";             \
        ASSERT_EQ(actual.destReg, expected.destReg) << msg << " - destReg mismatch";          \
        ASSERT_EQ(actual.constant, expected.constant) << msg << " - constant mismatch";       \
        ASSERT_EQ(actual.inlineFlag, expected.inlineFlag) << msg << " - inlineFlag mismatch"; \
        ASSERT_EQ(actual.signFlag, expected.signFlag) << msg << " - signFlag mismatch";       \
    } while (0)

#define ASSERT_J_INSTRUCTION_EQ(actualInst, expectedInst, msg)                                \
    do {                                                                                      \
        JInstruction actual         = decodeJInstruction(actualInst);                         \
        const JInstruction expected = (expectedInst);                                         \
        ASSERT_EQ(actual.opcode, expected.opcode) << msg << " - opcode mismatch";             \
        ASSERT_EQ(actual.jumpOffset, expected.jumpOffset) << msg << " - jumpOffset mismatch"; \
        ASSERT_EQ(actual.signFlag, expected.signFlag) << msg << " - signFlag mismatch";       \
    } while (0)

/*
 │ Value Helpers
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#undef BOOL_VALUE
static inline Value createBoolValue(bool b) {
    Value val;
    val.header = VALUE_TYPE_BOOL;
    val.as.b   = b;
    return val;
}
#define BOOL_VALUE(val) (createBoolValue(val))

#undef INT_VALUE
static inline Value createIntValue(IntValue i) {
    Value val;
    val.header = VALUE_TYPE_INT;
    val.as.i   = i;
    return val;
}
#define INT_VALUE(val) (createIntValue(val))

#undef FLOAT_VALUE
static inline Value createFloatValue(FloatValue f) {
    Value val;
    val.header = VALUE_TYPE_FLOAT;
    val.as.f   = f;
    return val;
}
#define FLOAT_VALUE(val) (createFloatValue(val))

#undef INLINE_STRING_VALUE_0
static inline Value createInlineStringValue0(void) {
    Value val;
    val.header       = VALUE_TYPE_INLINE_STRING;
    val.as.is.c[0]   = 0;
    val.as.is.c[1]   = 0;
    val.as.is.length = 0;
    return val;
}
#define INLINE_STRING_VALUE_0() (createInlineStringValue0())

#undef INLINE_STRING_VALUE_1
static inline Value createInlineStringValue1(char c1) {
    Value val;
    val.header       = VALUE_TYPE_INLINE_STRING;
    val.as.is.c[0]   = c1;
    val.as.is.c[1]   = 0;
    val.as.is.length = 1;
    return val;
}
#define INLINE_STRING_VALUE_1(c1) (createInlineStringValue1(c1))

#undef INLINE_STRING_VALUE_2
static inline Value createInlineStringValue2(char c1, char c2) {
    Value val;
    val.header       = VALUE_TYPE_INLINE_STRING;
    val.as.is.c[0]   = c1;
    val.as.is.c[1]   = c2;
    val.as.is.length = 2;
    return val;
}
#define INLINE_STRING_VALUE_2(c1, c2) (createInlineStringValue2(c1, c2))

#undef INLINE_RANGE_VALUE
static inline Value createInlineRangeValue(IntValue start, IntValue end) {
    Value val;
    val.header      = VALUE_TYPE_INLINE_RANGE;
    val.as.ir.start = start;
    val.as.ir.end   = end;
    return val;
}
#define INLINE_RANGE_VALUE(start, end) (createInlineRangeValue(start, end))

#undef FUNCTION_VALUE
static inline Value createFunctionValue(FunctionProto* func) {
    Value val;
    val.header = VALUE_TYPE_FUNCTION_PROTO;
    val.as.ptr = func;
    return val;
}
#define FUNCTION_VALUE(val) (createFunctionValue(val))

/*
 │ Test State Helpers
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

class VMTest : public ::testing::Test {
   public:
    SemiVM* vm;

    void SetUp() override {
        vm = semiCreateVM(NULL);
        ASSERT_NE(vm, nullptr) << "Failed to create VM";
    }

    void TearDown() override {
        if (vm) {
            semiDestroyVM(vm);
            vm = nullptr;
        }
    }

    void AddGlobalVariable(const char* name, Value value) {
        ErrorId result = semiVMAddGlobalVariable(vm, name, strlen(name), value);
        ASSERT_EQ(result, 0) << "Adding global variable '" << name << "' should succeed";
    }

    FunctionProto* CreateFunctionObject(uint8_t arity,
                                        Instruction* code,
                                        uint32_t codeSize,
                                        uint8_t maxStackSize,
                                        uint8_t upvalueCount,
                                        uint8_t coarity) {
        FunctionProto* func   = semiFunctionProtoCreate(&vm->gc, upvalueCount);
        Instruction* codeCopy = (Instruction*)semiMalloc(&vm->gc, sizeof(Instruction) * codeSize);
        memcpy(codeCopy, code, sizeof(Instruction) * codeSize);
        func->moduleId       = SEMI_REPL_MODULE_ID;
        func->arity          = arity;
        func->coarity        = coarity;
        func->chunk.data     = codeCopy;
        func->chunk.size     = codeSize;
        func->chunk.capacity = codeSize;
        func->maxStackSize   = maxStackSize;

        return func;
    }

    ErrorId RunModule(SemiModule* module) {
        const char* moduleName   = "test_module";
        uint8_t moduleNameLength = (uint8_t)strlen(moduleName);

        InternedChar* moduleNameInterned = semiSymbolTableInsert(&vm->symbolTable, moduleName, moduleNameLength);
        IdentifierId moduleNameId        = semiSymbolTableGetId(moduleNameInterned);

        semiDictSet(&vm->gc, &vm->modules, semiValueNewInt(moduleNameId), semiValueNewPtr(module, VALUE_TYPE_UNSET));

        return semiRunModule(vm, moduleName, moduleNameLength);
    }
};

class CompilerTest : public ::testing::Test {
   public:
    Compiler compiler;
    SemiVM* vm;
    SemiModule* module;

    // If calling `ParseStatement`, this will hold the block scope.
    BlockScope testBlock;

    void SetUp() override {
        vm = semiCreateVM(NULL);
        semiCompilerInit(&compiler);

        compiler.gc                = &vm->gc;
        compiler.symbolTable       = &vm->symbolTable;
        compiler.classes           = &vm->classes;
        compiler.globalIdentifiers = &vm->globalIdentifiers;

        VariableListEnsureCapacity(compiler.gc, &compiler.variables, 32);
        compiler.artifactModule = semiVMModuleCreate(compiler.gc, SEMI_REPL_MODULE_ID);
        semiPrimitivesInitBuiltInModuleTypes(compiler.gc, compiler.symbolTable, compiler.artifactModule);
        module = compiler.artifactModule;
    }

    void TearDown() override {
        semiVMModuleDestroy(&vm->gc, module);
        semiCompilerCleanup(&compiler);
        semiDestroyVM(vm);
    }

    void PrintCode() {
        Chunk* chunk;
        if (module != nullptr && module->moduleInit != nullptr) {
            chunk = &module->moduleInit->chunk;
        } else {
            chunk = &compiler.rootFunction.chunk;
        }

        disassembleCode(chunk->data, chunk->size);
    }

    size_t GetCodeSize() {
        if (module != nullptr && module->moduleInit != nullptr) {
            return module->moduleInit->chunk.size;
        }
        return compiler.rootFunction.chunk.size;
    }

    Instruction GetInstruction(size_t index) {
        if (module != nullptr && module->moduleInit != nullptr) {
            return module->moduleInit->chunk.data[index];
        }
        return compiler.rootFunction.chunk.data[index];
    }

    VariableDescription* FindVariable(const char* identifier) {
        InternedChar* internedIdentifier = semiSymbolTableGet(compiler.symbolTable, identifier, strlen(identifier));
        if (internedIdentifier == nullptr) {
            return nullptr;  // Variable not found
        }

        IdentifierId identifierId = semiSymbolTableGetId(internedIdentifier);
        for (uint16_t i = 0; i < compiler.variables.size; ++i) {
            if (compiler.variables.data[i].identifierId == identifierId) {
                return compiler.variables.data + i;
            }
        }

        return nullptr;
    }

    void InitializeVariable(const char* var_name) {
        // Insert identifier into symbol table
        InternedChar* identifier = semiSymbolTableInsert(compiler.symbolTable, var_name, strlen(var_name));
        ASSERT_NE(identifier, nullptr) << "Failed to insert identifier '" << var_name << "' into symbol table";
        IdentifierId identifierId = semiSymbolTableGetId(identifier);

        // Reserve a register for the variable
        LocalRegisterId registerId = compiler.currentFunction->nextRegisterId;
        compiler.currentFunction->nextRegisterId++;

        VariableListAppend(compiler.gc,
                           &compiler.variables,
                           (VariableDescription){.identifierId = identifierId, .registerId = registerId});
        compiler.currentFunction->currentBlock->variableStackEnd = compiler.variables.size;
    }

    ErrorId ParseModule(const char* input) {
        SemiModuleSource moduleSource = {
            .source     = input,
            .length     = (unsigned int)strlen(input),
            .name       = "test_module",
            .nameLength = (uint8_t)strlen("test_module"),
        };

        if (setjmp(compiler.errorJmpBuf.env) == 0) {
            semiCompilerCompileModule(&compiler, &moduleSource, module);
        }
        return compiler.errorJmpBuf.errorId;
    }

    ErrorId ParseExpression(const char* input, PrattExpr* expr) {
        if (setjmp(compiler.errorJmpBuf.env) == 0) {
            FunctionScope* currentFunction = compiler.currentFunction;
            LocalRegisterId registerId     = currentFunction->nextRegisterId++;
            if (currentFunction->nextRegisterId > currentFunction->maxUsedRegisterCount) {
                currentFunction->maxUsedRegisterCount = currentFunction->nextRegisterId;
            }

            PrattState state = {
                .rightBindingPower = PRECEDENCE_NONE,
                .targetRegister    = registerId,
            };
            semiTextInitLexer(&compiler.lexer, &compiler, input, (uint32_t)strlen(input));
            enterTestBlock(&compiler, &testBlock);
            semiParseExpression(&compiler, state, expr);
            return 0;
        }

        return compiler.errorJmpBuf.errorId;
    }

    ErrorId ParseStatement(const char* input, bool inBlock) {
        if (setjmp(compiler.errorJmpBuf.env) == 0) {
            semiTextInitLexer(&compiler.lexer, &compiler, input, (uint32_t)strlen(input));
            if (inBlock) {
                enterTestBlock(&compiler, &testBlock);
            }
            semiParseStatement(&compiler);
            return 0;
        }
        return compiler.errorJmpBuf.errorId;
    }

    ErrorId GetCompilerError() {
        return compiler.errorJmpBuf.errorId;
    }

    ModuleVariableId GetModuleVariableId(const char* identifier, bool* isExport = nullptr) {
        InternedChar* internedIdentifier = semiSymbolTableGet(compiler.symbolTable, identifier, strlen(identifier));
        if (internedIdentifier == nullptr) {
            if (isExport) *isExport = false;
            return INVALID_MODULE_VARIABLE_ID;
        }

        IdentifierId identifierId = semiSymbolTableGetId(internedIdentifier);

        // Check exports first
        ValueHash hash  = semiHash64Bits(identifierId);
        Value v         = semiValueNewInt(identifierId);
        TupleId tupleId = semiDictFindTupleId(&compiler.artifactModule->exports, v, hash);
        if (tupleId >= 0 && tupleId <= UINT32_MAX) {
            if (isExport) *isExport = true;
            return (ModuleVariableId)tupleId;
        }

        // Check globals
        tupleId = semiDictFindTupleId(&compiler.artifactModule->globals, v, hash);
        if (tupleId >= 0 && tupleId <= UINT32_MAX) {
            if (isExport) *isExport = false;
            return (ModuleVariableId)tupleId;
        }

        if (isExport) *isExport = false;
        return INVALID_MODULE_VARIABLE_ID;
    }

    void InitializeModuleVariable(const char* varName, bool isExport = false) {
        // Insert identifier into symbol table
        InternedChar* identifier = semiSymbolTableInsert(compiler.symbolTable, varName, strlen(varName));
        ASSERT_NE(identifier, nullptr) << "Failed to insert identifier '" << varName << "' into symbol table";
        IdentifierId identifierId = semiSymbolTableGetId(identifier);

        // Add variable to appropriate dictionary
        ObjectDict* targetDict = isExport ? &compiler.artifactModule->exports : &compiler.artifactModule->globals;
        Value keyValue         = semiValueNewInt(identifierId);
        Value dummyValue       = semiValueNewInt(0);  // Dummy value
        ValueHash hash         = semiHash64Bits(identifierId);

        bool result = semiDictSetWithHash(compiler.gc, targetDict, keyValue, dummyValue, hash);
        ASSERT_TRUE(result) << "Failed to add module variable '" << varName << "'";
    }

    void AddGlobalVariable(const char* varName, Value value) {
        ErrorId result = semiVMAddGlobalVariable(vm, varName, (unsigned int)strlen(varName), value);
        ASSERT_EQ(result, 0) << "Adding global variable '" << varName << "' should succeed";
    }
};

#endif /* SEMI_TEST_COMMON_HPP */
