// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_PRIMITIVES_H
#define SEMI_PRIMITIVES_H

#include "./gc.h"
#include "./primitive_x_macro.h"
#include "./symbol_table.h"
#include "./value.h"
#include "semi/error.h"

bool semiBuiltInEquals(Value a, Value b);
ValueHash semiBuiltInHash(const Value v);

/*
 │ Built-in Primitives
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct ClassTable {
    MagicMethodsTable* classMethods;
    uint16_t classCount;
    uint16_t classCapacity;
} ClassTable;

void semiPrimitivesFinalizeMagicMethodsTable(MagicMethodsTable* table);
void semiPrimitivesInitBuiltInModuleTypes(GC* gc, SymbolTable* symbolTable, SemiModule* module);
void semiPrimitivesIntializeBuiltInPrimitives(GC* gc, ClassTable* classes, SymbolTable* symbolTable);
void semiPrimitivesCleanupClassTable(GC* gc, ClassTable* classes);
ErrorId semiPrimitivesDispatchHash(MagicMethodsTable* table, GC* gc, ValueHash* ret, Value* a);
ErrorId semiPrimitivesDispatch1Operand(MagicMethodsTable* table, GC* gc, Opcode method, Value* ret, Value* a);
ErrorId semiPrimitivesDispatch2Operands(MagicMethodsTable* table, GC* gc, Opcode method, Value* a, Value* b, Value* c);
#endif /* SEMI_PRIMITIVES_H */
