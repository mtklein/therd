#include "therd.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define K 4
#define vec(T) T __attribute__((vector_size(K * sizeof(T))))

struct Slot;
typedef vec(float) F;
typedef void Fn(struct Slot const*, int, int, void*[], F,F,F,F, F,F,F,F);

struct Slot {
    Fn *fn;
    union { float imm; int ix; void *ptr; };
};

struct Builder {
    int          slots,depth;
    struct Slot *slot;
};

struct Builder* builder(void) {
    struct Builder *b = calloc(1, sizeof *b);
    return b;
}

struct Program {
    int         slots,unused;
    struct Slot slot[];
};

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static void push_(struct Builder *b, struct Slot s) {
    if (is_pow2_or_zero(b->slots)) {
        b->slot = realloc(b->slot, sizeof *b->slot * (size_t)(b->slots ? 2*b->slots : 1));
    }
    b->slot[b->slots++] = s;
}
#define push(b, ...) push_(b, (struct Slot){__VA_ARGS__})

static void done(struct Slot const *s, int i, int n, void* ptr[],
                 F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    (void)s; (void)i; (void)n; (void)ptr;
    (void)v0; (void)v1; (void)v2; (void)v3; (void)v4; (void)v5; (void)v6; (void)v7;
}

struct Program* compile(struct Builder *b) {
    push(b, .fn=done);

    struct Program *p = calloc(1, sizeof *p + sizeof *p->slot * (size_t)b->slots);
    p->slots = b->slots;
    memcpy(p->slot, b->slot, sizeof *p->slot * (size_t)b->slots);

    free(b);
    return p;
}

#define next s[1].fn(s+1,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7); return
#define defn(name) static void name(struct Slot const *s, int i, int n, void* ptr[], \
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
    if (b->slot[b->slots-1].fn == mul3) {
        b->slot[b->slots-1].fn =  mad3;
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
defn(store1) { store_(ptr[s->ix], v0, i,n); next; }
defn(store2) { store_(ptr[s->ix], v1, i,n); next; }
defn(store3) { store_(ptr[s->ix], v2, i,n); next; }
defn(store4) { store_(ptr[s->ix], v3, i,n); next; }
defn(store5) { store_(ptr[s->ix], v4, i,n); next; }
defn(store6) { store_(ptr[s->ix], v5, i,n); next; }
defn(store7) { store_(ptr[s->ix], v6, i,n); next; }
defn(store8) { store_(ptr[s->ix], v7, i,n); next; }
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
defn(load0) { v0 = load_(ptr[s->ix], i,n); next; }
defn(load1) { v1 = load_(ptr[s->ix], i,n); next; }
defn(load2) { v2 = load_(ptr[s->ix], i,n); next; }
defn(load3) { v3 = load_(ptr[s->ix], i,n); next; }
defn(load4) { v4 = load_(ptr[s->ix], i,n); next; }
defn(load5) { v5 = load_(ptr[s->ix], i,n); next; }
defn(load6) { v6 = load_(ptr[s->ix], i,n); next; }
defn(load7) { v7 = load_(ptr[s->ix], i,n); next; }
void load(struct Builder *b, int ix) {
    assert(b->depth < 8);
    static Fn *fn[9] = {load0,load1,load2,load3,load4,load5,load6,load7,0};
    push(b, .fn=fn[b->depth++], .ix=ix);
}

defn(splat0) { v0 = ((F){0} + 1) * s->imm; next; }
defn(splat1) { v1 = ((F){0} + 1) * s->imm; next; }
defn(splat2) { v2 = ((F){0} + 1) * s->imm; next; }
defn(splat3) { v3 = ((F){0} + 1) * s->imm; next; }
defn(splat4) { v4 = ((F){0} + 1) * s->imm; next; }
defn(splat5) { v5 = ((F){0} + 1) * s->imm; next; }
defn(splat6) { v6 = ((F){0} + 1) * s->imm; next; }
defn(splat7) { v7 = ((F){0} + 1) * s->imm; next; }
void splat(struct Builder *b, float imm) {
    assert(b->depth < 8);
    static Fn *fn[9] = {splat0,splat1,splat2,splat3,splat4,splat5,splat6,splat7,0};
    push(b, .fn=fn[b->depth++], .imm=imm);
}

void execute(struct Program const *p, int const n, void* ptr[]) {
    F z = {0};
    for (int i = 0; i < n/K*K; i += K) { p->slot->fn(p->slot,i,n,ptr, z,z,z,z,z,z,z,z); }
    for (int i = n/K*K; i < n; i += 1) { p->slot->fn(p->slot,i,n,ptr, z,z,z,z,z,z,z,z); }
}
