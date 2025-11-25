// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./gc.h"

#include <string.h>

#include "./semi_common.h"
#include "./value.h"
#include "./vm.h"

void semiGCInit(GC* gc, SemiReallocateFn reallocFn, void* reallocateUserData) {
    *gc = (GC){
        .reallocateFn       = reallocFn,
        .reallocateUserData = reallocateUserData,
        .head               = NULL,
        .grayHead           = NULL,
        .allocatedSize      = 0,
    };
}

void semiGCCleanup(GC* gc) {
    Object* current = gc->head;
    while (current) {
        Object* next = current->next;
        semiGCFreeObject(gc, current);
        current = next;
    }
    gc->head = NULL;
}

void* semiMalloc(GC* gc, size_t size) {
    gc->allocatedSize += size;
    if (gc->allocatedSize > gc->gcThreshold) {
        // If the new allocation exceeds the threshold, we need to run GC first.
        // TODO: mark and sweep
    }

    return gc->reallocateFn(NULL, size, gc->reallocateUserData);
}

void semiFree(GC* gc, void* ptr, size_t oldSize) {
    if (ptr == NULL) {
        return;
    }
    gc->allocatedSize -= oldSize;
    gc->reallocateFn(ptr, 0, gc->reallocateUserData);
    return;
}

// Malloc/Free/Realloc but with usage tracking. MUST NOT use this during sweep phase. Instead, this
// will start GC if the allocated size exceeds the threshold.
void* semiRealloc(GC* gc, void* ptr, size_t oldSize, size_t newSize) {
    gc->allocatedSize += newSize;
    gc->allocatedSize -= oldSize;
    if (gc->allocatedSize > gc->gcThreshold) {
        // If the new allocation exceeds the threshold, we need to run GC first.
        // TODO: mark and sweep
    }

    return gc->reallocateFn(ptr, newSize, gc->reallocateUserData);
}

static void semiGCGrayObject(GC* gc, Object* obj) {
gray_begin:
    if (obj == NULL || IS_REACHABLE_OBJECT(obj)) {
        return;
    }
    MARK_OBJECT_REACHABLE(obj);

    switch (OBJECT_TYPE(obj)) {
        // These objects do not have references to other objects, so we don't need to gray them.
        case OBJECT_TYPE_STRING:
            return;

        // These objects may contain references to other objects, so we need to gray them.
        case OBJECT_TYPE_RANGE:
        case OBJECT_TYPE_LIST:
        case OBJECT_TYPE_DICT:
        case OBJECT_TYPE_FUNCTION:
            break;

        // Upvalue may reference another object.
        case OBJECT_TYPE_UPVALUE: {
            ObjectUpvalue* upvalue = (ObjectUpvalue*)obj;
            if (IS_OBJECT(upvalue->value)) {
                obj = AS_OBJECT(upvalue->value);
                goto gray_begin;
            }
            return;
        }

        default:
            SEMI_UNREACHABLE();
            break;
    }

    obj->grayNext = gc->grayHead;
    gc->grayHead  = obj;
}

static inline void grayValue(GC* gc, Value* value) {
    if (IS_OBJECT(value)) {
        semiGCGrayObject(gc, AS_OBJECT(value));
    }
}

static void semiGCMarkRoots(SemiVM* vm) {
    for (uint32_t i = 0; i < vm->valueCount; i++) {
        grayValue(&vm->gc, &vm->values[i]);
    }

    for (uint32_t i = 0; i < vm->frameCount; i++) {
        Frame* frame = &vm->frames[i];
        semiGCGrayObject(&vm->gc, (Object*)frame->function);
        for (uint8_t j = 0; j < frame->function->upvalueCount; j++) {
            semiGCGrayObject(&vm->gc, (Object*)frame->function->upvalues[j]);
        }
    }

    for (uint16_t i = 0; i < vm->modules.len; i++) {
        SemiModule* module = AS_PTR(&vm->modules.values[i], SemiModule);

        semiGCGrayObject(&vm->gc, (Object*)&module->exports);
        semiGCGrayObject(&vm->gc, (Object*)&module->globals);
        semiGCGrayObject(&vm->gc, (Object*)module->constantTable.constantMap);
    }

    if (vm->globalConstants != NULL) {
        for (uint16_t i = 0; i < vm->globalIdentifiers.size; i++) {
            grayValue(&vm->gc, &vm->globalConstants[i]);
        }
    }
}

void semiGCMarkAndSweep(GC* gc) {
    gc->grayHead = NULL;

    // GC must be the first field of SemiVM
    SemiVM* vm = (SemiVM*)gc;

    // Mark phase
    semiGCMarkRoots(vm);

    while (gc->grayHead != NULL) {
        Object* obj  = gc->grayHead;
        gc->grayHead = obj->grayNext;

    gray_object:
        if (IS_REACHABLE_OBJECT(obj)) {
            continue;
        }
        MARK_OBJECT_REACHABLE(obj);

        switch (OBJECT_TYPE(obj)) {
            case OBJECT_TYPE_STRING:
            case OBJECT_TYPE_RANGE:
                break;

            case OBJECT_TYPE_LIST: {
                ObjectList* list = (ObjectList*)obj;
                for (uint32_t i = 0; i < list->size; i++) {
                    grayValue(gc, &list->values[i]);
                }
                break;
            }
            case OBJECT_TYPE_DICT: {
                ObjectDict* dict = (ObjectDict*)obj;
                for (uint32_t i = 0; i < dict->used; i++) {
                    grayValue(gc, &dict->keys[i].key);
                    grayValue(gc, &dict->values[i]);
                }
                break;
            }

            case OBJECT_TYPE_FUNCTION: {
                ObjectFunction* function = (ObjectFunction*)obj;
                for (uint8_t i = 0; i < function->upvalueCount; i++) {
                    semiGCGrayObject(gc, (Object*)function->upvalues[i]);
                }
                break;
            }

            case OBJECT_TYPE_UPVALUE: {
                ObjectUpvalue* upvalue = (ObjectUpvalue*)obj;
                obj                    = AS_OBJECT(upvalue->value);
                if (obj != NULL && !IS_REACHABLE_OBJECT(obj)) {
                    goto gray_object;
                }
                break;
            }

            default:
                SEMI_UNREACHABLE();
                break;
        }
    }

    // Sweep phase
    Object** current = &gc->head;
    while (*current != NULL) {
        if (!IS_REACHABLE_OBJECT(*current)) {
            // Not marked, free it
            Object* next = (*current)->next;
            semiGCFreeObject(gc, *current);
            *current = next;
        } else {
            // Marked, whiten it
            UNMARK_OBJECT_REACHABLE(*current);
            current = &(*current)->next;
        }
    }
}

void semiGCFreeObject(GC* gc, Object* obj) {
    switch (OBJECT_TYPE(obj)) {
        case OBJECT_TYPE_STRING: {
            semiObjectStringDestroy(gc, (ObjectString*)obj);
            break;
        }
        case OBJECT_TYPE_RANGE:
            semiObjectRangeDestroy(gc, (ObjectRange*)obj);
            break;

        case OBJECT_TYPE_LIST: {
            semiObjectListDestroy(gc, (ObjectList*)obj);
            break;
        }

        case OBJECT_TYPE_DICT: {
            semiObjectDictDestroy(gc, (ObjectDict*)obj);
            break;
        }

        case OBJECT_TYPE_UPVALUE: {
            semiObjectUpvalueDestroy(gc, (ObjectUpvalue*)obj);
            break;
        }

        case OBJECT_TYPE_FUNCTION: {
            semiObjectFunctionDestroy(gc, (ObjectFunction*)obj);
            break;
        }

        default:
            SEMI_UNREACHABLE();
            break;
    }
}
