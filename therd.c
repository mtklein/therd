#include "therd.h"
#if 1 && defined(__ARM_NEON)
    #include <arm_neon.h>
    #define HAVE_NEON_INTRINSICS
#endif

#define K ((int) (sizeof(F) / sizeof(float)))
#define splat(T,v) (((T){0} + 1) * (v))

#define next ip[1].fn(ip+1,n,stack,i,sp); return
#define defn(name) \
    static void name##_(struct inst const *ip, int n, F *stack, int i, F *sp)

defn(mul) { F x = *--sp, y = *--sp;            *sp++ = x*y  ; next; }
defn(add) { F x = *--sp, y = *--sp;            *sp++ = x+y  ; next; }
defn(mad) { F x = *--sp, y = *--sp, z = *--sp; *sp++ = x*y+z; next; }

struct inst const mul = { .fn=mul_ };
struct inst const add = { .fn=add_ };
struct inst const mad = { .fn=mad_ };

defn(st1) {
    F x = *--sp;
    float *p = ip->ptr, *dst = p+i;

    if (n%K) {
        *dst = x[0];
    } else {
        *(F*)dst = x;
    }
    next;
}
struct inst st1(float ptr[]) { return (struct inst){.fn=st1_, .ptr=ptr}; }

defn(st3) {
    F x = *--sp,
      y = *--sp,
      z = *--sp;
    float *p = ip->ptr, *dst = p+3*i;

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
    next;
}
struct inst st3(float ptr[]) { return (struct inst){.fn=st3_, .ptr=ptr}; }

defn(ld1) {
    float const *p = ip->cptr, *src = p+i;
    *sp++ = n%K ? (F){*src} : *(F const*)src;
    next;
}
struct inst ld1(float const cptr[]) { return (struct inst){.fn=ld1_, .cptr=cptr}; }

defn(uni) {
    float const *src = ip->cptr;
    *sp++ = splat(F, *src);
    next;
}
struct inst uni(float const *cptr) { return (struct inst){.fn=uni_, .cptr=cptr}; }


defn(imm) {
    *sp++ = splat(F, ip->imm);
    next;
}
struct inst imm(float imm) { return (struct inst){.fn=imm_, .imm=imm}; }

defn(id) {
    *sp++ = splat(F, (float)i) + (F){0,1,2,3};
    next;
}
struct inst const id = {.fn=id_};

defn(loop) {
    i += n%K ? 1 : K;
    n -= n%K ? 1 : K;
    if (n > 0) {
        struct inst const *top = ip->cptr;
        sp = stack;
        top->fn(top,n,stack, i,sp);
    }
}
struct inst ret(struct inst const *top) { return (struct inst){.fn=loop_, .cptr=top}; }

void run(struct inst const *p, int n, F *stack) {
    p->fn(p,n,stack, 0,stack);
}
