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

static void done(union Slot const *s, int i, F v1, F v2, F v3, F v4, F v5, F v6, F v7, F v8) {
    (void)s;
    (void)i;
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

#define next s[1].fn(s+1,i, v1,v2,v3,v4,v5,v6,v7,v8); return
#define defn(name) \
    static void name(union Slot const *s, int i, F v1, F v2, F v3, F v4, F v5, F v6, F v7, F v8)

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

defn(store1) { memcpy((++s)->dst + i, &v1, sizeof v1); next; }
defn(store2) { memcpy((++s)->dst + i, &v2, sizeof v2); next; }
defn(store3) { memcpy((++s)->dst + i, &v3, sizeof v3); next; }
defn(store4) { memcpy((++s)->dst + i, &v4, sizeof v4); next; }
defn(store5) { memcpy((++s)->dst + i, &v5, sizeof v5); next; }
defn(store6) { memcpy((++s)->dst + i, &v6, sizeof v6); next; }
defn(store7) { memcpy((++s)->dst + i, &v7, sizeof v7); next; }
defn(store8) { memcpy((++s)->dst + i, &v8, sizeof v8); next; }
void store(struct Builder *b, float *dst) {
    assert(b->depth >= 1);
    static Fn *fn[9] = {0,store1,store2,store3,store4,store5,store6,store7,store8};
    push(b, .fn=fn[b->depth]);
    push(b, .dst=dst);
}

defn(load0) { memcpy(&v1, (++s)->src + i, sizeof v1); next; }
defn(load1) { memcpy(&v2, (++s)->src + i, sizeof v2); next; }
defn(load2) { memcpy(&v3, (++s)->src + i, sizeof v3); next; }
defn(load3) { memcpy(&v4, (++s)->src + i, sizeof v4); next; }
defn(load4) { memcpy(&v5, (++s)->src + i, sizeof v5); next; }
defn(load5) { memcpy(&v6, (++s)->src + i, sizeof v6); next; }
defn(load6) { memcpy(&v7, (++s)->src + i, sizeof v7); next; }
defn(load7) { memcpy(&v8, (++s)->src + i, sizeof v8); next; }
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

void execute(struct Program const *p, int n) {
    F z = {0};
    for (int i = 0; i < n/K*K; i += K) { p->slot->fn(p->slot,i, z,z,z,z,z,z,z,z); }
}
