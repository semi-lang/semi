// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_COMPILER_H
#define SEMI_COMPILER_H

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#include "./const_table.h"
#include "./gc.h"
#include "./instruction.h"
#include "./primitives.h"
#include "./semi_common.h"
#include "./symbol_table.h"
#include "./types.h"
#include "./value.h"
#include "./vm.h"
#include "semi/config.h"
#include "semi/error.h"

#define MAX_NUMBER_CHAR 1024

#define MAX_IDENTIFIER_LENGTH 256

// All of the tokens Lexer can recognize.
//
// IMPORTANT: Whenever this enum table is modified, grep "TOKEN_ORDER_NOTE"
//            to modify relevant constants.
typedef enum Token {
    // Unset state for peeking the current token without parsing it again.
    TK_NON_TOKEN,

    // Lexing ends here because it reaches the end of the source code,
    // or there is an error.
    TK_EOF,

    // Statement separators like "/n". Whitespaces around it will also be
    // consumed.
    TK_SEPARATOR,

    /* --------------------------------
       Tokens Allowed in Expressions
       -------------------------------- */

    // "+"
    TK_PLUS,
    // "-"
    TK_MINUS,
    // "*"
    TK_STAR,
    // "**"
    TK_DOUBLE_STAR,
    // "/"
    TK_SLASH,
    // "//"
    TK_DOUBLE_SLASH,
    // "%"
    TK_PERCENT,
    // "&"
    TK_AMPERSAND,
    // "|"
    TK_VERTICAL_BAR,
    // "^"
    TK_CARET,
    // "~"
    TK_TILDE,
    // "<<"
    TK_DOUBLE_LEFT_ARROW,
    // ">>"
    TK_DOUBLE_RIGHT_ARROW,
    // "=="
    TK_EQ,
    // "!="
    TK_NOT_EQ,
    // "<"
    TK_LT,
    // "<="
    TK_LTE,
    // ">"
    TK_GT,
    // ">="
    TK_GTE,
    // "!"
    TK_BANG,
    // "?."
    TK_QUESTION_DOT,
    // "?"
    TK_QUESTION,
    // ":"
    TK_COLON,
    // ";"
    TK_SEMICOLON,
    // ":="
    TK_BINDING,
    // "="
    TK_ASSIGN,
    // ","
    TK_COMMA,
    // "."
    TK_DOT,
    // "("
    TK_OPEN_PAREN,
    // ")"
    TK_CLOSE_PAREN,
    // "and"
    TK_AND,
    // "or"
    TK_OR,
    // "in"
    TK_IN,
    // "is"
    TK_IS,
    // "true"
    TK_TRUE,
    // "false"
    TK_FALSE,
    // <identifier> for variables
    TK_IDENTIFIER,
    // <identifier> for types
    TK_TYPE_IDENTIFIER,
    // <integer>
    TK_INTEGER,
    // <double>
    TK_DOUBLE,
    // "<string>" or `string`
    TK_STRING,

    /* --------------------------------
       Expression Stop-Tokens
       -------------------------------- */

    // ".."
    TK_DOUBLE_DOTS,
    // "{"
    TK_OPEN_BRACE,
    // "}"
    TK_CLOSE_BRACE,
    // "["
    TK_OPEN_BRACKET,
    // "]"
    TK_CLOSE_BRACKET,
    // "if"
    TK_IF,
    // "elif"
    TK_ELIF,
    // "else"
    TK_ELSE,
    // "for"
    TK_FOR,
    // "import"
    TK_IMPORT,
    // "export"
    TK_EXPORT,
    // "as"
    TK_AS,
    // "defer"
    TK_DEFER,
    // "fn"
    TK_FN,
    // "return"
    TK_RETURN,
    // "raise"
    TK_RAISE,
    // "break"
    TK_BREAK,
    // "step"
    TK_STEP,
    // "struct"
    TK_STRUCT,
    // "continue"
    TK_CONTINUE,
    // "unset"
    TK_UNSET,
} Token;

struct Compiler;

typedef struct Lexer {
    struct Compiler* compiler;

    const char* lineStart;
    const char* end;
    const char* curr;
    // NOTE: line number never overflow because the maximum number of lines is limited by the length of the source code.
    uint32_t line;

    bool ignoreSeparators;

    Token token;
    union {
        Value constant;
        struct {
            const char* name;
            IdentifierLength length;
        } identifier;
    } tokenValue;

    char buffer[MAX_NUMBER_CHAR];
} Lexer;

typedef enum {
    PRECEDENCE_INVALID  = 0,
    PRECEDENCE_NONE     = 2,
    PRECEDENCE_TERNARY  = 4,   // ?:
    PRECEDENCE_OR       = 6,   // or
    PRECEDENCE_AND      = 8,   // and
    PRECEDENCE_IN       = 10,  // in
    PRECEDENCE_IS       = 12,  // is
    PRECEDENCE_EQ       = 14,  // == !=
    PRECEDENCE_CMP      = 16,  // < > <= >=
    PRECEDENCE_TERM     = 18,  // + - | ^
    PRECEDENCE_FACTOR   = 20,  // * / % << >>
    PRECEDENCE_EXPONENT = 22,  // **
    PRECEDENCE_UNARY    = 24,  // ! - ~
    PRECEDENCE_ACCESS   = 26,  // . [] () <TYPE_IDENTIFIER>{}
} Precedence;

#define PRECEDENCE_NON_KEYWORD PRECEDENCE_EQ

#define MAX_UPVALUE_COUNT (UINT8_MAX - 1)
#define MAX_LOOP_LEVEL    8
#define MAX_STRUCT_FIELDS 64

// The maximum number of registers minus 2 for the returned value and error.
#define MAX_FUNCTION_CALL_ARGS (MAX_LOCAL_REGISTER_ID - 2)
#define MAX_BRACKET_COUNT      255

typedef enum PrattExprType {
    PRATT_EXPR_TYPE_UNSET,
    // This expression type is used for constant values. Constant values don't have a register
    // allocated, and no instructions are generated for them.
    PRATT_EXPR_TYPE_CONSTANT,
    // This expression type is used for expressions that are stored in temporary registers.
    PRATT_EXPR_TYPE_REG,
    // This expression type is used for variables that are previously used in the current scope.
    // While it also stores a register ID, it is different from PRATT_EXPR_TYPE_REG in that the register
    // is not a temporary register.
    PRATT_EXPR_TYPE_VAR,
    // This expression type is used for type. No register is allocated for it.
    PRATT_EXPR_TYPE_TYPE,
} PrattExprType;

typedef union PrattExprValue {
    Value constant;
    TypeId type;
    LocalRegisterId reg;
} PrattExprValue;

#define PRATT_EXPR_UNSET()             \
    ((PrattExpr){                      \
        .type = PRATT_EXPR_TYPE_UNSET, \
    })
#define PRATT_EXPR_CONSTANT(val)           \
    ((PrattExpr){                          \
        .type  = PRATT_EXPR_TYPE_CONSTANT, \
        .value = ((PrattExprValue){        \
            .constant = (val),             \
        }),                                \
    })
#define PRATT_EXPR_REG(regId)         \
    ((PrattExpr){                     \
        .type  = PRATT_EXPR_TYPE_REG, \
        .value = ((PrattExprValue){   \
            .reg = (regId),           \
        }),                           \
    })
#define PRATT_EXPR_VAR(regId)         \
    ((PrattExpr){                     \
        .type  = PRATT_EXPR_TYPE_VAR, \
        .value = ((PrattExprValue){   \
            .reg = (regId),           \
        }),                           \
    })
#define PRATT_EXPR_TYPE(typeId)        \
    ((PrattExpr){                      \
        .type  = PRATT_EXPR_TYPE_TYPE, \
        .value = ((PrattExprValue){    \
            .type = (typeId),          \
        }),                            \
    })

// PrattExpr is a tagged union storing the result of pratt parsing that would be later used by its
// parent expression. Main ideas behind this design:
//
//   1. Constant folding: If the expression is a constant, we can store the value directly in order
//      to defer the "LOAD CONSTANT" instruction until we really need to. And we
//      can compute its value at compile time.
//   2. Inline values: T-type and K-type expressions have flags to enable inline values. Parent
//      expressions can determine if the value can be inline or not. For instance,
//      we return IntValue `1` to support generating `a + 1` in one instruction in
//      the `binaryLed` function.
//   3. Lazily dereferencing: For index and field expressions, we need to defer dereferencing until
//      we know it is for assignment or not.
//
// Note that an expression must give a consistent result even in branching expressions (e.g. `x or
// y`). That, however, doesn't mean the same operator will always return the same type. For example,
// if the expression is `true or result`, we returns `result` directly without dereferencing it to a
// register. But if the expression is `err or result`, we need to return a register id, whereas the
// actual value is only known at runtime.
typedef struct PrattExpr {
    PrattExprType type;
    PrattExprValue value;
} PrattExpr;

// PrattState is used to track the state of the Pratt compiler as we parse
// tokens from left to right.
//
// One important field is `targetRegister` that is only used when parsing the right hand side of an
// assignment. It allows us to specify a register that the child expression will use, and save one extra `MOVE`
// instruction.
//
// Here is an example:
//
//     a = b + c
//
// When parsing `b + c`, we know the result will be assigned to `a`, so `state.targetRegister` will be
// the register for `a`. Therefore, when parsing `b + c`, we can directly emit:
//
//     ADD R_a, R_b, R_c
//
// Instead of:
//
//     ADD R_tmp, R_b, R_c
//     MOVE R_a, R_tmp
typedef struct PrattState {
    Precedence rightBindingPower;
    LocalRegisterId targetRegister;
} PrattState;

typedef enum {
    LHS_EXPR_TYPE_UNASSIGNABLE,
    LHS_EXPR_TYPE_UNINIT_VAR,
    LHS_EXPR_TYPE_MODULE_VAR,
    LHS_EXPR_TYPE_GLOBAL_VAR,
    LHS_EXPR_TYPE_VAR,
    LHS_EXPR_TYPE_UPVALUE,
    LHS_EXPR_TYPE_FIELD,
    LHS_EXPR_TYPE_INDEX,
} LhsExprType;

#define LHS_EXPR_UNASSIGNABLE(_rvalue)              \
    ((LhsExpr){                                     \
        .type         = LHS_EXPR_TYPE_UNASSIGNABLE, \
        .value.rvalue = (_rvalue),                  \
    })
#define LHS_EXPR_UNINIT_VAR(_identifierId)              \
    ((LhsExpr){                                         \
        .type               = LHS_EXPR_TYPE_UNINIT_VAR, \
        .value.identifierId = (_identifierId),          \
    })
#define LHS_EXPR_MODULE_VAR(_moduleVarId, _isExport)          \
    ((LhsExpr){                                               \
        .type                     = LHS_EXPR_TYPE_MODULE_VAR, \
        .value.moduleVar.id       = (_moduleVarId),           \
        .value.moduleVar.isExport = (_isExport),              \
    })
#define LHS_EXPR_GLOBAL_VAR(_registerId)          \
    ((LhsExpr){                                   \
        .type         = LHS_EXPR_TYPE_GLOBAL_VAR, \
        .baseRegister = (_registerId),            \
    })
#define LHS_EXPR_VAR(_registerId)          \
    ((LhsExpr){                            \
        .type         = LHS_EXPR_TYPE_VAR, \
        .baseRegister = (_registerId),     \
    })
#define LHS_EXPR_UPVALUE(_registerId)          \
    ((LhsExpr){                                \
        .type         = LHS_EXPR_TYPE_UPVALUE, \
        .baseRegister = (_registerId),         \
    })
#define LHS_EXPR_FIELD(_registerId, _fieldName)       \
    ((LhsExpr){                                       \
        .type                  = LHS_EXPR_TYPE_FIELD, \
        .baseRegister          = (_registerId),       \
        .value.field.fieldName = (_fieldName),        \
    })
#define LHS_EXPR_INDEX(_registerId, _operand, _operandInlined) \
    ((LhsExpr){                                                \
        .type                       = LHS_EXPR_TYPE_INDEX,     \
        .baseRegister               = (_registerId),           \
        .value.index.operand        = (_operand),              \
        .value.index.operandInlined = (_operandInlined),       \
    })

typedef struct LhsExpr {
    LhsExprType type;
    LocalRegisterId baseRegister;
    union {
        PrattExpr rvalue;
        IdentifierId identifierId;
        struct {
            ModuleVariableId id;
            bool isExport;
        } moduleVar;
        struct {
            IdentifierId fieldName;
        } field;
        struct {
            uint8_t operand;
            bool operandInlined;
        } index;
    } value;
} LhsExpr;

typedef enum {
    BLOCK_SCOPE_TYPE_NORMAL,
    BLOCK_SCOPE_TYPE_LOOP,
    BLOCK_SCOPE_TYPE_IF,
    BLOCK_SCOPE_TYPE_FN,
} BlockScopeType;

// BlockScope is the visibility boundary for variables defined inside the scope.
//
// ```text
// var1 := 1
// var4 := 4
// var5 := 5
// fn function(arg) {        ─┐
//   var1 = -1                │
//   var2 := 2                │
//   if xxx {     ─┐          │ function block
//     var2 = -2   │ if block │ scope
//     var3 := 3   │ scope    │
//   }            ─┘          │
//   print(var5)              │
// }                         ─┘
// ```
//
// Since scopes have stack semantics, we only need to store the start and end indices of the variables that are visible
// in the scope. `[variableStackStart, variableStackEnd)` is the range of indices in the Compiler's `variables` array
// that are visible in the scope.
typedef struct BlockScope {
    // The parent block scope. `NULL` if this is the root block scope of a function scope.
    struct BlockScope* parent;

    // The index of the first variable in `variables` that is visible in this block scope. This has nothing to do with
    // the IDs of the variables in the scope.
    uint16_t variableStackStart;
    // The index of the first variable in `variables` that is NOT visible in this block scope. This has nothing to do
    // with the IDs of the variables in the scope.
    uint16_t variableStackEnd;

    // The union tag specifying the type of this block scope.
    BlockScopeType type;

    // Track if the last control-flow statement in this block is terminal (guarantees return on all paths).
    // UINT8_MAX = no terminal statement yet or not terminal
    // 0-254 = terminal statement with this coarity
    uint8_t terminalCoarity;
} BlockScope;

typedef struct LoopScope {
    BlockScope base;

    // This is the location of the instruction where all of the `continue` statements in the loop body will jump to.
    PCLocation loopStartLocation;

    // All `break` statements in the loop body will jump to the end of the loop, whose location is not known at the
    // time of parsing the `break` statement. To avoid scanning the entire instructions to patch the jump location,
    // we reuse the placeholder jump instructions for `break` to maintain a linked list of locations we will patch. When
    // the compiler meets the first `break`, it emits JUMP(INVALID_PC_LOCATION) and sets `previousJumpLocation` to this
    // locaiton; the next `break` will emit JUMP(previousJumpLocation) and update `previousJumpLocation` to this
    // location; and so on. When the compiler reaches the end of the loop, it traverses the linked list all the way
    // until the location is `INVALID_PC_LOCATION`.
    PCLocation previousJumpLocation;

    // TODO: add `hasUpvalue`. If it is false, we can remove the `CLOSE_UPVALUE` instructions at the end of the loop.
} LoopScope;

typedef struct IfScope {
    BlockScope base;

    // TODO: add `hasUpvalue`. If it is false, we can remove the `CLOSE_UPVALUE` instructions at the end of the block.
} IfScope;

DECLARE_DARRAY(UpvalueList, UpvalueDescription, uint8_t)

typedef struct FunctionScope {
    BlockScope rootBlock;

    // The chunk storing the emitted code for this function scope.
    Chunk chunk;

    // The current / innermost block scope. Never `NULL`. When not in any nested block scope, it points to
    // `&rootBlock`.
    BlockScope* currentBlock;

    // The parent function scope. `NULL` if this is the root function scope.
    struct FunctionScope* parent;

    UpvalueList upvalues;

    // The next available register ID. Valid register IDs are in the range `[0, MAX_LOCAL_REGISTER_ID]`.
    //
    // Register allocation has stack semantics. When we request a new register, it returns the current `nextRegisterId`
    // and increments it; when we release registers, we specify the new `nextRegisterId`, meaning that every register
    // after it is released. This approach limits the way we allocate registers for variables. Since we never spill
    // variable registers, we must ensure variables are allocated before temp registers. Inside any (nested) block
    // scope, registers are always allocated in the following way:
    // ```
    //                                                             nextRegisterId ─┐
    //                                                                             │
    //      level1             level1             level2             level2        │
    //     variables       temp registers        variables       temp registers    │
    // ┌───────────────┐┌───────────────────┐┌───────────────┐┌─────────────────── v
    //
    // ```
    //
    // Note that temp registers are not just for temporary values in expressions. For example, a for-loop needs at least
    // 2 temp registers for the iterator and the current value.
    LocalRegisterId nextRegisterId;

    // The maximum number of registers we used for the function. For example, if we have used register 0, 1, 4, this
    // should be `5`.
    uint8_t maxUsedRegisterCount;
    // The number of return values. `UINT8_MAX` means it's not set yet.
    uint8_t nReturns;

    bool isDeferredFunction;
} FunctionScope;

typedef struct VariableDescription {
    IdentifierId identifierId;
    LocalRegisterId registerId;
} VariableDescription;

DECLARE_DARRAY(VariableList, VariableDescription, uint16_t)

typedef struct ErrorJmpBuf {
    jmp_buf env;
    ErrorId errorId;

#ifdef SEMI_DEBUG_MSG
    // TODO: Currently all messages are constants so we don't need to free them.
    const char* message;
#endif
} ErrorJmpBuf;

typedef struct Compiler {
    Lexer lexer;

    // These are weak references to the VM's corresponding fields.
    GC* gc;
    ClassTable* classes;
    SymbolTable* symbolTable;
    GlobalIdentifierList* globalIdentifiers;

    // The artifact module that is being compiled. This is not owned by the compiler.
    SemiModule* artifactModule;

    FunctionScope rootFunction;
    // The current, innermost function scope. Never `NULL`. If it's currnetly not in a function
    // scope (i.e. it's in the module scope), it points to `rootFunction`.
    FunctionScope* currentFunction;

    VariableList variables;

    uint32_t newlineState;

    ErrorJmpBuf errorJmpBuf;
} Compiler;

void semiCompilerInit(Compiler* compiler);
void semiCompilerCleanup(Compiler* compiler);
void semiParseExpression(Compiler* compiler, const PrattState state, PrattExpr* expr);
void semiParseStatement(Compiler* compiler);
void semiCompilerCompileModule(Compiler* compiler, SemiModuleSource* moduleSource, SemiModule* target);

SemiModule* semiVMCompileModule(SemiVM* vm, SemiModuleSource* moduleSource);

#ifdef SEMI_TEST

void semiTextInitLexer(struct Lexer* lexer, Compiler* compiler, const char* source, uint32_t length);
Token semiTestNextToken(struct Lexer* lexer);
Token semiTestPeekToken(struct Lexer* lexer);

#endif /* SEMI_TEST */

#endif /* SEMI_COMPILER_H */
