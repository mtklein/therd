#include "therd.h"
#if 1 && defined(__ARM_NEON)
    #include <arm_neon.h>
    #define HAVE_NEON_INTRINSICS
#endif

#define K ((int) (sizeof(F) / sizeof(float)))
#define splat(T,v) (((T){0} + 1) * (v))

typedef void (*fn)(struct inst const*, int, int, void*[], F,F,F,F,F,F,F,F,F*);

#define next ip[1].fn(ip+1,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7,v); return
#define defn(name) static void name(struct inst const *ip, int i, int n, void* ptr[], \
                                    F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7, F *v)

defn(mul_2) { v0 *= v1; next; }
defn(mul_3) { v1 *= v2; next; }
defn(mul_4) { v2 *= v3; next; }
defn(mul_5) { v3 *= v4; next; }
defn(mul_6) { v4 *= v5; next; }
defn(mul_7) { v5 *= v6; next; }
defn(mul_8) { v6 *= v7; next; }
static fn mul_fn[9] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8};

struct builder mul(struct builder b) {
    b.inst[b.insts++] = (struct inst){ .fn=mul_fn[b.depth--] };
    return b;
}

defn(add_2) { v0 += v1; next; }
defn(add_3) { v1 += v2; next; }
defn(add_4) { v2 += v3; next; }
defn(add_5) { v3 += v4; next; }
defn(add_6) { v4 += v5; next; }
defn(add_7) { v5 += v6; next; }
defn(add_8) { v6 += v7; next; }
static fn add_fn[9] = {0,0,add_2,add_3,add_4,add_5,add_6,add_7,add_8};

defn(mad_3) { v0 += v1*v2; next; }
defn(mad_4) { v1 += v2*v3; next; }
defn(mad_5) { v2 += v3*v4; next; }
defn(mad_6) { v3 += v4*v5; next; }
defn(mad_7) { v4 += v5*v6; next; }
defn(mad_8) { v5 += v6*v7; next; }
static fn mad_fn[9] = {0,0,0,mad_3,mad_4,mad_5,mad_6,mad_7,mad_8};

struct builder add(struct builder b) {
    if (b.inst[b.insts-1].fn == mul_fn[1 + b.depth]) {
        b.inst[b.insts-1].fn =  mad_fn[1 + b.depth--];
        return b;
    }
    b.inst[b.insts++] = (struct inst){ .fn=add_fn[b.depth--] };
    return b;
}

defn(st1_1) { float *p = ptr[ip->ix]; if (n%K) p[i] = v0[0]; else *(F*)(p+i) = v0; next; }
defn(st1_2) { float *p = ptr[ip->ix]; if (n%K) p[i] = v1[0]; else *(F*)(p+i) = v1; next; }
defn(st1_3) { float *p = ptr[ip->ix]; if (n%K) p[i] = v2[0]; else *(F*)(p+i) = v2; next; }
defn(st1_4) { float *p = ptr[ip->ix]; if (n%K) p[i] = v3[0]; else *(F*)(p+i) = v3; next; }
defn(st1_5) { float *p = ptr[ip->ix]; if (n%K) p[i] = v4[0]; else *(F*)(p+i) = v4; next; }
defn(st1_6) { float *p = ptr[ip->ix]; if (n%K) p[i] = v5[0]; else *(F*)(p+i) = v5; next; }
defn(st1_7) { float *p = ptr[ip->ix]; if (n%K) p[i] = v6[0]; else *(F*)(p+i) = v6; next; }
defn(st1_8) { float *p = ptr[ip->ix]; if (n%K) p[i] = v7[0]; else *(F*)(p+i) = v7; next; }
static fn st1_fn[9] = {0,st1_1,st1_2,st1_3,st1_4,st1_5,st1_6,st1_7,st1_8};

struct builder st1(struct builder b, int ix) {
    b.inst[b.insts++] = (struct inst){ .fn=st1_fn[b.depth--], .ix=ix };
    return b;
}

static void st3_(float *p, int n, F x, F y, F z) {
#if defined(HAVE_NEON_INTRINSICS)
    float32x4x3_t const xyz = {{x,y,z}};
    n%K ? vst3q_lane_f32(p,xyz,0) : vst3q_f32(p,xyz);
#else
    if (n%K) {
        *p++ = x[0]; *p++ = y[0]; *p++ = z[0];
    } else {
        *p++ = x[0]; *p++ = y[0]; *p++ = z[0];
        *p++ = x[1]; *p++ = y[1]; *p++ = z[1];
        *p++ = x[2]; *p++ = y[2]; *p++ = z[2];
        *p++ = x[3]; *p++ = y[3]; *p++ = z[3];
    }
#endif
}
defn(st3_3) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v0,v1,v2); next; }
defn(st3_4) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v1,v2,v3); next; }
defn(st3_5) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v2,v3,v4); next; }
defn(st3_6) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v3,v4,v5); next; }
defn(st3_7) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v4,v5,v6); next; }
defn(st3_8) { float *p = ptr[ip->ix]; st3_(p+3*i, n, v5,v6,v7); next; }
static fn st3_fn[9] = {0,0,0,st3_3,st3_4,st3_5,st3_6,st3_7,st3_8};

struct builder st3(struct builder b, int ix) {
    b.inst[b.insts++] = (struct inst){ .fn=st3_fn[b.depth], .ix=ix };
    b.depth -= 3;
    return b;
}

defn(ld1_0) { float *p = ptr[ip->ix]; if (n%K) v0[0] = p[i]; else v0 = *(F*)(p+i); next; }
defn(ld1_1) { float *p = ptr[ip->ix]; if (n%K) v1[0] = p[i]; else v1 = *(F*)(p+i); next; }
defn(ld1_2) { float *p = ptr[ip->ix]; if (n%K) v2[0] = p[i]; else v2 = *(F*)(p+i); next; }
defn(ld1_3) { float *p = ptr[ip->ix]; if (n%K) v3[0] = p[i]; else v3 = *(F*)(p+i); next; }
defn(ld1_4) { float *p = ptr[ip->ix]; if (n%K) v4[0] = p[i]; else v4 = *(F*)(p+i); next; }
defn(ld1_5) { float *p = ptr[ip->ix]; if (n%K) v5[0] = p[i]; else v5 = *(F*)(p+i); next; }
defn(ld1_6) { float *p = ptr[ip->ix]; if (n%K) v6[0] = p[i]; else v6 = *(F*)(p+i); next; }
defn(ld1_7) { float *p = ptr[ip->ix]; if (n%K) v7[0] = p[i]; else v7 = *(F*)(p+i); next; }
static fn ld1_fn[9] = {ld1_0,ld1_1,ld1_2,ld1_3,ld1_4,ld1_5,ld1_6,ld1_7,0};

struct builder ld1(struct builder b, int ix) {
    b.inst[b.insts++] = (struct inst){ .fn=ld1_fn[b.depth++], .ix=ix };
    return b;
}

defn(uni_0) { float *p = ptr[ip->ix]; v0 = splat(F, *p); next; }
defn(uni_1) { float *p = ptr[ip->ix]; v1 = splat(F, *p); next; }
defn(uni_2) { float *p = ptr[ip->ix]; v2 = splat(F, *p); next; }
defn(uni_3) { float *p = ptr[ip->ix]; v3 = splat(F, *p); next; }
defn(uni_4) { float *p = ptr[ip->ix]; v4 = splat(F, *p); next; }
defn(uni_5) { float *p = ptr[ip->ix]; v5 = splat(F, *p); next; }
defn(uni_6) { float *p = ptr[ip->ix]; v6 = splat(F, *p); next; }
defn(uni_7) { float *p = ptr[ip->ix]; v7 = splat(F, *p); next; }
static fn uni_fn[9] = {uni_0,uni_1,uni_2,uni_3,uni_4,uni_5,uni_6,uni_7,0};

struct builder uni(struct builder b, int ix) {
    b.inst[b.insts++] = (struct inst){ .fn=uni_fn[b.depth++], .ix=ix };
    return b;
}

defn(imm_0) { v0 = splat(F, ip->imm); next; }
defn(imm_1) { v1 = splat(F, ip->imm); next; }
defn(imm_2) { v2 = splat(F, ip->imm); next; }
defn(imm_3) { v3 = splat(F, ip->imm); next; }
defn(imm_4) { v4 = splat(F, ip->imm); next; }
defn(imm_5) { v5 = splat(F, ip->imm); next; }
defn(imm_6) { v6 = splat(F, ip->imm); next; }
defn(imm_7) { v7 = splat(F, ip->imm); next; }
static fn splat_fn[9] = {imm_0,imm_1,imm_2,imm_3,imm_4,imm_5,imm_6,imm_7,0};

struct builder imm(struct builder b, float imm) {
    b.inst[b.insts++] = (struct inst){ .fn=splat_fn[b.depth++], .imm=imm };
    return b;
}

defn(id_0) { v0 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_1) { v1 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_2) { v2 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_3) { v3 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_4) { v4 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_5) { v5 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_6) { v6 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_7) { v7 = splat(F, (float)i) + (F){0,1,2,3}; next; }
static fn id_fn[9] = {id_0,id_1,id_2,id_3,id_4,id_5,id_6,id_7,0};

struct builder id(struct builder b) {
    b.inst[b.insts++] = (struct inst){ .fn=id_fn[b.depth++] };
    return b;
}

defn(loop) {
    i += n%K ? 1 : K;
    n -= n%K ? 1 : K;
    if (n > 0) {
        struct inst const *top = ip->ptr;
        top->fn(top,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7,v);
    }
}
void ret(struct builder b) {
    b.inst[b.insts] = (struct inst){.fn=loop, .ptr=b.inst};
}

void run(struct inst const *p, int n, F *v, void* ptr[]) {
    if (n > 0) {
        F z = {0};
        p->fn(p,0,n,ptr, z,z,z,z,z,z,z,z,v);
    }
}
