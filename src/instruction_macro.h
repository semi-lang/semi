// Copyright (c) 2025 Ian Chen
// SPDX-License-Identifier: MPL-2.0

#ifndef SEMI_INSTRUCTION_MACRO_H
#define SEMI_INSTRUCTION_MACRO_H

#define MAKE_INSTRUCTION_MACRO_K(...) FOR_EACH(MAKE_INSTRUCTION_K, __VA_ARGS__)

#define MAKE_INSTRUCTION_MACRO_T(...) FOR_EACH(MAKE_INSTRUCTION_T, __VA_ARGS__)

#define MAKE_INSTRUCTION_MACRO_J(...) FOR_EACH(MAKE_INSTRUCTION_J, __VA_ARGS__)

#define MAKE_INSTRUCTION_FOR_EACH_1(macro, x)                macro(x)
#define MAKE_INSTRUCTION_FOR_EACH_2(macro, x, y)             macro(x) macro(y)
#define MAKE_INSTRUCTION_FOR_EACH_3(macro, x, y, z)          macro(x) macro(y) macro(z)
#define MAKE_INSTRUCTION_FOR_EACH_4(macro, a, b, c, d)       macro(a) macro(b) macro(c) macro(d)
#define MAKE_INSTRUCTION_FOR_EACH_5(macro, a, b, c, d, e)    macro(a) macro(b) macro(c) macro(d) macro(e)
#define MAKE_INSTRUCTION_FOR_EACH_6(macro, a, b, c, d, e, f) macro(a) macro(b) macro(c) macro(d) macro(e) macro(f)
#define MAKE_INSTRUCTION_FOR_EACH_7(macro, a, b, c, d, e, f, g)    \
    macro(a) macro(b) macro(c) macro(d) macro(e) macro(f) macro(g)
#define MAKE_INSTRUCTION_FOR_EACH_8(macro, a, b, c, d, e, f, g, h)          \
    macro(a) macro(b) macro(c) macro(d) macro(e) macro(f) macro(g) macro(h)
#define MAKE_INSTRUCTION_FOR_EACH_9(macro, a, b, c, d, e, f, g, h, i)                \
    macro(a) macro(b) macro(c) macro(d) macro(e) macro(f) macro(g) macro(h) macro(i)
#define MAKE_INSTRUCTION_FOR_EACH_10(macro, a, b, c, d, e, f, g, h, i, j)                     \
    macro(a) macro(b) macro(c) macro(d) macro(e) macro(f) macro(g) macro(h) macro(i) macro(j)

#define GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, NAME, ...) NAME
#define FOR_EACH(macro, ...)                \
    GET_MACRO(__VA_ARGS__,                  \
              MAKE_INSTRUCTION_FOR_EACH_10, \
              MAKE_INSTRUCTION_FOR_EACH_9,  \
              MAKE_INSTRUCTION_FOR_EACH_8,  \
              MAKE_INSTRUCTION_FOR_EACH_7,  \
              MAKE_INSTRUCTION_FOR_EACH_6,  \
              MAKE_INSTRUCTION_FOR_EACH_5,  \
              MAKE_INSTRUCTION_FOR_EACH_4,  \
              MAKE_INSTRUCTION_FOR_EACH_3,  \
              MAKE_INSTRUCTION_FOR_EACH_2,  \
              MAKE_INSTRUCTION_FOR_EACH_1,  \
              dummy)(macro, __VA_ARGS__)

#endif /* SEMI_INSTRUCTION_MACRO_H */
