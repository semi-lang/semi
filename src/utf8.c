// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#include "./utf8.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "./semi_common.h"

/**
 * Returns the expected number of bytes in a UTF-8 sequence based on the first
 * byte. If the first byte is invalid, returns 0.
 */
static unsigned int semiUTF8CodepointSize_unsafe(unsigned char b) {
    if ((b & 0x80) == 0) {
        // 0xxxxxxx (ASCII)
        return 1;
    } else if (b <= 0xC1) {
        // 10xxxxxx, 11000000 (0xC0), 11000001 (0xC1)
        return 0;
    } else if (b <= 0xDF) {
        // 110xxxxx (excluding 0xC0, 0xC1)
        return 2;
    } else if (b <= 0xEF) {
        // 1110xxxx
        return 3;
    } else if (b <= 0xF4) {
        // 11110xxx (where x can't make it 0xF5 or higher)
        return 4;
    }

    return 0;
}

/**
 * Decode the next UTF-8 code point from *p, up to 'length' bytes.
 * Advances *p by the number of bytes consumed.
 * Returns the Unicode code point, or 0 on error.
 */
Codepoint semiUTF8NextCodepoint(const char** p, size_t length) {
    if (length == 0) {
        return EOZ;
    }
    const char* s    = *p;
    unsigned char b0 = (unsigned char)s[0];

    // 1-byte (ASCII)
    if (b0 < 0x80) {
        *p = s + 1;
        return (Codepoint)b0;
    }

    size_t expected = (size_t)semiUTF8CodepointSize_unsafe(b0);
    if (expected == 0 || expected > length) {
        return EOZ;
    }

    // Decode continuation bytes
    uint32_t codepoint = 0;
    switch (expected) {
        case 2:
            codepoint = b0 & 0x1F;
            break;  // 110xxxxx -> 5 bits
        case 3:
            codepoint = b0 & 0x0F;
            break;  // 1110xxxx -> 4 bits
        case 4:
            codepoint = b0 & 0x07;
            break;  // 11110xxx -> 3 bits
        default:
            break;
    }
    for (unsigned int i = 1; i < expected; ++i) {
        char bx = s[i];
        if ((bx & 0xC0) != 0x80) {
            return EOZ;
        }
        codepoint = (codepoint << 6) | (bx & 0x3F);
    }

    // Reject surrogates and overlongs
    if ((expected == 3 && codepoint >= 0xD800 && codepoint <= 0xDFFF) || (codepoint < 0x80) ||
        (codepoint < 0x800 && expected > 2) || (codepoint < 0x10000 && expected > 3) || (codepoint > 0x10FFFF)) {
        return EOZ;
    }

    *p = s + expected;
    return codepoint;
}

Codepoint semiUTF8CodepointAt(const char* str, size_t length, size_t index) {
    const char* p   = str;
    const char* end = str + length;
    for (size_t i = 0; i < index; i++) {
        semiUTF8NextCodepoint(&p, (size_t)(end - p));
    }

    return semiUTF8NextCodepoint(&p, (size_t)(end - p));
}

unsigned int semiUTF8EncodeCodepoint(Codepoint value, char* writer) {
    // See also http://tools.ietf.org/html/rfc3629

    if (value <= 0x7f) {
        // 0xxxxxxx
        *writer = value & 0x7f;
        return 1;
    } else if (value <= 0x7ff) {
        // 110xxxxx 10xxxxxx
        writer[0] = (char)(0xc0 | ((value >> 6) & 0x1f));
        writer[1] = (char)(0x80 | (value & 0x3f));
        return 2;
    } else if (value <= 0xffff) {
        // 1110xxxx 10xxxxxx 10xxxxxx
        writer[0] = (char)(0xe0 | ((value >> 12) & 0x0f));
        writer[1] = (char)(0x80 | ((value >> 6) & 0x3f));
        writer[2] = (char)(0x80 | (value & 0x3f));
        return 3;
    } else if (value <= 0x10ffff) {
        // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        writer[0] = (char)(0xf0 | ((value >> 18) & 0x07));
        writer[1] = (char)(0x80 | ((value >> 12) & 0x3f));
        writer[2] = (char)(0x80 | ((value >> 6) & 0x3f));
        writer[3] = (char)(0x80 | (value & 0x3f));
        return 4;
    }

    // Invalid codepoint
    SEMI_UNREACHABLE();
    return 0;
}
