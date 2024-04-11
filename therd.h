#pragma once
#include <stddef.h>

size_t          program_size(int insts);
struct program* program(void*, size_t);
void            execute(struct program const*, int n, void* ptr[]);

void store(struct program*, int);
void  load(struct program*, int);
void   uni(struct program*, int);
void   imm(struct program*, float);
void    id(struct program*);
void   mul(struct program*);
void   add(struct program*);
void  done(struct program*);
