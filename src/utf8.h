// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_UTF8_H
#define SEMI_UTF8_H

#include <stddef.h>
#include <stdint.h>

// End of stream
#define EOZ 0

typedef uint32_t Codepoint;

Codepoint semiUTF8NextCodepoint(const char** p, size_t length);

Codepoint semiUTF8CodepointAt(const char* str, size_t length, size_t index);

unsigned int semiUTF8EncodeCodepoint(Codepoint value, char* writer);
#endif  // SEMI_UTF8_H
