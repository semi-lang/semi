// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./primitives.h"

#include <math.h>
#include <string.h>

#include "./value.h"
#include "./vm.h"

#define FLOAT_EPSILON 1e-6

// This expands to struct field initializers for the given baseType.
#define FIELD_INIT_MACRO(name, args, baseType) .name = MAGIC_METHOD_SIGNATURE_NAME(baseType, name),

static IntValue fastIntPower(IntValue base, IntValue exponent) {
    IntValue result      = 1;
    IntValue currentBase = base;

    while (exponent > 0) {
        if (exponent & 1) {
            result *= currentBase;
        }
        currentBase *= currentBase;
        exponent >>= 1;
    }

    return result;
}

// This expands to:
//   ErrorId semiPrimitive_INVALID_hash ( ... args ... );
// that can be used as function declarations.
#define GENERATE_SIG_FOR_TYPE_MACRO(name, args, baseType)            \
    static ErrorId MAGIC_METHOD_SIGNATURE_NAME(baseType, name) args;

/*
 │ Invalid
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
HASH_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, INVALID)
NUMERIC_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, INVALID)
COMPARISON_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, INVALID)
CONVERSION_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, INVALID)
COLLECTION_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, INVALID)

/*
 │ Bool
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

HASH_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, BOOL)

/*
 │ Number
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

HASH_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, NUMBER)
NUMERIC_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, NUMBER)
COMPARISON_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, NUMBER)
CONVERSION_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, NUMBER)

/*
 │ String
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

HASH_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, STRING)
COMPARISON_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, STRING)
CONVERSION_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, STRING)

#undef GENERATE_SIG_FOR_STRING

/*
 │ Range
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

/*
 │ List
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

COLLECTION_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, LIST)

/*
 │ Dictionary
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

COLLECTION_X_MACRO(GENERATE_SIG_FOR_TYPE_MACRO, DICT)

#undef GENERATE_SIG_FOR_TYPE_MACRO

/*
 │ Invalid
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, hash)(GC* gc, ValueHash* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, add)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, subtract)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, multiply)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, divide)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, floorDivide)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, modulo)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, power)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, negate)(GC* gc, Value* ret, Value* left) {
    (void)gc;
    (void)ret;
    (void)left;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, bitwiseAnd)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, bitwiseOr)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, bitwiseXor)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, bitwiseInvert)(GC* gc, Value* ret, Value* left) {
    (void)gc;
    (void)ret;
    (void)left;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, bitwiseShiftLeft)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, bitwiseShiftRight)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, gt)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, gte)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, lt)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, lte)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, eq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, neq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    (void)ret;
    (void)left;
    (void)right;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, inverse)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, toInt)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, toBool)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, toFloat)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, toString)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, toType)(GC* gc, Value* ret, Value* type, Value* operand) {
    (void)gc;
    (void)ret;
    (void)type;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, iter)(GC* gc, Value* ret, Value* iterable) {
    (void)gc;
    (void)ret;
    (void)iterable;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, contain)(GC* gc, Value* ret, Value* item, Value* collection) {
    (void)gc;
    (void)ret;
    (void)collection;
    (void)item;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, len)(GC* gc, Value* ret, Value* collection) {
    (void)gc;
    (void)ret;
    (void)collection;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, getItem)(GC* gc, Value* ret, Value* collection, Value* key) {
    (void)gc;
    (void)ret;
    (void)collection;
    (void)key;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, setItem)(GC* gc, Value* collection, Value* key, Value* value) {
    (void)gc;
    (void)collection;
    (void)key;
    (void)value;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, delItem)(GC* gc, Value* ret, Value* collection, Value* key) {
    (void)gc;
    (void)ret;
    (void)collection;
    (void)key;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, append)(GC* gc, Value* collection, Value* item) {
    (void)gc;
    (void)collection;
    (void)item;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, extend)(GC* gc, Value* collection, Value* iterable) {
    (void)gc;
    (void)collection;
    (void)iterable;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, pop)(GC* gc, Value* ret, Value* collection) {
    (void)gc;
    (void)ret;
    (void)collection;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(INVALID, next)(GC* gc, Value* ret, Value* iterator) {
    (void)gc;
    (void)ret;
    (void)iterator;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

#pragma endregion

/*
 │ Bool
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, inverse)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    *ret = semiValueNewBool(!AS_BOOL(operand));
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, hash)(GC* gc, ValueHash* ret, Value* operand) {
    (void)gc;
    *ret = AS_BOOL(operand) ? 0x1 : 0x0;
    return 0;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, toInt)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, toBool)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    *ret = *operand;
    return 0;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, toFloat)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, toString)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}
static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, toType)(GC* gc, Value* ret, Value* type, Value* operand) {
    (void)gc;
    (void)ret;
    (void)type;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, eq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (!IS_BOOL(right)) {
        *ret = semiValueNewBool(false);
        return 0;
    }
    *ret = semiValueNewBool(AS_BOOL(left) == AS_BOOL(right));
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(BOOL, neq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (!IS_BOOL(right)) {
        *ret = semiValueNewBool(false);
        return 0;
    }
    *ret = semiValueNewBool(AS_BOOL(left) != AS_BOOL(right));
    return 0;
}

#pragma endregion

/*
 │ Number
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, hash)(GC* gc, ValueHash* ret, Value* operand) {
    (void)gc;
    uint64_t key;
    if (IS_INT(operand)) {
        key = (uint64_t)AS_INT(operand);
    } else {
        // Character-wise copy: strict-aliasing-safe but not endianness-portable
        memcpy(&key, (void*)&AS_FLOAT(operand), sizeof(key));
    }
    *ret = semiHash64Bits(key);
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, add)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) + AS_INT(right));
    } else if ((IS_INT(left) && IS_FLOAT(right))) {
        *ret = semiValueNewFloat((FloatValue)AS_INT(left) + AS_FLOAT(right));
    } else if ((IS_FLOAT(left) && IS_INT(right))) {
        *ret = semiValueNewFloat(AS_FLOAT(left) + (FloatValue)AS_INT(right));
    } else if (IS_FLOAT(left) && IS_FLOAT(right)) {
        *ret = semiValueNewFloat(AS_FLOAT(left) + AS_FLOAT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, subtract)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) - AS_INT(right));
    } else if ((IS_INT(left) && IS_FLOAT(right))) {
        *ret = semiValueNewFloat((FloatValue)AS_INT(left) - AS_FLOAT(right));
    } else if ((IS_FLOAT(left) && IS_INT(right))) {
        *ret = semiValueNewFloat(AS_FLOAT(left) - (FloatValue)AS_INT(right));
    } else if (IS_FLOAT(left) && IS_FLOAT(right)) {
        *ret = semiValueNewFloat(AS_FLOAT(left) - AS_FLOAT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, multiply)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) * AS_INT(right));
    } else if ((IS_INT(left) && IS_FLOAT(right))) {
        *ret = semiValueNewFloat((FloatValue)AS_INT(left) * AS_FLOAT(right));
    } else if ((IS_FLOAT(left) && IS_INT(right))) {
        *ret = semiValueNewFloat(AS_FLOAT(left) * (FloatValue)AS_INT(right));
    } else if (IS_FLOAT(left) && IS_FLOAT(right)) {
        *ret = semiValueNewFloat(AS_FLOAT(left) * AS_FLOAT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, divide)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        if (AS_INT(right) == 0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewInt(AS_INT(left) / AS_INT(right));
    } else if ((IS_INT(left) && IS_FLOAT(right))) {
        if (AS_FLOAT(right) == 0.0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewFloat((FloatValue)AS_INT(left) / AS_FLOAT(right));
    } else if ((IS_FLOAT(left) && IS_INT(right))) {
        if (AS_INT(right) == 0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewFloat(AS_FLOAT(left) / (FloatValue)AS_INT(right));
    } else if (IS_FLOAT(left) && IS_FLOAT(right)) {
        if (AS_FLOAT(right) == 0.0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewFloat(AS_FLOAT(left) / AS_FLOAT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, floorDivide)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        if (AS_INT(right) == 0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewInt(AS_INT(left) / AS_INT(right));
    } else if ((IS_INT(left) && IS_FLOAT(right))) {
        if (AS_FLOAT(right) == 0.0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewInt((IntValue)floor((FloatValue)AS_INT(left) / AS_FLOAT(right)));
    } else if ((IS_FLOAT(left) && IS_INT(right))) {
        if (AS_INT(right) == 0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewInt((IntValue)floor(AS_FLOAT(left) / (FloatValue)AS_INT(right)));
    } else if (IS_FLOAT(left) && IS_FLOAT(right)) {
        if (AS_FLOAT(right) == 0.0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewInt((IntValue)floor(AS_FLOAT(left) / AS_FLOAT(right)));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, modulo)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        if (AS_INT(right) == 0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewInt(AS_INT(left) % AS_INT(right));
    } else if ((IS_INT(left) && IS_FLOAT(right))) {
        if (AS_FLOAT(right) == 0.0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewFloat(fmod((FloatValue)AS_INT(left), AS_FLOAT(right)));
    } else if ((IS_FLOAT(left) && IS_INT(right))) {
        if (AS_INT(right) == 0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewFloat(fmod(AS_FLOAT(left), (FloatValue)AS_INT(right)));
    } else if (IS_FLOAT(left) && IS_FLOAT(right)) {
        if (AS_FLOAT(right) == 0.0) {
            return SEMI_ERROR_DIVIDE_BY_ZERO;
        }
        *ret = semiValueNewFloat(fmod(AS_FLOAT(left), AS_FLOAT(right)));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, power)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        IntValue exponent = AS_INT(right);
        if (exponent >= 0) {
            *ret = semiValueNewInt(fastIntPower(AS_INT(left), exponent));
        } else {
            *ret = semiValueNewFloat(pow((FloatValue)AS_INT(left), (FloatValue)AS_INT(right)));
        }
    } else if ((IS_INT(left) && IS_FLOAT(right))) {
        *ret = semiValueNewFloat(pow((FloatValue)AS_INT(left), AS_FLOAT(right)));
    } else if ((IS_FLOAT(left) && IS_INT(right))) {
        *ret = semiValueNewFloat(pow(AS_FLOAT(left), (FloatValue)AS_INT(right)));
    } else if (IS_FLOAT(left) && IS_FLOAT(right)) {
        *ret = semiValueNewFloat(pow(AS_FLOAT(left), AS_FLOAT(right)));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, negate)(GC* gc, Value* ret, Value* left) {
    (void)gc;
    if (IS_INT(left)) {
        *ret = semiValueNewInt(-AS_INT(left));
    } else if (IS_FLOAT(left)) {
        *ret = semiValueNewFloat(-(FloatValue)AS_FLOAT(left));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, bitwiseAnd)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) & AS_INT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, bitwiseOr)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) | AS_INT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, bitwiseXor)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) ^ AS_INT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, bitwiseInvert)(GC* gc, Value* ret, Value* left) {
    (void)gc;
    if (IS_INT(left)) {
        *ret = semiValueNewInt(~AS_INT(left));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, bitwiseShiftLeft)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) << AS_INT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, bitwiseShiftRight)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewInt(AS_INT(left) >> AS_INT(right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, gt)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewBool(AS_INT(left) > AS_INT(right));
    } else if (IS_NUMBER(left) && IS_NUMBER(right)) {
        *ret = semiValueNewBool(semiValueNumberToFloat(*left) > semiValueNumberToFloat(*right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, gte)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewBool(AS_INT(left) >= AS_INT(right));
    } else if (IS_NUMBER(left) && IS_NUMBER(right)) {
        *ret = semiValueNewBool(semiValueNumberToFloat(*left) >= semiValueNumberToFloat(*right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, lt)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewBool(AS_INT(left) < AS_INT(right));
    } else if (IS_NUMBER(left) && IS_NUMBER(right)) {
        *ret = semiValueNewBool(semiValueNumberToFloat(*left) < semiValueNumberToFloat(*right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, lte)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewBool(AS_INT(left) <= AS_INT(right));
    } else if (IS_NUMBER(left) && IS_NUMBER(right)) {
        *ret = semiValueNewBool(semiValueNumberToFloat(*left) <= semiValueNumberToFloat(*right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, eq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewBool(AS_INT(left) == AS_INT(right));
    } else if (IS_NUMBER(left) && IS_NUMBER(right)) {
        *ret = semiValueNewBool(fabs(semiValueNumberToFloat(*left) - semiValueNumberToFloat(*right)) < FLOAT_EPSILON);
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, neq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (IS_INT(left) && IS_INT(right)) {
        *ret = semiValueNewBool(AS_INT(left) != AS_INT(right));
    } else if (IS_NUMBER(left) && IS_NUMBER(right)) {
        *ret = semiValueNewBool(semiValueNumberToFloat(*left) != semiValueNumberToFloat(*right));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, toBool)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    if (IS_INT(operand)) {
        *ret = semiValueNewBool(AS_INT(operand) != 0);
    } else if (IS_FLOAT(operand)) {
        *ret = semiValueNewBool(AS_FLOAT(operand) != 0.0);
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, toInt)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    if (IS_INT(operand)) {
        *ret = *operand;
    } else if (IS_FLOAT(operand)) {
        *ret = semiValueNewInt((IntValue)AS_FLOAT(operand));
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, toFloat)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    if (IS_INT(operand)) {
        *ret = semiValueNewFloat((FloatValue)AS_INT(operand));
    } else if (IS_FLOAT(operand)) {
        *ret = *operand;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, toString)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNIMPLEMENTED_FEATURE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, toType)(GC* gc, Value* ret, Value* type, Value* operand) {
    (void)gc;
    (void)ret;
    (void)type;
    (void)operand;
    return SEMI_ERROR_UNIMPLEMENTED_FEATURE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(NUMBER, inverse)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

#pragma endregion

/*
 │ String
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, hash)(GC* gc, ValueHash* ret, Value* operand) {
    (void)gc;
    uint32_t size;
    const char* str;

    if (IS_INLINE_STRING(operand)) {
        size = AS_INLINE_STRING(operand).length;
        str  = AS_INLINE_STRING(operand).c;
    } else {
        size = (uint32_t)AS_OBJECT_STRING(operand)->length;
        str  = AS_OBJECT_STRING(operand)->str;
    }

    *ret = semiHashString(str, size);
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, add)(GC* gc, Value* ret, Value* left, Value* right) {
    // TODO: We can define string as a list of ObjectString* to avoid copying on concatenation.

    uint32_t leftSize, rightSize;
    const char *leftStr, *rightStr;

    if (IS_INLINE_STRING(left)) {
        leftSize = AS_INLINE_STRING(left).length;
        leftStr  = AS_INLINE_STRING(left).c;
    } else {
        leftSize = (uint32_t)AS_OBJECT_STRING(right)->length;
        leftStr  = AS_OBJECT_STRING(right)->str;
    }

    if (IS_INLINE_STRING(right)) {
        rightSize += AS_INLINE_STRING(right).length;
        rightStr = AS_INLINE_STRING(right).c;
    } else if (IS_OBJECT_STRING(right)) {
        rightSize += (uint32_t)AS_OBJECT_STRING(right)->length;
        rightStr = AS_OBJECT_STRING(right)->str;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    if (UINT32_MAX - leftSize < rightSize) {
        return SEMI_ERROR_STRING_TOO_LONG;
    }

    ObjectString* retStr = semiObjectStringCreateUninit(gc, leftSize + rightSize);
    memcpy((void*)retStr->str, leftStr, leftSize);
    memcpy((void*)(retStr->str + leftSize), rightStr, rightSize);
    *ret = (Value){.header = VALUE_TYPE_OBJECT_STRING, .as = {.obj = (Object*)retStr}};

    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, gt)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (!IS_STRING(left) || !IS_STRING(right)) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    uint32_t leftSize, rightSize;
    const char *leftStr, *rightStr;

    if (IS_INLINE_STRING(left)) {
        leftSize = AS_INLINE_STRING(left).length;
        leftStr  = AS_INLINE_STRING(left).c;
    } else {
        leftSize = (uint32_t)AS_OBJECT_STRING(left)->length;
        leftStr  = AS_OBJECT_STRING(left)->str;
    }

    if (IS_INLINE_STRING(right)) {
        rightSize = AS_INLINE_STRING(right).length;
        rightStr  = AS_INLINE_STRING(right).c;
    } else {
        rightSize = (uint32_t)AS_OBJECT_STRING(right)->length;
        rightStr  = AS_OBJECT_STRING(right)->str;
    }

    int cmp = memcmp(leftStr, rightStr, leftSize < rightSize ? leftSize : rightSize);
    if (cmp > 0 || (cmp == 0 && leftSize > rightSize)) {
        *ret = semiValueNewBool(true);
    } else {
        *ret = semiValueNewBool(false);
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, gte)(GC* gc, Value* ret, Value* left, Value* right) {
    ErrorId err = MAGIC_METHOD_SIGNATURE_NAME(STRING, gt)(gc, ret, left, right);
    if (err != 0) {
        return err;
    }

    if (AS_BOOL(ret)) {
        return 0;
    }

    err = MAGIC_METHOD_SIGNATURE_NAME(STRING, eq)(gc, ret, left, right);
    if (err != 0) {
        return err;
    }

    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, lt)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (!IS_STRING(left) || !IS_STRING(right)) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    uint32_t leftSize, rightSize;
    const char *leftStr, *rightStr;

    if (IS_INLINE_STRING(left)) {
        leftSize = AS_INLINE_STRING(left).length;
        leftStr  = AS_INLINE_STRING(left).c;
    } else {
        leftSize = (uint32_t)AS_OBJECT_STRING(left)->length;
        leftStr  = AS_OBJECT_STRING(left)->str;
    }

    if (IS_INLINE_STRING(right)) {
        rightSize = AS_INLINE_STRING(right).length;
        rightStr  = AS_INLINE_STRING(right).c;
    } else {
        rightSize = (uint32_t)AS_OBJECT_STRING(right)->length;
        rightStr  = AS_OBJECT_STRING(right)->str;
    }

    int cmp = memcmp(leftStr, rightStr, leftSize < rightSize ? leftSize : rightSize);
    if (cmp < 0 || (cmp == 0 && leftSize < rightSize)) {
        *ret = semiValueNewBool(true);
    } else {
        *ret = semiValueNewBool(false);
    }
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, lte)(GC* gc, Value* ret, Value* left, Value* right) {
    ErrorId err = MAGIC_METHOD_SIGNATURE_NAME(STRING, lt)(gc, ret, left, right);
    if (err != 0) {
        return err;
    }

    if (AS_BOOL(ret)) {
        return 0;
    }

    err = MAGIC_METHOD_SIGNATURE_NAME(STRING, eq)(gc, ret, left, right);
    if (err != 0) {
        return err;
    }

    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, eq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;
    if (!IS_STRING(left) || !IS_STRING(right)) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    uint32_t leftSize, rightSize;
    const char *leftStr, *rightStr;

    if (IS_INLINE_STRING(left)) {
        leftSize = AS_INLINE_STRING(left).length;
        leftStr  = AS_INLINE_STRING(left).c;
    } else {
        leftSize = (uint32_t)AS_OBJECT_STRING(left)->length;
        leftStr  = AS_OBJECT_STRING(left)->str;
    }

    if (IS_INLINE_STRING(right)) {
        rightSize = AS_INLINE_STRING(right).length;
        rightStr  = AS_INLINE_STRING(right).c;
    } else {
        rightSize = (uint32_t)AS_OBJECT_STRING(right)->length;
        rightStr  = AS_OBJECT_STRING(right)->str;
    }

    if (leftSize != rightSize) {
        *ret = semiValueNewBool(false);
        return 0;
    }

    *ret = semiValueNewBool(memcmp(leftStr, rightStr, leftSize) == 0);
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, neq)(GC* gc, Value* ret, Value* left, Value* right) {
    ErrorId err = MAGIC_METHOD_SIGNATURE_NAME(STRING, eq)(gc, ret, left, right);
    if (err != 0) {
        return err;
    }

    *ret = semiValueNewBool(!AS_BOOL(ret));
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, toBool)(GC* gc, Value* ret, Value* operand) {
    (void)gc;

    uint32_t size;
    if (IS_INLINE_STRING(operand)) {
        size = AS_INLINE_STRING(operand).length;
    } else {
        size = (uint32_t)AS_OBJECT_STRING(operand)->length;
    }

    *ret = semiValueNewBool(size != 0);
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, inverse)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, toInt)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, toFloat)(GC* gc, Value* ret, Value* operand) {
    (void)gc;
    (void)ret;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, toString)(GC* gc, Value* ret, Value* operand) {
    (void)gc;

    *ret = *operand;
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, toType)(GC* gc, Value* ret, Value* type, Value* operand) {
    (void)gc;
    (void)ret;
    (void)type;
    (void)operand;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, contain)(GC* gc, Value* ret, Value* item, Value* collection) {
    (void)gc;
    (void)ret;

    const char *itemStr, *collectionStr;
    uint32_t itemSize, collectionSize;

    if (IS_INLINE_STRING(collection)) {
        collectionSize = AS_INLINE_STRING(collection).length;
        collectionStr  = AS_INLINE_STRING(collection).c;
    } else if (IS_OBJECT_STRING(collection)) {
        collectionSize = (uint32_t)AS_OBJECT_STRING(collection)->length;
        collectionStr  = AS_OBJECT_STRING(collection)->str;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    if (IS_INLINE_STRING(item)) {
        itemSize = AS_INLINE_STRING(item).length;
        itemStr  = AS_INLINE_STRING(item).c;
    } else if (IS_OBJECT_STRING(item)) {
        itemSize = (uint32_t)AS_OBJECT_STRING(item)->length;
        itemStr  = AS_OBJECT_STRING(item)->str;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    if (itemSize == 0) {
        *ret = semiValueNewBool(true);
        return 0;
    }

    if (collectionSize < itemSize) {
        *ret = semiValueNewBool(false);
        return 0;
    }

    for (uint32_t i = 0; i <= collectionSize - itemSize; i++) {
        if (collectionStr[i] == itemStr[0] && memcmp(collectionStr + i, itemStr, itemSize) == 0) {
            *ret = semiValueNewBool(true);
            return 0;
        }
    }

    *ret = semiValueNewBool(false);
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, len)(GC* gc, Value* ret, Value* collection) {
    (void)gc;

    uint32_t size;
    if (IS_INLINE_STRING(collection)) {
        size = AS_INLINE_STRING(collection).length;
    } else if (IS_OBJECT_STRING(collection)) {
        size = (uint32_t)AS_OBJECT_STRING(collection)->length;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    *ret = semiValueNewInt((IntValue)size);
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(STRING, getItem)(GC* gc, Value* ret, Value* collection, Value* key) {
    (void)gc;

    if (!IS_INT(key)) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    IntValue index = AS_INT(key);
    uint32_t size;
    const char* str;

    if (IS_INLINE_STRING(collection)) {
        size = AS_INLINE_STRING(collection).length;
        str  = AS_INLINE_STRING(collection).c;
    } else if (IS_OBJECT_STRING(collection)) {
        size = (uint32_t)AS_OBJECT_STRING(collection)->length;
        str  = AS_OBJECT_STRING(collection)->str;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    if (index < 0) {
        index += size;
    }
    if (index < 0 || (uint32_t)index >= size) {
        return SEMI_ERROR_INDEX_OOB;
    }

    *ret = semiValueNewInlineString1(str[index]);
    return 0;
}

#pragma endregion

/*
 │ Range
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(RANGE, next)(GC* gc, Value* ret, Value* iterator) {
    if (IS_INLINE_RANGE(iterator)) {
        InlineRange range = AS_INLINE_RANGE(iterator);
        if (range.start >= range.end) {
            *ret = INVALID_VALUE;
        } else {
            *ret = semiValueNewInt(range.start);
            AS_INLINE_RANGE(iterator).start += 1;
        }
        return 0;

    } else if (IS_OBJECT_RANGE(iterator)) {
        ObjectRange* range = AS_OBJECT_RANGE(iterator);
        ErrorId err;
        Value cmpResult;
        err = MAGIC_METHOD_SIGNATURE_NAME(NUMBER, lt)(gc, &cmpResult, &range->start, &range->end);
        if (err != 0) {
            return err;
        }
        if (!AS_BOOL(&cmpResult)) {
            *ret = INVALID_VALUE;
            return 0;
        }

        *ret = range->start;
        return MAGIC_METHOD_SIGNATURE_NAME(NUMBER, add)(gc, &range->start, &range->start, &range->step);
    }

    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(RANGE, eq)(GC* gc, Value* ret, Value* left, Value* right) {
    (void)gc;

    Value lStart, lEnd, lStep;
    if (IS_INLINE_RANGE(left)) {
        InlineRange range = AS_INLINE_RANGE(left);
        lStart            = semiValueNewInt(range.start);
        lEnd              = semiValueNewInt(range.end);
        lStep             = semiValueNewInt(1);
    } else if (IS_OBJECT_RANGE(left)) {
        ObjectRange* range = AS_OBJECT_RANGE(left);
        lStart             = range->start;
        lEnd               = range->end;
        lStep              = range->step;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    if (IS_INLINE_RANGE(right)) {
        InlineRange range = AS_INLINE_RANGE(right);
        Value rStart      = semiValueNewInt(range.start);
        Value rEnd        = semiValueNewInt(range.end);
        Value rStep       = semiValueNewInt(1);

        *ret = semiValueNewBool(semiBuiltInEquals(lStart, rStart) && semiBuiltInEquals(lEnd, rEnd) &&
                                semiBuiltInEquals(lStep, rStep));
        return 0;

    } else if (IS_OBJECT_RANGE(right)) {
        ObjectRange* range = AS_OBJECT_RANGE(right);
        Value leftStart    = range->start;
        Value leftEnd      = range->end;
        Value leftStep     = range->step;

        Value rStart = range->start;
        Value rEnd   = range->end;
        Value rStep  = range->step;

        *ret = semiValueNewBool(semiBuiltInEquals(lStart, rStart) && semiBuiltInEquals(lEnd, rEnd) &&
                                semiBuiltInEquals(lStep, rStep));

        return 0;
    } else {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(RANGE, neq)(GC* gc, Value* ret, Value* left, Value* right) {
    ErrorId err = MAGIC_METHOD_SIGNATURE_NAME(RANGE, eq)(gc, ret, left, right);
    if (err != 0) {
        return err;
    }

    *ret = semiValueNewBool(!AS_BOOL(ret));
    return 0;
}

#pragma endregion

/*
 │ List
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, iter)(GC* gc, Value* ret, Value* iterable) {
    (void)gc;
    (void)ret;
    (void)iterable;
    return SEMI_ERROR_UNIMPLEMENTED_FEATURE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, contain)(GC* gc, Value* ret, Value* item, Value* collection) {
    ObjectList* list = AS_LIST(collection);
    *ret             = semiValueNewBool(semiListHas(gc, list, *item));
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, len)(GC* gc, Value* ret, Value* collection) {
    (void)gc;
    ObjectList* list = AS_LIST(collection);
    *ret             = semiValueNewInt((IntValue)semiListLen(list));
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, getItem)(GC* gc, Value* ret, Value* collection, Value* key) {
    (void)gc;
    ObjectList* list = AS_LIST(collection);

    if (!IS_INT(key)) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    IntValue index = AS_INT(key);
    if (index < 0) {
        index += list->size;
    }
    if (index < 0 || (uint32_t)index >= list->size) {
        return SEMI_ERROR_INDEX_OOB;
    }

    *ret = list->values[index];
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, setItem)(GC* gc, Value* collection, Value* key, Value* value) {
    (void)gc;
    ObjectList* list = AS_LIST(collection);

    if (!IS_INT(key)) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    IntValue index = AS_INT(key);
    if (index < 0) {
        index += list->size;
    }
    if (index < 0 || (uint32_t)index >= list->size) {
        return SEMI_ERROR_INDEX_OOB;
    }

    list->values[index] = *value;
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, delItem)(GC* gc, Value* ret, Value* collection, Value* key) {
    (void)gc;
    ObjectList* list = AS_LIST(collection);

    if (!IS_INT(key)) {
        return SEMI_ERROR_UNEXPECTED_TYPE;
    }

    IntValue index = AS_INT(key);
    if (index < 0) {
        index += list->size;
    }
    if (index < 0 || (uint32_t)index >= list->size) {
        return SEMI_ERROR_INDEX_OOB;
    }

    *ret = list->values[index];

    if ((uint32_t)index < list->size - 1) {
        memcpy(&list->values[index], &list->values[index + 1], (list->size - (uint32_t)index - 1) * sizeof(Value));
    }
    list->size--;

    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, append)(GC* gc, Value* collection, Value* item) {
    ObjectList* list = AS_LIST(collection);
    semiListAppend(gc, list, *item);
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, extend)(GC* gc, Value* collection, Value* iterable) {
    (void)gc;
    (void)collection;
    (void)iterable;
    return SEMI_ERROR_UNIMPLEMENTED_FEATURE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(LIST, pop)(GC* gc, Value* ret, Value* collection) {
    ObjectList* list = AS_LIST(collection);

    if (list->size == 0) {
        return SEMI_ERROR_INDEX_OOB;
    }

    *ret = list->values[list->size - 1];
    semiListPop(gc, list);
    return 0;
}

#pragma endregion

/*
 │ Dictionary
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#pragma region

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, iter)(GC* gc, Value* ret, Value* iterable) {
    (void)gc;
    (void)ret;
    (void)iterable;
    return SEMI_ERROR_UNIMPLEMENTED_FEATURE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, contain)(GC* gc, Value* ret, Value* item, Value* collection) {
    (void)gc;
    ObjectDict* dict = AS_DICT(collection);
    *ret             = semiValueNewBool(semiDictHas(dict, *item));
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, len)(GC* gc, Value* ret, Value* collection) {
    (void)gc;
    ObjectDict* dict = AS_DICT(collection);
    *ret             = semiValueNewInt((IntValue)semiDictLen(dict));
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, getItem)(GC* gc, Value* ret, Value* collection, Value* key) {
    (void)gc;

    ObjectDict* dict = AS_DICT(collection);
    Value result     = semiDictGet(dict, *key);

    if (IS_INVALID(&result)) {
        return SEMI_ERROR_KEY_NOT_FOUND;
    }

    *ret = result;
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, setItem)(GC* gc, Value* collection, Value* key, Value* value) {
    ObjectDict* dict = AS_DICT(collection);

    if (!semiDictSet(gc, dict, *key, *value)) {
        return SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;
    }

    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, delItem)(GC* gc, Value* ret, Value* collection, Value* key) {
    ObjectDict* dict = AS_DICT(collection);

    Value result = semiDictDelete(gc, dict, *key);
    if (IS_INVALID(&result)) {
        return SEMI_ERROR_KEY_NOT_FOUND;
    }

    *ret = result;
    return 0;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, append)(GC* gc, Value* collection, Value* item) {
    (void)gc;
    (void)collection;
    (void)item;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, extend)(GC* gc, Value* collection, Value* iterable) {
    (void)gc;
    (void)collection;
    (void)iterable;
    return SEMI_ERROR_UNIMPLEMENTED_FEATURE;
}

static ErrorId MAGIC_METHOD_SIGNATURE_NAME(DICT, pop)(GC* gc, Value* ret, Value* collection) {
    (void)gc;
    (void)ret;
    (void)collection;
    return SEMI_ERROR_UNEXPECTED_TYPE;
}

#pragma endregion

bool semiBuiltInEquals(Value a, Value b) {
    BaseValueType baseTypeA = BASE_TYPE(&a);
    BaseValueType baseTypeB = BASE_TYPE(&b);
    ValueType typeB         = VALUE_TYPE(&b);

    if (baseTypeA != baseTypeB) {
        return false;
    }

    Value ret;
    switch (BASE_TYPE(&a)) {
        case BASE_VALUE_TYPE_BOOL:
            return AS_BOOL(&a) == AS_BOOL(&b);

        case BASE_VALUE_TYPE_NUMBER:
            MAGIC_METHOD_SIGNATURE_NAME(NUMBER, eq)(NULL, &ret, &a, &b);
            return AS_BOOL(&ret);

        case BASE_VALUE_TYPE_STRING:
            MAGIC_METHOD_SIGNATURE_NAME(STRING, eq)(NULL, &ret, &a, &b);
            return AS_BOOL(&ret);

        case BASE_VALUE_TYPE_RANGE:
            MAGIC_METHOD_SIGNATURE_NAME(RANGE, eq)(NULL, &ret, &a, &b);
            return AS_BOOL(&ret);

        default:
            return false;  // Unsupported type for comparison
    }
}

ValueHash semiBuiltInHash(const Value value) {
    switch (VALUE_TYPE(&value)) {
        case VALUE_TYPE_BOOL:
            return AS_BOOL(&value) ? 0x1 : 0x0;

        case VALUE_TYPE_INT:
            // signed-to-unsigned conversion is safe and defined.
            return semiHash64Bits((unsigned)AS_INT(&value));

        case VALUE_TYPE_FLOAT: {
            uint64_t key;
            // Dharacter-wise copy: strict-aliasing-safe but not endianness-portable
            memcpy(&key, (void*)&AS_FLOAT(&value), sizeof(key));
            return semiHash64Bits(key);
        }

        case VALUE_TYPE_INLINE_STRING:
            return semiHashString(AS_INLINE_STRING(&value).c, AS_INLINE_STRING(&value).length);

        case VALUE_TYPE_OBJECT_STRING:
            return AS_OBJECT_STRING(&value)->hash;

        case VALUE_TYPE_FUNCTION_TEMPLATE:
            return semiHash64Bits((uint64_t)(uintptr_t)AS_FUNCTION_TEMPLATE(&value));

        default:
            return SEMI_ERROR_UNEXPECTED_TYPE;
    }
}

/*
 │ Built-in Primitives Setter
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
static NumericMethods invalidNumericMethods       = {NUMERIC_X_MACRO(FIELD_INIT_MACRO, INVALID)};
static ComparisonMethods invalidComparisonMethods = {COMPARISON_X_MACRO(FIELD_INIT_MACRO, INVALID)};
static ConversionMethods invalidConversionMethods = {CONVERSION_X_MACRO(FIELD_INIT_MACRO, INVALID)};
static CollectionMethods invalidCollectionMethods = {COLLECTION_X_MACRO(FIELD_INIT_MACRO, INVALID)};

static const MagicMethodsTable invalidMagicMethodsTable = {
    .hash              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, hash),
    .numericMethods    = &invalidNumericMethods,
    .comparisonMethods = &invalidComparisonMethods,
    .conversionMethods = &invalidConversionMethods,
    .collectionMethods = &invalidCollectionMethods,
    .next              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, next),
};

static ComparisonMethods boolComparisonMethods = {
    .gt  = MAGIC_METHOD_SIGNATURE_NAME(INVALID, gt),
    .gte = MAGIC_METHOD_SIGNATURE_NAME(INVALID, gte),
    .lt  = MAGIC_METHOD_SIGNATURE_NAME(INVALID, lt),
    .lte = MAGIC_METHOD_SIGNATURE_NAME(INVALID, lte),
    .eq  = MAGIC_METHOD_SIGNATURE_NAME(BOOL, eq),
    .neq = MAGIC_METHOD_SIGNATURE_NAME(BOOL, neq),
};
static ConversionMethods boolConversionMethods = {CONVERSION_X_MACRO(FIELD_INIT_MACRO, BOOL)};

static const MagicMethodsTable boolMagicMethodsTable = {
    .hash              = MAGIC_METHOD_SIGNATURE_NAME(BOOL, hash),
    .numericMethods    = &invalidNumericMethods,
    .comparisonMethods = &boolComparisonMethods,
    .conversionMethods = &boolConversionMethods,
    .collectionMethods = &invalidCollectionMethods,
    .next              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, next),
};

static NumericMethods numberNumericMethods       = {NUMERIC_X_MACRO(FIELD_INIT_MACRO, NUMBER)};
static ComparisonMethods numberComparisonMethods = {COMPARISON_X_MACRO(FIELD_INIT_MACRO, NUMBER)};
static ConversionMethods numberConversionMethods = {CONVERSION_X_MACRO(FIELD_INIT_MACRO, NUMBER)};

static const MagicMethodsTable numberMagicMethodsTable = {
    .hash              = MAGIC_METHOD_SIGNATURE_NAME(NUMBER, hash),
    .numericMethods    = &numberNumericMethods,
    .comparisonMethods = &numberComparisonMethods,
    .conversionMethods = &numberConversionMethods,
    .collectionMethods = &invalidCollectionMethods,
    .next              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, next),
};

static ComparisonMethods stringComparisonMethods = {COMPARISON_X_MACRO(FIELD_INIT_MACRO, STRING)};
static ConversionMethods stringConversionMethods = {CONVERSION_X_MACRO(FIELD_INIT_MACRO, STRING)};
static CollectionMethods stringCollectionMethods = {
    .iter    = MAGIC_METHOD_SIGNATURE_NAME(INVALID, iter),
    .contain = MAGIC_METHOD_SIGNATURE_NAME(STRING, contain),
    .len     = MAGIC_METHOD_SIGNATURE_NAME(STRING, len),
    .getItem = MAGIC_METHOD_SIGNATURE_NAME(STRING, getItem),
    .setItem = MAGIC_METHOD_SIGNATURE_NAME(INVALID, setItem),
    .delItem = MAGIC_METHOD_SIGNATURE_NAME(INVALID, delItem),
    .append  = MAGIC_METHOD_SIGNATURE_NAME(INVALID, append),
    .extend  = MAGIC_METHOD_SIGNATURE_NAME(INVALID, extend),
    .pop     = MAGIC_METHOD_SIGNATURE_NAME(INVALID, pop),
};

static const MagicMethodsTable stringMagicMethodsTable = {
    .hash              = MAGIC_METHOD_SIGNATURE_NAME(STRING, hash),
    .numericMethods    = &invalidNumericMethods,
    .comparisonMethods = &stringComparisonMethods,
    .conversionMethods = &stringConversionMethods,
    .collectionMethods = &stringCollectionMethods,
    .next              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, next),
};

static ComparisonMethods rangeComparisonMethods = {
    .gt  = MAGIC_METHOD_SIGNATURE_NAME(INVALID, gt),
    .gte = MAGIC_METHOD_SIGNATURE_NAME(INVALID, gte),
    .lt  = MAGIC_METHOD_SIGNATURE_NAME(INVALID, lt),
    .lte = MAGIC_METHOD_SIGNATURE_NAME(INVALID, lte),
    .eq  = MAGIC_METHOD_SIGNATURE_NAME(RANGE, eq),
    .neq = MAGIC_METHOD_SIGNATURE_NAME(RANGE, neq),
};

static const MagicMethodsTable rangeMagicMethodsTable = {
    .hash              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, hash),
    .numericMethods    = &invalidNumericMethods,
    .comparisonMethods = &rangeComparisonMethods,
    .conversionMethods = &invalidConversionMethods,
    .collectionMethods = &invalidCollectionMethods,
    .next              = MAGIC_METHOD_SIGNATURE_NAME(RANGE, next),
};

static CollectionMethods listCollectionMethods = {COLLECTION_X_MACRO(FIELD_INIT_MACRO, LIST)};

static const MagicMethodsTable listMagicMethodsTable = {
    .hash              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, hash),
    .numericMethods    = &invalidNumericMethods,
    .comparisonMethods = &invalidComparisonMethods,
    .conversionMethods = &invalidConversionMethods,
    .collectionMethods = &listCollectionMethods,
    .next              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, next),
};

static CollectionMethods dictCollectionMethods = {COLLECTION_X_MACRO(FIELD_INIT_MACRO, DICT)};

static const MagicMethodsTable dictMagicMethodsTable = {
    .hash              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, hash),
    .numericMethods    = &invalidNumericMethods,
    .comparisonMethods = &invalidComparisonMethods,
    .conversionMethods = &invalidConversionMethods,
    .collectionMethods = &dictCollectionMethods,
    .next              = MAGIC_METHOD_SIGNATURE_NAME(INVALID, next),
};

void semiPrimitivesFinalizeMagicMethodsTable(MagicMethodsTable* table) {
    if (table->hash == NULL) {
        table->hash = MAGIC_METHOD_SIGNATURE_NAME(INVALID, hash);
    }
    if (table->numericMethods == NULL) {
        table->numericMethods = &invalidNumericMethods;
    }
    if (table->comparisonMethods == NULL) {
        table->comparisonMethods = &invalidComparisonMethods;
    }
    if (table->conversionMethods == NULL) {
        table->conversionMethods = &invalidConversionMethods;
    }
    if (table->collectionMethods == NULL) {
        table->collectionMethods = &invalidCollectionMethods;
    }
}

void semiPrimitivesIntializeBuiltInPrimitives(GC* gc, ClassTable* classes) {
    static const MagicMethodsTable builtInClasses[] = {
        [BASE_VALUE_TYPE_INVALID]           = invalidMagicMethodsTable,
        [BASE_VALUE_TYPE_BOOL]              = boolMagicMethodsTable,
        [BASE_VALUE_TYPE_NUMBER]            = numberMagicMethodsTable,
        [BASE_VALUE_TYPE_STRING]            = stringMagicMethodsTable,
        [BASE_VALUE_TYPE_RANGE]             = rangeMagicMethodsTable,
        [BASE_VALUE_TYPE_LIST]              = listMagicMethodsTable,
        [BASE_VALUE_TYPE_DICT]              = dictMagicMethodsTable,
        [BASE_VALUE_TYPE_FUNCTION_TEMPLATE] = invalidMagicMethodsTable,
        [BASE_VALUE_TYPE_CLASS]             = invalidMagicMethodsTable,
    };

    uint16_t newCapacity               = (uint16_t)(sizeof(builtInClasses) / sizeof(MagicMethodsTable));
    MagicMethodsTable* newClassMethods = semiMalloc(gc, newCapacity * sizeof(MagicMethodsTable));

    classes->classMethods  = newClassMethods;
    classes->classCount    = newCapacity;
    classes->classCapacity = newCapacity;

    memcpy(classes->classMethods, builtInClasses, sizeof(builtInClasses));
}

void semiPrimitivesCleanupClassTable(GC* gc, ClassTable* classes) {
    semiFree(gc, classes->classMethods, classes->classCapacity * sizeof(MagicMethodsTable));
    classes->classMethods  = NULL;
    classes->classCount    = 0;
    classes->classCapacity = 0;
}

ErrorId semiPrimitivesDispatchHash(MagicMethodsTable* table, GC* gc, ValueHash* ret, Value* a) {
    return table->hash(gc, ret, a);
}

ErrorId semiPrimitivesDispatch1Operand(MagicMethodsTable* table, GC* gc, Opcode method, Value* ret, Value* a) {
    (void)table;
    (void)gc;
    (void)method;
    (void)ret;
    (void)a;
    return SEMI_ERROR_UNIMPLEMENTED_FEATURE;
}
ErrorId semiPrimitivesDispatch2Operands(MagicMethodsTable* table, GC* gc, Opcode method, Value* a, Value* b, Value* c) {
    switch (method) {
        case OP_ADD:
            return table->numericMethods->add(gc, a, b, c);
        case OP_SUBTRACT:
            return table->numericMethods->subtract(gc, a, b, c);
        case OP_MULTIPLY:
            return table->numericMethods->multiply(gc, a, b, c);
        case OP_DIVIDE:
            return table->numericMethods->divide(gc, a, b, c);
        case OP_FLOOR_DIVIDE:
            return table->numericMethods->floorDivide(gc, a, b, c);
        case OP_MODULO:
            return table->numericMethods->modulo(gc, a, b, c);
        case OP_POWER:
            return table->numericMethods->power(gc, a, b, c);
        case OP_BITWISE_AND:
            return table->numericMethods->bitwiseAnd(gc, a, b, c);
        case OP_BITWISE_OR:
            return table->numericMethods->bitwiseOr(gc, a, b, c);
        case OP_BITWISE_XOR:
            return table->numericMethods->bitwiseXor(gc, a, b, c);
        case OP_BITWISE_L_SHIFT:
            return table->numericMethods->bitwiseShiftLeft(gc, a, b, c);
        case OP_BITWISE_R_SHIFT:
            return table->numericMethods->bitwiseShiftRight(gc, a, b, c);
        case OP_GT:
            return table->comparisonMethods->gt(gc, a, b, c);
        case OP_GE:
            return table->comparisonMethods->gte(gc, a, b, c);
        case OP_EQ:
            return table->comparisonMethods->eq(gc, a, b, c);
        case OP_NEQ:
            return table->comparisonMethods->neq(gc, a, b, c);
        case OP_GET_ITEM:
            return table->collectionMethods->getItem(gc, a, b, c);
        case OP_SET_ITEM:
            return table->collectionMethods->setItem(gc, a, b, c);
        case OP_DEL_ITEM:
            return table->collectionMethods->delItem(gc, a, b, c);
        default:
            return SEMI_ERROR_INTERNAL_ERROR;
    }
}

#undef FIELD_INIT_MACRO
