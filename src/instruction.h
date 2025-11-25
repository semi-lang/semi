// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_INSTRUCTION_H
#define SEMI_INSTRUCTION_H

#include <stdbool.h>
#include <stdint.h>

/*=================================================================================================
  All instructions have 32 bits, with its opcode in the low 7 bits. As a
  "premature optimization", we align the operand addresses to 8 bits.

  All of the jump locations are relative to the offset of the current instruction.
  This allows us to emit instructions in multiple chunks simultaneously and combine
  them together without needing to patch the jump locations.

    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   |      A(8)     |      B(8)     |      C(8)     |k|k|   Op(6)   |   T-type (Ternary)
   |      A(8)     |              K(16)            |i|s|   Op(6)   |   K-type (Constant)
   |                     J(24)                     |s| |   Op(6)   |   J-type (Jump)

  Symbols:
    * A, B, C: Operand registers (A, B, C)
    * J: Jump offset
    * K: Constant value
  Flags:
    * k: Constant flags for register
    * s: Sign flag
    * i: Inline constant flag
=================================================================================================*/

typedef uint32_t Instruction;

#define OPCODE_BITS   6
#define OPCODE_MASK   ((Instruction)(1 << OPCODE_BITS) - 1)
#define MAX_OPERAND_K ((Instruction)(UINT16_MAX))
#define MAX_OPERAND_J ((Instruction)(1 << (24)) - 1)

#define GET_OPCODE(instruction) ((instruction) & OPCODE_MASK)  // Bits 0-5

// N-type instructions
#define MAKE_INSTRUCTION_N(name)                         \
    static inline Instruction INSTRUCTION_##name(void) { \
        return 0;                                        \
    }

// T-type instructions
#define MAKE_INSTRUCTION_T(name)                                                                      \
    static inline Instruction INSTRUCTION_##name(uint8_t A, uint8_t B, uint8_t C, bool kb, bool kc) { \
        return ((((Instruction)(OP_##name)) & OPCODE_MASK) | (((Instruction)(A)) << 24) |             \
                (((Instruction)(B) & 0xFF) << 16) | (((Instruction)(C) & 0xFF) << 8) |                \
                (((Instruction)(bool)(kb)) << 7) | (((Instruction)(bool)(kc)) << 6));                 \
    }
#define OPERAND_T_A(instruction)  ((uint8_t)(instruction >> 24))           // Bits 24-31
#define OPERAND_T_B(instruction)  ((uint8_t)((instruction >> 16) & 0xFF))  // Bits 16-23
#define OPERAND_T_C(instruction)  ((uint8_t)((instruction >> 8) & 0xFF))   // Bits 8-15
#define OPERAND_T_KB(instruction) ((instruction & (1 << 7)) != 0)          // Bits 7
#define OPERAND_T_KC(instruction) ((instruction & (1 << 6)) != 0)          // Bits 6

// K-type instructions
#define MAKE_INSTRUCTION_K(name)                                                          \
    static inline Instruction INSTRUCTION_##name(uint8_t A, uint16_t K, bool i, bool s) { \
        return ((((Instruction)(OP_##name)) & OPCODE_MASK) | (((Instruction)(A)) << 24) | \
                (((Instruction)(K) & 0xFFFF) << 8) | (((Instruction)(bool)(i)) << 7) |    \
                (((Instruction)(bool)(s)) << 6));                                         \
    }
#define OPERAND_K_A(instruction) ((uint8_t)(instruction >> 24))             // Bits 24-31
#define OPERAND_K_K(instruction) ((uint16_t)((instruction >> 8) & 0xFFFF))  // Bits 8-23
#define OPERAND_K_I(instruction) ((instruction & (1 << 7)) != 0)            // Bits 7
#define OPERAND_K_S(instruction) ((instruction & (1 << 6)) != 0)            // Bits 6

// J-type instructions
#define MAKE_INSTRUCTION_J(name)                                                         \
    static inline Instruction INSTRUCTION_##name(uint32_t J, bool s) {                   \
        return ((((Instruction)(OP_##name)) & OPCODE_MASK) | (((Instruction)(J)) << 8) | \
                (((Instruction)(bool)(s)) << 7));                                        \
    }
#define OPERAND_J_S(instruction) ((instruction & (1 << 7)) != 0)  // Bits 7
#define OPERAND_J_J(instruction) ((uint32_t)(instruction >> 8))   // Bits 8-31

// X(name, type)
//   * name: opcode name without OP_ prefix
//   * type: instruction type - N (none), J (jump), K (constant), T (ternary)

#define OPCODE_X_MACRO(X)     \
    X(NOOP, N)                \
    X(JUMP, J)                \
    X(EXTRA_ARG, J)           \
    X(TRAP, K)                \
    X(C_JUMP, K)              \
    X(LOAD_CONSTANT, K)       \
    X(LOAD_BOOL, K)           \
    X(LOAD_INLINE_INTEGER, K) \
    X(LOAD_INLINE_STRING, K)  \
    X(GET_MODULE_VAR, K)      \
    X(SET_MODULE_VAR, K)      \
    X(DEFER_CALL, K)          \
    X(ITER_NEXT, K)           \
    X(RANGE_NEXT, K)          \
    X(MOVE, T)                \
    X(GET_UPVALUE, T)         \
    X(SET_UPVALUE, T)         \
    X(CLOSE_UPVALUES, T)      \
    X(ADD, T)                 \
    X(SUBTRACT, T)            \
    X(MULTIPLY, T)            \
    X(DIVIDE, T)              \
    X(FLOOR_DIVIDE, T)        \
    X(MODULO, T)              \
    X(POWER, T)               \
    X(NEGATE, T)              \
    X(GT, T)                  \
    X(GE, T)                  \
    X(EQ, T)                  \
    X(NEQ, T)                 \
    X(BITWISE_AND, T)         \
    X(BITWISE_OR, T)          \
    X(BITWISE_XOR, T)         \
    X(BITWISE_L_SHIFT, T)     \
    X(BITWISE_R_SHIFT, T)     \
    X(BITWISE_INVERT, T)      \
    X(MAKE_RANGE, T)          \
    X(ITER_PREPARE, T)        \
    X(BOOL_NOT, T)            \
    X(GET_ATTR, T)            \
    X(SET_ATTR, T)            \
    X(NEW_COLLECTION, T)      \
    X(GET_ITEM, T)            \
    X(SET_ITEM, T)            \
    X(DEL_ITEM, T)            \
    X(CONTAIN, T)             \
    X(APPEND_LIST, T)         \
    X(APPEND_MAP, T)          \
    X(CALL, T)                \
    X(RETURN, T)              \
    X(CHECK_TYPE, T)

// Opcode definitions
//
// Explanation of the symbols:
//   * R[X]:                  Register of n, where n is the value of `X` in the instruction.
//   * K[X]:                  Constant value at symbol table with index X.
//   * to_bool(x):            Convert x to boolean, where 0 is false and non-zero is true.
//   * RK(X, k):              If k is false, it is the value of register X, otherwise it is an
//                            integer value X-128.
//   * uRK(X, k):             If k is false, it is the value of register X, otherwise it is an
//                            integer value X.
//   * range(from, to, step): Create a range object with start, end, and step.
//   * inline_range(K):       Create an inline range object with start=(K>>8), end=(K&(2^8-1)), step=1.
typedef enum {
    // clang-format off
    OP_NOOP = 0,                // |       |  no operation
    
    OP_JUMP,                    // |   J   |  if J != 0, pc += (s ? J : -J)
    OP_EXTRA_ARG,               // |   J   |  reset extraarg to 0 if s, then extraarg = (extraarg << 24) + J
    
    OP_TRAP,                    // |   K   |  trap and exit with K
    OP_C_JUMP,                  // |   K   |  if to_bool(R[A]) == i and K != 0, pc += (s ? K : -K)
    OP_LOAD_CONSTANT,           // |   K   |  R[A] := i ? VM.globals[K] : Mod.constants[K]
                                //            Mod is the module of the current frame
    OP_LOAD_BOOL,               // |   K   |  R[A] := i
    OP_LOAD_INLINE_INTEGER,     // |   K   |  R[A] := s ? K : -K
    OP_LOAD_INLINE_STRING,      // |   K   |  R[A] := K as inline string
    OP_GET_MODULE_VAR,          // |   K   |  R[A] := s ? Mod.exports[K] : Mod.globals[K];
                                //            Mod is the module of the current frame
    OP_SET_MODULE_VAR,          // |   K   |  s ? (Mod.exports[K] := R[A]) : (Mod.globals[K] := R[A])
                                //            Mod is the module of the current frame
    OP_DEFER_CALL,              // |   K   |  push Mod.constants[K] to the defer stack
                                //            Mod is the module of the current frame
    OP_ITER_NEXT,               // |   K   |  R[A](R[A+1], R[A+2], R[A+3]) (set the next value to R[A+3])
                                //            If R[A+3] != nil, update counter R[A+4] if i is true.
                                //            If R[A+3] == nil, set pc += K
    OP_RANGE_NEXT,              // |   K   |  range R[A] can proceed ? R[A+1] := next value : pc += K
                                //            If i is true, (R[A+2], R[A+3]) := (next value, counter) if it can proceed.

    OP_MOVE,                    // |   T   |  R[A] := R[B]
    OP_GET_UPVALUE,             // |   T   |  R[A] := Upvalue[B]
    OP_SET_UPVALUE,             // |   T   |  Upvalue[A] := R[B]
    OP_CLOSE_UPVALUES,          // |   T   |  close all upvalues >= R[A]
    OP_ADD,                     // |   T   |  R[A] := RK(B, kb)  + RK(C, kc)
    OP_SUBTRACT,                // |   T   |  R[A] := RK(B, kb)  - RK(C, kc)
    OP_MULTIPLY,                // |   T   |  R[A] := RK(B, kb)  * RK(C, kc)
    OP_DIVIDE,                  // |   T   |  R[A] := RK(B, kb)  / RK(C, kc)
    OP_FLOOR_DIVIDE,            // |   T   |  R[A] := RK(B, kb) // RK(C, kc)
    OP_MODULO,                  // |   T   |  R[A] := RK(B, kb)  % RK(C, kc)
    OP_POWER,                   // |   T   |  R[A] := RK(B, kb) ** RK(C, kc)
    OP_NEGATE,                  // |   T   |  R[A] := -R[B]
    OP_GT,                      // |   T   |  R[A] := RK(B, kb)  > RK(C, kc)
    OP_GE,                      // |   T   |  R[A] := RK(B, kb) >= RK(C, kc)
    OP_EQ,                      // |   T   |  R[A] := RK(B, kb) == RK(C, kc)
    OP_NEQ,                     // |   T   |  R[A] := RK(B, kb) != RK(C, kc)
    OP_BITWISE_AND,             // |   T   |  R[A] := RK(B, kb)  & RK(C, kc)
    OP_BITWISE_OR,              // |   T   |  R[A] := RK(B, kb)  | RK(C, kc)
    OP_BITWISE_XOR,             // |   T   |  R[A] := RK(B, kb)  ^ RK(C, kc)
    OP_BITWISE_L_SHIFT,         // |   T   |  R[A] := RK(B, kb) << RK(C, kc)
    OP_BITWISE_R_SHIFT,         // |   T   |  R[A] := RK(B, kb) >> RK(C, kc)
    OP_BITWISE_INVERT,          // |   T   |  R[A] := ~R[B]
    OP_MAKE_RANGE,              // |   T   |  R[A] := make_range(R[A], RK(B, kb), RK(C, kc))
    OP_ITER_PREPARE,            // |   T   |  TYPE(R[A]).__iter__(&R[A], &R[A+1], &R[A+2]), which has the effect of:
                                //            R[A], R[A+1], R[A+2] := next function, invariant data, iter state
    OP_BOOL_NOT,                // |   T   |  R[A] := !R[B]
    OP_GET_ATTR,                // |   T   |  R[A] := GET_ATTR(R[B], uRK(C, kc), kb)
                                //            GET_ATTR(object, index, type_index_or_symbol_index)
    OP_SET_ATTR,                // |   T   |  SET_ATTR(R[A], uRK(B, kb), kc, R[C])
                                //            SET_ATTR(object, index, type_index_or_symbol_index, value)
    OP_NEW_COLLECTION,          // |   T   |  R[A] := new collection of type uRK(B, kb) with initial capacity C
    OP_GET_ITEM,                // |   T   |  R[A] := R[B][RK(C, kc)]
    OP_SET_ITEM,                // |   T   |  R[A][RK(B, kb)] = R[C]
    OP_DEL_ITEM,                // |   T   |  R[A] = delete R[B][RK(C, kc)]
    OP_CONTAIN,                 // |   T   |  R[A] := RK(B, kb)] in R[C]
    OP_APPEND_LIST,             // |   T   |  append C elements to R[A], starting from R[B]
    OP_APPEND_MAP,              // |   T   |  append C pairs to R[A], starting from (R[B], R[B+1]) as key-value pairs
    OP_CALL,                    // |   T   |  R[A](R[A+1], R[A+2], ..., R[A+B]), B is the number of arguments.
                                //            The return value is stored in R[A].
    OP_RETURN,                  // |   T   |  return from function; if A != 255, copy R[A] to the caller register.
    OP_CHECK_TYPE,              // |   T   |  R[A] := R[B] is of type RK(C, kc)
    // clang-format on
} Opcode;

#define OPCODE_COUNT (((uint8_t)OP_CHECK_TYPE) + 1)

// Generate all instruction creation functions using OPCODE_X_MACRO
// This automatically creates INSTRUCTION_* functions for all opcodes by using
// the centralized opcode list and dispatching to the appropriate type-specific macro
#define MAKE_INSTRUCTION_BY_TYPE(name, type) MAKE_INSTRUCTION_##type(name)
OPCODE_X_MACRO(MAKE_INSTRUCTION_BY_TYPE)
#undef MAKE_INSTRUCTION_BY_TYPE

#endif /* SEMI_INSTRUCTION_H */
