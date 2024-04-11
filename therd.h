#pragma once
#include <stddef.h>

size_t program_size(int insts);
struct Program* program(void*, size_t);
void run(struct Program const*, int N, void* ptr[]);

void store(struct Program*, int);
void  load(struct Program*, int);
void   uni(struct Program*, int);
void   imm(struct Program*, float);
void    id(struct Program*);
void   mul(struct Program*);
void   add(struct Program*);
void  done(struct Program*);
