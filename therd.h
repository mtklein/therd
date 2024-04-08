#pragma once
#include <stddef.h>

size_t program_size(int insts);
struct Program* program(void*, size_t);

void store(struct Program*, int);
void load (struct Program*, int);
void uni  (struct Program*, int);
void imm  (struct Program*, float);

void mul(struct Program*);
void add(struct Program*);

void run(struct Program const*, int n, void* ptr[]);
