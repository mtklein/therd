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

static void test_noop(int const loops) {
    struct inst p = ret(&p);
    for (int i = 0; i < loops; i++) {
        run(&p, 10, NULL);
    }
}

static void test_build(int const loops) {
    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          three = 3.0f,
          dst[len(src)] = {0};

    struct inst p[6];
    for (int i = 0; i < loops; i++) {
        p[0] = imm(2.0f);
        p[1] = ld1(src);
        p[2] = uni(&three);
        p[3] = mad;
        p[4] = st1(dst);
        p[5] = ret(p);
    }

    F stack[len(p)];
    run(p, len(src), stack);
    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

static void test_3xp2(int const loops) {
    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          three = 3.0f,
          dst[len(src)] = {0};
    struct inst p[6] = { imm(2.0f), ld1(src), uni(&three), mad, st1(dst), ret(p) };

    for (int i = 0; i < loops; i++) {
        F stack[len(p)];
        run(p, len(src), stack);
    }
    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

// Regression test for a bug when n%K == 0.
static void test_all_body(int const loops) {
    float buf[] = {1,2,3,4,5,6,7,8,9,10,11,12},
          three = 3.0f;
    struct inst p[6] = { imm(2.0f), ld1(buf), uni(&three), mad, st1(buf), ret(p) };

    for (int i = 0; i < loops; i++) {
        F stack[len(p)];
        run(p, len(buf), stack);
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n>1 && n%K == 1.
static void test_one_head(int const loops) {
    float buf[] = {1,2,3,4,5},
          three = 3.0f;
    struct inst p[6] = { imm(2.0f), ld1(buf), uni(&three), mad, st1(buf), ret(p) };

    for (int i = 0; i < loops; i++) {
        F stack[len(p)];
        run(p, len(buf), stack);
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n == 1.
static void test_just_one(int const loops) {
    float buf[] = {1,2,3,4,5},
          three = 3.0f;
    struct inst p[6] = { imm(2.0f), ld1(buf), uni(&three), mad, st1(buf), ret(p) };

    for (int i = 0; i < loops; i++) {
        F stack[len(p)];
        run(p, 1, stack);
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], i == 0 ? 3*(float)(i+1) + 2
                                  :   (float)(i+1)    ));
    }
}

static void test_demo(int const loops) {
    struct inst p[] = {
        id,
        uni(NULL),
        mul,
        imm(0.5f),
        uni(NULL),
        st3(NULL),
        ret(p),
    };

    void const* *uni0 = &p[1].cptr;
    void const* *uni1 = &p[4].cptr;
    void*       *dst  = &p[5].ptr;

    int const w = 319,
              h = 240;
    struct { float r,g,b; } * const rgb = calloc((size_t)(w*h), sizeof *rgb);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++) {
            float inv_w   =            (1/(float)w),
                  y_inv_h = (float)y * (1/(float)h);

            *uni0 = &inv_w;
            *uni1 = &y_inv_h;
            *dst  = rgb + w*y;

            F stack[len(p)];
            run(p, w, stack);
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

    test(noop);
    test(build);
    test(3xp2);
    test(all_body);
    test(one_head);
    test(just_one);

    test(demo);
    return 0;
}
