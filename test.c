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

static struct inst* build_3xp2(struct inst p[5]) {
    *p++ = imm(2.0f);
    *p++ = ld1(0);
    *p++ = uni(2);
    *p++ = mad;
    *p++ = st1(1);
    return p;
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
        *build_3xp2(p) = ret(p);
    }
    F stack[len(p)];
    check_3xp2(p,1,stack);
}

static void test_3xp2(int const loops) {
    struct inst p[6];
    *build_3xp2(p) = ret(p);

    F stack[len(p)];
    check_3xp2(p,loops,stack);
}

// Regression test for a bug when n%K == 0.
static void test_all_body(int const loops) {
    struct inst p[6];
    *build_3xp2(p) = ret(p);

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
    *build_3xp2(p) = ret(p);

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
    *build_3xp2(p) = ret(p);

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

static void test_demo(int const loops) {
    struct inst p[] = {
        id,         // x
        uni(1),     // x inv_w
        mul,        // x*inv_w
        imm(0.5f),  // x*inv_w 0.5
        uni(2),     // x*inv_w 0.5 y_inv_h
        st3(0),     //
        ret(p),
    };

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

    test(noop);
    test(build);
    test(3xp2);
    test(all_body);
    test(one_head);
    test(just_one);

    test(demo);
    return 0;
}
