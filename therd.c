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
    struct Inst *body,*tail;
};

struct Builder* builder(void) {
    struct Builder *b = calloc(1, sizeof *b);
    return b;
}

struct Program {
    int         insts,unused;
    struct Inst inst[];
};

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static void push(struct Builder *b, struct Inst body, struct Inst tail) {
    if (is_pow2_or_zero(b->insts)) {
        b->body = realloc(b->body, sizeof *b->body * (size_t)(b->insts ? 2*b->insts : 1));
        b->tail = realloc(b->tail, sizeof *b->tail * (size_t)(b->insts ? 2*b->insts : 1));
    }
    b->body[b->insts] = body;
    b->tail[b->insts] = tail;
    b->insts++;
}

#define next ip[1].fn(ip+1,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7); return

static void body(struct Inst const *ip, int i, int const n, void* ptr[],
                 F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    i += K;
    if (i+K <= n) {
        struct Inst const *top = ip + ip->ix;
        top->fn(top,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
        return;
    }
    if (i < n) {
        next;
    }
}
static void tail(struct Inst const *ip, int i, int const n, void* ptr[],
                 F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    i += 1;
    if (i < n) {
        struct Inst const *top = ip + ip->ix;
        top->fn(top,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
        return;
    }
}
struct Program* compile(struct Builder *b) {
    struct Program *p = calloc(1, sizeof *p + sizeof *p->inst * ((size_t)b->insts * 2 + 2));

    for (int i = 0; i < b->insts; i++) { p->inst[p->insts++] = b->body[i]; }
    p->inst[p->insts++] = (struct Inst){.fn=body, .ix=-b->insts};
    for (int i = 0; i < b->insts; i++) { p->inst[p->insts++] = b->tail[i]; }
    p->inst[p->insts++] = (struct Inst){.fn=tail, .ix=-b->insts};

    free(b->body);
    free(b->tail);
    free(b);
    return p;
}

#define defn(name) static void name(struct Inst const *ip, int i, int n, void* ptr[], \
                                    F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7)
defn(mul_2) { v0 *= v1; next; }
defn(mul_3) { v1 *= v2; next; }
defn(mul_4) { v2 *= v3; next; }
defn(mul_5) { v3 *= v4; next; }
defn(mul_6) { v4 *= v5; next; }
defn(mul_7) { v5 *= v6; next; }
defn(mul_8) { v6 *= v7; next; }
void mul(struct Builder *b) {
    assert(b->depth >= 2);
    static Fn *fn[9] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8};
    struct Inst inst = { .fn=fn[b->depth] };
    push(b,inst,inst);
    b->depth -= 1;
}

defn(add_2) { v0 += v1; next; }
defn(add_3) { v1 += v2; next; }
defn(add_4) { v2 += v3; next; }
defn(add_5) { v3 += v4; next; }
defn(add_6) { v4 += v5; next; }
defn(add_7) { v5 += v6; next; }
defn(add_8) { v6 += v7; next; }

defn(mad_3) { v0 += v1*v2; next; }

void add(struct Builder *b) {
    assert(b->depth >= 2);
    if (b->body[b->insts-1].fn == mul_3) {
        b->body[b->insts-1].fn =  mad_3;
        b->tail[b->insts-1].fn =  mad_3;
        b->depth -= 1;
        return;
    }
    static Fn *fn[9] = {0,0,add_2,add_3,add_4,add_5,add_6,add_7,add_8};
    struct Inst inst = { .fn=fn[b->depth] };
    push(b,inst,inst);
    b->depth -= 1;
}

defn(storeK_1) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v0, sizeof v0   ); next; }
defn(storeK_2) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v1, sizeof v1   ); next; }
defn(storeK_3) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v2, sizeof v2   ); next; }
defn(storeK_4) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v3, sizeof v3   ); next; }
defn(storeK_5) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v4, sizeof v4   ); next; }
defn(storeK_6) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v5, sizeof v5   ); next; }
defn(storeK_7) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v6, sizeof v6   ); next; }
defn(storeK_8) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v7, sizeof v7   ); next; }

defn(store1_1) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v0, sizeof v0[0]); next; }
defn(store1_2) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v1, sizeof v1[0]); next; }
defn(store1_3) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v2, sizeof v2[0]); next; }
defn(store1_4) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v3, sizeof v3[0]); next; }
defn(store1_5) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v4, sizeof v4[0]); next; }
defn(store1_6) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v5, sizeof v5[0]); next; }
defn(store1_7) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v6, sizeof v6[0]); next; }
defn(store1_8) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v7, sizeof v7[0]); next; }

void store(struct Builder *b, int ix) {
    assert(b->depth >= 1);
    static Fn
        *body[9] = {0,storeK_1,storeK_2,storeK_3,storeK_4,storeK_5,storeK_6,storeK_7,storeK_8},
        *tail[9] = {0,store1_1,store1_2,store1_3,store1_4,store1_5,store1_6,store1_7,store1_8};
    push(b, (struct Inst){.fn=body[b->depth], .ix=ix}
          , (struct Inst){.fn=tail[b->depth], .ix=ix});
}

defn(loadK_0) { float const *src = ptr[ip->ix]; memcpy(&v0, src+i, sizeof v0   ); next; }
defn(loadK_1) { float const *src = ptr[ip->ix]; memcpy(&v1, src+i, sizeof v1   ); next; }
defn(loadK_2) { float const *src = ptr[ip->ix]; memcpy(&v2, src+i, sizeof v2   ); next; }
defn(loadK_3) { float const *src = ptr[ip->ix]; memcpy(&v3, src+i, sizeof v3   ); next; }
defn(loadK_4) { float const *src = ptr[ip->ix]; memcpy(&v4, src+i, sizeof v4   ); next; }
defn(loadK_5) { float const *src = ptr[ip->ix]; memcpy(&v5, src+i, sizeof v5   ); next; }
defn(loadK_6) { float const *src = ptr[ip->ix]; memcpy(&v6, src+i, sizeof v6   ); next; }
defn(loadK_7) { float const *src = ptr[ip->ix]; memcpy(&v7, src+i, sizeof v7   ); next; }

defn(load1_0) { float const *src = ptr[ip->ix]; memcpy(&v0, src+i, sizeof v0[0]); next; }
defn(load1_1) { float const *src = ptr[ip->ix]; memcpy(&v1, src+i, sizeof v1[0]); next; }
defn(load1_2) { float const *src = ptr[ip->ix]; memcpy(&v2, src+i, sizeof v2[0]); next; }
defn(load1_3) { float const *src = ptr[ip->ix]; memcpy(&v3, src+i, sizeof v3[0]); next; }
defn(load1_4) { float const *src = ptr[ip->ix]; memcpy(&v4, src+i, sizeof v4[0]); next; }
defn(load1_5) { float const *src = ptr[ip->ix]; memcpy(&v5, src+i, sizeof v5[0]); next; }
defn(load1_6) { float const *src = ptr[ip->ix]; memcpy(&v6, src+i, sizeof v6[0]); next; }
defn(load1_7) { float const *src = ptr[ip->ix]; memcpy(&v7, src+i, sizeof v7[0]); next; }

void load(struct Builder *b, int ix) {
    assert(b->depth < 8);
    static Fn *body[9] = {loadK_0,loadK_1,loadK_2,loadK_3,loadK_4,loadK_5,loadK_6,loadK_7,0},
              *tail[9] = {load1_0,load1_1,load1_2,load1_3,load1_4,load1_5,load1_6,load1_7,0};
    push(b, (struct Inst){.fn=body[b->depth], .ix=ix}
          , (struct Inst){.fn=tail[b->depth], .ix=ix});
    b->depth += 1;
}

defn(splat_0) { v0 = ((F){0} + 1) * ip->imm; next; }
defn(splat_1) { v1 = ((F){0} + 1) * ip->imm; next; }
defn(splat_2) { v2 = ((F){0} + 1) * ip->imm; next; }
defn(splat_3) { v3 = ((F){0} + 1) * ip->imm; next; }
defn(splat_4) { v4 = ((F){0} + 1) * ip->imm; next; }
defn(splat_5) { v5 = ((F){0} + 1) * ip->imm; next; }
defn(splat_6) { v6 = ((F){0} + 1) * ip->imm; next; }
defn(splat_7) { v7 = ((F){0} + 1) * ip->imm; next; }
void splat(struct Builder *b, float imm) {
    assert(b->depth < 8);
    static Fn *fn[9] = {splat_0,splat_1,splat_2,splat_3,splat_4,splat_5,splat_6,splat_7,0};
    struct Inst inst = {.fn=fn[b->depth], .imm=imm};
    push(b,inst,inst);
    b->depth += 1;
}

void execute(struct Program const *p, int const n, void* ptr[]) {
    int const tail = p->insts / 2;
    struct Inst const *start = n >= K ? p->inst : p->inst + tail;

    if (n > 0) {
        F z = {0};
        start->fn(start,0,n,ptr, z,z,z,z, z,z,z,z);
    }
}
