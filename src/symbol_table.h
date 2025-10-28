// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_symbolTable_H
#define SEMI_symbolTable_H

#include <stddef.h>

#include "./gc.h"
#include "./value.h"
#include "semi/config.h"

typedef uint8_t IdentifierLength;

typedef uint32_t IdentifierId;

#define MAX_RESERVED_IDENTIFIER_ID 256

// Memory layout for interned strings (InternedChar*):
//
//                            InternedChar* points here
//                                       │
//                                       v
// ┌─────────────────┐┌─────────────────┐┌─────────────────────────────────┐
// │ IdentifierLength││   IdentifierId  ││         String Data             │
// │    (uint8_t)    ││    (uint32_t)   ││        (InternedChar*)          │
// │     1 byte      ││     4 bytes     ││        variable length          │
// └─────────────────┘└─────────────────┘└─────────────────────────────────┘
//
typedef char InternedChar;

typedef struct SymbolTable {
    GC* gc;

    ObjectDict identifierMap;
    IdentifierId nextId;
} SymbolTable;

// Initialize the string table
void semiSymbolTableInit(GC* gc, SymbolTable* table);

// Free the string table and all its resources
void semiSymbolTableCleanup(SymbolTable* table);

// Check if a string is in the symbol table. Returned the interned string if it exists.
// Return NULL if the string is not found.
InternedChar* semiSymbolTableGet(struct SymbolTable* table, const char* str, IdentifierLength length);

// Insert an identifier into the symbol table. We compute a hash to
// So symbol comparison can be done with pointer comparison.
InternedChar* semiSymbolTableInsert(struct SymbolTable* table, const char* str, IdentifierLength length);

// Get the length of a string.
IdentifierLength semiSymbolTableLength(const InternedChar* str);

// Get the identifier ID of an interned string.
IdentifierId semiSymbolTableGetId(const InternedChar* str);

#endif /* SEMI_symbolTable_H */
