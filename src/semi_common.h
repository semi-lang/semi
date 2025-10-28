// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_COMMON_H
#define SEMI_COMMON_H

#include <stdint.h>

#define SEMI_EXPORT __attribute__((visibility("default")))

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if __has_builtin(__builtin_expect)
#define SEMI_LIKELY(expr)   (__builtin_expect(((expr) != 0), 1))
#define SEMI_UNLIKELY(expr) (__builtin_expect(((expr) != 0), 0))
#else
#define SEMI_LIKELY(expr)   (expr)
#define SEMI_UNLIKELY(expr) (expr)
#endif

#if defined(SEMI_DEBUG)
#include <stdio.h>
#include <stdlib.h>

static void semiDebugPrintRegisters(uint64_t* mask) {
    for (int i = 0; i < 32 / (int)sizeof(*mask); i++) {
        uint64_t pattern = (uint64_t)1 << (sizeof(*mask) * 8 - 1);
        while (pattern) {
            if (*mask & pattern) printf("1");
            else printf("0");
            pattern >>= 1;
        }
        mask++;
        printf("\n");
    }
}
#define SEMI_DEBUG_PRINT_REGISTERS(mask) semiDebugPrintRegisters(mask)

static void semiDebugAssertionFailure(const char* file, int line, const char* msg) {
    fprintf(stderr, "Assertion failed at %s:%d: %s\n", file, line, msg);
    abort();
}
#define ASSERT(condition, msg) (!(condition) ? semiDebugAssertionFailure(__FILE__, __LINE__, msg) : (void)0)

#else /* SEMI_DEBUG */
#define SEMI_DEBUG_PRINT_REGISTERS(mask) (void)0

#define ASSERT(condition, msg) \
    do {                       \
    } while (false)

#endif /* SEMI_DEBUG */

#if __has_builtin(__builtin_unreachable)
#define SEMI_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
#define SEMI_UNREACHABLE() __assume(0)
#else
#include <stdlib.h>
#define SEMI_UNREACHABLE()                    \
    ASSERT(true, "Reached unreachable code"); \
    abort()
#endif  // __has_builtin(__builtin_unreachable)

#endif /* SEMI_COMMON_H */
