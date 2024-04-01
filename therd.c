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
    struct Inst inst = { .fn=fn[b->depth] };
    push(b,inst,inst);
    b->depth -= 1;
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
    if (b->body[b->insts-1].fn == mul3) {
        b->body[b->insts-1].fn =  mad3;
        b->tail[b->insts-1].fn =  mad3;
        b->depth -= 1;
        return;
    }
    static Fn *fn[9] = {0,0,add2,add3,add4,add5,add6,add7,add8};
    struct Inst inst = { .fn=fn[b->depth] };
    push(b,inst,inst);
    b->depth -= 1;
}

defn(bstore1) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v0, sizeof v0   ); next; }
defn(bstore2) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v1, sizeof v1   ); next; }
defn(bstore3) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v2, sizeof v2   ); next; }
defn(bstore4) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v3, sizeof v3   ); next; }
defn(bstore5) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v4, sizeof v4   ); next; }
defn(bstore6) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v5, sizeof v5   ); next; }
defn(bstore7) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v6, sizeof v6   ); next; }
defn(bstore8) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v7, sizeof v7   ); next; }

defn(tstore1) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v0, sizeof v0[0]); next; }
defn(tstore2) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v1, sizeof v1[0]); next; }
defn(tstore3) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v2, sizeof v2[0]); next; }
defn(tstore4) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v3, sizeof v3[0]); next; }
defn(tstore5) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v4, sizeof v4[0]); next; }
defn(tstore6) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v5, sizeof v5[0]); next; }
defn(tstore7) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v6, sizeof v6[0]); next; }
defn(tstore8) { float *dst = ptr[ip->ix]; memcpy(dst+i, &v7, sizeof v7[0]); next; }

void store(struct Builder *b, int ix) {
    assert(b->depth >= 1);
    static Fn *body[9] = {0,bstore1,bstore2,bstore3,bstore4,bstore5,bstore6,bstore7,bstore8},
              *tail[9] = {0,tstore1,tstore2,tstore3,tstore4,tstore5,tstore6,tstore7,tstore8};
    push(b, (struct Inst){.fn=body[b->depth], .ix=ix}
          , (struct Inst){.fn=tail[b->depth], .ix=ix});
}

defn(bload0) { float const *src = ptr[ip->ix]; memcpy(&v0, src+i, sizeof v0   ); next; }
defn(bload1) { float const *src = ptr[ip->ix]; memcpy(&v1, src+i, sizeof v1   ); next; }
defn(bload2) { float const *src = ptr[ip->ix]; memcpy(&v2, src+i, sizeof v2   ); next; }
defn(bload3) { float const *src = ptr[ip->ix]; memcpy(&v3, src+i, sizeof v3   ); next; }
defn(bload4) { float const *src = ptr[ip->ix]; memcpy(&v4, src+i, sizeof v4   ); next; }
defn(bload5) { float const *src = ptr[ip->ix]; memcpy(&v5, src+i, sizeof v5   ); next; }
defn(bload6) { float const *src = ptr[ip->ix]; memcpy(&v6, src+i, sizeof v6   ); next; }
defn(bload7) { float const *src = ptr[ip->ix]; memcpy(&v7, src+i, sizeof v7   ); next; }

defn(tload0) { float const *src = ptr[ip->ix]; memcpy(&v0, src+i, sizeof v0[0]); next; }
defn(tload1) { float const *src = ptr[ip->ix]; memcpy(&v1, src+i, sizeof v1[0]); next; }
defn(tload2) { float const *src = ptr[ip->ix]; memcpy(&v2, src+i, sizeof v2[0]); next; }
defn(tload3) { float const *src = ptr[ip->ix]; memcpy(&v3, src+i, sizeof v3[0]); next; }
defn(tload4) { float const *src = ptr[ip->ix]; memcpy(&v4, src+i, sizeof v4[0]); next; }
defn(tload5) { float const *src = ptr[ip->ix]; memcpy(&v5, src+i, sizeof v5[0]); next; }
defn(tload6) { float const *src = ptr[ip->ix]; memcpy(&v6, src+i, sizeof v6[0]); next; }
defn(tload7) { float const *src = ptr[ip->ix]; memcpy(&v7, src+i, sizeof v7[0]); next; }

void load(struct Builder *b, int ix) {
    assert(b->depth < 8);
    static Fn *body[9] = {bload0,bload1,bload2,bload3,bload4,bload5,bload6,bload7,0},
              *tail[9] = {tload0,tload1,tload2,tload3,tload4,tload5,tload6,tload7,0};
    push(b, (struct Inst){.fn=body[b->depth], .ix=ix}
          , (struct Inst){.fn=tail[b->depth], .ix=ix});
    b->depth += 1;
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
