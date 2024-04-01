#pragma once

struct Builder* builder(void);
struct Program* compile(struct Builder*);
void            execute(struct Program const*, int n);

void store(struct Builder*, float      *);
void load (struct Builder*, float const*);
void splat(struct Builder*, float);

void mul(struct Builder*);

