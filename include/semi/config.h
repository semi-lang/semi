#ifndef SEMI_CONFIG_H
#define SEMI_CONFIG_H

// Each VM has its own heap-allocated stack that grows and shrinks dynamically. These constants
// define the limits of the stack size. Note that currently the size of a stack slot is 2 words
// after padding.
//
// Also, this doesn't include heap-allocated objects managed by the GC.

#define SEMI_MIN_STACK_SIZE (1 << 8)
#define SEMI_MAX_STACK_SIZE (1 << 20)

// Frame information is stored in a separate, heap-allocated array. These constants
// define the limits of the frame size.

#define SEMI_MIN_FRAME_SIZE (1 << 2)
#define SEMI_MAX_FRAME_SIZE (1 << 10)

#endif /* SEMI_CONFIG_H */
