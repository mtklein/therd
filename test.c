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

static void test_build(int const loops) {
    for (int i = 0; i < loops; i++) {
        struct inst p[7];
        {
            struct builder b = {.inst=p};
            imm  (&b, 2.0f);
            load (&b,    0);
            uni  (&b,    2);
            mul  (&b      );
            add  (&b      );
            store(&b,    1);
            ret  (&b      );
        }
    }
}

static void test_noop(int const loops) {
    struct inst p;
    {
        struct builder b = {.inst=&p};
        ret(&b);
    }

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        run(&p, len(src), (void*[]){src,dst,&uni});
    }

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 0));
    }
}

static void build_3xp2(struct inst p[7]) {
    struct builder b = {.inst=p};
    imm  (&b, 2.0f);
    load (&b,    0);
    uni  (&b,    2);
    mul  (&b      );
    add  (&b      );
    store(&b,    1);
    ret  (&b      );
}

static void test_3xp2(int const loops) {
    struct inst p[7];
    build_3xp2(p);
    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};
    for (int i = 0; i < loops; i++) {
        run(p, len(src), (void*[]){src,dst,&uni});
    }
    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

// Regression test for a bug when n%K == 0.
static void test_all_body(int const loops) {
    struct inst p[7];
    build_3xp2(p);
    float buf[] = {1,2,3,4,5,6,7,8,9,10,11,12},
          uni   = 3.0f;
    for (int i = 0; i < loops; i++) {
        run(p, len(buf), (void*[]){buf,buf,&uni});
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n>1 && n%K == 1.
static void test_one_head(int const loops) {
    struct inst p[7];
    build_3xp2(p);
    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;
    for (int i = 0; i < loops; i++) {
        run(p, len(buf), (void*[]){buf,buf,&uni});
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n == 1.
static void test_just_one(int const loops) {
    struct inst p[7];
    build_3xp2(p);
    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;
    for (int i = 0; i < loops; i++) {
        run(p, 1, (void*[]){buf,buf,&uni});
    }
    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], i == 0 ? 3*(float)(i+1) + 2
                                  :   (float)(i+1)    ));
    }
}

static void write_hdr(float const *R, float const *G, float const *B, int w, int h) {
    struct { float r,g,b; } * const rgb = calloc((size_t)(w*h), sizeof *rgb);
    for (int i = 0; i < w*h; i++) {
        rgb[i].r = R[i];
        rgb[i].g = G[i];
        rgb[i].b = B[i];
    }
    stbi_write_hdr("/dev/stdout", w,h,3, &rgb->r);
    free(rgb);
}

static void test_demo(int const mode, int const loops) {
    struct inst p[9];
    struct builder b = {.inst=p};
    {
        enum {R,G,B, InvW,YInvH};
        if (mode == 0) {
            id   (&b       );  // x
            uni  (&b,  InvW);  // x InvW
            mul  (&b       );  // x*InvW
            store(&b,     R);  //
            imm  (&b,  0.5f);  // 0.5
            store(&b,     G);  //
            uni  (&b, YInvH);  // YInvH
            store(&b,     B);  //
        } else {
            // Faster!  Probably due to better branch prediction, functions at different depths?
            id   (&b       );  // x
            uni  (&b,  InvW);  // x InvW
            imm  (&b,  0.5f);  // x InvW 0.5
            uni  (&b, YInvH);  // x InvW 0.5 YInvH
            store(&b,     B);  // x InvH 0.5
            store(&b,     G);  // x InvH
            mul  (&b       );  // x*InvH
            store(&b,     R);  //
        }
        ret(&b);
    }

    int const w = 319,
              h = 240;

    float * const R = calloc((size_t)(w*h), sizeof *R),
          * const G = calloc((size_t)(w*h), sizeof *G),
          * const B = calloc((size_t)(w*h), sizeof *B);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++) {
            float InvW =            (1/(float)w),
                 YInvH = (float)y * (1/(float)h);
            int const row = w*y;
            run(p, w, (void*[]){R+row,G+row,B+row, &InvW,&YInvH});
        }
    }
    if (loops == 1) {
        write_hdr(R,G,B, w,h);
    }

    free(R);
    free(G);
    free(B);
}
static void test_demo0(int const loops) { test_demo(0,loops); }
static void test_demo1(int const loops) { test_demo(1,loops); }

#define test(fn) test_##fn(strcmp(bench, #fn) ? 1 : loops)
int main(int argc, char* argv[]) {
    int  const  loops = argc > 1 ? atoi(argv[1]) : 1;
    char const *bench = argc > 2 ?      argv[2]  : "demo0";

    test(build);

    test(noop);
    test(3xp2);
    test(all_body);
    test(one_head);
    test(just_one);

    test(demo0);
    test(demo1);
    return 0;
}
