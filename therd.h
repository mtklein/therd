#pragma once

struct Builder* builder(void);
struct Inst*    compile(struct Builder*);
void            start(struct Inst const*, int n, void* ptr[]);

void store(struct Builder*, int);
void load (struct Builder*, int);
void uni  (struct Builder*, int);
void imm  (struct Builder*, float);

void mul(struct Builder*);
void add(struct Builder*);
