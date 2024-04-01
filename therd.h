#pragma once

struct Builder* builder(void);
struct Program* compile(struct Builder*);
void            execute(struct Program const*, int n, void* ptr[]);

void store(struct Builder*, int);
void load (struct Builder*, int);
void splat(struct Builder*, float);

void mul(struct Builder*);
void add(struct Builder*);
