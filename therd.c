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
    union { float imm; int ix; void *unused; };
};

struct Program {
    int   depth,insts;
    struct Inst inst[];
};

#define next ip[1].fn(ip+1,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7); return

static void head(struct Inst const *ip, int i, int const n, void* ptr[],
                 F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    if (n % K) {
        struct Inst const *top = ip + ip->ix;
        top->fn(top,i+1,n-1,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
        return;
    }
    if (n) {
        next;
    }
}
static void body(struct Inst const *ip, int i, int const n, void* ptr[],
                 F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    if (n > K) {
        struct Inst const *top = ip + ip->ix;
        top->fn(top,i+K,n-K,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}

struct Program* program(void) {
    struct Program *p = malloc(sizeof *p + 2 * sizeof *p->inst);
    p->depth          = 0;
    p->insts          = 2;
    p->inst[0]        = (struct Inst){.fn=head, .ix=0};
    p->inst[1]        = (struct Inst){.fn=body, .ix=0};
    return p;
}

static _Bool is_pow2_or_zero(int x) {
    return (x & (x-1)) == 0;
}

static struct Program* push(struct Program *p, struct Inst a, struct Inst A) {
    if (is_pow2_or_zero(p->insts)) {
        assert(p->insts);
        p = realloc(p, sizeof *p + sizeof *p->inst * (size_t)p->insts * 2);
    }

    // head <, body =, user insts (a,A) (b,B) (c,C), unused allocation .
    // N=2:  <=                    (alloc=2)
    // N=4:  a<A=                  (alloc=4)
    // N=6:  ab<A B=..             (alloc=8)
    // N=8:  abc< ABC=             (alloc=8)
    // N=10: abcd <ABC D=.. ....   (alloc=16)
    int const N = p->insts;
    assert(p->inst[N  -1].fn == body);
    assert(p->inst[N/2-1].fn == head);

    p->inst[N  +1] = (struct Inst){.fn=body, .ix=p->inst[N  -1].ix-1};
    p->inst[N    ] = A;
    for (int i = N; i --> N/2;) {
        p->inst[i] = p->inst[i-1];
    }
    p->inst[N/2  ] = (struct Inst){.fn=head, .ix=p->inst[N/2-1].ix-1};
    p->inst[N/2-1] = a;

    p->insts = N+2;
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

struct Program* mul(struct Program *p) {
    assert(p->depth >= 2);
    static Fn *fn[9] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8};
    struct Inst inst = { .fn=fn[p->depth--] };
    return push(p,inst,inst);
}

defn(add_2) { v0 += v1; next; }
defn(add_3) { v1 += v2; next; }
defn(add_4) { v2 += v3; next; }
defn(add_5) { v3 += v4; next; }
defn(add_6) { v4 += v5; next; }
defn(add_7) { v5 += v6; next; }
defn(add_8) { v6 += v7; next; }

defn(mad_3) { v0 += v1*v2; next; }

struct Program* add(struct Program *p) {
    assert(p->depth >= 2);
    if (p->inst[p->insts   - 2].fn == mul_3) {
        p->inst[p->insts   - 2].fn =  mad_3;
        p->inst[p->insts/2 - 2].fn =  mad_3;
        p->depth -= 1;
        return p;
    }
    static Fn *fn[9] = {0,0,add_2,add_3,add_4,add_5,add_6,add_7,add_8};
    struct Inst inst = { .fn=fn[p->depth--] };
    return push(p,inst,inst);
}

defn(storeH_1) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v0, sizeof v0[0]); next; }
defn(storeH_2) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v1, sizeof v1[0]); next; }
defn(storeH_3) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v2, sizeof v2[0]); next; }
defn(storeH_4) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v3, sizeof v3[0]); next; }
defn(storeH_5) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v4, sizeof v4[0]); next; }
defn(storeH_6) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v5, sizeof v5[0]); next; }
defn(storeH_7) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v6, sizeof v6[0]); next; }
defn(storeH_8) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v7, sizeof v7[0]); next; }

defn(storeB_1) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v0, sizeof v0   ); next; }
defn(storeB_2) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v1, sizeof v1   ); next; }
defn(storeB_3) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v2, sizeof v2   ); next; }
defn(storeB_4) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v3, sizeof v3   ); next; }
defn(storeB_5) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v4, sizeof v4   ); next; }
defn(storeB_6) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v5, sizeof v5   ); next; }
defn(storeB_7) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v6, sizeof v6   ); next; }
defn(storeB_8) { float *dst = ptr[ip->ix]; __builtin_memcpy(dst+i, &v7, sizeof v7   ); next; }

struct Program* store(struct Program *p, int ix) {
    assert(p->depth >= 1);
    static Fn
        *head[9] = {0,storeH_1,storeH_2,storeH_3,storeH_4,storeH_5,storeH_6,storeH_7,storeH_8},
        *body[9] = {0,storeB_1,storeB_2,storeB_3,storeB_4,storeB_5,storeB_6,storeB_7,storeB_8};
    return push(p, (struct Inst){.fn=head[p->depth], .ix=ix}
                 , (struct Inst){.fn=body[p->depth], .ix=ix});
}

defn(loadH_0) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v0, src+i, sizeof v0[0]); next; }
defn(loadH_1) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v1, src+i, sizeof v1[0]); next; }
defn(loadH_2) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v2, src+i, sizeof v2[0]); next; }
defn(loadH_3) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v3, src+i, sizeof v3[0]); next; }
defn(loadH_4) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v4, src+i, sizeof v4[0]); next; }
defn(loadH_5) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v5, src+i, sizeof v5[0]); next; }
defn(loadH_6) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v6, src+i, sizeof v6[0]); next; }
defn(loadH_7) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v7, src+i, sizeof v7[0]); next; }

defn(loadB_0) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v0, src+i, sizeof v0   ); next; }
defn(loadB_1) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v1, src+i, sizeof v1   ); next; }
defn(loadB_2) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v2, src+i, sizeof v2   ); next; }
defn(loadB_3) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v3, src+i, sizeof v3   ); next; }
defn(loadB_4) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v4, src+i, sizeof v4   ); next; }
defn(loadB_5) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v5, src+i, sizeof v5   ); next; }
defn(loadB_6) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v6, src+i, sizeof v6   ); next; }
defn(loadB_7) { float const *src = ptr[ip->ix]; __builtin_memcpy(&v7, src+i, sizeof v7   ); next; }

struct Program* load(struct Program *p, int ix) {
    assert(p->depth < 8);
    static Fn *head[9] = {loadH_0,loadH_1,loadH_2,loadH_3,loadH_4,loadH_5,loadH_6,loadH_7,0},
              *body[9] = {loadB_0,loadB_1,loadB_2,loadB_3,loadB_4,loadB_5,loadB_6,loadB_7,0};

    p = push(p, (struct Inst){.fn=head[p->depth], .ix=ix}
              , (struct Inst){.fn=body[p->depth], .ix=ix});
    p->depth += 1;
    return p;
}

defn(uni_0) { float const *uni = ptr[ip->ix]; v0 = ((F){0} + 1) * *uni; next; }
defn(uni_1) { float const *uni = ptr[ip->ix]; v1 = ((F){0} + 1) * *uni; next; }
defn(uni_2) { float const *uni = ptr[ip->ix]; v2 = ((F){0} + 1) * *uni; next; }
defn(uni_3) { float const *uni = ptr[ip->ix]; v3 = ((F){0} + 1) * *uni; next; }
defn(uni_4) { float const *uni = ptr[ip->ix]; v4 = ((F){0} + 1) * *uni; next; }
defn(uni_5) { float const *uni = ptr[ip->ix]; v5 = ((F){0} + 1) * *uni; next; }
defn(uni_6) { float const *uni = ptr[ip->ix]; v6 = ((F){0} + 1) * *uni; next; }
defn(uni_7) { float const *uni = ptr[ip->ix]; v7 = ((F){0} + 1) * *uni; next; }
struct Program* uni(struct Program *p, int ix) {
    assert(p->depth < 8);
    static Fn *fn[9] = {uni_0,uni_1,uni_2,uni_3,uni_4,uni_5,uni_6,uni_7,0};
    struct Inst inst = {.fn=fn[p->depth++], .ix=ix};
    return push(p,inst,inst);
}

defn(imm_0) { v0 = ((F){0} + 1) * ip->imm; next; }
defn(imm_1) { v1 = ((F){0} + 1) * ip->imm; next; }
defn(imm_2) { v2 = ((F){0} + 1) * ip->imm; next; }
defn(imm_3) { v3 = ((F){0} + 1) * ip->imm; next; }
defn(imm_4) { v4 = ((F){0} + 1) * ip->imm; next; }
defn(imm_5) { v5 = ((F){0} + 1) * ip->imm; next; }
defn(imm_6) { v6 = ((F){0} + 1) * ip->imm; next; }
defn(imm_7) { v7 = ((F){0} + 1) * ip->imm; next; }
struct Program* imm(struct Program *p, float imm) {
    assert(p->depth < 8);
    static Fn *fn[9] = {imm_0,imm_1,imm_2,imm_3,imm_4,imm_5,imm_6,imm_7,0};
    struct Inst inst = {.fn=fn[p->depth++], .imm=imm};
    return push(p,inst,inst);
}

void start(struct Program const *p, int const n, void* ptr[]) {
    if (n > 0) {
        F z = {0};
        p->inst->fn(p->inst,0,n,ptr, z,z,z,z, z,z,z,z);
    }
}
