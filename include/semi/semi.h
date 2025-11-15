// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_H
#define SEMI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "semi/error.h"

#define SEMI_EXPORT __attribute__((visibility("default")))

#define SEMI_VERSION_MAJOR  0
#define SEMI_VERSION_MINOR  0
#define SEMI_VERSION_PATCH  1
#define SEMI_VERSION_STRING "0.0.1"
#define SEMI_VERSION_NUMBER (SEMI_VERSION_MAJOR * 1000000 + SEMI_VERSION_MINOR * 1000 + SEMI_VERSION_PATCH)

// A virtual machine for the Semi programming language.
//
// SemiVM doesn't have any global state. All of the VM state and configuration are stored in this
// struct.
typedef struct SemiVM SemiVM;

typedef struct SemiModuleSource {
    // The source code of the module.
    const char* source;

    // The length of the source code.
    unsigned int length;

    // The name of the module. The name must be a valid identifier, and must be unique across all
    // modules in the same VM instance.
    const char* name;

    // The length of the name.
    uint8_t nameLength;
} SemiModuleSource;

typedef struct SemiModule SemiModule;

// The allocation function SemiVM uses for memory management. It will be called whem SemiVM
//
// * requests for new memory (malloc): `ptr` is `NULL`, `size` is the size to allocate. It must
//   return a pointer to the allocated memory on success, or `NULL` on failure.
//
// * free memory (free): `ptr` is the pointer to free, `size` is 0. It must return `NULL`. If `ptr`
//   is `NULL`, the function must do nothing and return `NULL`.
//
// * request for growing/shrinking memory (realloc): `ptr` is the pointer to reallocate, `size` is
//   the new size. For growing, it must return a pointer to the reallocated memory on success, or
//   `NULL` on failure. The old memory must not be freed when it fails to reallocate.
//
// The `reallocData` parameter is user-defined data that can be used to pass additional context to
// the allocation function. It is passed to the SemiVM when it is created and never modified by
// SemiVM.
typedef void* (*SemiReallocateFn)(void* ptr, size_t size, void* reallocData);

// TODO: Setting this in `SemiVMConfig` allows SemiVM to import modules by module names.
// typedef SemiModuleSource* (*SemiModuleSourceImportFn)(const char* moduleName, size_t moduleNameLength);

// TODO: Setting this in `SemiVMConfig` allows SemiVM to resolve modules.
// typedef void (*SemiModuleResolveFn)(const char* sourceModulePath,
//                                     size_t sourceModulePathLength,
//                                     const char* targetModulePath,
//                                     size_t targetModulePathLength,
//                                     const char** moduleName,
//                                     size_t* moduleNameLength);

// Functions that can be called from SemiVM.
typedef void (*SemiExteralFunction)(SemiVM* vm, uint8_t argsCount);

typedef struct SemiVMConfig {
    // The allocation function to use for memory management. Search `defaultReallocFn` for the
    // default implementation.
    SemiReallocateFn reallocateFn;

    // The function for importing modules on demand. If this is `NULL`, the VM will not support
    // module imports. The function should return a pointer to a `SemiModuleSource` struct that
    // contains the source code of the module. The VM will not modify the source code. The memory of
    // the `SemiModuleSource` struct can be freed after the compilation of the module is done. If
    // the module is not found, the function should return `NULL`, which triggers
    // `SEMI_ERROR_MODULE_NOT_FOUND`.
    //
    // Since the VM caches the compiled modules, the function will only be called once for each
    // unique module name.
    // SemiModuleSourceImportFn moduleSourceImportFn;

    // This function is used to resolve modules by their (relative) paths. It is called when the VM
    // encounters an import statement. The only limitation of the module name is that it must be
    // unique across all modules in the same VM instance.
    // SemiModuleResolveFn moduleResolveFn;

    // User-defined data to pass to the allocation function.
    void* reallocateUserData;
} SemiVMConfig;

// Initializes the configuration with default values.
SEMI_EXPORT void semiInitConfig(SemiVMConfig* config);

// Creates a new VM with the given configuration. When `NULL` is passed, the default configuration
// is used.
SEMI_EXPORT SemiVM* semiCreateVM(SemiVMConfig* config);

// Free all resources associated with the VM.
SEMI_EXPORT void semiDestroyVM(SemiVM* vm);

typedef struct SemiCompileErrorDetails {
    unsigned int line;
    size_t column;
} SemiCompileErrorDetails;

typedef struct SemiRuntimeErrorDetails {
    // TODO: add more runtime error details
    unsigned int dummy;
} SemiRuntimeErrorDetails;

// Compiles the source code of a module. On success, add the module to the VM's module
// list.
//
// TODO: If `transitive` is `true`, modules transistively imported by this module will also be
// compiled and added to the VM's module list using a BFS approach.
SEMI_EXPORT ErrorId semiVMAddModule(SemiVM* vm, SemiModuleSource moduleSource, bool transitive);

// Runs the module with the given name. The module must have been added to the VM using
// `semiVMAddModule()`.
SEMI_EXPORT ErrorId semiRunModule(SemiVM* vm, const char* moduleName, uint8_t moduleNameLength);

/* TODO: Snapshot the VM state for quick resuming later.

ErrorId semiVMSnapshot(SemiVM* vm, void** snapshot, size_t* size);

ErrorId semiVMResume(SemiVM* vm, const void* snapshot, size_t size);
 */

/*
 │ FFI
─┴───────────────────────────────────────────────────────────────────────────────────────────────*/

// TODO: Implement FFI

// ErrorId SemiRegisterExternalFunction(
//     SemiVM* vm, const char* name, size_t nameLength, SemiExteralFunction fn, uint8_t argsCount);

// int64_t SemiFFIGetArgumentInt(SemiVM* vm, uint8_t index);
// double SemiFFIGetArgumentFloat(SemiVM* vm, uint8_t index);

// // TODO: Support more types, especially those that require GC.

// ErrorId SemiFFIReturnInt(SemiVM* vm, int64_t value);
// ErrorId SemiFFIReturnFloat(SemiVM* vm, double value);

#endif /* SEMI_H */
