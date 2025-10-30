#include <iomanip>
#include <iostream>
extern "C" {
#include "../src/const_table.h"
#include "../src/instruction.h"
}

static const char* opcodeNames[MAX_OPCODE + 1];
static bool isOpcodeInitialized = false;

static void initOpcodeNames() {
    if (isOpcodeInitialized) {
        return;
    }
    isOpcodeInitialized = true;

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

static const char* getInstructionType(Opcode opcode) {
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

static void printInstruction(Instruction instruction, PCLocation pc) {
    initOpcodeNames();

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

static void disassembleCode(Instruction* instructions, size_t count) {
    initOpcodeNames();

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
