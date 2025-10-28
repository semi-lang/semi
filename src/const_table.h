// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_CONST_TABLE_H
#define SEMI_CONST_TABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./gc.h"
#include "./value.h"

typedef uint32_t ConstantIndex;
#define CONST_INDEX_INVALID (~(ConstantIndex)0)

typedef struct ConstantTable {
    GC* gc;

    ObjectDict* constantMap;
} ConstantTable;

void semiConstantTableInit(GC* gc, ConstantTable* table);
void semiConstantTableCleanup(ConstantTable* tables);

ConstantIndex semiConstantTableInsert(ConstantTable* table, Value key);
Value semiConstantTableGet(const ConstantTable* tables, ConstantIndex index);
size_t semiConstantTableSize(const ConstantTable* tables);

#endif /* SEMI_CONST_TABLE_H */
