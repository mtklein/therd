#pragma once
#include <stdint.h>

typedef float __attribute__(( vector_size(32), aligned(4) )) F;

struct inst {
    void (*fn)(struct inst const*, int, F*, int, F*);
    union { float imm; void *ptr; void const *cptr; };
};

extern struct inst const idx,mul,add,mad;
struct inst st1(float[]);
struct inst st3(float[]);
struct inst ld1(float const[]);
struct inst uni(float const*);
struct inst imm(float);
struct inst ret(struct inst const*);

void run(struct inst const*, int n, F *stack);
