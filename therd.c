#include "therd.h"
#include <assert.h>
#include <string.h>

// This code uses GCC vector extensions, so it needs GCC or Clang,
// but should otherwise be pretty portable.
#define vec(T) T __attribute__((vector_size(K * sizeof(T))))

struct Inst;
struct Program;

// The F and Fn types are geared for my primary dev machine, an M1 Mac.
// Its calling convention lets us pass around 8 4-float vector registers.
// x86-64 machines with System V ABI (Mac, Linux) should also perform well,
// though they could probably benefit from kicking K up to 8 (AVX) or 16 (512).
#define K 4
typedef vec(float) F;
typedef void Fn(struct Inst const*, int, int, void*[], F,F,F,F, F,F,F,F);

// An instruction has two parts: code to run, and some small data payload.
// Different instructions use that data payload in different ways.
struct Inst {
    Fn *fn;
    union { float imm; int ix; void *ptr; };
};

// A program is structured as two contiguous arrays of Insts, maybe with a gap
// between, each terminated by a special head_loop or body_loop instruction.
//
//    [   inst    ]    (H)
//    [   inst    ]
//    [   inst    ]
//    [   inst    ]
//    [ head_loop ]
//    (maybe a gap)
//    [   inst    ]    (B)
//    [   inst    ]
//    [   inst    ]
//    [   inst    ]
//    [ body_loop ]
//
// The two sequences of Insts conceptually do the same thing, except the head
// instructions operate on 1 value at a time, while the body instructions
// operate on full vectors of K values at once.  run() starts things off at the
// right place, and from there on the head_loop and body_loop special
// instructions handle all control flow, including eventually returning back to
// run().
//
// In the code below you'll see head and body Inst*.  These point to the Insts
// marked with (H) and (B), the beginning of the head and body sequences.
//
// Other instructions have more regular control flow.  They just do their thing
// then tail-call into the next instruction.  You'll see them use this next
// macro to do this:
#define next ip[1].fn(ip+1,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7); return

struct Program {
    struct Inst *body,*head;
};

struct Builder {
    union {
        struct {
            struct Inst *body;
            int          last,depth;
        };
        struct Program p;
    };
    struct Inst head[];
};

size_t builder_size(int insts) {
    insts += 1;
    insts *= 2;
    return sizeof(struct Builder) + sizeof(struct Inst) * (size_t)insts;
}

struct Builder* builder(void *buf, size_t sz) {
    struct Builder *b = buf;
    b->last  = -1;
    b->depth =  0;
    b->body  =  b->head + (sz - sizeof *b)/2 / sizeof(struct Inst);
    return b;
}

static void push(struct Builder *b, int delta, struct Inst head, struct Inst body) {
    b->depth += delta;
    b->head[++b->last] = head;
    b->body[  b->last] = body;
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
    push(b,-1,inst,inst);
}

defn(add_2) { v0 += v1; next; }
defn(add_3) { v1 += v2; next; }
defn(add_4) { v2 += v3; next; }
defn(add_5) { v3 += v4; next; }
defn(add_6) { v4 += v5; next; }
defn(add_7) { v5 += v6; next; }
defn(add_8) { v6 += v7; next; }

defn(mad_3) { v0 += v1*v2; next; }
defn(mad_4) { v1 += v2*v3; next; }
defn(mad_5) { v2 += v3*v4; next; }
defn(mad_6) { v3 += v4*v5; next; }
defn(mad_7) { v4 += v5*v6; next; }
defn(mad_8) { v5 += v6*v7; next; }

void add(struct Builder *b) {
    assert(b->depth >= 2);

    static Fn *mul[9] = {0,0,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8,0},
              *mad[9] = {0,0,mad_3,mad_4,mad_5,mad_6,mad_7,mad_8,0};
    if (b->head[b->last].fn == mul[b->depth]) {
        b->last--;
        struct Inst inst = { .fn=mad[b->depth] };
        push(b,-1,inst,inst);
        return;
    }

    static Fn *fn[9] = {0,0,add_2,add_3,add_4,add_5,add_6,add_7,add_8};
    struct Inst inst = { .fn=fn[b->depth] };
    push(b,-1,inst,inst);
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

void store(struct Builder *b, int ix) {
    assert(b->depth >= 1);
    static Fn *hfn[9] = {0,storeH_1,storeH_2,storeH_3,storeH_4,storeH_5,storeH_6,storeH_7,storeH_8},
              *bfn[9] = {0,storeB_1,storeB_2,storeB_3,storeB_4,storeB_5,storeB_6,storeB_7,storeB_8};
    push(b,-1, (struct Inst){.fn=hfn[b->depth], .ix=ix}
             , (struct Inst){.fn=bfn[b->depth], .ix=ix});
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

void load(struct Builder *b, int ix) {
    assert(b->depth < 8);
    static Fn *hfn[9] = {loadH_0,loadH_1,loadH_2,loadH_3,loadH_4,loadH_5,loadH_6,loadH_7,0},
              *bfn[9] = {loadB_0,loadB_1,loadB_2,loadB_3,loadB_4,loadB_5,loadB_6,loadB_7,0};

    push(b,+1, (struct Inst){.fn=hfn[b->depth], .ix=ix}
             , (struct Inst){.fn=bfn[b->depth], .ix=ix});
}

#define splat(T,v) (((T){0} + 1) * (v))

defn(uni_0) { float const *uni = ptr[ip->ix]; v0 = splat(F, *uni); next; }
defn(uni_1) { float const *uni = ptr[ip->ix]; v1 = splat(F, *uni); next; }
defn(uni_2) { float const *uni = ptr[ip->ix]; v2 = splat(F, *uni); next; }
defn(uni_3) { float const *uni = ptr[ip->ix]; v3 = splat(F, *uni); next; }
defn(uni_4) { float const *uni = ptr[ip->ix]; v4 = splat(F, *uni); next; }
defn(uni_5) { float const *uni = ptr[ip->ix]; v5 = splat(F, *uni); next; }
defn(uni_6) { float const *uni = ptr[ip->ix]; v6 = splat(F, *uni); next; }
defn(uni_7) { float const *uni = ptr[ip->ix]; v7 = splat(F, *uni); next; }

void uni(struct Builder *b, int ix) {
    assert(b->depth < 8);
    static Fn *fn[9] = {uni_0,uni_1,uni_2,uni_3,uni_4,uni_5,uni_6,uni_7,0};
    struct Inst inst = {.fn=fn[b->depth], .ix=ix};
    push(b,+1,inst,inst);
}

defn(imm_0) { v0 = splat(F, ip->imm); next; }
defn(imm_1) { v1 = splat(F, ip->imm); next; }
defn(imm_2) { v2 = splat(F, ip->imm); next; }
defn(imm_3) { v3 = splat(F, ip->imm); next; }
defn(imm_4) { v4 = splat(F, ip->imm); next; }
defn(imm_5) { v5 = splat(F, ip->imm); next; }
defn(imm_6) { v6 = splat(F, ip->imm); next; }
defn(imm_7) { v7 = splat(F, ip->imm); next; }

void imm(struct Builder *b, float imm) {
    assert(b->depth < 8);
    static Fn *fn[9] = {imm_0,imm_1,imm_2,imm_3,imm_4,imm_5,imm_6,imm_7,0};
    struct Inst inst = {.fn=fn[b->depth], .imm=imm};
    push(b,+1,inst,inst);
}

static void head_loop(struct Inst const *ip, int i, int n, void* ptr[],
                      F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    struct Program const *p = ip->ptr;
    if (n % K) {
        p->head->fn(p->head,i+1,n-1,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    } else if (n) {
        p->body->fn(p->body,i  ,n  ,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}
static void body_loop(struct Inst const *ip, int i, int n, void* ptr[],
                      F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    struct Inst const *body = ip->ptr;
    if (n > K) {
        body->fn(body,i+K,n-K,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}

struct Program* done(struct Builder *b) {
    struct Program *p = &b->p;
    push(b, 0, (struct Inst){.fn=head_loop, .ptr=p}
             , (struct Inst){.fn=body_loop, .ptr=p->body});
    p->head = b->head;
    assert((void*)b == (void*)p);
    return p;
}

void run(struct Program const *p, int N, void* ptr[]) {
    if (N > 0) {
        F z = {0};
        p->head->fn(p->head,0,N,ptr, z,z,z,z, z,z,z,z);
    }
}
