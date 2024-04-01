#include "therd.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define K 4
#define vec(T) T __attribute__((vector_size(K * sizeof(T))))

struct Inst;
typedef vec(float) F;
typedef void Fn(struct Inst const*, int, int, void*[], F,F,F,F, F,F,F,F);

struct Inst {
    Fn *fn;
    union { float imm; int ix; void *ptr; };
};

struct Builder {
    int          insts,depth;
    struct Inst *inst;
};

struct Builder* builder(void) {
    struct Builder *b = calloc(1, sizeof *b);
    return b;
}

struct Program {
    int         unused[2];
    struct Inst inst[];
};

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static void push_(struct Builder *b, struct Inst s) {
    if (is_pow2_or_zero(b->insts)) {
        b->inst = realloc(b->inst, sizeof *b->inst * (size_t)(b->insts ? 2*b->insts : 1));
    }
    b->inst[b->insts++] = s;
}
#define push(b, ...) push_(b, (struct Inst){__VA_ARGS__})

static void loop(struct Inst const *ip, int i, int const n, void* ptr[],
                 F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    i += (i+K <= n) ? K : 1;
    if (i < n) {
        struct Inst const *top = ip - ip->ix;
        top->fn(top,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}

struct Program* compile(struct Builder *b) {
    push(b, .fn=loop, .ix=b->insts);

    struct Program *p = calloc(1, sizeof *p + sizeof *p->inst * (size_t)b->insts);
    memcpy(p->inst, b->inst, sizeof *p->inst * (size_t)b->insts);

    free(b);
    return p;
}

#define next ip[1].fn(ip+1,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7); return
#define defn(name) static void name(struct Inst const *ip, int i, int n, void* ptr[], \
                                    F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7)
defn(mul2) { v0 *= v1; next; }
defn(mul3) { v1 *= v2; next; }
defn(mul4) { v2 *= v3; next; }
defn(mul5) { v3 *= v4; next; }
defn(mul6) { v4 *= v5; next; }
defn(mul7) { v5 *= v6; next; }
defn(mul8) { v6 *= v7; next; }
void mul(struct Builder *b) {
    assert(b->depth >= 2);
    static Fn *fn[9] = {0,0,mul2,mul3,mul4,mul5,mul6,mul7,mul8};
    push(b, .fn=fn[b->depth--]);
}

defn(add2) { v0 += v1; next; }
defn(add3) { v1 += v2; next; }
defn(add4) { v2 += v3; next; }
defn(add5) { v3 += v4; next; }
defn(add6) { v4 += v5; next; }
defn(add7) { v5 += v6; next; }
defn(add8) { v6 += v7; next; }

defn(mad3) { v0 += v1*v2; next; }

void add(struct Builder *b) {
    assert(b->depth >= 2);
    if (b->inst[b->insts-1].fn == mul3) {
        b->inst[b->insts-1].fn =  mad3;
        b->depth -= 1;
        return;
    }
    static Fn *fn[9] = {0,0,add2,add3,add4,add5,add6,add7,add8};
    push(b, .fn=fn[b->depth--]);
}

static void store_(float *dst, F src, int i, int n) {
    (i+K <= n) ? memcpy(dst+i, &src, sizeof src   )
               : memcpy(dst+i, &src, sizeof src[0]);
}
defn(store1) { store_(ptr[ip->ix], v0, i,n); next; }
defn(store2) { store_(ptr[ip->ix], v1, i,n); next; }
defn(store3) { store_(ptr[ip->ix], v2, i,n); next; }
defn(store4) { store_(ptr[ip->ix], v3, i,n); next; }
defn(store5) { store_(ptr[ip->ix], v4, i,n); next; }
defn(store6) { store_(ptr[ip->ix], v5, i,n); next; }
defn(store7) { store_(ptr[ip->ix], v6, i,n); next; }
defn(store8) { store_(ptr[ip->ix], v7, i,n); next; }
void store(struct Builder *b, int ix) {
    assert(b->depth >= 1);
    static Fn *fn[9] = {0,store1,store2,store3,store4,store5,store6,store7,store8};
    push(b, .fn=fn[b->depth], .ix=ix);
}

static F load_(float const *src, int i, int n) {
    F dst;
    (i+K <= n) ? memcpy(&dst, src+i, sizeof dst   )
               : memcpy(&dst, src+i, sizeof dst[0]);
    return dst;
}
defn(load0) { v0 = load_(ptr[ip->ix], i,n); next; }
defn(load1) { v1 = load_(ptr[ip->ix], i,n); next; }
defn(load2) { v2 = load_(ptr[ip->ix], i,n); next; }
defn(load3) { v3 = load_(ptr[ip->ix], i,n); next; }
defn(load4) { v4 = load_(ptr[ip->ix], i,n); next; }
defn(load5) { v5 = load_(ptr[ip->ix], i,n); next; }
defn(load6) { v6 = load_(ptr[ip->ix], i,n); next; }
defn(load7) { v7 = load_(ptr[ip->ix], i,n); next; }
void load(struct Builder *b, int ix) {
    assert(b->depth < 8);
    static Fn *fn[9] = {load0,load1,load2,load3,load4,load5,load6,load7,0};
    push(b, .fn=fn[b->depth++], .ix=ix);
}

defn(splat0) { v0 = ((F){0} + 1) * ip->imm; next; }
defn(splat1) { v1 = ((F){0} + 1) * ip->imm; next; }
defn(splat2) { v2 = ((F){0} + 1) * ip->imm; next; }
defn(splat3) { v3 = ((F){0} + 1) * ip->imm; next; }
defn(splat4) { v4 = ((F){0} + 1) * ip->imm; next; }
defn(splat5) { v5 = ((F){0} + 1) * ip->imm; next; }
defn(splat6) { v6 = ((F){0} + 1) * ip->imm; next; }
defn(splat7) { v7 = ((F){0} + 1) * ip->imm; next; }
void splat(struct Builder *b, float imm) {
    assert(b->depth < 8);
    static Fn *fn[9] = {splat0,splat1,splat2,splat3,splat4,splat5,splat6,splat7,0};
    push(b, .fn=fn[b->depth++], .imm=imm);
}

void execute(struct Program const *p, int const n, void* ptr[]) {
    if (n) {
        F z = {0};
        p->inst->fn(p->inst,0,n,ptr, z,z,z,z, z,z,z,z);
    }
}
