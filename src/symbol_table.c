// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./gc.h"

typedef struct IdentifierEntry {
    InternedChar* str;
    IdentifierLength length;
} IdentifierEntry;

bool keyCompareFn(Value a, Value b) {
    IdentifierEntry* oa = AS_PTR(&a, IdentifierEntry);
    IdentifierEntry* ob = AS_PTR(&b, IdentifierEntry);

    return oa->length == ob->length && (memcmp(oa->str, ob->str, oa->length) == 0);
}

void semiSymbolTableInit(GC* gc, SymbolTable* table) {
    table->gc = gc;
    semiObjectStackDictInit(&table->identifierMap);
    table->identifierMap.keyCmpFn = keyCompareFn;

    table->nextId = MAX_RESERVED_IDENTIFIER_ID + 1;
}

void semiSymbolTableCleanup(SymbolTable* table) {
    GC* gc = table->gc;

    // We have to manully free memory allocated via `semiMalloc` because we hide an extra header
    // before the entry pointer. The rest of the memory will be cleaned up by the GC.
    for (uint32_t i = 0; i < table->identifierMap.len; i++) {
        IdentifierEntry* entry = AS_PTR(&table->identifierMap.keys[i].key, IdentifierEntry);
        semiFree(gc,
                 (char*)(entry->str) - sizeof(IdentifierLength) - sizeof(IdentifierId),
                 entry->length + sizeof(IdentifierLength) + sizeof(IdentifierId));
        semiFree(gc, entry, sizeof(IdentifierEntry));
    }

    semiObjectStackDictCleanup(gc, &table->identifierMap);
}

InternedChar* semiSymbolTableInsert(struct SymbolTable* table,
                                    const char* identifier,
                                    IdentifierLength identifierLength) {
    if (identifier == NULL || identifierLength == 0 || table->nextId == UINT32_MAX) {
        return NULL;
    }

    ValueHash hash             = semiHashString(identifier, identifierLength);
    IdentifierEntry stackEntry = (IdentifierEntry){
        .str    = (InternedChar*)identifier,
        .length = identifierLength,
    };
    Value stackEntryValue = semiValueNewPtr((void*)&stackEntry, VALUE_TYPE_UNSET);

    Value existing = semiDictGetWithHash(&table->identifierMap, stackEntryValue, hash);
    if (IS_VALID(&existing)) {
        // String already exists, return the existing interned string
        return AS_PTR(&existing, IdentifierEntry)->str;
    }

    IdentifierEntry* newEntry = semiMalloc(table->gc, sizeof(IdentifierEntry));
    if (newEntry == NULL) {
        return NULL;  // Memory allocation failed
    }

    char* data = semiMalloc(table->gc, sizeof(IdentifierLength) + sizeof(IdentifierId) + identifierLength);
    if (data == NULL) {
        semiFree(table->gc, newEntry, sizeof(IdentifierEntry));
        return NULL;  // Memory allocation failed
    }

    memcpy(data, &identifierLength, sizeof(IdentifierLength));

    IdentifierId id = table->nextId++;
    memcpy(data + sizeof(IdentifierLength), &id, sizeof(IdentifierId));

    InternedChar* strData = (InternedChar*)(data + sizeof(IdentifierLength) + sizeof(IdentifierId));
    memcpy(strData, identifier, identifierLength);

    newEntry->str    = strData;
    newEntry->length = identifierLength;

    Value entryValue = semiValueNewPtr((void*)newEntry, VALUE_TYPE_UNSET);
    semiDictSetWithHash(table->gc, &table->identifierMap, entryValue, entryValue, hash);

    return strData;
}

InternedChar* semiSymbolTableGet(struct SymbolTable* table, const char* str, IdentifierLength length) {
    if (str == NULL || length == 0) {
        return NULL;
    }

    ValueHash hash             = semiHashString(str, length);
    IdentifierEntry stackEntry = (IdentifierEntry){
        .str    = (InternedChar*)str,
        .length = length,
    };
    Value stackEntryValue = semiValueNewPtr((void*)&stackEntry, VALUE_TYPE_UNSET);
    Value existing        = semiDictGetWithHash(&table->identifierMap, stackEntryValue, hash);
    if (IS_VALID(&existing)) {
        // String already exists, return the existing interned string
        return AS_PTR(&existing, IdentifierEntry)->str;
    }

    return NULL;
}

inline IdentifierId semiSymbolTableGetId(const InternedChar* str) {
    IdentifierId id;
    memcpy(&id, (char*)str - sizeof(IdentifierId), sizeof(IdentifierId));
    return id;
}

inline IdentifierLength semiSymbolTableLength(const InternedChar* str) {
    IdentifierLength length;
    memcpy(&length, (const char*)str - sizeof(IdentifierId) - sizeof(IdentifierLength), sizeof(IdentifierLength));
    return length;
}
