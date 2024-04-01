#include "therd.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define K 4
#define vec(T) T __attribute__((vector_size(K * sizeof(T))))

union Slot;
typedef vec(float) F;
typedef void Fn(union Slot const*, int const, F,F,F,F, F,F,F,F);

union Slot {
    Fn          *fn;
    float       *dst;
    float const *src;
    float        imm;
};

struct Builder {
    int         depth,slots;
    union Slot *slot;
};

struct Builder* builder(void) {
    struct Builder *b = calloc(1, sizeof *b);
    return b;
}

struct Program {
    int        slots,unused;
    union Slot slot[];
};

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static void push_(struct Builder *b, union Slot s) {
    if (is_pow2_or_zero(b->slots)) {
        b->slot = realloc(b->slot, sizeof *b->slot * (size_t)(b->slots ? 2*b->slots : 1));
    }
    b->slot[b->slots++] = s;
}
#define push(b, ...) push_(b, (union Slot){__VA_ARGS__})

static void done(union Slot const *s, int end, F v1, F v2, F v3, F v4, F v5, F v6, F v7, F v8) {
    (void)s;
    (void)end;
    (void)v1; (void)v2; (void)v3; (void)v4;
    (void)v5; (void)v6; (void)v7; (void)v8;
}

struct Program* compile(struct Builder *b) {
    push(b, .fn=done);

    struct Program *p = calloc(1, sizeof *p + sizeof *p->slot * (size_t)b->slots);
    p->slots = b->slots;
    memcpy(p->slot, b->slot, sizeof *p->slot * (size_t)b->slots);

    free(b);
    return p;
}

#define next s[1].fn(s+1,end, v1,v2,v3,v4,v5,v6,v7,v8); return
#define defn(name) \
    static void name(union Slot const *s, int end, F v1, F v2, F v3, F v4, F v5, F v6, F v7, F v8)

defn(mul2) { v1 *= v2; next; }
defn(mul3) { v2 *= v3; next; }
defn(mul4) { v3 *= v4; next; }
defn(mul5) { v4 *= v5; next; }
defn(mul6) { v5 *= v6; next; }
defn(mul7) { v6 *= v7; next; }
defn(mul8) { v7 *= v8; next; }
void mul(struct Builder *b) {
    assert(b->depth >= 2);
    static Fn *fn[9] = {0,0,mul2,mul3,mul4,mul5,mul6,mul7,mul8};
    push(b, .fn=fn[b->depth--]);
}

defn(store1) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v1, sizeof v1[0])
                           : memcpy((++s)->dst + end-K, &v1, sizeof v1   ); next; }
defn(store2) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v2, sizeof v2[0])
                           : memcpy((++s)->dst + end-K, &v2, sizeof v2   ); next; }
defn(store3) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v3, sizeof v3[0])
                           : memcpy((++s)->dst + end-K, &v3, sizeof v3   ); next; }
defn(store4) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v4, sizeof v4[0])
                           : memcpy((++s)->dst + end-K, &v4, sizeof v4   ); next; }
defn(store5) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v5, sizeof v5[0])
                           : memcpy((++s)->dst + end-K, &v5, sizeof v5   ); next; }
defn(store6) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v6, sizeof v6[0])
                           : memcpy((++s)->dst + end-K, &v6, sizeof v6   ); next; }
defn(store7) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v7, sizeof v7[0])
                           : memcpy((++s)->dst + end-K, &v7, sizeof v7   ); next; }
defn(store8) { (end&(K-1)) ? memcpy((++s)->dst + end-1, &v8, sizeof v8[0])
                           : memcpy((++s)->dst + end-K, &v8, sizeof v8   ); next; }
void store(struct Builder *b, float *dst) {
    assert(b->depth >= 1);
    static Fn *fn[9] = {0,store1,store2,store3,store4,store5,store6,store7,store8};
    push(b, .fn=fn[b->depth]);
    push(b, .dst=dst);
}

defn(load0) { (end&(K-1)) ? memcpy(&v1, (++s)->src + end-1, sizeof v1[0])
                          : memcpy(&v1, (++s)->src + end-K, sizeof v1   ); next; }
defn(load1) { (end&(K-1)) ? memcpy(&v2, (++s)->src + end-1, sizeof v2[0])
                          : memcpy(&v2, (++s)->src + end-K, sizeof v2   ); next; }
defn(load2) { (end&(K-1)) ? memcpy(&v3, (++s)->src + end-1, sizeof v3[0])
                          : memcpy(&v3, (++s)->src + end-K, sizeof v3   ); next; }
defn(load3) { (end&(K-1)) ? memcpy(&v4, (++s)->src + end-1, sizeof v4[0])
                          : memcpy(&v4, (++s)->src + end-K, sizeof v4   ); next; }
defn(load4) { (end&(K-1)) ? memcpy(&v5, (++s)->src + end-1, sizeof v5[0])
                          : memcpy(&v5, (++s)->src + end-K, sizeof v5   ); next; }
defn(load5) { (end&(K-1)) ? memcpy(&v6, (++s)->src + end-1, sizeof v6[0])
                          : memcpy(&v6, (++s)->src + end-K, sizeof v6   ); next; }
defn(load6) { (end&(K-1)) ? memcpy(&v7, (++s)->src + end-1, sizeof v7[0])
                          : memcpy(&v7, (++s)->src + end-K, sizeof v7   ); next; }
defn(load7) { (end&(K-1)) ? memcpy(&v8, (++s)->src + end-1, sizeof v8[0])
                          : memcpy(&v8, (++s)->src + end-K, sizeof v8   ); next; }
void load(struct Builder *b, float const *src) {
    assert(b->depth < 8);
    static Fn *fn[9] = {load0,load1,load2,load3,load4,load5,load6,load7,0};
    push(b, .fn=fn[b->depth++]);
    push(b, .src=src);
}

defn(splat0) { v1 = ((F){0} + 1) * (++s)->imm; next; }
defn(splat1) { v2 = ((F){0} + 1) * (++s)->imm; next; }
defn(splat2) { v3 = ((F){0} + 1) * (++s)->imm; next; }
defn(splat3) { v4 = ((F){0} + 1) * (++s)->imm; next; }
defn(splat4) { v5 = ((F){0} + 1) * (++s)->imm; next; }
defn(splat5) { v6 = ((F){0} + 1) * (++s)->imm; next; }
defn(splat6) { v7 = ((F){0} + 1) * (++s)->imm; next; }
defn(splat7) { v8 = ((F){0} + 1) * (++s)->imm; next; }
void splat(struct Builder *b, float imm) {
    assert(b->depth < 8);
    static Fn *fn[9] = {splat0,splat1,splat2,splat3,splat4,splat5,splat6,splat7,0};
    push(b, .fn=fn[b->depth++]);
    push(b, .imm=imm);
}

void execute(struct Program const *p, int const n) {
    F z = {0};
    for (int i = 0; i < n/K*K; i += K) { p->slot->fn(p->slot,i+K, z,z,z,z,z,z,z,z); }
    for (int i = n/K*K; i < n; i += 1) { p->slot->fn(p->slot,i+1, z,z,z,z,z,z,z,z); }
}
