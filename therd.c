#include "therd.h"
#if 1 && defined(__ARM_NEON)
    #include <arm_neon.h>
    #define HAVE_NEON_INTRINSICS
#endif

#define len(arr) (int)(sizeof arr / sizeof *arr)

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
    float32x4x3_t const lo = {{
        __builtin_shufflevector(x,x,0,1,2,3),
        __builtin_shufflevector(y,y,0,1,2,3),
        __builtin_shufflevector(z,z,0,1,2,3),
    }}, hi = {{
        __builtin_shufflevector(x,x,4,5,6,7),
        __builtin_shufflevector(y,y,4,5,6,7),
        __builtin_shufflevector(z,z,4,5,6,7),
    }};

    if (n%K) {
        vst3q_lane_f32(dst,lo,0);
    } else {
        vst3q_f32(dst+ 0, lo);
        vst3q_f32(dst+12, hi);
    }
#else
    if (n%K) {
        *dst++ = x[0];
        *dst++ = y[0];
        *dst++ = z[0];
    } else {
        #pragma GCC unroll 32768
        for (int j = 0; j < K; j++) {
            *dst++ = x[j];
            *dst++ = y[j];
            *dst++ = z[j];
        }
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

defn(idx) {
    union { float f[8]; F v; } iota = {{0,1,2,3,4,5,6,7}};
    _Static_assert(len(iota.f) >= K, "");

    *sp++ = splat(F, (float)i) + iota.v;
    next;
}
struct inst const idx = {.fn=idx_};

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
