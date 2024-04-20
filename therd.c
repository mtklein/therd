#include "therd.h"
#if 1 && defined(__ARM_NEON)
    #include <arm_neon.h>
    #define HAVE_NEON_INTRINSICS
#endif

// TODO:
//  [ ] some sort of control flow
//        - jumps?
//        - gosub?
//        - call_if to independent programs?
//  [ ] compare performance with top-of-stack-in-register impl.
//        - control flow is a lot easier there when instructions are stack-depth-independent

#define len(x) (int)( sizeof(x) / sizeof(*x) )

#define K ((int) (sizeof(F) / sizeof(float)))
#define splat(T,v) (((T){0} + 1) * (v))

typedef void (*fn)(struct inst const*, int, F*, void*[], int, F*, F,F,F,F,F,F,F,F);

#define next ip[1].fn(ip+1,n,stack,ptr,i,sp, v0,v1,v2,v3,v4,v5,v6,v7); return
#define defn(name) static void name(struct inst const *ip, int n, F *stack, void* ptr[], \
                                    int i, F *sp, F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7)

static struct builder append_(struct builder b, int delta, fn fn[], int fns, struct inst inst) {
    inst.fn = fn[b.depth < fns-1 ? b.depth : fns-1];
    *b.p++ = inst;
    b.depth += delta;
    return b;
}
#define append(b,delta,fn,...) append_(b,delta,fn,len(fn), (struct inst){__VA_ARGS__})

defn(mul_2) { v0     *= v1   ;       next; }
defn(mul_3) { v1     *= v2   ;       next; }
defn(mul_4) { v2     *= v3   ;       next; }
defn(mul_5) { v3     *= v4   ;       next; }
defn(mul_6) { v4     *= v5   ;       next; }
defn(mul_7) { v5     *= v6   ;       next; }
defn(mul_8) { v6     *= v7   ;       next; }
defn(mul_9) { v7     *= sp[0]; sp--; next; }
defn(mul_X) { sp[-1] *= sp[0]; sp--; next; }
static fn mul_fn[] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8,mul_9,mul_X};
struct builder mul(struct builder b) { return append(b, -1, mul_fn, 0); }

defn(add_2) { v0     += v1   ;       next; }
defn(add_3) { v1     += v2   ;       next; }
defn(add_4) { v2     += v3   ;       next; }
defn(add_5) { v3     += v4   ;       next; }
defn(add_6) { v4     += v5   ;       next; }
defn(add_7) { v5     += v6   ;       next; }
defn(add_8) { v6     += v7   ;       next; }
defn(add_9) { v7     += sp[0]; sp--; next; }
defn(add_X) { sp[-1] += sp[0]; sp--; next; }
static fn add_fn[] = {0,0,add_2,add_3,add_4,add_5,add_6,add_7,add_8,add_9,add_X};
struct builder add(struct builder b) { return append(b, -1, add_fn, 0); }

defn(mad_3 ) { v0     += v1    * v2    ;          next; }
defn(mad_4 ) { v1     += v2    * v3    ;          next; }
defn(mad_5 ) { v2     += v3    * v4    ;          next; }
defn(mad_6 ) { v3     += v4    * v5    ;          next; }
defn(mad_7 ) { v4     += v5    * v6    ;          next; }
defn(mad_8 ) { v5     += v6    * v7    ;          next; }
defn(mad_9 ) { v6     += v7    * sp[ 0]; sp -= 1; next; }
defn(mad_10) { v7     += sp[0] * sp[-1]; sp -= 2; next; }
defn(mad_X ) { sp[-2] += sp[0] * sp[-1]; sp -= 2; next; }
static fn mad_fn[] = {0,0,0,mad_3,mad_4,mad_5,mad_6,mad_7,mad_8,mad_9,mad_10,mad_X};
struct builder mad(struct builder b) { return append(b, -2, mad_fn, 0); }

static void st1_(float *dst, int n, F x) {
    if (n%K) {
        *dst = x[0];
    } else {
        *(F*)dst = x;
    }
}
defn(st1_1) { float *p = ptr[ip->ix]; st1_(p+i, n, v0   ); next; }
defn(st1_2) { float *p = ptr[ip->ix]; st1_(p+i, n, v1   ); next; }
defn(st1_3) { float *p = ptr[ip->ix]; st1_(p+i, n, v2   ); next; }
defn(st1_4) { float *p = ptr[ip->ix]; st1_(p+i, n, v3   ); next; }
defn(st1_5) { float *p = ptr[ip->ix]; st1_(p+i, n, v4   ); next; }
defn(st1_6) { float *p = ptr[ip->ix]; st1_(p+i, n, v5   ); next; }
defn(st1_7) { float *p = ptr[ip->ix]; st1_(p+i, n, v6   ); next; }
defn(st1_8) { float *p = ptr[ip->ix]; st1_(p+i, n, v7   ); next; }
defn(st1_X) { float *p = ptr[ip->ix]; st1_(p+i, n, *sp--); next; }
static fn st1_fn[] = {0,st1_1,st1_2,st1_3,st1_4,st1_5,st1_6,st1_7,st1_8,st1_X};
struct builder st1(struct builder b, int ix) { return append(b, -1, st1_fn, .ix=ix); }

static void st3_(float *dst, int n, F x, F y, F z) {
#if defined(HAVE_NEON_INTRINSICS)
    float32x4x3_t const xyz = {{x,y,z}};
    n%K ? vst3q_lane_f32(dst,xyz,0) : vst3q_f32(dst,xyz);
#else
    if (n%K) {
        *dst++ = x[0]; *dst++ = y[0]; *dst++ = z[0];
    } else {
        *dst++ = x[0]; *dst++ = y[0]; *dst++ = z[0];
        *dst++ = x[1]; *dst++ = y[1]; *dst++ = z[1];
        *dst++ = x[2]; *dst++ = y[2]; *dst++ = z[2];
        *dst++ = x[3]; *dst++ = y[3]; *dst++ = z[3];
    }
#endif
}
defn(st3_3 ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v0    , v1    , v2   );          next; }
defn(st3_4 ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v1    , v2    , v3   );          next; }
defn(st3_5 ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v2    , v3    , v4   );          next; }
defn(st3_6 ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v3    , v4    , v5   );          next; }
defn(st3_7 ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v4    , v5    , v6   );          next; }
defn(st3_8 ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v5    , v6    , v7   );          next; }
defn(st3_9 ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v6    , v7    , sp[0]); sp -= 1; next; }
defn(st3_10) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v7    , sp[-1], sp[0]); sp -= 2; next; }
defn(st3_X ) { float *p = ptr[ip->ix]; st3_(p+3*i, n, sp[-2], sp[-1], sp[0]); sp -= 3; next; }
static fn st3_fn[] = {0,0,0,st3_3,st3_4,st3_5,st3_6,st3_7,st3_8,st3_9,st3_10,st3_X};
struct builder st3(struct builder b, int ix) { return append(b, -3, st3_fn, .ix=ix); }

static F ld1_(float const *src, int n) {
    if (n%K) {
        return (F){*src};
    } else {
        return *(F const*)src;
    }
}

defn(ld1_0) { float const *p = ptr[ip->ix];    v0 = ld1_(p+i, n); next; }
defn(ld1_1) { float const *p = ptr[ip->ix];    v1 = ld1_(p+i, n); next; }
defn(ld1_2) { float const *p = ptr[ip->ix];    v2 = ld1_(p+i, n); next; }
defn(ld1_3) { float const *p = ptr[ip->ix];    v3 = ld1_(p+i, n); next; }
defn(ld1_4) { float const *p = ptr[ip->ix];    v4 = ld1_(p+i, n); next; }
defn(ld1_5) { float const *p = ptr[ip->ix];    v5 = ld1_(p+i, n); next; }
defn(ld1_6) { float const *p = ptr[ip->ix];    v6 = ld1_(p+i, n); next; }
defn(ld1_7) { float const *p = ptr[ip->ix];    v7 = ld1_(p+i, n); next; }
defn(ld1_X) { float const *p = ptr[ip->ix]; *++sp = ld1_(p+i, n); next; }
static fn ld1_fn[] = {ld1_0,ld1_1,ld1_2,ld1_3,ld1_4,ld1_5,ld1_6,ld1_7,ld1_X};
struct builder ld1(struct builder b, int ix) { return append(b, +1, ld1_fn, .ix=ix); }

defn(uni_0) { float *p = ptr[ip->ix];    v0 = splat(F, *p); next; }
defn(uni_1) { float *p = ptr[ip->ix];    v1 = splat(F, *p); next; }
defn(uni_2) { float *p = ptr[ip->ix];    v2 = splat(F, *p); next; }
defn(uni_3) { float *p = ptr[ip->ix];    v3 = splat(F, *p); next; }
defn(uni_4) { float *p = ptr[ip->ix];    v4 = splat(F, *p); next; }
defn(uni_5) { float *p = ptr[ip->ix];    v5 = splat(F, *p); next; }
defn(uni_6) { float *p = ptr[ip->ix];    v6 = splat(F, *p); next; }
defn(uni_7) { float *p = ptr[ip->ix];    v7 = splat(F, *p); next; }
defn(uni_X) { float *p = ptr[ip->ix]; *++sp = splat(F, *p); next; }
static fn uni_fn[] = {uni_0,uni_1,uni_2,uni_3,uni_4,uni_5,uni_6,uni_7,uni_X};
struct builder uni(struct builder b, int ix) { return append(b, +1, uni_fn, .ix=ix); }

defn(imm_0) {    v0 = splat(F, ip->imm); next; }
defn(imm_1) {    v1 = splat(F, ip->imm); next; }
defn(imm_2) {    v2 = splat(F, ip->imm); next; }
defn(imm_3) {    v3 = splat(F, ip->imm); next; }
defn(imm_4) {    v4 = splat(F, ip->imm); next; }
defn(imm_5) {    v5 = splat(F, ip->imm); next; }
defn(imm_6) {    v6 = splat(F, ip->imm); next; }
defn(imm_7) {    v7 = splat(F, ip->imm); next; }
defn(imm_X) { *++sp = splat(F, ip->imm); next; }
static fn splat_fn[] = {imm_0,imm_1,imm_2,imm_3,imm_4,imm_5,imm_6,imm_7,imm_X};
struct builder imm(struct builder b, float imm) { return append(b, +1, splat_fn, .imm=imm); }

defn(id_0) {    v0 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_1) {    v1 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_2) {    v2 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_3) {    v3 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_4) {    v4 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_5) {    v5 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_6) {    v6 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_7) {    v7 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_X) { *++sp = splat(F, (float)i) + (F){0,1,2,3}; next; }
static fn id_fn[] = {id_0,id_1,id_2,id_3,id_4,id_5,id_6,id_7,id_X};
struct builder id(struct builder b) { return append(b, +1, id_fn, 0); }

defn(loop) {
    i += n%K ? 1 : K;
    n -= n%K ? 1 : K;
    if (n > 0) {
        struct inst const *top = ip->ptr;
        sp = stack;
        top->fn(top,n,stack,ptr, i,sp, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}
void ret(struct builder b, struct inst *top) {
    *b.p = (struct inst){.fn=loop, .ptr=top};
}

void run(struct inst const *p, int n, F *stack, void* ptr[]) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
    F u;
    p->fn(p,n,stack,ptr, 0,stack, u,u,u,u,u,u,u,u);
#pragma GCC diagnostic pop
}
