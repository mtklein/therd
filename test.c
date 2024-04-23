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
    float const src[] = {1,2,3,4,5,6,7,8,9,10,11};
    for (int n = 0; n <= len(src) ; n++) {
        float dst[len(src)] = {0};
        for (int i = 0; i < (n == len(src) ? loops : 1); i++) {
            F stack[3];
            for (struct vm vm = {stack,0,n}; vm.n; vm = loop(vm,stack)) {
                vm = val(vm, 2.0f);
                vm = ld1(vm, src);
                vm = val(vm, 3.0f);
                vm = mad(vm);
                vm = st1(vm, dst);
            }
        }
        for (int i = 0; i < len(src); i++) {
            want(equiv(dst[i], i < n ? 3*src[i] + 2 : 0));
        }
    }
}

static void test_demo(int const loops) {
    int const w = 319,
              h = 240;
    struct { float r,g,b; } * const rgb = calloc((size_t)(w*h), sizeof *rgb);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++) {
            F stack[3];
            for (struct vm vm = {stack,0,w}; vm.n; vm = loop(vm,stack)) {
                vm = idx(vm);
                vm = val(vm, 1/(float)w);
                vm = mul(vm);
                vm = val(vm, 0.5f);
                vm = val(vm, (float)y * (1/(float)h));
                vm = st3(vm, &(rgb + w*y)->r);
            }
        }
    }

    if (loops == 1) {
        stbi_write_hdr("/dev/stdout", w,h,3, &rgb->r);
    }
    free(rgb);
}

static void test_naive(int const loops) {
    int const w = 319,
              h = 240;
    struct { float r,g,b; } * const rgb = calloc((size_t)(w*h), sizeof *rgb);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            rgb[w*y + x].r = (float)x * (1/(float)w);
            rgb[w*y + x].g = 0.5f;
            rgb[w*y + x].b = (float)y * (1/(float)h);
        }
    }

    if (loops == 1) {
        stbi_write_hdr("/dev/stdout", w,h,3, &rgb->r);
    }
    free(rgb);
}

#define test(fn) if (loops == 1 || 0 == strcmp(bench, #fn)) test_##fn(loops)
int main(int argc, char* argv[]) {
    char const *bench = argc > 1 ?      argv[1]  : "demo";
    int  const  loops = argc > 2 ? atoi(argv[2]) : 1;

    test(3xp2);
    test(demo);
    test(naive);
    return 0;
}
