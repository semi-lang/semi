// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_PRIMITIVE_X_MACRO_H
#define SEMI_PRIMITIVE_X_MACRO_H

/*
 │ Hash Method
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#define HASH_X_MACRO(MACRO, ...) MACRO(hash, (GC * gc, ValueHash * ret, Value * operand), __VA_ARGS__)

/*
 │ Numeric Methods
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#define NUMERIC_X_MACRO(MACRO, ...)                                                            \
    /* ret = left + right */                                                                   \
    MACRO(add, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)               \
    /* ret = left - right */                                                                   \
    MACRO(subtract, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)          \
    /* ret = left * right */                                                                   \
    MACRO(multiply, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)          \
    /* ret = left / right */                                                                   \
    MACRO(divide, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)            \
    /* ret = left // right */                                                                  \
    MACRO(floorDivide, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)       \
    /* ret = left % right */                                                                   \
    MACRO(modulo, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)            \
    /* ret = left ** right */                                                                  \
    MACRO(power, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)             \
    /* ret = -left */                                                                          \
    MACRO(negate, (GC * gc, Value * ret, Value * left), __VA_ARGS__)                           \
    /* ret = left & right */                                                                   \
    MACRO(bitwiseAnd, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)        \
    /* ret = left | right */                                                                   \
    MACRO(bitwiseOr, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)         \
    /* ret = left ^ right */                                                                   \
    MACRO(bitwiseXor, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)        \
    /* ret = ~left */                                                                          \
    MACRO(bitwiseInvert, (GC * gc, Value * ret, Value * left), __VA_ARGS__)                    \
    /* ret = left << right */                                                                  \
    MACRO(bitwiseShiftLeft, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)  \
    /* ret = left >> right */                                                                  \
    MACRO(bitwiseShiftRight, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)

/*
 │ Comparison Methods
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#define COMPARISON_X_MACRO(MACRO, ...)                                           \
    /* ret = left > right  */                                                    \
    MACRO(gt, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)  \
    /* ret = left >= right */                                                    \
    MACRO(gte, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__) \
    /* ret = left < right */                                                     \
    MACRO(lt, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)  \
    /* ret = left <= right */                                                    \
    MACRO(lte, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__) \
    /* ret = left == right */                                                    \
    MACRO(eq, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)  \
    /* ret = left != right */                                                    \
    MACRO(neq, (GC * gc, Value * ret, Value * left, Value * right), __VA_ARGS__)

/*
 │ Conversion Methods
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#define CONVERSION_X_MACRO(MACRO, ...)                                                \
    /* ret = bool(operand)  */                                                        \
    MACRO(toBool, (GC * gc, Value * ret, Value * operand), __VA_ARGS__)               \
    /* ret = !operand */                                                              \
    MACRO(inverse, (GC * gc, Value * ret, Value * operand), __VA_ARGS__)              \
    /* ret = int(operand)  */                                                         \
    MACRO(toInt, (GC * gc, Value * ret, Value * operand), __VA_ARGS__)                \
    /* ret = float(operand)  */                                                       \
    MACRO(toFloat, (GC * gc, Value * ret, Value * operand), __VA_ARGS__)              \
    /* ret = str(operand)  */                                                         \
    MACRO(toString, (GC * gc, Value * ret, Value * operand), __VA_ARGS__)             \
    /* ret = operand.(type)  */                                                       \
    MACRO(toType, (GC * gc, Value * ret, Value * type, Value * operand), __VA_ARGS__)

/*
 │ Iteration Methods
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/
#define ITERATION_X_MACRO(MACRO, ...) MACRO(next, (GC * gc, Value * ret, Value * iterator), __VA_ARGS__)

/*
 │ Collection Methods
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

#define COLLECTION_X_MACRO(MACRO, ...)                                                     \
    /* ret = iter(iterable)  */                                                            \
    MACRO(iter, (GC * gc, Value * ret, Value * iterable), __VA_ARGS__)                     \
    /* ret = item in collection */                                                         \
    MACRO(contain, (GC * gc, Value * ret, Value * item, Value * collection), __VA_ARGS__)  \
    /* ret = len(collection) */                                                            \
    MACRO(len, (GC * gc, Value * ret, Value * collection), __VA_ARGS__)                    \
    /* ret = collection[key] */                                                            \
    MACRO(getItem, (GC * gc, Value * ret, Value * collection, Value * key), __VA_ARGS__)   \
    /* collection[key] = value */                                                          \
    MACRO(setItem, (GC * gc, Value * collection, Value * key, Value * value), __VA_ARGS__) \
    /* ret = del collection[key] */                                                        \
    MACRO(delItem, (GC * gc, Value * ret, Value * collection, Value * key), __VA_ARGS__)   \
    /* append(collection, item) */                                                         \
    MACRO(append, (GC * gc, Value * collection, Value * item), __VA_ARGS__)                \
    /* extend(collection, iterable) */                                                     \
    MACRO(extend, (GC * gc, Value * collection, Value * iterable), __VA_ARGS__)            \
    /* ret = pop(collection) */                                                            \
    MACRO(pop, (GC * gc, Value * ret, Value * collection), __VA_ARGS__)

#endif /* SEMI_PRIMITIVE_X_MACRO_H */
