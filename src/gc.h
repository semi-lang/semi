// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_GC_H
#define SEMI_GC_H

#include <stdint.h>

#include "semi/semi.h"

typedef enum {
    OBJECT_TYPE_STRING,
    OBJECT_TYPE_RANGE,
    OBJECT_TYPE_LIST,
    OBJECT_TYPE_DICT,
    OBJECT_TYPE_UPVALUE,
    OBJECT_TYPE_FUNCTION,
} ObjectType;

// In order to save space, we encode the gc state `isReachable` - whether the object is accessible
// from the gc roots - in the top most bit of the ObjectType
#define OBJECT_HEADER_TYPE_MASK (~((uint32_t)0) >> 1)
#define OBJECT_HEADER_GC_MASK   (~((uint32_t)0) ^ OBJECT_HEADER_TYPE_MASK)
#define OBJECT_TYPE(obj)        ((ObjectType)((obj->header) & OBJECT_HEADER_TYPE_MASK))

#define IS_REACHABLE_OBJECT(obj) (((obj)->header) & OBJECT_HEADER_GC_MASK)
#define MARK_OBJECT_REACHABLE(obj)              \
    do {                                        \
        (obj)->header |= OBJECT_HEADER_GC_MASK; \
    } while (0)
#define UNMARK_OBJECT_REACHABLE(obj)                 \
    do {                                             \
        (obj)->header &= (~OBJECT_HEADER_TYPE_MASK); \
    } while (0)

// Object is the base struct for all GC-managed objects. It must be the first field of all structs that can be managed
// by GC.
typedef struct Object {
    uint32_t header;

    struct Object* next;
    struct Object* grayNext;
} Object;

// A standard mark-and-sweep garbage collector.
typedef struct GC {
    SemiReallocateFn reallocateFn;
    void* reallocateUserData;

    struct Object* head;
    struct Object* grayHead;

    size_t allocatedSize;
    size_t gcThreshold;
} GC;

void* semiMalloc(GC* gc, size_t size);
void semiFree(GC* gc, void* ptr, size_t oldSize);
void* semiRealloc(GC* gc, void* ptr, size_t oldSize, size_t newSize);

void semiGCInit(GC* gc, SemiReallocateFn reallocFn, void* reallocateUserData);
void semiGCCleanup(GC* gc);
static inline void semiGCAttachObject(GC* gc, Object* obj) {
    obj->next = gc->head;
    gc->head  = obj;
}

void semiGCMarkAndSweep(GC* gc);
void semiGCFreeObject(GC* gc, struct Object* object);

#endif /* SEMI_GC_H */
