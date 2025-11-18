#include <iomanip>
#include <iostream>
extern "C" {
#include "../src/const_table.h"
#include "../src/instruction.h"
}

// Global opcode name and type lookup tables
// These are statically initialized to avoid runtime overhead

// Suppress C99 designated initializer warnings - they're valid C99 and will be standard in C++20
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-designator"
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc99-extensions"
#endif

// Static opcode name strings - initialized via X-macro
static const char* opcodeNames[MAX_OPCODE + 1] = {
#define OPCODE_NAME_INIT(name, type) [OP_##name] = "OP_" #name,
    OPCODE_X_MACRO(OPCODE_NAME_INIT)
#undef OPCODE_NAME_INIT
};

// Static opcode type strings - initialized via X-macro
static const char* opcodeTypes[MAX_OPCODE + 1] = {
#define OPCODE_TYPE_INIT_N(name, type) [OP_##name] = "N",
#define OPCODE_TYPE_INIT_J(name, type) [OP_##name] = "J",
#define OPCODE_TYPE_INIT_K(name, type) [OP_##name] = "K",
#define OPCODE_TYPE_INIT_T(name, type) [OP_##name] = "T",
#define OPCODE_TYPE_INIT(name, type)   OPCODE_TYPE_INIT_##type(name, type)
    OPCODE_X_MACRO(OPCODE_TYPE_INIT)
#undef OPCODE_TYPE_INIT_N
#undef OPCODE_TYPE_INIT_J
#undef OPCODE_TYPE_INIT_K
#undef OPCODE_TYPE_INIT_T
#undef OPCODE_TYPE_INIT
};

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

// Inline accessor functions
static inline const char* getOpcodeName(Opcode opcode) {
    return (opcode <= MAX_OPCODE) ? opcodeNames[opcode] : "UNKNOWN";
}

static inline const char* getOpcodeType(Opcode opcode) {
    return (opcode <= MAX_OPCODE) ? opcodeTypes[opcode] : "UNKNOWN";
}

static void printInstruction(Instruction instruction, PCLocation pc) {
    Opcode opcode          = (Opcode)GET_OPCODE(instruction);
    const char* opcodeName = getOpcodeName(opcode);
    const char* type       = getOpcodeType(opcode);

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

static void disassembleCode(Instruction* instructions, size_t count) {
    std::cout << std::left << std::setw(4) << "Loc" << std::setw(25) << "Opcode" << std::setw(7) << "Type"
              << "Operands" << std::endl;
    std::cout << "-----------------------------------------------------------------------" << std::endl;

    for (PCLocation pc = 0; pc < count; pc++) {
        printInstruction(instructions[pc], pc);
    }

    std::cout << std::endl;
}

static void printValue(Value v) {
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
        case VALUE_TYPE_LIST: {
            ObjectList* list = AS_LIST(value);
            std::cout << "List[";
            for (uint32_t j = 0; j < list->size; j++) {
                printValue(list->values[j]);
                if (j + 1 < list->size) {
                    std::cout << ", ";
                }
            }
            std::cout << " ]";
            break;
        }
        case VALUE_TYPE_DICT: {
            ObjectDict* dict = AS_DICT(value);
            if (dict->len == 0) {
                std::cout << "Dict[]";
                break;
            }

            std::cout << "Dict[ ";
            uint32_t j = 0;
            for (;;) {
                if (IS_VALID(&dict->keys[j].key)) {
                    printValue(dict->keys[j].key);
                    std::cout << ": ";
                    printValue(dict->values[j]);
                }
                j++;
                if (j < dict->len) {
                    std::cout << ", ";
                } else {
                    break;
                }
            }
            std::cout << " ]";
            break;
        }
        default:
            std::cout << "<unprintable value type " << (int)VALUE_TYPE(value) << ">";
            break;
    }
}

static void printConstantsInfo(ConstantTable* constTable) {
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
