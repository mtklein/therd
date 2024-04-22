#pragma once

typedef float __attribute__(( vector_size(32), aligned(4) )) F;

struct vm {
    F  *sp;
    int i,n;
};

struct vm idx(struct vm);
struct vm val(struct vm, float);
struct vm ld1(struct vm, float const[]);
struct vm st1(struct vm, float[]);
struct vm st3(struct vm, float[]);

struct vm mul(struct vm);
struct vm add(struct vm);
struct vm mad(struct vm);

struct vm loop(struct vm, F *stack);
