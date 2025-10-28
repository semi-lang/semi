// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_TYPES_H
#define SEMI_TYPES_H

#include <stddef.h>
#include <stdint.h>

#include "./symbol_table.h"

//
// Structs
//

typedef struct StructField {
    InternedChar* name;
    struct StructType* type;
} StructField;

typedef struct StructType {
    InternedChar* name;

    StructField* fields;
    size_t fieldCount;
} StructType;

#endif /* SEMI_TYPES_H */
