#include "therd.h"
#if 1 && defined(__ARM_NEON)
    #include <arm_neon.h>
    #define HAVE_NEON_INTRINSICS
#endif

#define len(x) (int)(sizeof x / sizeof x[0])
#define K len((F){0})
#define splat(T,v) (((T){0} + 1) * (v))

struct vm mul(struct vm vm) {
    F x = *--vm.sp,
      y = *--vm.sp;
    *vm.sp++ = x*y;
    return vm;
}

struct vm add(struct vm vm) {
    F x = *--vm.sp,
      y = *--vm.sp;
    *vm.sp++ = x+y;
    return vm;
}

struct vm mad(struct vm vm) {
    F x = *--vm.sp,
      y = *--vm.sp,
      z = *--vm.sp;
    *vm.sp++ = x*y+z;
    return vm;
}

struct vm st1(struct vm vm, float dst[]) {
    F x = *--vm.sp;
    dst += vm.i;
    if (vm.n % K) {
        *dst = x[0];
    } else {
        *(F*)dst = x;
    }
    return vm;
}

struct vm st3(struct vm vm, float dst[]) {
    F x = *--vm.sp,
      y = *--vm.sp,
      z = *--vm.sp;
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
    return vm;
}

struct vm ld1(struct vm vm, float const src[]) {
    src += vm.i;
    *vm.sp++ = (vm.n % K) ? (F){*src} : *(F const*)src;
    return vm;
}

struct vm val(struct vm vm, float imm) {
    *vm.sp++ = splat(F, imm);
    return vm;
}

struct vm idx(struct vm vm) {
    union { float f[8]; F v; } iota = {{0,1,2,3,4,5,6,7}};
    _Static_assert(len(iota.f) >= K, "");

    *vm.sp++ = splat(F, (float)vm.i) + iota.v;
    return vm;
}

struct vm loop(struct vm vm, F *stack) {
    vm.sp = stack;
    vm.i += (vm.n % K) ? 1 : K;
    vm.n -= (vm.n % K) ? 1 : K;
    return vm;
}
