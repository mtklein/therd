#pragma once
#include <stddef.h>

struct allocator {
    void* (*realloc)(void*, size_t, void *ctx);
    void *ctx;
};

struct Program* program(struct allocator, void *init);

struct Program* store(struct Program*, int);
struct Program* load (struct Program*, int);
struct Program* uni  (struct Program*, int);
struct Program* imm  (struct Program*, float);

struct Program* mul(struct Program*);
struct Program* add(struct Program*);

void run(struct Program const*, int n, void* ptr[]);
