#include "stb/stb_image_write.h"
#include "therd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define len(x) (int)( sizeof(x) / sizeof(*x) )
#define want(x) if (!(x)) dprintf(2, "%s:%d want(%s)\n", __FILE__, __LINE__, #x), __builtin_trap()

static _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

static void test_3xp2(int const loops) {
    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          three = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        F stack[6];
        for (struct vm vm = {stack,0,len(src)}; vm.n; vm = loop(vm,stack)) {
            vm = imm(vm, 2.0f);
            vm = ld1(vm, src);
            vm = uni(vm, &three);
            vm = mad(vm);
            vm = st1(vm, dst);
        }
    }
    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

static void test_demo(int const loops) {
    int const w = 319,
              h = 240;
    struct { float r,g,b; } * const rgb = calloc((size_t)(w*h), sizeof *rgb);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++) {
            float inv_w   =            (1/(float)w),
                  y_inv_h = (float)y * (1/(float)h);
            F stack[6];
            for (struct vm vm = {stack,0,w}; vm.n; vm = loop(vm,stack)) {
                vm = idx(vm);
                vm = uni(vm, &inv_w);
                vm = mul(vm);
                vm = imm(vm, 0.5f);
                vm = uni(vm, &y_inv_h);
                vm = st3(vm, &(rgb + w*y)->r);
            }
        }
    }
    if (loops == 1) {
        stbi_write_hdr("/dev/stdout", w,h,3, &rgb->r);
    }

    free(rgb);
}

#define test(fn) if (loops == 1 || 0 == strcmp(bench, #fn)) test_##fn(loops)
int main(int argc, char* argv[]) {
    int  const  loops = argc > 1 ? atoi(argv[1]) : 1;
    char const *bench = argc > 2 ?      argv[2]  : "demo";

    test(3xp2);
    test(demo);
    return 0;
}
