#pragma once

typedef float __attribute__(( vector_size(16), aligned(4) )) F;

struct inst {
    void (*fn)(struct inst const*, int, int, void*[], F,F,F,F,F,F,F,F);
    union { float imm; int ix; void *ptr; };
};

struct builder {
    struct inst * const inst;
    int                 insts,depth;
};
void store(struct builder*, int ptr);
void  load(struct builder*, int ptr);
void   uni(struct builder*, int ptr);
void   imm(struct builder*,  float );
void    id(struct builder*         );
void   mul(struct builder*         );
void   add(struct builder*         );
void   ret(struct builder*         );

void run(struct inst const*, int n, void* ptr[]);
