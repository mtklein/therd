#pragma once
#include <stddef.h>

size_t builder_size(int insts);
struct Builder* builder(void*, size_t);

void store(struct Builder*, int);
void load (struct Builder*, int);
void uni  (struct Builder*, int);
void imm  (struct Builder*, float);

void mul(struct Builder*);
void add(struct Builder*);

struct Program* done(struct Builder*);
void run(struct Program const*, int n, void* ptr[]);
