// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_COMMON_H
#define SEMI_COMMON_H

#include <limits.h>
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

static inline uint32_t nextPowerOfTwoCapacity(uint32_t x) {
    if (x <= 8) {
        return 8;
    }

#if __has_builtin(__builtin_clzl)
    // From https://gcc.gnu.org/onlinedocs/gcc/Bit-Operation-Builtins.html
    //
    //   * __builtin_clz:  Returns the number of leading 0-bits in x, starting at the most
    //                     significant bit position. If x is 0, the result is undefined.
    //   * __builtin_clzl: Similar to __builtin_clz, except the argument type is unsigned long.
    return (uint32_t)1 << (sizeof(unsigned long) * CHAR_BIT - (size_t)__builtin_clzl(x));
#else

    // From https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    return x;
#endif
}

#endif /* SEMI_COMMON_H */
