#pragma once
#include <stdint.h>

typedef float __attribute__(( vector_size(16), aligned(4) )) F;

struct inst {
    void (*fn)(struct inst const*, int, F*, void*[], int, F*);
    union { float imm; int ix; void const *ptr; };
};

extern struct inst const id, mul,add,mad;
struct inst st1(int ix);
struct inst st3(int ix);
struct inst ld1(int ix);
struct inst uni(int ix);
struct inst imm(float);
struct inst ret(struct inst const*);

void run(struct inst const*, int n, F *stack, void* ptr[]);
