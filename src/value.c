// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./value.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#include "./gc.h"
#include "./primitives.h"
#include "./semi_common.h"
#include "./utf8.h"

static inline Object* newObject(GC* gc, ObjectType type, size_t size) {
    Object* s = (Object*)semiMalloc(gc, size);
    if (!s) {
        return NULL;  // Allocation failed
    }
    s->header = (ObjectType)type;
    semiGCAttachObject(gc, s);
    return s;
}

ValueHash SemiHashNumber(uint64_t key) {
    // MurmurHash3's 64-bit finalizer
    key ^= (key >> 33);
    key *= 0xff51afd7ed558ccd;
    key ^= (key >> 33);
    key *= 0xc4ceb9fe1a85ec53;
    key ^= (key >> 33);

    return (ValueHash)key;
}

/*
 │ InlineString & ObjectString
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

ObjectString* semiObjectStringCreateUninit(GC* gc, size_t length) {
    ObjectString* o = (ObjectString*)newObject(gc, OBJECT_TYPE_STRING, sizeof(ObjectString) + length);
    if (!o) {
        return NULL;  // Allocation failed
    }
    o->length = length;
    return o;
}

ObjectString* semiObjectStringCreate(GC* gc, const char* text, size_t length) {
    ObjectString* o = (ObjectString*)newObject(gc, OBJECT_TYPE_STRING, sizeof(ObjectString) + length);
    if (!o) {
        return NULL;  // Allocation failed
    }

    o->length = length;
    memcpy((void*)(o->str), text, length);
    o->hash = semiHashString(text, length);
    return o;
}

Value semiValueStringCreate(GC* gc, const char* text, size_t length) {
    if (length == 0) {
        return semiValueInlineStringCreate0();
    } else if (length == 1 && IS_VALID_INLINE_CHAR(text[0])) {
        return semiValueInlineStringCreat1(text[0]);
    } else if (length == 2 && IS_VALID_INLINE_CHAR(text[0]) && IS_VALID_INLINE_CHAR(text[1])) {
        return semiValueInlineStringCreate2(text[0], text[1]);
    } else {
        ObjectString* o = semiObjectStringCreate(gc, text, length);
        return o ? (Value){.header = VALUE_TYPE_OBJECT_STRING, .as.obj = (Object*)o} : INVALID_VALUE;
    }
}

#pragma endregion

/*
 │ InlineRange & ObjectRange
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

ObjectRange* semiObjectRangeCreate(GC* gc, Value start, Value end, Value step) {
    ObjectRange* o = (ObjectRange*)newObject(gc, OBJECT_TYPE_RANGE, sizeof(ObjectRange));
    if (!o) {
        return NULL;  // Allocation failed
    }

    o->start = start;
    o->end   = end;
    o->step  = step;
    return o;
}

Value semiValueRangeCreate(GC* gc, Value start, Value end, Value step) {
    bool stepIsOne    = (IS_INT(&step) && AS_INT(&step) == 1);
    bool satrtIsInt32 = IS_INT(&start) && AS_INT(&start) >= INT32_MIN && AS_INT(&start) <= INT32_MAX;
    bool endIsInt32   = IS_INT(&end) && AS_INT(&end) >= INT32_MIN && AS_INT(&end) <= INT32_MAX;

    if (stepIsOne && satrtIsInt32 && endIsInt32) {
        return semiValueInlineRangeCreate((int32_t)start.as.i, (int32_t)end.as.i);
    } else {
        ObjectRange* o = semiObjectRangeCreate(gc, start, end, step);
        return o ? (Value){.header = VALUE_TYPE_OBJECT_RANGE, .as.obj = (Object*)o} : INVALID_VALUE;
    }
}

#pragma endregion

/*
 │ ObjectList
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

#define LIST_GROW_FACTOR 2

ObjectList* semiObjectListCreate(GC* gc, uint32_t capacity) {
    ObjectList* o = (ObjectList*)newObject(gc, OBJECT_TYPE_LIST, sizeof(ObjectList));
    if (!o) {
        return NULL;  // Allocation failed
    }

    o->values   = (Value*)semiMalloc(gc, sizeof(Value) * capacity);
    o->size     = 0;
    o->capacity = capacity;
    return o;
}

void semiListEnsureCapacity(GC* gc, ObjectList* list, uint32_t capacity) {
    if (list->capacity < capacity) {
        uint32_t newCapacity = nextPowerOfTwoCapacity(capacity);
        list->values =
            (Value*)semiRealloc(gc, list->values, sizeof(Value) * list->capacity, sizeof(Value) * newCapacity);
        list->capacity = newCapacity;
    }
}

void semiListAppend(GC* gc, ObjectList* list, Value value) {
    if (list->size == list->capacity) {
        semiListEnsureCapacity(gc, list, list->capacity + 1);
    }

    list->values[list->size++] = value;
}

void semiListInsert(GC* gc, ObjectList* list, uint32_t index, Value value) {
    if (list->size == list->capacity) {
        semiListEnsureCapacity(gc, list, list->capacity + 1);
    }

    if (index >= list->size) {
        list->values[list->size++] = value;
    } else {
        // Shift elements to the right
        for (uint32_t i = list->size; i > index; --i) {
            list->values[i] = list->values[i - 1];
        }
        list->values[index] = value;
        list->size++;
    }
}

void semiListShrink(GC* gc, ObjectList* list) {
    uint32_t newCapacity = nextPowerOfTwoCapacity(list->capacity / LIST_GROW_FACTOR);
    if (newCapacity >= list->size) {
        list->values =
            (Value*)semiRealloc(gc, list->values, sizeof(Value) * list->capacity, sizeof(Value) * newCapacity);
        list->capacity = newCapacity;
    }
}

IntValue semiListRemove(GC* gc, ObjectList* list, Value value) {
    for (uint32_t i = 0; i < list->size; i++) {
        if (semiBuiltInEquals(list->values[i], value)) {
            for (uint32_t j = i; j < list->size - 1; j++) {
                list->values[j] = list->values[j + 1];
            }
            list->size--;

            semiListShrink(gc, list);
            return i;
        }
    }
    return -1;
}

bool semiListPop(GC* gc, ObjectList* list) {
    if (list->size == 0) {
        return false;
    }

    list->size--;
    semiListShrink(gc, list);
    return true;
}

bool semiListHas(GC* gc, ObjectList* list, Value value) {
    (void)gc;
    for (uint32_t i = 0; i < list->size; i++) {
        if (semiBuiltInEquals(list->values[i], value)) {
            return true;
        }
    }
    return false;
}

IntValue semiListIndex(GC* gc, ObjectList* list, Value value) {
    (void)gc;
    for (uint32_t i = 0; i < list->size; i++) {
        if (semiBuiltInEquals(list->values[i], value)) {
            return i;
        }
    }
    return -1;
}

#pragma endregion

/*
 │ ObjectDict
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

ObjectDict* semiObjectDictCreate(GC* gc) {
    ObjectDict* dict = (ObjectDict*)newObject(gc, OBJECT_TYPE_DICT, sizeof(ObjectDict));
    if (!dict) {
        return NULL;  // Allocation failed
    }

    dict->keyCmpFn  = semiBuiltInEquals;
    dict->keys      = NULL;
    dict->tids      = NULL;
    dict->values    = NULL;
    dict->indexSize = 0;
    dict->used      = 0;
    dict->len       = 0;
    return dict;
}

void semiObjectStackDictInit(ObjectDict* dict) {
    dict->keyCmpFn  = semiBuiltInEquals;
    dict->keys      = NULL;
    dict->tids      = NULL;
    dict->values    = NULL;
    dict->indexSize = 0;
    dict->used      = 0;
    dict->len       = 0;
}

void semiObjectStackDictCleanup(GC* gc, ObjectDict* dict) {
    semiFree(gc, dict->keys, sizeof(ObjectDictKey) * OBJECT_DICT_MAX_INDEX_LOAD(dict->indexSize));
    semiFree(gc, dict->tids, sizeof(TupleId) * dict->indexSize);
    semiFree(gc, dict->values, sizeof(Value) * OBJECT_DICT_MAX_INDEX_LOAD(dict->indexSize));

    dict->keys      = NULL;
    dict->tids      = NULL;
    dict->values    = NULL;
    dict->indexSize = 0;
    dict->used      = 0;
    dict->len       = 0;
}

// Return the index of an empty slot in the `tid` table for insertion, assuming that the dict
// has been resized, and that the hash does not exist in the dict.
static inline TupleId dictFindEmptyIndex(ObjectDict* dict, ValueHash hash) {
    uint64_t perturb = hash;
    uint32_t mask    = dict->indexSize - 1;
    uint32_t index   = hash & mask;

    while (true) {
        if (dict->tids[index] < 0) {
            return index;  // Found an empty slot
        }

        // Probing the next slot index with linear congruential generator
        perturb >>= 5;
        index = (uint32_t)(perturb + 1 + index * 5) & mask;
    }

    SEMI_UNREACHABLE();
    return -1;
}

// Return the Tuple ID of the key.
TupleId semiDictFindTupleId(ObjectDict* dict, Value key, ValueHash hash) {
    if (dict->keys == NULL) {
        return -1;  // Empty dictionary
    }

    uint64_t perturb = hash;
    uint32_t mask    = dict->indexSize - 1;
    uint32_t index   = hash & mask;

    while (true) {
        TupleId tid = dict->tids[index];
        if (tid == OBJECT_DICT_KEY_EMPTY) {
            return -1;  // Not found
        }
        if (tid >= 0) {
            if (dict->keys[tid].hash == hash && dict->keyCmpFn(dict->keys[tid].key, key)) {
                return tid;  // Found
            }
        }

        // Probing the next slot index with linear congruential generator
        perturb >>= 5;
        index = (uint32_t)(perturb + 1 + index * 5) & mask;
    }

    SEMI_UNREACHABLE();
    return -1;
}

static bool dictResize(GC* gc, ObjectDict* dict) {
    uint32_t indexSize = nextPowerOfTwoCapacity(dict->len * 3);
    if (indexSize <= OBJECT_DICT_MIN_INDEX_SIZE) {
        indexSize = OBJECT_DICT_MIN_INDEX_SIZE;
    }
    if (indexSize == dict->indexSize) {
        return true;
    }

    uint32_t oldTupleTableSize = OBJECT_DICT_MAX_INDEX_LOAD(dict->indexSize);
    uint32_t newTupleTableSize = OBJECT_DICT_MAX_INDEX_LOAD(indexSize);

    TupleId* newTIDs =
        (TupleId*)semiRealloc(gc, dict->tids, sizeof(TupleId) * dict->indexSize, sizeof(TupleId) * indexSize);
    if (!newTIDs) {
        return false;  // Allocation failed
    }

    // Initialize new tids array to empty
    for (uint32_t i = 0; i < indexSize; i++) {
        newTIDs[i] = OBJECT_DICT_KEY_EMPTY;
    }

    ObjectDict tempDict = {.tids = newTIDs, .indexSize = indexSize};

    uint32_t toBeFilled = 0;
    for (uint32_t curr = 0; curr < oldTupleTableSize; curr += 1) {
        if (IS_INVALID(&dict->keys[curr].key)) {
            continue;
        }

        ValueHash hash      = dict->keys[curr].hash;
        TupleId emptyIndex  = dictFindEmptyIndex(&tempDict, hash);
        newTIDs[emptyIndex] = toBeFilled;

        dict->keys[toBeFilled]   = dict->keys[curr];
        dict->values[toBeFilled] = dict->values[curr];
        toBeFilled += 1;
    }

    ObjectDictKey* newKeys = (ObjectDictKey*)semiRealloc(
        gc, dict->keys, sizeof(ObjectDictKey) * oldTupleTableSize, sizeof(ObjectDictKey) * newTupleTableSize);
    Value* newValues =
        (Value*)semiRealloc(gc, dict->values, sizeof(Value) * oldTupleTableSize, sizeof(Value) * newTupleTableSize);
    if (!newKeys || !newValues) {
        return false;
    }

    dict->tids      = newTIDs;
    dict->keys      = newKeys;
    dict->values    = newValues;
    dict->indexSize = indexSize;
    dict->used      = dict->len;
    return true;
}

bool semiDictHas(ObjectDict* dict, Value key) {
    if (dict->keys == NULL) {
        return false;  // Empty dictionary
    }

    ValueHash hash = semiBuiltInHash(key);
    TupleId tid    = semiDictFindTupleId(dict, key, hash);
    return tid >= 0;
}

bool semiDictHasWithHash(ObjectDict* dict, Value key, ValueHash hash) {
    if (dict->keys == NULL) {
        return false;  // Empty dictionary
    }

    TupleId tid = semiDictFindTupleId(dict, key, hash);
    return tid >= 0;
}

Value semiDictGet(ObjectDict* dict, Value key) {
    if (dict->keys == NULL) {
        return INVALID_VALUE;  // Empty dictionary
    }

    ValueHash hash = semiBuiltInHash(key);
    TupleId tid    = semiDictFindTupleId(dict, key, hash);
    if (tid < 0) {
        return INVALID_VALUE;  // Not found
    }

    return dict->values[tid];
}

Value semiDictGetWithHash(ObjectDict* dict, Value key, ValueHash hash) {
    TupleId tid = semiDictFindTupleId(dict, key, hash);
    if (tid < 0) {
        return INVALID_VALUE;  // Not found
    }

    return dict->values[tid];
}

bool semiDictSetWithHash(GC* gc, ObjectDict* dict, Value key, Value value, ValueHash hash) {
    if (dict->keys == NULL) {
        dict->indexSize         = OBJECT_DICT_MIN_INDEX_SIZE;
        uint32_t tupleTableSize = OBJECT_DICT_MAX_INDEX_LOAD(OBJECT_DICT_MIN_INDEX_SIZE);
        dict->tids              = (TupleId*)semiMalloc(gc, sizeof(TupleId) * dict->indexSize);
        dict->keys              = (ObjectDictKey*)semiMalloc(gc, sizeof(ObjectDictKey) * tupleTableSize);
        dict->values            = (Value*)semiMalloc(gc, sizeof(Value) * tupleTableSize);
        if (!dict->keys || !dict->tids || !dict->values) {
            return false;  // Allocation failed
        }

        // Initialize tids array to empty
        for (uint32_t i = 0; i < dict->indexSize; i++) {
            dict->tids[i] = OBJECT_DICT_KEY_EMPTY;
        }
    }

    TupleId tid = semiDictFindTupleId(dict, key, hash);
    if (tid >= 0) {
        dict->values[tid] = value;
        return true;
    }

    if (dict->used >= OBJECT_DICT_MAX_INDEX_LOAD(dict->indexSize)) {
        dictResize(gc, dict);
    }

    TupleId emptyIndex = dictFindEmptyIndex(dict, hash);
    if (emptyIndex < 0) {
        return false;  // No free slot found
    }
    dict->tids[emptyIndex] = dict->used;
    dict->keys[dict->used] = (ObjectDictKey){
        .hash = hash,
        .key  = key,
    };
    dict->values[dict->used] = value;
    dict->used++;
    dict->len++;
    return true;
}

bool semiDictSet(GC* gc, ObjectDict* dict, Value key, Value value) {
    ValueHash hash = semiBuiltInHash(key);
    return semiDictSetWithHash(gc, dict, key, value, hash);
}

Value semiDictDelete(GC* gc, ObjectDict* dict, Value key) {
    if (dict->keys == NULL) {
        return INVALID_VALUE;  // Empty dictionary
    }

    ValueHash hash = semiBuiltInHash(key);
    TupleId tid    = semiDictFindTupleId(dict, key, hash);
    if (tid < 0) {
        return INVALID_VALUE;  // Not found
    }

    Value deletedValue = dict->values[tid];

    dict->tids[tid]            = OBJECT_DICT_KEY_TOMBSTONE;
    dict->keys[tid].key.header = VALUE_TYPE_INVALID;
    dict->values[tid].header   = VALUE_TYPE_INVALID;
    dict->len--;

    // After resizing, `used` = `len` and we want to make sure `used <<< newIndexSize * 2/3`.
    if (dict->indexSize > OBJECT_DICT_MIN_INDEX_SIZE && dict->len < dict->indexSize / 8) {
        dictResize(gc, dict);
    }

    return deletedValue;
}

#pragma endregion

/*
 │ Native Function
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

Value semiValueNativeFunctionCreate(NativeFunction* function) {
    return (Value){.header = VALUE_TYPE_NATIVE_FUNCTION, .as.ptr = (void*)function};
}

#pragma endregion

/*
 │ Function Proto
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

FunctionProto* semiFunctionProtoCreate(GC* gc, uint8_t upvalueCount) {
    FunctionProto* o =
        (FunctionProto*)semiMalloc(gc, sizeof(FunctionProto) + sizeof(UpvalueDescription) * upvalueCount);
    if (!o) {
        return NULL;  // Allocation failed
    }

    o->arity        = 0;
    o->coarity      = 0;
    o->maxStackSize = 0;
    o->upvalueCount = upvalueCount;
    ChunkInit(&o->chunk);
    memset(o->upvalues, 0, sizeof(UpvalueDescription) * upvalueCount);
    return o;
}
#pragma endregion

void semiFunctionProtoDestroy(GC* gc, FunctionProto* function) {
    ChunkCleanup(gc, &function->chunk);
    semiFree(gc, function, sizeof(FunctionProto) + sizeof(UpvalueDescription) * function->upvalueCount);
}

Value semiValueFunctionProtoCreate(FunctionProto* function) {
    return semiValuePtrCreate(function, VALUE_TYPE_FUNCTION_PROTO);
}

/*
 │ Object Upvalue
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

ObjectUpvalue* semiObjectUpvalueCreate(GC* gc, Value* value) {
    ObjectUpvalue* o = (ObjectUpvalue*)newObject(gc, OBJECT_TYPE_UPVALUE, sizeof(ObjectUpvalue));
    if (!o) {
        return NULL;  // Allocation failed
    }

    o->value = value;
    return o;
}

#pragma endregion

/*
 │ ObjectFunction
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

ObjectFunction* semiObjectFunctionCreate(GC* gc, FunctionProto* proto) {
    ObjectFunction* o = (ObjectFunction*)newObject(
        gc, OBJECT_TYPE_FUNCTION, sizeof(ObjectFunction) + sizeof(ObjectUpvalue*) * proto->upvalueCount);
    if (!o) {
        return NULL;  // Allocation failed
    }

    o->proto          = proto;
    o->prevDeferredFn = NULL;
    o->upvalueCount   = proto->upvalueCount;
    return o;
}

#pragma endregion
