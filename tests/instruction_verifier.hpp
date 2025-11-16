// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_INSTRUCTION_VERIFIER_HPP
#define SEMI_INSTRUCTION_VERIFIER_HPP

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include "../src/compiler.h"
#include "../src/instruction.h"
#include "../src/value.h"
}

namespace InstructionVerifier {

/*
 │ Parsed Structures
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

// Parsed instruction representation
struct ParsedInstruction {
    size_t pc;
    char opcodeName[32];
    Opcode opcode;

    enum Type { K_TYPE, T_TYPE, J_TYPE } type;

    // K-type operands
    struct {
        uint8_t A;
        uint16_t K;
        bool i;
        bool s;
    } k;

    // T-type operands
    struct {
        uint8_t A;
        uint8_t B;
        uint8_t C;
        bool kb;
        bool kc;
    } t;

    // J-type operands
    struct {
        uint32_t J;
        bool s;
    } j;
};

// Parsed constant representation
struct ParsedConstant {
    size_t index;
    char typeName[32];
    char properties[256];
    char label[64];  // Optional label for FunctionProto (e.g., @testFunc)
};

// Parsed upvalue description
struct ParsedUpvalue {
    uint8_t slot;
    uint8_t index;
    bool isLocal;
};

// Parsed export entry
struct ParsedExport {
    size_t index;
    char identifier[64];
};

// Parsed global entry
struct ParsedGlobal {
    size_t index;
    char identifier[64];
};

// Parsed type entry
struct ParsedType {
    size_t typeId;
    char typeName[64];
};

// Parsed function with instructions and upvalues
struct ParsedFunction {
    char label[64];
    std::vector<ParsedInstruction> instructions;
    std::vector<ParsedUpvalue> upvalues;
};

// PreDefine variable
struct PreDefineVariable {
    char identifier[64];
    char typeName[32];
    char value[128];
};

// Complete parsed specification
struct ParsedSpec {
    std::vector<PreDefineVariable> predefineModuleVars;
    std::vector<PreDefineVariable> predefineGlobalVars;

    std::map<std::string, ParsedFunction> functions;  // Key: label (empty string for main)
    std::vector<ParsedConstant> constants;
    std::vector<ParsedExport> exports;
    std::vector<ParsedGlobal> globals;
    std::vector<ParsedType> types;
};

/*
 │ Parser
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

class SpecParser {
   public:
    explicit SpecParser(const char* spec);

    ParsedSpec parse();

   private:
    const char* spec;
    size_t pos;
    size_t line;
    size_t col;

    enum SectionType {
        SECTION_NONE,
        SECTION_PREDEFINE_MODULE_VARS,
        SECTION_PREDEFINE_GLOBAL_VARS,
        SECTION_INSTRUCTIONS,
        SECTION_CONSTANTS,
        SECTION_EXPORTS,
        SECTION_GLOBALS,
        SECTION_TYPES,
        SECTION_UPVALUE_DESCRIPTION
    };

    SectionType currentSection;
    std::string currentFunctionLabel;

    // Parsing helpers
    void skipWhitespace();
    void skipToNextLine();
    bool isAtEnd() const;
    char peek() const;
    char advance();
    bool match(char expected);
    bool matchKeyword(const char* keyword);

    // Section parsing
    void parseSectionHeader();
    ParsedInstruction parseInstruction();
    ParsedConstant parseConstant();
    ParsedUpvalue parseUpvalue();
    ParsedExport parseExport();
    ParsedGlobal parseGlobal();
    ParsedType parseType();
    PreDefineVariable parsePreDefineVariable();

    // Field parsing
    bool parseRowHeader(char expectedKey, size_t& outIndex);
    Opcode parseOpcode(char* name, size_t nameSize);
    uint8_t parseHexByte();
    uint16_t parseHexWord();
    uint32_t parseHexDword();
    uint32_t parseDecimal();
    bool parseFlag();
    void parseString(char* buffer, size_t bufferSize);
    void parseIdentifier(char* buffer, size_t bufferSize);
    void parseLabel(char* buffer, size_t bufferSize);

    // Error reporting
    [[noreturn]] void error(const char* msg);
    [[noreturn]] void errorFmt(const char* fmt, ...);
};

/*
 │ Verifier
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

class Verifier {
   public:
    explicit Verifier(const ParsedSpec& spec);

    void verify(SemiModule* module);
    void verify(Compiler* compiler);

   private:
    const ParsedSpec& spec;

    void verifyInstructions(FunctionProto* func, const ParsedFunction& expected, const char* label);
    void verifyConstants(SemiModule* module);
    void verifyExports(SemiModule* module);
    void verifyGlobals(SemiModule* module);
    void verifyTypes(SemiVM* vm);
    void verifyUpvalues(FunctionProto* func, const std::vector<ParsedUpvalue>& expected, const char* label);

    // Constant verifiers
    void verifyConstantInt(const Value* value, const ParsedConstant& constant);
    void verifyConstantFloat(const Value* value, const ParsedConstant& constant);
    void verifyConstantBool(const Value* value, const ParsedConstant& constant);
    void verifyConstantString(const Value* value, const ParsedConstant& constant);
    void verifyConstantRange(const Value* value, const ParsedConstant& constant);
    void verifyConstantSymbol(const Value* value, const ParsedConstant& constant);
    void verifyConstantFunctionProto(const Value* value, const ParsedConstant& constant);

    // Instruction comparison
    std::string formatInstruction(const ParsedInstruction& instr);
    std::string formatInstruction(Instruction instr, Opcode opcode);
    bool compareInstructions(const ParsedInstruction& expected, Instruction actual);
};

/*
 │ Public API
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

void VerifyCompilation(SemiModule* module, const char* spec);
void VerifyCompilation(Compiler* compiler, const char* spec);

}  // namespace InstructionVerifier

#endif  // SEMI_INSTRUCTION_VERIFIER_HPP
