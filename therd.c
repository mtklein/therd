#include "therd.h"
#if 1 && defined(__ARM_NEON)
    #include <arm_neon.h>
    #define HAVE_NEON_INTRINSICS
#endif

#define K 8
#define vec(T) T __attribute__(( vector_size(K * sizeof(T)), aligned(1) ))
typedef vec(float) F;

#define len(x) (int)(sizeof x / sizeof x[0])
#define splat(T,v) (((T){0} + 1) * (v))

struct vm mul(struct vm vm) {
    F *sp = vm.stack, x = *--sp, y = *--sp;
    *sp++ = x*y;
    return (struct vm){sp, vm.i, vm.n};
}

struct vm add(struct vm vm) {
    F *sp = vm.stack, x = *--sp, y = *--sp;
    *sp++ = x+y;
    return (struct vm){sp, vm.i, vm.n};
}

struct vm mad(struct vm vm) {
    F *sp = vm.stack, x = *--sp, y = *--sp, z = *--sp;
    *sp++ = x*y+z;
    return (struct vm){sp, vm.i, vm.n};
}

struct vm st1(struct vm vm, float dst[]) {
    F *sp = vm.stack, x = *--sp;
    dst += vm.i;
    if (vm.n % K) {
        *dst = x[0];
    } else {
        *(F*)dst = x;
    }
    return (struct vm){sp, vm.i, vm.n};
}

struct vm st3(struct vm vm, float dst[]) {
    F *sp = vm.stack, z = *--sp, y = *--sp, x = *--sp;
    dst += 3*vm.i;

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

    if (vm.n % K) {
        vst3q_lane_f32(dst,lo,0);
    } else {
        vst3q_f32(dst+ 0, lo);
        vst3q_f32(dst+12, hi);
    }
#else
    if (vm.n % K) {
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
    return (struct vm){sp, vm.i, vm.n};
}

struct vm ld1(struct vm vm, float const src[]) {
    F *sp = vm.stack;
    src += vm.i;
    *sp++ = (vm.n % K) ? (F){*src} : *(F const*)src;
    return (struct vm){sp, vm.i, vm.n};
}

struct vm val(struct vm vm, float imm) {
    F *sp = vm.stack;
    *sp++ = splat(F, imm);
    return (struct vm){sp, vm.i, vm.n};
}

struct vm idx(struct vm vm) {
    union { float f[8]; F v; } iota = {{0,1,2,3,4,5,6,7}};
    _Static_assert(len(iota.f) >= K, "");

    F *sp = vm.stack;
    *sp++ = splat(F, (float)vm.i) + iota.v;
    return (struct vm){sp, vm.i, vm.n};
}

struct vm loop(struct vm vm, void *stack) {
    vm.i += (vm.n % K) ? 1 : K;
    vm.n -= (vm.n % K) ? 1 : K;
    return (struct vm){stack, vm.i, vm.n};
}
