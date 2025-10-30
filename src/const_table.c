// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./const_table.h"

#include <stdlib.h>
#include <string.h>

#include "./value.h"

void semiConstantTableInit(GC* gc, ConstantTable* table) {
    table->gc          = gc;
    table->constantMap = semiObjectDictCreate(gc);
}

void semiConstantTableCleanup(ConstantTable* table) {
    (void)table;

    for (uint32_t i = 0; i < semiDictLen(table->constantMap); i++) {
        Value key = table->constantMap->keys[i].key;
        switch (VALUE_TYPE(&key)) {
            case VALUE_TYPE_FUNCTION_PROTO: {
                FunctionProto* fnProto = (FunctionProto*)AS_PTR(&key, FunctionProto);
                semiFunctionProtoDestroy(table->gc, fnProto);
                break;
            }

            default:
                break;
        }
    }

    // The dictionary will be cleaned up by the GC.
}

ConstantIndex semiConstantTableInsert(ConstantTable* table, Value key) {
    Value existingValue = semiDictGet(table->constantMap, key);
    if (IS_VALID(&existingValue)) {
        return (ConstantIndex)AS_INT(&existingValue);
    }

    ConstantIndex index = (ConstantIndex)(semiDictLen(table->constantMap));
    Value indexValue    = semiValueNewInt(index);
    return semiDictSet(table->gc, table->constantMap, key, indexValue) ? index : CONST_INDEX_INVALID;
}

Value semiConstantTableGet(const ConstantTable* table, ConstantIndex index) {
    return index < semiDictLen(table->constantMap) ? table->constantMap->keys[index].key : INVALID_VALUE;
}

size_t semiConstantTableSize(const ConstantTable* table) {
    return semiDictLen(table->constantMap);
}
