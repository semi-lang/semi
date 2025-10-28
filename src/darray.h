// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./gc.h"
#include "semi/error.h"

#define DECLARE_DARRAY(NAME, ITEM_TYPE, INDEX_TYPE)                       \
    typedef struct NAME {                                                 \
        ITEM_TYPE* data;                                                  \
        INDEX_TYPE size;                                                  \
        INDEX_TYPE capacity;                                              \
    } NAME;                                                               \
                                                                          \
    void NAME##Init(NAME* arr);                                           \
                                                                          \
    void NAME##Cleanup(GC* gc, NAME* arr);                                \
                                                                          \
    ErrorId NAME##EnsureCapacity(GC* gc, NAME* arr, INDEX_TYPE capacity); \
                                                                          \
    ErrorId NAME##Append(GC* gc, NAME* arr, ITEM_TYPE value);

#define DEFINE_DARRAY(NAME, ITEM_TYPE, INDEX_TYPE, MAX_CAPACITY)                                                 \
                                                                                                                 \
    void NAME##Init(NAME* arr) {                                                                                 \
        arr->data     = NULL;                                                                                    \
        arr->size     = 0;                                                                                       \
        arr->capacity = 0;                                                                                       \
    }                                                                                                            \
                                                                                                                 \
    void NAME##Cleanup(GC* gc, NAME* arr) {                                                                      \
        semiFree(gc, arr->data, sizeof(ITEM_TYPE) * arr->capacity);                                              \
        arr->data     = NULL;                                                                                    \
        arr->size     = 0;                                                                                       \
        arr->capacity = 0;                                                                                       \
    }                                                                                                            \
                                                                                                                 \
    ErrorId NAME##EnsureCapacity(GC* gc, NAME* arr, INDEX_TYPE capacity) {                                       \
        if (arr->capacity >= capacity) {                                                                         \
            return 0;                                                                                            \
        }                                                                                                        \
                                                                                                                 \
        if (capacity > MAX_CAPACITY) {                                                                           \
            return SEMI_ERROR_REACH_ALLOCATION_LIMIT;                                                            \
        }                                                                                                        \
                                                                                                                 \
        INDEX_TYPE new_capacity = 8;                                                                             \
        while (new_capacity < capacity && new_capacity <= MAX_CAPACITY / 2) {                                    \
            new_capacity *= 2;                                                                                   \
        }                                                                                                        \
        if (new_capacity > MAX_CAPACITY) {                                                                       \
            new_capacity = MAX_CAPACITY;                                                                         \
        }                                                                                                        \
                                                                                                                 \
        ITEM_TYPE* temp =                                                                                        \
            semiRealloc(gc, arr->data, arr->capacity * sizeof(ITEM_TYPE), new_capacity * sizeof(ITEM_TYPE));     \
        if (temp == NULL) {                                                                                      \
            return SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;                                                         \
        }                                                                                                        \
        arr->data     = temp;                                                                                    \
        arr->capacity = new_capacity;                                                                            \
        return 0;                                                                                                \
    }                                                                                                            \
                                                                                                                 \
    ErrorId NAME##Append(GC* gc, NAME* arr, ITEM_TYPE value) {                                                   \
        if (arr->size >= arr->capacity) {                                                                        \
            if (arr->capacity == MAX_CAPACITY) {                                                                 \
                return SEMI_ERROR_REACH_ALLOCATION_LIMIT;                                                        \
            }                                                                                                    \
                                                                                                                 \
            INDEX_TYPE new_capacity;                                                                             \
            if (arr->capacity > MAX_CAPACITY / 2) {                                                              \
                new_capacity = MAX_CAPACITY;                                                                     \
            } else {                                                                                             \
                new_capacity = (arr->capacity == 0) ? 8 : arr->capacity * 2;                                     \
                if (new_capacity > MAX_CAPACITY) {                                                               \
                    new_capacity = MAX_CAPACITY;                                                                 \
                }                                                                                                \
            }                                                                                                    \
                                                                                                                 \
            ITEM_TYPE* temp =                                                                                    \
                semiRealloc(gc, arr->data, arr->capacity * sizeof(ITEM_TYPE), new_capacity * sizeof(ITEM_TYPE)); \
            if (temp == NULL) {                                                                                  \
                return SEMI_ERROR_MEMORY_ALLOCATION_FAILURE;                                                     \
            }                                                                                                    \
            arr->data     = temp;                                                                                \
            arr->capacity = new_capacity;                                                                        \
        }                                                                                                        \
        arr->data[arr->size++] = value;                                                                          \
        return 0;                                                                                                \
    }
