#pragma once
#include <stddef.h>

// Therd is a toy, hack, demo!
// The LICENSE file is serious when it says it's not fit for anything!


// Therd is an interpreter for a toy stack language, the name a play on Forth.
// The main design choices lean towards maximizing performance without needing
// a JIT or inline assembly and while still being embedded in C:
//
//   - threaded code (in the interpreter sense) using tail calls;
//   - SIMT execution model allowing pervasive vector instructions;
//   - late-bound pointer parameters let you reuse programs on new data;
//   - stack depth capped at 8 so it can live entirely in registers.
//
// That last point is the project's raison d'etre, something I've never tried.


// Therd's API is a very sharp knife, and you need to do some bookkeeping
// yourself to use it correctly.  Things to keep in mind or they may bite you:
//
//   A) memory management is your job, and to do it right you probably need to
//      know ahead of time how many instructions you'll add to your program;
//   B) everything will explode the instant you exceed 8 elements on the stack.

// The general lifecycle is:
//   1) allocate enough memory to fit your program using builder_size();
//   2) pass that memory and its size into builder() to get started;
//   3) ... add instructions using calls on the Builder ...
//   4) call done() to convert that Builder into a runnable program
//   5) run() the program as you like.
//   6) clean up whatever you allocated in 1).
//
// The tests in therd_test.c demonstrate all of this.


// 1) How many bytes does builder() need to fit this many instructions?
size_t builder_size(int insts);

// 2) Use this block of memory to build a program.  Align to _Alignof(void*).
struct Builder* builder(void*, size_t);

// 3) ... all instructions are below run() ...

// 4) Finalize a program, making it runnable.
struct Program* done(struct Builder*);

// 5) Run N instances of a program, using these pointers.  Thread-safe.
void run(struct Program const*, int N, void* ptr[]);


// 3) Instructions!  Count these calls to know what to pass to builder_size().

// Therd uses an execution model that's similar to a GPU, running N independent
// instances of one single program.  Several acronyms apply: SIMT, SIMD, SPMD.
// If you want to ignore all that and use it as a simple scalar interpreter,
// you can pass N=1.
//
// For best performance, let each run() handle as much as possible, with N set
// to, e.g., the number of pixels in a row of an image you're drawing.  When
// N>1 you'll need to think about different kinds of values, distinguished by
// how they may vary between program instances:
//
//   - varyings are values that may vary between different program instances;
//   - uniforms may change between calls to run() but are the same for every
//     program instance during a run(), sort of run-time constant;
//   - immediates are compile-time constants.
//
// Whether varying, uniform, or immediate, all Therd values are 32-bit floats.


// store() and load() let you work with arrays of floats in memory as varyings.
// Given a float *ptr, they store or load from ptr[0], ptr[1], ... ptr[N-1].
//
// uni() is like load() but uniform, always loading the same float, ptr[0].
//
// These instructions take an index into the ptr[] you'll pass to run() later.
// This makes it easier to reuse Programs with new data.
void store(struct Builder*, int ptr);  // Pop a varying, write it to a float[].
void  load(struct Builder*, int ptr);  // Push a varying loaded from a float[].
void   uni(struct Builder*, int ptr);  // Push a uniform loaded from a float*.

void imm(struct Builder*, float imm);  // Push an immediate onto the stack.

void id(struct Builder*);              // Push varying program instance ID.

void mul(struct Builder*);  // Pop top two values, multiply them, push result.
void add(struct Builder*);  //      "                    add           "
