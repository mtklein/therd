#include "therd.h"

#define K 4
#define vec(T)     T __attribute__(( vector_size(K * sizeof(T)), aligned(sizeof(T)) ))
#define splat(T,v) (((T){0} + 1) * (v))

struct inst;
typedef vec(float) F;
typedef void Fn(struct inst const*, int, int, void*[], F,F,F,F, F,F,F,F);

struct inst {
    Fn *fn;
    union { float imm; int ix; void *ptr; };
};

struct program {
    int          last,depth;
    struct inst *body;
    struct inst  head[];
};

size_t program_size(int insts) {
    return sizeof(struct program) + 2 * sizeof(struct inst) * (size_t)insts;
}

struct program* program(void *buf, size_t sz) {
    struct program *p = buf;
    p->last  = -1;
    p->depth =  0;
    p->body  =  p->head + (sz - sizeof *p)/2 / sizeof(struct inst);
    return p;
}

static void append(struct program *p, int delta, struct inst head, struct inst body) {
    p->head[++p->last] = head;
    p->body[  p->last] = body;
    p->depth += delta;
}
static void rewind(struct program *p, int delta) {
    p->depth -= delta;
    p->last--;
}

#define next ip[1].fn(ip+1,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7); return
#define defn(name) static void name(struct inst const *ip, int i, int n, void* ptr[], \
                                    F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7)

defn(mul_2) { v0 *= v1; next; }
defn(mul_3) { v1 *= v2; next; }
defn(mul_4) { v2 *= v3; next; }
defn(mul_5) { v3 *= v4; next; }
defn(mul_6) { v4 *= v5; next; }
defn(mul_7) { v5 *= v6; next; }
defn(mul_8) { v6 *= v7; next; }

void mul(struct program *p) {
    Fn *fn[9] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8};
    struct inst inst = { .fn=fn[p->depth] };
    append(p,-1,inst,inst);
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

void add(struct program *p) {
    Fn *mul[9] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8};
    int const mul_delta = -1;
    if (p->head[p->last].fn == mul[p->depth - mul_delta]) {
        rewind(p, mul_delta);

        Fn *mad[9] = {0,0,0,mad_3,mad_4,mad_5,mad_6,mad_7,mad_8};
        struct inst inst = { .fn=mad[p->depth] };
        append(p,-2,inst,inst);
        return;
    }

    Fn *fn[9] = {0,0,add_2,add_3,add_4,add_5,add_6,add_7,add_8};
    struct inst inst = { .fn=fn[p->depth] };
    append(p,-1,inst,inst);
}

defn(head_store_1) { float *dst = ptr[ip->ix]; dst[i] = v0[0]; next; }
defn(head_store_2) { float *dst = ptr[ip->ix]; dst[i] = v1[0]; next; }
defn(head_store_3) { float *dst = ptr[ip->ix]; dst[i] = v2[0]; next; }
defn(head_store_4) { float *dst = ptr[ip->ix]; dst[i] = v3[0]; next; }
defn(head_store_5) { float *dst = ptr[ip->ix]; dst[i] = v4[0]; next; }
defn(head_store_6) { float *dst = ptr[ip->ix]; dst[i] = v5[0]; next; }
defn(head_store_7) { float *dst = ptr[ip->ix]; dst[i] = v6[0]; next; }
defn(head_store_8) { float *dst = ptr[ip->ix]; dst[i] = v7[0]; next; }

defn(body_store_1) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v0; next; }
defn(body_store_2) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v1; next; }
defn(body_store_3) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v2; next; }
defn(body_store_4) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v3; next; }
defn(body_store_5) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v4; next; }
defn(body_store_6) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v5; next; }
defn(body_store_7) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v6; next; }
defn(body_store_8) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v7; next; }

void store(struct program *p, int ix) {
    Fn *head_fn[9] = {0, head_store_1, head_store_2, head_store_3, head_store_4
                       , head_store_5, head_store_6, head_store_7, head_store_8},
       *body_fn[9] = {0, body_store_1, body_store_2, body_store_3, body_store_4
                       , body_store_5, body_store_6, body_store_7, body_store_8};
    append(p,-1, (struct inst){.fn=head_fn[p->depth], .ix=ix}
               , (struct inst){.fn=body_fn[p->depth], .ix=ix});
}

defn(head_load_0) { float const *src = ptr[ip->ix]; v0[0] = src[i]; next; }
defn(head_load_1) { float const *src = ptr[ip->ix]; v1[0] = src[i]; next; }
defn(head_load_2) { float const *src = ptr[ip->ix]; v2[0] = src[i]; next; }
defn(head_load_3) { float const *src = ptr[ip->ix]; v3[0] = src[i]; next; }
defn(head_load_4) { float const *src = ptr[ip->ix]; v4[0] = src[i]; next; }
defn(head_load_5) { float const *src = ptr[ip->ix]; v5[0] = src[i]; next; }
defn(head_load_6) { float const *src = ptr[ip->ix]; v6[0] = src[i]; next; }
defn(head_load_7) { float const *src = ptr[ip->ix]; v7[0] = src[i]; next; }

defn(body_load_0) { float const *src = ptr[ip->ix]; v0 = *(F const*)(src+i); next; }
defn(body_load_1) { float const *src = ptr[ip->ix]; v1 = *(F const*)(src+i); next; }
defn(body_load_2) { float const *src = ptr[ip->ix]; v2 = *(F const*)(src+i); next; }
defn(body_load_3) { float const *src = ptr[ip->ix]; v3 = *(F const*)(src+i); next; }
defn(body_load_4) { float const *src = ptr[ip->ix]; v4 = *(F const*)(src+i); next; }
defn(body_load_5) { float const *src = ptr[ip->ix]; v5 = *(F const*)(src+i); next; }
defn(body_load_6) { float const *src = ptr[ip->ix]; v6 = *(F const*)(src+i); next; }
defn(body_load_7) { float const *src = ptr[ip->ix]; v7 = *(F const*)(src+i); next; }

void load(struct program *p, int ix) {
    Fn *head_fn[9] = {head_load_0, head_load_1, head_load_2, head_load_3,
                      head_load_4, head_load_5, head_load_6, head_load_7, 0},
       *body_fn[9] = {body_load_0, body_load_1, body_load_2, body_load_3,
                      body_load_4, body_load_5, body_load_6, body_load_7, 0};
    append(p,+1, (struct inst){.fn=head_fn[p->depth], .ix=ix}
               , (struct inst){.fn=body_fn[p->depth], .ix=ix});
}

defn(uni_0) { float const *uni = ptr[ip->ix]; v0 = splat(F, *uni); next; }
defn(uni_1) { float const *uni = ptr[ip->ix]; v1 = splat(F, *uni); next; }
defn(uni_2) { float const *uni = ptr[ip->ix]; v2 = splat(F, *uni); next; }
defn(uni_3) { float const *uni = ptr[ip->ix]; v3 = splat(F, *uni); next; }
defn(uni_4) { float const *uni = ptr[ip->ix]; v4 = splat(F, *uni); next; }
defn(uni_5) { float const *uni = ptr[ip->ix]; v5 = splat(F, *uni); next; }
defn(uni_6) { float const *uni = ptr[ip->ix]; v6 = splat(F, *uni); next; }
defn(uni_7) { float const *uni = ptr[ip->ix]; v7 = splat(F, *uni); next; }

void uni(struct program *p, int ix) {
    Fn *fn[9] = {uni_0,uni_1,uni_2,uni_3,uni_4,uni_5,uni_6,uni_7,0};
    struct inst inst = {.fn=fn[p->depth], .ix=ix};
    append(p,+1,inst,inst);
}

defn(imm_0) { v0 = splat(F, ip->imm); next; }
defn(imm_1) { v1 = splat(F, ip->imm); next; }
defn(imm_2) { v2 = splat(F, ip->imm); next; }
defn(imm_3) { v3 = splat(F, ip->imm); next; }
defn(imm_4) { v4 = splat(F, ip->imm); next; }
defn(imm_5) { v5 = splat(F, ip->imm); next; }
defn(imm_6) { v6 = splat(F, ip->imm); next; }
defn(imm_7) { v7 = splat(F, ip->imm); next; }

void imm(struct program *p, float imm) {
    Fn *fn[9] = {imm_0,imm_1,imm_2,imm_3,imm_4,imm_5,imm_6,imm_7,0};
    struct inst inst = {.fn=fn[p->depth], .imm=imm};
    append(p,+1,inst,inst);
}

defn(id_0) { v0 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_1) { v1 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_2) { v2 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_3) { v3 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_4) { v4 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_5) { v5 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_6) { v6 = splat(F, (float)i) + (F){0,1,2,3}; next; }
defn(id_7) { v7 = splat(F, (float)i) + (F){0,1,2,3}; next; }

void id(struct program *p) {
    Fn *fn[9] = {id_0,id_1,id_2,id_3,id_4,id_5,id_6,id_7,0};
    struct inst inst = {.fn=fn[p->depth] };
    append(p,+1,inst,inst);
}

static void head_loop(struct inst const *ip, int i, int n, void* ptr[],
                      F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    i += 1;
    n -= 1;
    if (n > 0) {
        struct program const *p = ip->ptr;
        struct inst const *jump = n%K ? p->head
                                      : p->body;
        jump->fn(jump,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}
static void body_loop(struct inst const *ip, int i, int n, void* ptr[],
                      F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    i += K;
    n -= K;
    if (n > 0) {
        struct inst const *body = ip->ptr;
        body->fn(body,i,n,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}
void done(struct program *p) {
    append(p, 0, (struct inst){.fn=head_loop, .ptr=p}
               , (struct inst){.fn=body_loop, .ptr=p->body});
}

void execute(struct program const *p, int n, void* ptr[]) {
    if (n > 0) {
        struct inst const *start = n%K ? p->head
                                       : p->body;
        F z = {0};
        start->fn(start,0,n,ptr, z,z,z,z, z,z,z,z);
    }
}
