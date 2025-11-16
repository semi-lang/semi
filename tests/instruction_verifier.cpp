// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "instruction_verifier.hpp"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <regex>
#include <sstream>

extern "C" {
#include "../src/const_table.h"
#include "../src/symbol_table.h"
#include "../src/vm.h"
}

namespace InstructionVerifier {

/*
 │ SpecParser Implementation
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

SpecParser::SpecParser(const char* spec)
    : spec(spec), pos(0), line(1), col(1), currentSection(SECTION_NONE), currentFunctionLabel("") {}

ParsedSpec SpecParser::parse() {
    ParsedSpec result;

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        // Skip empty lines
        if (peek() == '\n') {
            advance();
            continue;
        }

        if (peek() == '[') {
            parseSectionHeader();
            continue;
        }

        // Parse based on current section
        switch (currentSection) {
            case SECTION_PREDEFINE_MODULE_VARS:
                if (!isAtEnd() && peek() != '[') {
                    result.predefineModuleVars.push_back(parsePreDefineVariable());
                }
                break;

            case SECTION_PREDEFINE_GLOBAL_VARS:
                if (!isAtEnd() && peek() != '[') {
                    result.predefineGlobalVars.push_back(parsePreDefineVariable());
                }
                break;

            case SECTION_INSTRUCTIONS: {
                if (!isAtEnd() && peek() != '[') {
                    ParsedInstruction instr = parseInstruction();
                    result.functions[currentFunctionLabel].instructions.push_back(instr);
                    if (currentFunctionLabel.empty()) {
                        strcpy(result.functions[currentFunctionLabel].label, "");
                    } else {
                        strcpy(result.functions[currentFunctionLabel].label, currentFunctionLabel.c_str());
                    }
                }
                break;
            }

            case SECTION_CONSTANTS:
                if (!isAtEnd() && peek() != '[') {
                    result.constants.push_back(parseConstant());
                }
                break;

            case SECTION_UPVALUE_DESCRIPTION:
                if (!isAtEnd() && peek() != '[') {
                    ParsedUpvalue upval = parseUpvalue();
                    result.functions[currentFunctionLabel].upvalues.push_back(upval);
                }
                break;

            case SECTION_EXPORTS:
                if (!isAtEnd() && peek() != '[') {
                    result.exports.push_back(parseExport());
                }
                break;

            case SECTION_GLOBALS:
                if (!isAtEnd() && peek() != '[') {
                    result.globals.push_back(parseGlobal());
                }
                break;

            case SECTION_TYPES:
                if (!isAtEnd() && peek() != '[') {
                    result.types.push_back(parseType());
                }
                break;

            case SECTION_NONE:
                skipToNextLine();
                break;
        }
    }

    return result;
}

void SpecParser::parseSectionHeader() {
    if (!match('[')) error("Expected '['");

    if (matchKeyword("PreDefine:ModuleVariables]")) {
        currentSection = SECTION_PREDEFINE_MODULE_VARS;
    } else if (matchKeyword("PreDefine:GlobalVariables]")) {
        currentSection = SECTION_PREDEFINE_GLOBAL_VARS;
    } else if (matchKeyword("Instructions]")) {
        currentSection       = SECTION_INSTRUCTIONS;
        currentFunctionLabel = "";
    } else if (matchKeyword("Instructions:")) {
        currentSection = SECTION_INSTRUCTIONS;
        char label[64];
        parseLabel(label, sizeof(label));
        currentFunctionLabel = label;
        if (!match(']')) error("Expected ']' after function label");
    } else if (matchKeyword("Constants]")) {
        currentSection = SECTION_CONSTANTS;
    } else if (matchKeyword("UpvalueDescription:")) {
        currentSection = SECTION_UPVALUE_DESCRIPTION;
        char label[64];
        parseLabel(label, sizeof(label));
        currentFunctionLabel = label;
        if (!match(']')) error("Expected ']' after function label");
    } else if (matchKeyword("Exports]")) {
        currentSection = SECTION_EXPORTS;
    } else if (matchKeyword("Globals]")) {
        currentSection = SECTION_GLOBALS;
    } else if (matchKeyword("Types]")) {
        currentSection = SECTION_TYPES;
    } else {
        error("Unknown section header");
    }

    skipToNextLine();
}

bool SpecParser::parseRowHeader(char expectedKey, size_t& outIndex) {
    // Save current position for potential rollback
    size_t savedPos  = pos;
    size_t savedLine = line;
    size_t savedCol  = col;

    // Build the string from current position to end of line or next whitespace
    std::string headerStr;
    while (!isAtEnd() && peek() != '\n' && peek() != '\r' && peek() != ' ' && peek() != '\t') {
        headerStr += peek();
        advance();
    }

    // Try to match pattern: <key>[<number>]:
    std::regex headerPattern(R"(^([A-Z])\[(\d+)\]:$)");
    std::smatch match;

    if (std::regex_match(headerStr, match, headerPattern)) {
        char key = match[1].str()[0];
        if (key == expectedKey) {
            outIndex = std::stoul(match[2].str());
            skipWhitespace();
            return true;
        }
    }

    // Rollback on failure
    pos  = savedPos;
    line = savedLine;
    col  = savedCol;
    return false;
}

ParsedInstruction SpecParser::parseInstruction() {
    ParsedInstruction instr = {};

    // Parse PC
    instr.pc = parseDecimal();
    if (!match(':')) error("Expected ':' after PC");
    skipWhitespace();

    // Parse opcode
    instr.opcode = parseOpcode(instr.opcodeName, sizeof(instr.opcodeName));
    skipWhitespace();

    // Determine instruction type and parse operands
    // Try to parse as K-type first
    if (matchKeyword("A=")) {
        instr.k.A = parseHexByte();
        skipWhitespace();

        if (matchKeyword("K=")) {
            // K-type instruction
            instr.type = ParsedInstruction::K_TYPE;
            instr.k.K  = parseHexWord();
            skipWhitespace();

            if (matchKeyword("i=")) {
                instr.k.i = parseFlag();
                skipWhitespace();
            }
            if (matchKeyword("s=")) {
                instr.k.s = parseFlag();
                skipWhitespace();
            }
        } else if (matchKeyword("B=")) {
            // T-type instruction
            instr.type = ParsedInstruction::T_TYPE;
            instr.t.A  = instr.k.A;  // Copy A value
            instr.t.B  = parseHexByte();
            skipWhitespace();

            if (!matchKeyword("C=")) error("Expected 'C=' in T-type instruction");
            instr.t.C = parseHexByte();
            skipWhitespace();

            if (matchKeyword("kb=")) {
                instr.t.kb = parseFlag();
                skipWhitespace();
            }
            if (matchKeyword("kc=")) {
                instr.t.kc = parseFlag();
                skipWhitespace();
            }
        } else {
            error("Expected 'K=' or 'B=' after 'A='");
        }
    } else if (matchKeyword("J=")) {
        // J-type instruction
        instr.type = ParsedInstruction::J_TYPE;
        instr.j.J  = parseHexDword();
        skipWhitespace();

        if (matchKeyword("s=")) {
            instr.j.s = parseFlag();
            skipWhitespace();
        }
    } else {
        error("Expected 'A=' or 'J=' for instruction operands");
    }

    skipToNextLine();
    return instr;
}

ParsedConstant SpecParser::parseConstant() {
    ParsedConstant constant = {};

    // Parse K[index]:
    if (!parseRowHeader('K', constant.index)) {
        error("Expected 'K[<index>]:' for constant");
    }

    // Parse type name
    parseIdentifier(constant.typeName, sizeof(constant.typeName));
    skipWhitespace();

    // Parse properties (rest of line)
    size_t propIdx = 0;
    while (!isAtEnd() && peek() != '\n' && peek() != '\r') {
        if (propIdx < sizeof(constant.properties) - 1) {
            constant.properties[propIdx++] = peek();
        }
        advance();
    }
    constant.properties[propIdx] = '\0';

    // Trim trailing whitespace from properties
    while (propIdx > 0 && isspace((unsigned char)constant.properties[propIdx - 1])) {
        constant.properties[--propIdx] = '\0';
    }

    // Check for label (-> @label)
    const char* arrow = strstr(constant.properties, "->");
    if (arrow) {
        const char* at = strchr(arrow, '@');
        if (at) {
            at++;  // Skip '@'
            size_t labelIdx = 0;
            while (*at && !isspace((unsigned char)*at) && labelIdx < sizeof(constant.label) - 1) {
                constant.label[labelIdx++] = *at++;
            }
            constant.label[labelIdx] = '\0';
        }
    }

    skipToNextLine();
    return constant;
}

ParsedUpvalue SpecParser::parseUpvalue() {
    ParsedUpvalue upval = {};

    // Parse U[slot]:
    size_t slot;
    if (!parseRowHeader('U', slot)) {
        error("Expected 'U[<slot>]:' for upvalue");
    }
    upval.slot = (uint8_t)slot;

    // Parse index=
    if (!matchKeyword("index=")) error("Expected 'index=' for upvalue");
    upval.index = (uint8_t)parseDecimal();
    skipWhitespace();

    // Parse isLocal=
    if (!matchKeyword("isLocal=")) error("Expected 'isLocal=' for upvalue");
    upval.isLocal = parseFlag();

    skipToNextLine();
    return upval;
}

ParsedExport SpecParser::parseExport() {
    ParsedExport exp = {};

    // Parse E[index]:
    if (!parseRowHeader('E', exp.index)) {
        error("Expected 'E[<index>]:' for export");
    }

    // Parse identifier
    parseIdentifier(exp.identifier, sizeof(exp.identifier));

    skipToNextLine();
    return exp;
}

ParsedGlobal SpecParser::parseGlobal() {
    ParsedGlobal glob = {};

    // Parse G[index]:
    if (!parseRowHeader('G', glob.index)) {
        error("Expected 'G[<index>]:' for global");
    }

    // Parse identifier
    parseIdentifier(glob.identifier, sizeof(glob.identifier));

    skipToNextLine();
    return glob;
}

ParsedType SpecParser::parseType() {
    ParsedType type = {};

    // Parse T[typeId]:
    if (!parseRowHeader('T', type.typeId)) {
        error("Expected 'T[<typeId>]:' for type");
    }

    // Parse type name
    parseIdentifier(type.typeName, sizeof(type.typeName));

    skipToNextLine();
    return type;
}

PreDefineVariable SpecParser::parsePreDefineVariable() {
    PreDefineVariable var = {};

    // Parse identifier:
    parseIdentifier(var.identifier, sizeof(var.identifier));
    if (!match(':')) error("Expected ':' after identifier");
    skipWhitespace();

    // Parse type name
    parseIdentifier(var.typeName, sizeof(var.typeName));
    skipWhitespace();

    // Parse value (rest of line)
    size_t valIdx = 0;
    while (!isAtEnd() && peek() != '\n' && peek() != '\r') {
        if (valIdx < sizeof(var.value) - 1) {
            var.value[valIdx++] = peek();
        }
        advance();
    }
    var.value[valIdx] = '\0';

    // Trim trailing whitespace
    while (valIdx > 0 && isspace((unsigned char)var.value[valIdx - 1])) {
        var.value[--valIdx] = '\0';
    }

    skipToNextLine();
    return var;
}

Opcode SpecParser::parseOpcode(char* name, size_t nameSize) {
    size_t idx = 0;
    while (!isAtEnd() && (isalnum((unsigned char)peek()) || peek() == '_') && idx < nameSize - 1) {
        name[idx++] = peek();
        advance();
    }
    name[idx] = '\0';

    // Match opcode name to enum using string comparison
#define OPCODE_STR_CASE(opname, type)                         \
    if (strcmp(name, "OP_" #opname) == 0) return OP_##opname;

    OPCODE_X_MACRO(OPCODE_STR_CASE)
#undef OPCODE_STR_CASE

    errorFmt("Unknown opcode: %s", name);
}

uint8_t SpecParser::parseHexByte() {
    if (!matchKeyword("0x")) error("Expected '0x' for hex byte");

    uint8_t value = 0;
    for (int i = 0; i < 2; i++) {
        if (!isxdigit((unsigned char)peek())) error("Expected hex digit");
        char c = peek();
        advance();

        uint8_t digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
        else error("Invalid hex digit");

        value = (value << 4) | digit;
    }

    return value;
}

uint16_t SpecParser::parseHexWord() {
    if (!matchKeyword("0x")) error("Expected '0x' for hex word");

    uint16_t value = 0;
    for (int i = 0; i < 4; i++) {
        if (!isxdigit((unsigned char)peek())) error("Expected hex digit");
        char c = peek();
        advance();

        uint8_t digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
        else error("Invalid hex digit");

        value = (value << 4) | digit;
    }

    return value;
}

uint32_t SpecParser::parseHexDword() {
    if (!matchKeyword("0x")) error("Expected '0x' for hex dword");

    uint32_t value = 0;
    for (int i = 0; i < 6; i++) {
        if (!isxdigit((unsigned char)peek())) error("Expected hex digit");
        char c = peek();
        advance();

        uint8_t digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
        else error("Invalid hex digit");

        value = (value << 4) | digit;
    }

    return value;
}

uint32_t SpecParser::parseDecimal() {
    uint32_t value = 0;
    if (!isdigit((unsigned char)peek())) error("Expected decimal digit");

    while (isdigit((unsigned char)peek())) {
        value = value * 10 + (peek() - '0');
        advance();
    }

    return value;
}

bool SpecParser::parseFlag() {
    if (peek() == 'T') {
        advance();
        return true;
    } else if (peek() == 'F') {
        advance();
        return false;
    } else {
        error("Expected 'T' or 'F' for flag");
    }
}

void SpecParser::parseString(char* buffer, size_t bufferSize) {
    if (!match('"')) error("Expected '\"' for string");

    size_t idx = 0;
    while (!isAtEnd() && peek() != '"') {
        if (idx < bufferSize - 1) {
            buffer[idx++] = peek();
        }
        advance();
    }
    buffer[idx] = '\0';

    if (!match('"')) error("Expected closing '\"' for string");
}

void SpecParser::parseIdentifier(char* buffer, size_t bufferSize) {
    size_t idx = 0;
    while (!isAtEnd() && (isalnum((unsigned char)peek()) || peek() == '_') && idx < bufferSize - 1) {
        buffer[idx++] = peek();
        advance();
    }
    buffer[idx] = '\0';

    if (idx == 0) error("Expected identifier");
}

void SpecParser::parseLabel(char* buffer, size_t bufferSize) {
    size_t idx = 0;
    while (!isAtEnd() && peek() != ']' && idx < bufferSize - 1) {
        buffer[idx++] = peek();
        advance();
    }
    buffer[idx] = '\0';

    if (idx == 0) error("Expected label");
}

void SpecParser::skipWhitespace() {
    while (!isAtEnd() && (peek() == ' ' || peek() == '\t')) {
        advance();
    }
}

void SpecParser::skipToNextLine() {
    while (!isAtEnd() && peek() != '\n') {
        advance();
    }
    if (!isAtEnd() && peek() == '\n') {
        advance();
    }
}

bool SpecParser::isAtEnd() const {
    return spec[pos] == '\0';
}

char SpecParser::peek() const {
    return spec[pos];
}

char SpecParser::advance() {
    char c = spec[pos++];
    if (c == '\n') {
        line++;
        col = 1;
    } else {
        col++;
    }
    return c;
}

bool SpecParser::match(char expected) {
    if (isAtEnd() || peek() != expected) return false;
    advance();
    return true;
}

bool SpecParser::matchKeyword(const char* keyword) {
    size_t len = strlen(keyword);
    if (strncmp(spec + pos, keyword, len) == 0) {
        for (size_t i = 0; i < len; i++) {
            advance();
        }
        return true;
    }
    return false;
}

[[noreturn]] void SpecParser::error(const char* msg) {
    fprintf(stderr, "Parse error at line %zu, col %zu: %s\n", line, col, msg);
    throw std::runtime_error(msg);
}

[[noreturn]] void SpecParser::errorFmt(const char* fmt, ...) {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    error(buffer);
}

/*
 │ Verifier Implementation
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

Verifier::Verifier(const ParsedSpec& spec) : spec(spec) {}

void Verifier::verify(SemiModule* module) {
    // Verify main instructions
    auto mainIt = spec.functions.find("");
    if (mainIt != spec.functions.end() && module->moduleInit) {
        verifyInstructions(module->moduleInit, mainIt->second, "main");
    }

    // Verify constants and nested functions
    if (!spec.constants.empty()) {
        verifyConstants(module);
    }

    // Verify exports
    if (!spec.exports.empty()) {
        verifyExports(module);
    }

    // Verify globals
    if (!spec.globals.empty()) {
        verifyGlobals(module);
    }
}

void Verifier::verify(Compiler* compiler) {
    if (compiler->artifactModule == nullptr) {
        ADD_FAILURE() << "Compiler has no artifact module for verification.";
        return;
    }

    auto mainIt = spec.functions.find("");
    if (mainIt != spec.functions.end()) {
        FunctionProto* moduleInit = compiler->artifactModule->moduleInit;

        // Create a temporary FunctionProto to wrap the rootFunction's chunk
        // Note: This is a stack-allocated struct that mimics FunctionProto for verification only
        struct {
            Chunk chunk;
            ModuleId moduleId;
            uint8_t arity;
            uint8_t coarity;
            uint8_t maxStackSize;
            uint8_t upvalueCount;
        } tempFunc;

        if (moduleInit == nullptr) {
            tempFunc.chunk        = compiler->rootFunction.chunk;
            tempFunc.maxStackSize = compiler->rootFunction.maxUsedRegisterCount;
            tempFunc.arity        = 0;
            tempFunc.upvalueCount = 0;
            tempFunc.coarity      = 0;
            tempFunc.moduleId     = compiler->artifactModule->moduleId;

            moduleInit = (FunctionProto*)&tempFunc;
        }

        verifyInstructions(moduleInit, mainIt->second, "main");
    }

    if (!spec.constants.empty()) {
        verifyConstants(compiler->artifactModule);
    }
    if (!spec.exports.empty()) {
        verifyExports(compiler->artifactModule);
    }
    if (!spec.globals.empty()) {
        verifyGlobals(compiler->artifactModule);
    }
}

void Verifier::verifyInstructions(FunctionProto* func, const ParsedFunction& expected, const char* label) {
    const auto& expectedInstrs = expected.instructions;

    // Check instruction count
    if (func->chunk.size != expectedInstrs.size()) {
        ADD_FAILURE() << "Instruction count mismatch in [Instructions" << (label[0] ? ":" : "") << label << "]\n"
                      << "  Expected: " << expectedInstrs.size() << " instructions\n"
                      << "  Actual:   " << func->chunk.size << " instructions";
        return;
    }

    // Verify each instruction
    for (size_t i = 0; i < expectedInstrs.size(); i++) {
        const ParsedInstruction& exp = expectedInstrs[i];
        Instruction actual           = func->chunk.data[i];

        if (!compareInstructions(exp, actual)) {
            ADD_FAILURE() << "Mismatch at [Instructions" << (label[0] ? ":" : "") << label << "].(" << i << "):\n"
                          << "  Expected: " << formatInstruction(exp) << "\n"
                          << "  Actual:   " << formatInstruction(actual, (Opcode)GET_OPCODE(actual));
        }
    }

    // Verify upvalues if specified
    if (!expected.upvalues.empty()) {
        verifyUpvalues(func, expected.upvalues, label);
    }
}

bool Verifier::compareInstructions(const ParsedInstruction& expected, Instruction actual) {
    Opcode actualOpcode = (Opcode)GET_OPCODE(actual);

    if (expected.opcode != actualOpcode) {
        return false;
    }

    switch (expected.type) {
        case ParsedInstruction::K_TYPE: {
            uint8_t actualA  = OPERAND_K_A(actual);
            uint16_t actualK = OPERAND_K_K(actual);
            bool actualI     = OPERAND_K_I(actual);
            bool actualS     = OPERAND_K_S(actual);

            return expected.k.A == actualA && expected.k.K == actualK && expected.k.i == actualI &&
                   expected.k.s == actualS;
        }

        case ParsedInstruction::T_TYPE: {
            uint8_t actualA = OPERAND_T_A(actual);
            uint8_t actualB = OPERAND_T_B(actual);
            uint8_t actualC = OPERAND_T_C(actual);
            bool actualKb   = OPERAND_T_KB(actual);
            bool actualKc   = OPERAND_T_KC(actual);

            return expected.t.A == actualA && expected.t.B == actualB && expected.t.C == actualC &&
                   expected.t.kb == actualKb && expected.t.kc == actualKc;
        }

        case ParsedInstruction::J_TYPE: {
            uint32_t actualJ = OPERAND_J_J(actual);
            bool actualS     = OPERAND_J_S(actual);

            return expected.j.J == actualJ && expected.j.s == actualS;
        }
    }

    return false;
}

std::string Verifier::formatInstruction(const ParsedInstruction& instr) {
    char buffer[256];

    switch (instr.type) {
        case ParsedInstruction::K_TYPE:
            snprintf(buffer,
                     sizeof(buffer),
                     "%s A=0x%02X K=0x%04X i=%c s=%c",
                     instr.opcodeName,
                     instr.k.A,
                     instr.k.K,
                     instr.k.i ? 'T' : 'F',
                     instr.k.s ? 'T' : 'F');
            break;

        case ParsedInstruction::T_TYPE:
            snprintf(buffer,
                     sizeof(buffer),
                     "%s A=0x%02X B=0x%02X C=0x%02X kb=%c kc=%c",
                     instr.opcodeName,
                     instr.t.A,
                     instr.t.B,
                     instr.t.C,
                     instr.t.kb ? 'T' : 'F',
                     instr.t.kc ? 'T' : 'F');
            break;

        case ParsedInstruction::J_TYPE:
            snprintf(buffer, sizeof(buffer), "%s J=0x%06X s=%c", instr.opcodeName, instr.j.J, instr.j.s ? 'T' : 'F');
            break;
    }

    return std::string(buffer);
}

std::string Verifier::formatInstruction(Instruction instr, Opcode opcode) {
    char buffer[256];
    const char* opcodeName = "UNKNOWN";

    // Get opcode name using X-macro
    switch (opcode) {
#define SET_OPCODE_NAME(opname, type) \
    case OP_##opname:                 \
        opcodeName = "OP_" #opname;   \
        break;
        OPCODE_X_MACRO(SET_OPCODE_NAME)
#undef SET_OPCODE_NAME
        default:
            break;
    }

    // Determine type from opcode using X-macro
    const char* instrType = nullptr;
    switch (opcode) {
#define CASE_TYPE_N(opname, type) \
    case OP_##opname:             \
        instrType = "N";          \
        break;
#define CASE_TYPE_J(opname, type) \
    case OP_##opname:             \
        instrType = "J";          \
        break;
#define CASE_TYPE_K(opname, type) \
    case OP_##opname:             \
        instrType = "K";          \
        break;
#define CASE_TYPE_T(opname, type) \
    case OP_##opname:             \
        instrType = "T";          \
        break;
#define CASE_OPCODE_TYPE(opname, type) CASE_TYPE_##type(opname, type)
        OPCODE_X_MACRO(CASE_OPCODE_TYPE)
#undef CASE_TYPE_N
#undef CASE_TYPE_J
#undef CASE_TYPE_K
#undef CASE_TYPE_T
#undef CASE_OPCODE_TYPE
        default:
            instrType = "UNKNOWN";
            break;
    }

    bool isKType = (instrType && instrType[0] == 'K');
    bool isJType = (instrType && instrType[0] == 'J');

    if (isKType) {
        snprintf(buffer,
                 sizeof(buffer),
                 "%s A=0x%02X K=0x%04X i=%c s=%c",
                 opcodeName,
                 OPERAND_K_A(instr),
                 OPERAND_K_K(instr),
                 OPERAND_K_I(instr) ? 'T' : 'F',
                 OPERAND_K_S(instr) ? 'T' : 'F');
    } else if (isJType) {
        snprintf(
            buffer, sizeof(buffer), "%s J=0x%06X s=%c", opcodeName, OPERAND_J_J(instr), OPERAND_J_S(instr) ? 'T' : 'F');
    } else {
        snprintf(buffer,
                 sizeof(buffer),
                 "%s A=0x%02X B=0x%02X C=0x%02X kb=%c kc=%c",
                 opcodeName,
                 OPERAND_T_A(instr),
                 OPERAND_T_B(instr),
                 OPERAND_T_C(instr),
                 OPERAND_T_KB(instr) ? 'T' : 'F',
                 OPERAND_T_KC(instr) ? 'T' : 'F');
    }

    return std::string(buffer);
}

void Verifier::verifyConstantFunctionProto(const Value* value, const ParsedConstant& constant) {
    if (!IS_FUNCTION_PROTO(value)) {
        ADD_FAILURE() << "Type mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: FunctionProto\n"
                      << "  Actual:   (not a FunctionProto)";
        return;
    }

    FunctionProto* func = AS_PTR(value, FunctionProto);

    // Parse expected properties: "arity=X coarity=Y size=Z"
    uint8_t expectedArity = 0, expectedCoarity = 0;
    size_t expectedSize = 0;
    sscanf(constant.properties, "arity=%hhu coarity=%hhu size=%zu", &expectedArity, &expectedCoarity, &expectedSize);

    // Verify arity
    if (func->arity != expectedArity) {
        ADD_FAILURE() << "FunctionProto arity mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: arity=" << (int)expectedArity << "\n"
                      << "  Actual:   arity=" << (int)func->arity;
    }

    // Verify coarity
    if (func->coarity != expectedCoarity) {
        ADD_FAILURE() << "FunctionProto coarity mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: coarity=" << (int)expectedCoarity << "\n"
                      << "  Actual:   coarity=" << (int)func->coarity;
    }

    // Verify size
    if (func->chunk.size != expectedSize) {
        ADD_FAILURE() << "FunctionProto size mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: size=" << expectedSize << "\n"
                      << "  Actual:   size=" << func->chunk.size;
    }

    // Verify nested function instructions if label is provided
    if (constant.label[0] != '\0') {
        auto funcIt = spec.functions.find(constant.label);
        if (funcIt != spec.functions.end()) {
            verifyInstructions(func, funcIt->second, constant.label);
        }
    }
}

void Verifier::verifyConstantInt(const Value* value, const ParsedConstant& constant) {
    if (!IS_INT(value)) {
        ADD_FAILURE() << "Type mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: Int\n"
                      << "  Actual:   (not an Int)";
        return;
    }

    // Parse expected value
    int64_t expectedValue = 0;
    sscanf(constant.properties, "%lld", &expectedValue);

    int64_t actualValue = AS_INT(value);

    if (actualValue != expectedValue) {
        ADD_FAILURE() << "Int value mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: " << expectedValue << "\n"
                      << "  Actual:   " << actualValue;
    }
}

void Verifier::verifyConstantFloat(const Value* value, const ParsedConstant& constant) {
    if (!IS_FLOAT(value)) {
        ADD_FAILURE() << "Type mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: Float\n"
                      << "  Actual:   (not a Float)";
        return;
    }

    // Parse expected value
    double expectedValue = 0.0;
    sscanf(constant.properties, "%lf", &expectedValue);

    double actualValue = AS_FLOAT(value);

    // Use a small epsilon for floating point comparison
    const double epsilon = 1e-9;
    if (std::abs(actualValue - expectedValue) > epsilon) {
        ADD_FAILURE() << "Float value mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: " << expectedValue << "\n"
                      << "  Actual:   " << actualValue;
    }
}

void Verifier::verifyConstantString(const Value* value, const ParsedConstant& constant) {
    // Verify String constants (both InlineString and ObjectString)
    if (!IS_INLINE_STRING(value) && !IS_OBJECT_STRING(value)) {
        ADD_FAILURE() << "Type mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: String\n"
                      << "  Actual:   (not a String)";
        return;
    }

    // Parse expected properties: "text" length=X [bytes=Y]
    // Find the quoted string
    const char* start = strchr(constant.properties, '"');
    if (!start) {
        ADD_FAILURE() << "Invalid String properties format at [Constants].(" << constant.index
                      << "): missing opening quote";
        return;
    }
    start++;  // Skip opening quote

    const char* end = strchr(start, '"');
    if (!end) {
        ADD_FAILURE() << "Invalid String properties format at [Constants].(" << constant.index
                      << "): missing closing quote";
        return;
    }

    std::string expectedText(start, end - start);

    // Parse length and optional bytes
    size_t expectedLength = 0;
    size_t expectedBytes  = 0;
    const char* lengthPos = strstr(constant.properties, "length=");
    if (lengthPos) {
        sscanf(lengthPos, "length=%zu", &expectedLength);
    }

    const char* bytesPos = strstr(constant.properties, "bytes=");
    if (bytesPos) {
        sscanf(bytesPos, "bytes=%zu", &expectedBytes);
    } else {
        expectedBytes = expectedText.length();  // Default to text length if not specified
    }

    // Get actual string data
    const char* actualText;
    size_t actualLength, actualBytes;

    if (IS_INLINE_STRING(value)) {
        InlineString str = AS_INLINE_STRING(value);
        actualText       = str.c;
        actualLength     = str.length;
        actualBytes      = str.length;  // InlineString bytes = length
    } else {
        ObjectString* str = AS_OBJECT_STRING(value);
        actualText        = str->str;
        actualLength      = str->length;
        actualBytes       = str->length;  // For now, assuming UTF-8 length stored in bytes
    }

    // Verify length (character count)
    if (actualLength != expectedLength) {
        ADD_FAILURE() << "String length mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: length=" << expectedLength << "\n"
                      << "  Actual:   length=" << actualLength;
    }

    // Verify bytes (byte count)
    if (actualBytes != expectedBytes) {
        ADD_FAILURE() << "String bytes mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: bytes=" << expectedBytes << "\n"
                      << "  Actual:   bytes=" << actualBytes;
    }

    // Verify text content
    std::string actualTextStr(actualText, actualBytes);
    if (actualTextStr != expectedText) {
        ADD_FAILURE() << "String text mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: \"" << expectedText << "\"\n"
                      << "  Actual:   \"" << actualTextStr << "\"";
    }
}

void Verifier::verifyConstantRange(const Value* value, const ParsedConstant& constant) {
    // Verify Range constants (both InlineRange and ObjectRange)
    if (!IS_INLINE_RANGE(value) && !IS_OBJECT_RANGE(value)) {
        ADD_FAILURE() << "Type mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: Range\n"
                      << "  Actual:   (not a Range)";
        return;
    }

    // Parse expected properties: "start=X end=Y step=Z"
    int64_t expectedStart = 0, expectedEnd = 0, expectedStep = 0;
    sscanf(constant.properties, "start=%lld end=%lld step=%lld", &expectedStart, &expectedEnd, &expectedStep);

    int64_t actualStart, actualEnd, actualStep;

    if (IS_INLINE_RANGE(value)) {
        InlineRange range = AS_INLINE_RANGE(value);
        actualStart       = range.start;
        actualEnd         = range.end;
        actualStep        = 1;  // InlineRange always has step=1
    } else {
        ObjectRange* range = AS_OBJECT_RANGE(value);
        actualStart        = IS_INT(&range->start) ? AS_INT(&range->start) : 0;
        actualEnd          = IS_INT(&range->end) ? AS_INT(&range->end) : 0;
        actualStep         = IS_INT(&range->step) ? AS_INT(&range->step) : 0;
    }

    // Verify start
    if (actualStart != expectedStart) {
        ADD_FAILURE() << "Range start mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: start=" << expectedStart << "\n"
                      << "  Actual:   start=" << actualStart;
    }

    // Verify end
    if (actualEnd != expectedEnd) {
        ADD_FAILURE() << "Range end mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: end=" << expectedEnd << "\n"
                      << "  Actual:   end=" << actualEnd;
    }

    // Verify step
    if (actualStep != expectedStep) {
        ADD_FAILURE() << "Range step mismatch at [Constants].(" << constant.index << "):\n"
                      << "  Expected: step=" << expectedStep << "\n"
                      << "  Actual:   step=" << actualStep;
    }
}

void Verifier::verifyConstants(SemiModule* module) {
    for (const auto& constant : spec.constants) {
        if (constant.index >= semiConstantTableSize(&module->constantTable)) {
            ADD_FAILURE() << "Missing entry at [Constants].(" << constant.index << "):\n"
                          << "  Expected: K[" << constant.index << "]: " << constant.typeName << " "
                          << constant.properties << "\n"
                          << "  Actual:   (not found)";
            continue;
        }

        // Get the actual constant value
        Value actualValue = semiConstantTableGet(&module->constantTable, constant.index);

        // Dispatch to appropriate verifier based on type
        if (strcmp(constant.typeName, "Int") == 0) {
            verifyConstantInt(&actualValue, constant);
        } else if (strcmp(constant.typeName, "Float") == 0) {
            verifyConstantFloat(&actualValue, constant);
        } else if (strcmp(constant.typeName, "String") == 0) {
            verifyConstantString(&actualValue, constant);
        } else if (strcmp(constant.typeName, "Range") == 0) {
            verifyConstantRange(&actualValue, constant);
        } else if (strcmp(constant.typeName, "FunctionProto") == 0) {
            verifyConstantFunctionProto(&actualValue, constant);
        }
    }
}

void Verifier::verifyExports(SemiModule* module) {
    // Basic implementation - will be expanded in Phase 2
    EXPECT_EQ(module->exports.len, spec.exports.size()) << "Export count mismatch";
}

void Verifier::verifyGlobals(SemiModule* module) {
    // Basic implementation - will be expanded in Phase 2
    EXPECT_EQ(module->globals.len, spec.globals.size()) << "Global count mismatch";
}

void Verifier::verifyTypes(SemiVM* vm) {
    // Will be implemented in Phase 2
}

void Verifier::verifyUpvalues(FunctionProto* func, const std::vector<ParsedUpvalue>& expected, const char* label) {
    if (func->upvalueCount != expected.size()) {
        ADD_FAILURE() << "Upvalue count mismatch in [UpvalueDescription:" << label << "]\n"
                      << "  Expected: " << expected.size() << " upvalues\n"
                      << "  Actual:   " << (int)func->upvalueCount << " upvalues";
        return;
    }

    for (const auto& exp : expected) {
        if (exp.slot >= func->upvalueCount) {
            ADD_FAILURE() << "Invalid upvalue slot U[" << (int)exp.slot << "] in [UpvalueDescription:" << label << "]";
            continue;
        }

        const UpvalueDescription& actual = func->upvalues[exp.slot];

        if (actual.index != exp.index || actual.isLocal != exp.isLocal) {
            ADD_FAILURE() << "Mismatch at [UpvalueDescription:" << label << "].(" << (int)exp.slot << "):\n"
                          << "  Expected: U[" << (int)exp.slot << "]: index=" << (int)exp.index
                          << " isLocal=" << (exp.isLocal ? "T" : "F") << "\n"
                          << "  Actual:   U[" << (int)exp.slot << "]: index=" << (int)actual.index
                          << " isLocal=" << (actual.isLocal ? "T" : "F");
        }
    }
}

/*
 │ Public API
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

void VerifyCompilation(SemiModule* module, const char* spec) {
    try {
        SpecParser parser(spec);
        ParsedSpec parsed = parser.parse();

        Verifier verifier(parsed);
        verifier.verify(module);
    } catch (const std::exception& e) {
        ADD_FAILURE() << "Failed to parse DSL spec: " << e.what();
    }
}

void VerifyCompilation(Compiler* compiler, const char* spec) {
    try {
        SpecParser parser(spec);
        ParsedSpec parsed = parser.parse();

        Verifier verifier(parsed);
        verifier.verify(compiler);
    } catch (const std::exception& e) {
        ADD_FAILURE() << "Failed to parse DSL spec: " << e.what();
    }
}

}  // namespace InstructionVerifier
