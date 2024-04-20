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
    struct inst p;
    {
        struct builder b = {.p=&p};
        ret(b,&p);
    }

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        run(&p, len(src), NULL, (void*[]){src,dst,&uni});
    }

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 0));
    }
}

static struct builder build_3xp2(struct builder b) {
    b = imm(b, 2.0f);
    b = ld1(b,    0);
    b = uni(b,    2);
    b = mad(b      );
    b = st1(b,    1);
    return b;
}

static void check_3xp2(struct inst const *p, int const loops, F *stack) {
    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};
    for (int i = 0; i < loops; i++) {
        run(p, len(src), stack, (void*[]){src,dst,&uni});
    }
    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

static void test_build(int const loops) {
    struct inst p[6];
    for (int i = 0; i < loops; i++) {
        struct builder b = {.p=p};
        ret(build_3xp2(b), p);
    }
    F stack[len(p)];
    check_3xp2(p,1,stack);
}

static void test_3xp2(int const loops) {
    struct inst p[6];
    {
        struct builder b = {.p=p};
        ret(build_3xp2(b), p);
    }
    F stack[len(p)];
    check_3xp2(p,loops,stack);
}

static void test_pressure(int const loops) {
    struct inst p[20];
    for (int pressure = 0; pressure <= len(p)-6; pressure++) {
        struct builder b = {.p=p};
        for (int i = 0; i < pressure; i++) {
            b = imm(b, (float)i);
        }
        ret(build_3xp2(b),p);
        F stack[len(p)];
        check_3xp2(p,loops,stack);
    }
}

// Regression test for a bug when n%K == 0.
static void test_all_body(int const loops) {
    struct inst p[6];
    {
        struct builder b = {.p=p};
        ret(build_3xp2(b), p);
    }
    float buf[] = {1,2,3,4,5,6,7,8,9,10,11,12},
          uni   = 3.0f;
    for (int i = 0; i < loops; i++) {
        F stack[len(p)];
        run(p, len(buf), stack, (void*[]){buf,buf,&uni});
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n>1 && n%K == 1.
static void test_one_head(int const loops) {
    struct inst p[6];
    {
        struct builder b = {.p=p};
        ret(build_3xp2(b), p);
    }
    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;
    for (int i = 0; i < loops; i++) {
        F stack[len(p)];
        run(p, len(buf), stack, (void*[]){buf,buf,&uni});
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n == 1.
static void test_just_one(int const loops) {
    struct inst p[6];
    {
        struct builder b = {.p=p};
        ret(build_3xp2(b), p);
    }
    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;
    for (int i = 0; i < loops; i++) {
        F stack[len(p)];
        run(p, 1, stack, (void*[]){buf,buf,&uni});
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], i == 0 ? 3*(float)(i+1) + 2
                                  :   (float)(i+1)    ));
    }
}

static int pressure;
static void test_demo(int const loops) {
    struct inst p[7];
    {
        struct builder b = {.p=p, .depth=pressure};
        enum {rgb, inv_w, y_inv_h};

        b = id (b         );  // x
        b = uni(b,   inv_w);  // x inv_h
        b = mul(b         );  // x*inv_h
        b = imm(b,    0.5f);  // x*inv_h 0.5
        b = uni(b, y_inv_h);  // x*inv_h 0.5 y_inv_h
        b = st3(b,     rgb);
        ret(b,p);
    }

    int const w = 319,
              h = 240;

    struct { float r,g,b; } * const rgb = calloc((size_t)(w*h), sizeof *rgb);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++) {
            float inv_w =            (1/(float)w),
                y_inv_h = (float)y * (1/(float)h);
            F stack[len(p)];
            run(p, w, stack, (void*[]){rgb+w*y,&inv_w,&y_inv_h});
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
    pressure          = argc > 3 ? atoi(argv[3]) : 0;

    test(noop);
    test(build);
    test(3xp2);
    test(pressure);
    test(all_body);
    test(one_head);
    test(just_one);

    test(demo);
    return 0;
}
