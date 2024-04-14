#pragma once

typedef float __attribute__(( vector_size(16), aligned(4) )) F;

struct inst {
    void (*fn)(struct inst const*, int, int, void*[], F,F,F,F,F,F,F,F);
    union { float imm; int ix; void *ptr; };
};

struct builder {
    struct inst * inst;
    int           insts,depth;
};
struct builder st1(struct builder, int ptr);
struct builder ld1(struct builder, int ptr);
struct builder uni(struct builder, int ptr);
struct builder imm(struct builder,  float );
struct builder  id(struct builder         );
struct builder mul(struct builder         );
struct builder add(struct builder         );
void           ret(struct builder         );

void run(struct inst const*, int n, void* ptr[]);
