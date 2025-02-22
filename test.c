#include "stb/stb_image_write.h"
#include "therd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define len(x) (int)( sizeof(x) / sizeof(*x) )
#define want(x) if (!(x)) dprintf(2, "%s:%d want(%s)\n", __FILE__, __LINE__, #x), __builtin_trap()

static _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

static double test_3xp2(int const loops) {
    float const src[] = {1,2,3,4,5,6,7,8,9,10,11};
    for (int n = 0; n <= len(src) ; n++) {
        float dst[len(src)] = {0};
        for (int i = 0; i < (n == len(src) ? loops : 1); i++) {
            char stack[3*32];
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
    return len(src);
}

static double test_demo(int const loops) {
    int const w = 319,
              h = 240;
    struct { float r,g,b; } * const rgb = calloc((size_t)(w*h), sizeof *rgb);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++) {
            char stack[3*32];
            for (struct vm vm = {stack,0,w}; vm.n; vm = loop(vm,stack)) {
                vm = idx(vm);
                vm = val(vm, 2/(float)w);
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
    return w*h;
}

static double test_naive(int const loops) {
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
    return w*h;
}

static double time_ns(double (*fn)(int), int loops) {
    struct timespec b,e;
    want(clock_gettime(CLOCK_MONOTONIC, &b) == 0);
    fn(loops);
    want(clock_gettime(CLOCK_MONOTONIC, &e) == 0);
    return (double)(e.tv_sec  - b.tv_sec ) * 1e9
         + (double)(e.tv_nsec - b.tv_nsec);
}

static void bench(char const *label, double (*fn)(int)) {
    double const units = fn(2);

    int loops = 1;
    for (double elapsed = 0; elapsed < 5e6; ) {
        loops *= 2;
        elapsed = time_ns(fn, loops);
    }

    double min = 1.0/0.0;
    for (int i = 0; i < 20; i++) {
        double const sample = time_ns(fn, loops) / loops;
        min = min < sample ? min : sample;
    }

    printf("%s\t%10.1f\t(%.2f)\n", label,min, min/units);
}

#define test(fn) \
    if (0 == strcmp(focus, "") || 0 == strcmp(focus, #fn)) test_##fn(1); \
    if (0 == strcmp(focus, "bench")) bench(#fn, test_##fn)

int main(int argc, char* argv[]) {
    char const *focus = argc > 1 ? argv[1] : "";
    test(3xp2);
    test(demo);
    test(naive);
    return 0;
}
