// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_VALUE_H
#define SEMI_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./darray.h"
#include "./gc.h"
#include "./instruction.h"
#include "./primitive_x_macro.h"
#include "semi/error.h"

DECLARE_DARRAY(Chunk, Instruction, uint32_t)

typedef uint16_t ModuleId;

#define SEMI_REPL_MODULE_ID   ((ModuleId)(0))
#define SEMI_SYSTEM_MODULE_ID ((ModuleId)(UINT16_MAX))

typedef uint16_t ModuleVariableId;

#define INVALID_MODULE_VARIABLE_ID ((ModuleVariableId)(UINT16_MAX))

typedef uint32_t PCLocation;

#define INVALID_PC_LOCATION ((PCLocation)(MAX_OPERAND_J))

typedef uint8_t UpvalueId;

#define INVALID_UPVALUE_ID ((UpvalueId)(UINT8_MAX))

// Valid regsister IDs are in the range [0, MAX_LOCAL_REGISTER_ID],
// both ends inclusive.
typedef uint8_t LocalRegisterId;

#define INVALID_LOCAL_REGISTER_ID ((LocalRegisterId)(UINT8_MAX))
#define MAX_LOCAL_REGISTER_ID     ((LocalRegisterId)(UINT8_MAX - 1))

/*=================================================================================================
  ValueHeader layout (32 bits):

    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   |U|O|     variant (14 bits)     |      base type (16 bits)      |

  Symbols:
    * base type: BaseValueType, which is also used to index the class table for operations like
                 hashing and equality checking.
    * variant:   Variant number, which differentiates between different representations of the
                 same base type.
    * O:         Object flag (1 = object, 0 = inline value).
    * U:         Unused bit (bit 31) - cannot be used since enum values must be < INT32_MAX.

  Notes:
    * The C standard requires that enums are compatible with int. This means Semi is not supported
      on platforms where int is less than 32 bits.
    * The object flag is redundant since we can infer it from the base type. This is also why currently `VALUE_TYPE(v)`
      can be computed without masking out the object flag. However, having it explicitly makes certain operations
      faster, such as checking if a value is an object or inline value.
      In the future, there might be more flags added to the header, and that will make it necessary to mask out those
      flag when computing the value type.
=================================================================================================*/

typedef uint32_t ValueHeader;

typedef uint64_t ValueHash;

typedef uint16_t TypeId;

// BaseValueType represents the base type of the value. It is used to determine the vtable for operations like hashing
// and equality checking.
typedef enum {
    BASE_VALUE_TYPE_INVALID = 0,
    BASE_VALUE_TYPE_BOOL,
    BASE_VALUE_TYPE_INT,
    BASE_VALUE_TYPE_FLOAT,
    BASE_VALUE_TYPE_STRING,
    BASE_VALUE_TYPE_RANGE,
    BASE_VALUE_TYPE_LIST,
    BASE_VALUE_TYPE_DICT,
    BASE_VALUE_TYPE_UPVALUE,
    BASE_VALUE_TYPE_FUNCTION,
    BASE_VALUE_TYPE_FUNCTION_PROTO,
    BASE_VALUE_TYPE_CLASS,
} BaseValueType;

#define SEMI_BUILTIN_CLASS_COUNT   (BASE_VALUE_TYPE_CLASS + 1)
#define MIN_CUSTOM_BASE_VALUE_TYPE (BASE_VALUE_TYPE_CLASS + 1)
#define MAX_CUSTOM_BASE_VALUE_TYPE ((1 << 16) - 1)

// Masks
#define VALUE_HEADER_BASE_TYPE_MASK 0x0000FFFF  // Bits 0-15
#define VALUE_HEADER_VARIANT_MASK   0x3FFF0000  // Bits 16-29
#define VALUE_HEADER_OBJECT_MASK    0x40000000  // Bit 30
#define VALUE_HEADER_VARIANT_SHIFT  16

// Value type answers the question "how can I interpret the data in the Value struct?"
// Multiple ValueTypes can map to the same BaseValueType.
//
// Note that the C standard requires that enums are compatible with int. This means Semi
// is not supported on platforms where int is less than 32 bits.
typedef enum {
    // Special sentinel value types
    VALUE_TYPE_INVALID = 0,
    VALUE_TYPE_UNSET   = 1,

    // Boolean
    VALUE_TYPE_BOOL = BASE_VALUE_TYPE_BOOL,

    // Int
    VALUE_TYPE_INT = BASE_VALUE_TYPE_INT,

    // Float
    VALUE_TYPE_FLOAT = BASE_VALUE_TYPE_FLOAT,

    // String
    VALUE_TYPE_INLINE_STRING = BASE_VALUE_TYPE_STRING,
    VALUE_TYPE_OBJECT_STRING = BASE_VALUE_TYPE_STRING | (1 << VALUE_HEADER_VARIANT_SHIFT) | VALUE_HEADER_OBJECT_MASK,

    // Range
    VALUE_TYPE_INLINE_RANGE = BASE_VALUE_TYPE_RANGE,
    VALUE_TYPE_OBJECT_RANGE = BASE_VALUE_TYPE_RANGE | (1 << VALUE_HEADER_VARIANT_SHIFT) | VALUE_HEADER_OBJECT_MASK,

    // List
    VALUE_TYPE_LIST = BASE_VALUE_TYPE_LIST | VALUE_HEADER_OBJECT_MASK,

    // Dictionary
    VALUE_TYPE_DICT = BASE_VALUE_TYPE_DICT | VALUE_HEADER_OBJECT_MASK,

    // Functions
    VALUE_TYPE_COMPILED_FUNCTION = BASE_VALUE_TYPE_FUNCTION | VALUE_HEADER_OBJECT_MASK,
    VALUE_TYPE_NATIVE_FUNCTION   = BASE_VALUE_TYPE_FUNCTION | (1 << VALUE_HEADER_VARIANT_SHIFT),

    // Function proto
    VALUE_TYPE_FUNCTION_PROTO = BASE_VALUE_TYPE_FUNCTION_PROTO,

    // Class
    VALUE_TYPE_CLASS = BASE_VALUE_TYPE_CLASS | VALUE_HEADER_OBJECT_MASK,
} ValueType;

// Simple operations to extract information
#define BASE_TYPE(v)     ((BaseValueType)((v)->header & VALUE_HEADER_BASE_TYPE_MASK))
#define VALUE_VARIANT(v) ((uint16_t)(((v)->header & VALUE_HEADER_VARIANT_MASK) >> VALUE_HEADER_VARIANT_SHIFT))
#define VALUE_TYPE(v)    ((ValueType)((v)->header))

#define IS_BOOL(v)              (VALUE_TYPE(v) == VALUE_TYPE_BOOL)
#define IS_INT(v)               (VALUE_TYPE(v) == VALUE_TYPE_INT)
#define IS_FLOAT(v)             (VALUE_TYPE(v) == VALUE_TYPE_FLOAT)
#define IS_OBJECT_STRING(v)     (VALUE_TYPE(v) == VALUE_TYPE_OBJECT_STRING)
#define IS_INLINE_STRING(v)     (VALUE_TYPE(v) == VALUE_TYPE_INLINE_STRING)
#define IS_OBJECT_RANGE(v)      (VALUE_TYPE(v) == VALUE_TYPE_OBJECT_RANGE)
#define IS_INLINE_RANGE(v)      (VALUE_TYPE(v) == VALUE_TYPE_INLINE_RANGE)
#define IS_LIST(v)              (VALUE_TYPE(v) == VALUE_TYPE_LIST)
#define IS_DICT(v)              (VALUE_TYPE(v) == VALUE_TYPE_DICT)
#define IS_FUNCTION_PROTO(v)    (VALUE_TYPE(v) == VALUE_TYPE_FUNCTION_PROTO)
#define IS_COMPILED_FUNCTION(v) (VALUE_TYPE(v) == VALUE_TYPE_COMPILED_FUNCTION)
#define IS_NATIVE_FUNCTION(v)   (VALUE_TYPE(v) == VALUE_TYPE_NATIVE_FUNCTION)
#define IS_CLASS(v)             (VALUE_TYPE(v) == VALUE_TYPE_CLASS)

#define IS_VALID(v)   (VALUE_TYPE(v) != VALUE_TYPE_INVALID)
#define IS_INVALID(v) (VALUE_TYPE(v) == VALUE_TYPE_INVALID)
#define IS_OBJECT(v)  ((bool)((v)->header & VALUE_HEADER_OBJECT_MASK))
#define IS_NUMBER(v)  (BASE_TYPE(v) == BASE_VALUE_TYPE_INT || BASE_TYPE(v) == BASE_VALUE_TYPE_FLOAT)
#define IS_STRING(v)  (BASE_TYPE(v) == BASE_VALUE_TYPE_STRING)
#define IS_RANGE(v)   (BASE_TYPE(v) == BASE_VALUE_TYPE_RANGE)

#define AS_PTR(v, t)            ((t*)((v)->as.ptr))
#define AS_OBJECT(v)            (((v)->as.obj))
#define AS_BOOL(v)              ((v)->as.b)
#define AS_INT(v)               ((v)->as.i)
#define AS_FLOAT(v)             ((v)->as.f)
#define AS_INLINE_STRING(v)     ((v)->as.is)
#define AS_OBJECT_STRING(v)     ((ObjectString*)((v)->as.obj))
#define AS_INLINE_RANGE(v)      ((v)->as.ir)
#define AS_OBJECT_RANGE(v)      ((ObjectRange*)((v)->as.obj))
#define AS_LIST(v)              ((ObjectList*)((v)->as.obj))
#define AS_DICT(v)              ((ObjectDict*)((v)->as.obj))
#define AS_FUNCTION_PROTO(v)    (AS_PTR((v), FunctionProto))
#define AS_COMPILED_FUNCTION(v) ((ObjectFunction*)((v)->as.obj))
#define AS_NATIVE_FUNCTION(v)   (AS_PTR((v), NativeFunction))
#define AS_CLASS(v)             ((ObjectClass*)((v)->as.obj))

#define OBJECT_VALUE(o, t) ((Value){.header = (ValueType)(t), .as = {.obj = (Object*)(o)}})

#define INVALID_VALUE ((Value){.header = VALUE_TYPE_INVALID, .as = {.i = 0}})

typedef int64_t IntValue;

#define FLOAT_EPSILON 1e-6

typedef double FloatValue;

typedef struct InlineString {
    char c[2];
    uint8_t length;
} InlineString;

// Inline range that can be stored directly in the Value struct.
typedef struct InlineRange {
    int32_t start;
    int32_t end;
} InlineRange;

typedef struct Value {
    ValueHeader header;
    union {
        bool b;
        IntValue i;
        FloatValue f;
        InlineString is;
        InlineRange ir;
        Object* obj;
        void* ptr;
    } as;
} Value;

static inline Value semiValuePtrCreate(void* p, ValueType type) {
    return (Value){.header = type, .as = {.ptr = p}};
}

/*
 │ Magic methods
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#define MAGIC_METHOD_SIGNATURE_NAME(baseType, name) semiPrimitive_##baseType##_##name

#define HASH_METHOD_MACRO(name, args, _) typedef ErrorId(*SemiMagicMethodHash) args;
HASH_X_MACRO(HASH_METHOD_MACRO, _)

// This expands to:
//   ErrorId(*add)( ... args ...);
// that can be used as struct fields.
#define STRIN_FIELD_MACRO(name, args, _) ErrorId(*name) args;

typedef struct TypeInitMethods {
    TYPE_INIT_X_MACRO(STRIN_FIELD_MACRO, _)
} TypeInitMethods;

typedef struct NumericMethods {
    NUMERIC_X_MACRO(STRIN_FIELD_MACRO, _)
} NumericMethods;

typedef struct ComparisonMethods {
    COMPARISON_X_MACRO(STRIN_FIELD_MACRO, _)
} ComparisonMethods;

typedef struct ConversionMethods {
    CONVERSION_X_MACRO(STRIN_FIELD_MACRO, _)
} ConversionMethods;

typedef struct CollectionMethods {
    COLLECTION_X_MACRO(STRIN_FIELD_MACRO, _)
} CollectionMethods;

#undef HASH_METHOD_MACRO

typedef struct MagicMethodsTable {
    TypeInitMethods* typeInitMethods;
    SemiMagicMethodHash hash;
    NumericMethods* numericMethods;
    ComparisonMethods* comparisonMethods;
    ConversionMethods* conversionMethods;
    CollectionMethods* collectionMethods;
} MagicMethodsTable;

/*
 │ Bool
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

static inline Value semiValueBoolCreate(bool b) {
    return (Value){.header = VALUE_TYPE_BOOL, .as = {.b = b}};
}

/*
 │ IntValue & FloatValue
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

static inline Value semiValueIntCreate(IntValue i) {
    return (Value){.header = VALUE_TYPE_INT, .as = {.i = i}};
}

static inline Value semiValueFloatCreate(FloatValue f) {
    return (Value){.header = VALUE_TYPE_FLOAT, .as = {.f = f}};
}

static inline FloatValue semiValueNumberToFloat(const Value v) {
    return v.header == VALUE_TYPE_INT ? (FloatValue)v.as.i : v.as.f;
}

/*
 │ InlineString & ObjectString
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct ObjectString {
    Object obj;

    size_t length;
    ValueHash hash;
    const char str[];
} ObjectString;

#define IS_VALID_INLINE_CHAR(c) ((c) >= 0 && (c) <= 0x7F)

static inline Value semiValueInlineStringCreate0(void) {
    return (Value){.header = VALUE_TYPE_INLINE_STRING, .as = {.is = {.c = {0, 0}, .length = 0}}};
}

static inline Value semiValueInlineStringCreat1(char c1) {
    return (Value){.header = VALUE_TYPE_INLINE_STRING, .as = {.is = {.c = {c1, 0}, .length = 1}}};
}

static inline Value semiValueInlineStringCreate2(char c1, char c2) {
    return (Value){.header = VALUE_TYPE_INLINE_STRING, .as = {.is = {.c = {c1, c2}, .length = 2}}};
}

ObjectString* semiObjectStringCreateUninit(GC* gc, size_t length);
ObjectString* semiObjectStringCreate(GC* gc, const char* text, size_t length);
static inline void semiObjectStringDestroy(GC* gc, ObjectString* str) {
    semiFree(gc, str, sizeof(ObjectString) + str->length);
}
Value semiValueStringCreate(GC* gc, const char* text, size_t length);

/*
 │ InlineRange & ObjectRange
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct ObjectRange {
    Object obj;

    union {
        struct {
            IntValue start;
            IntValue end;
            IntValue step;
        } ir;
        struct {
            FloatValue start;
            FloatValue end;
            FloatValue step;
        } fr;
    } as;
    bool isIntRange;
} ObjectRange;

static inline Value semiValueInlineRangeCreate(int32_t start, int32_t end) {
    return (Value){.header = VALUE_TYPE_INLINE_RANGE, .as = {.ir = {.start = start, .end = end}}};
}

ObjectRange* semiObjectRangeCreate(GC* gc, Value start, Value end, Value step);
static inline void semiObjectRangeDestroy(GC* gc, ObjectRange* range) {
    semiFree(gc, range, sizeof(ObjectRange));
}
Value semiValueRangeCreate(GC* gc, Value start, Value end, Value step);
ObjectRange* semiObjectRangeCopy(GC* gc, const ObjectRange* range);
/*
 │ ObjectList
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct ObjectList {
    Object obj;

    Value* values;
    uint32_t size;
    uint32_t capacity;
} ObjectList;

ObjectList* semiObjectListCreate(GC* gc, uint32_t capacity);
static inline void semiObjectListDestroy(GC* gc, ObjectList* list) {
    semiFree(gc, list->values, sizeof(Value) * list->capacity);
    semiFree(gc, list, sizeof(ObjectList));
}

static inline Value semiValueListCreate(GC* gc, uint32_t capacity) {
    ObjectList* o = semiObjectListCreate(gc, capacity);
    return o ? OBJECT_VALUE(o, VALUE_TYPE_LIST) : INVALID_VALUE;
}

void semiListEnsureCapacity(GC* gc, ObjectList* list, uint32_t capacity);
void semiListAppend(GC* gc, ObjectList* list, Value value);
void semiListInsert(GC* gc, ObjectList* list, uint32_t index, Value value);
void semiListShrink(GC* gc, ObjectList* list);
int64_t semiListRemove(GC* gc, ObjectList* list, Value value);
bool semiListPop(GC* gc, ObjectList* list);
bool semiListHas(GC* gc, ObjectList* list, Value value);
int64_t semiListIndex(GC* gc, ObjectList* list, Value value);

static inline uint32_t semiListLen(const ObjectList* list) {
    return list->size;
}

/*
 │ ObjectDict
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#define OBJECT_DICT_MAX_INDEX_LOAD(indexSize) ((indexSize * 2) / 3)
#define OBJECT_DICT_MIN_INDEX_SIZE            8

#define OBJECT_DICT_KEY_EMPTY     (-1)
#define OBJECT_DICT_KEY_TOMBSTONE (-2)

typedef int64_t TupleId;

typedef bool (*DictKeyCompareFn)(Value a, Value b);

// This has to be exposed because we use it in the constant table.
typedef struct ObjectDictKey {
    ValueHash hash;
    Value key;
} ObjectDictKey;

typedef struct ObjectDict {
    Object obj;

    // The comparison function for keys. It defaults to `semiBuiltInEquals` and is Never NULL.
    DictKeyCompareFn keyCmpFn;

    // The hashtable storing tuple IDs, which we can use to locate the actual keys and values.
    // The term "Tuple ID (TID)" comes from PostgreSQL.
    //
    // The use of signed integers is needed because we use negative values to represent empty and
    // tombestone keys.
    TupleId* tids;

    // The key part of the tuple table.
    ObjectDictKey* keys;
    // The value part of the tuple table.
    Value* values;

    // The size of the index table.
    uint32_t indexSize;

    // The number of non-empty indexes in `tids`.
    uint32_t used;

    // The actual number of entries in the dictionary.
    uint32_t len;
} ObjectDict;

ObjectDict* semiObjectDictCreate(GC* gc);
static inline Value semiValueDictCreate(GC* gc) {
    ObjectDict* o = semiObjectDictCreate(gc);
    return o ? OBJECT_VALUE(o, VALUE_TYPE_DICT) : INVALID_VALUE;
}
static inline void semiObjectDictDestroy(GC* gc, ObjectDict* dict) {
    semiFree(gc, dict->keys, sizeof(ObjectDictKey) * OBJECT_DICT_MAX_INDEX_LOAD(dict->indexSize));
    semiFree(gc, dict->tids, sizeof(TupleId) * dict->indexSize);
    semiFree(gc, dict->values, sizeof(Value) * dict->indexSize);
    semiFree(gc, dict, sizeof(ObjectDict));
}

void semiObjectStackDictInit(ObjectDict* dict);
void semiObjectStackDictCleanup(GC* gc, ObjectDict* dict);
TupleId semiDictFindTupleId(ObjectDict* dict, Value key, ValueHash hash);
bool semiDictHas(ObjectDict* dict, Value key);
bool semiDictHasWithHash(ObjectDict* dict, Value key, ValueHash hash);
Value semiDictGet(ObjectDict* dict, Value key);
Value semiDictGetWithHash(ObjectDict* dict, Value key, ValueHash hash);
bool semiDictSet(GC* gc, ObjectDict* dict, Value key, Value value);
bool semiDictSetWithHash(GC* gc, ObjectDict* dict, Value key, Value value, ValueHash hash);
Value semiDictDelete(GC* gc, ObjectDict* dict, Value key);
static inline uint32_t semiDictLen(ObjectDict* dict) {
    return dict->len;
}

/*
 │ Native Function
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
typedef ErrorId(NativeFunction)(SemiVM* vm, uint8_t argCount, Value* args, Value* ret);

Value semiValueNativeFunctionCreate(NativeFunction* function);

/*
 │ Function Proto
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct UpvalueDescription {
    uint8_t index;
    bool isLocal;
} UpvalueDescription;

typedef struct FunctionProto {
    Chunk chunk;
    ModuleId moduleId;
    uint8_t arity;
    uint8_t coarity;
    uint8_t maxStackSize;
    uint8_t upvalueCount;

    UpvalueDescription upvalues[];
} FunctionProto;

FunctionProto* semiFunctionProtoCreate(GC* gc, uint8_t upvalueCount);
void semiFunctionProtoDestroy(GC* gc, FunctionProto* function);
Value semiValueFunctionProtoCreate(FunctionProto* function);

/*
 │ Object Upvalue
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct ObjectUpvalue {
    Object obj;

    // Pointer to the referenced value. If the upvalue is open, this will point to the local
    // variable in the VM's stack. If the upvalue is closed, this will point to the `closed` field.
    Value* value;

    union {
        // When the upvalue is closed, this will hold the value of the closed variable.
        Value closed;

        // When the upvalue is open, this will point to the next upvalue in the linked list.
        struct ObjectUpvalue* next;
    } payload;
} ObjectUpvalue;

ObjectUpvalue* semiObjectUpvalueCreate(GC* gc, Value* value);
static inline void semiObjectUpvalueDestroy(GC* gc, ObjectUpvalue* upvalue) {
    semiFree(gc, upvalue, sizeof(ObjectUpvalue));
}

/*
 │ Object Function
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct ObjectFunction {
    Object obj;

    // The function proto that this function is bound to.
    FunctionProto* proto;

    // This points to the previous deferred function in the chain. Set by the VM when a defer statement is executed.
    // NULL if there is no previous deferred function.
    struct ObjectFunction* prevDeferredFn;

    uint8_t upvalueCount;

    // The list of upvalues that this function captures.
    ObjectUpvalue* upvalues[];
} ObjectFunction;

ObjectFunction* semiObjectFunctionCreate(GC* gc, FunctionProto* function);
static inline void semiObjectFunctionDestroy(GC* gc, ObjectFunction* function) {
    semiFree(gc, function, sizeof(ObjectFunction) + sizeof(ObjectUpvalue*) * function->upvalueCount);
}
static inline Value semiValueFunctionCreate(GC* gc, FunctionProto* function) {
    ObjectFunction* o = semiObjectFunctionCreate(gc, function);
    return o ? OBJECT_VALUE(o, VALUE_TYPE_COMPILED_FUNCTION) : INVALID_VALUE;
}

/*
 │ Object Class
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

typedef struct ObjectClass {
    // Mapping form identifier id to field index in the fields array.
    ObjectDict fields;

    ObjectString* name;
} ObjectClass;

ObjectClass* semiObjectNewClass(GC* gc, ObjectString* name);

static inline ValueHash semiHashString(const char* str, size_t length) {
    // FNV-1a hash algorithm for strings
    // See also http://www.isthe.com/chongo/tech/comp/fnv/
    const ValueHash FNV_OFFSET_BASIS = 2166136261u;
    const ValueHash FNV_PRIME        = 16777619u;

    ValueHash hash = FNV_OFFSET_BASIS;
    for (uint32_t i = 0; i < length; i++) {
        hash ^= (unsigned char)str[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

static inline ValueHash semiHash64Bits(uint64_t key) {
    // MurmurHash3's 64-bit finalizer
    key ^= (key >> 33);
    key *= 0xff51afd7ed558ccd;
    key ^= (key >> 33);
    key *= 0xc4ceb9fe1a85ec53;
    key ^= (key >> 33);

    return (ValueHash)key;
}

#endif /* SEMI_VALUE_H */
