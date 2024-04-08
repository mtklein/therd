#pragma once
#include <stddef.h>

size_t program_size(int insts);
struct Program* program(void*, size_t);

struct Program* store(struct Program*, int);
struct Program* load (struct Program*, int);
struct Program* uni  (struct Program*, int);
struct Program* imm  (struct Program*, float);

struct Program* mul(struct Program*);
struct Program* add(struct Program*);

void run(struct Program const*, int n, void* ptr[]);
