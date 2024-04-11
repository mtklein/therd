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
        size_t const sz = program_size(7);
        struct program *p = program(malloc(sz), sz);
        imm  (p, 2.0f);
        load (p,    0);
        uni  (p,    2);
        mul  (p      );
        add  (p      );
        store(p,    1);
        done (p      );

        free(p);
    }
}

static void test_reuse(int const loops) {
    size_t const sz = program_size(7);
    void *buf = malloc(sz);
    for (int i = 0; i < loops; i++) {
        struct program *p = program(buf,sz);
        imm  (p, 2.0f);
        load (p,    0);
        uni  (p,    2);
        mul  (p      );
        add  (p      );
        store(p,    1);
        done (p      );
    }
    free(buf);
}

static void test_fixed(int const loops) {
    void *buf[128];
    want(sizeof buf >= program_size(7));
    for (int i = 0; i < loops; i++) {
        struct program *p = program(buf, sizeof buf);
        imm  (p, 2.0f);
        load (p,    0);
        uni  (p,    2);
        mul  (p      );
        add  (p      );
        store(p,    1);
        done (p       );
    }
}

static void test_noop(int const loops) {
    size_t const sz = program_size(1);
    struct program *p = program(malloc(sz),sz);
    done(p);

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        execute(p, len(src), (void*[]){src,dst,&uni});
    }
    free(p);

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 0));
    }
}

static void test_3xp2(int const loops) {
    size_t const sz = program_size(7);
    struct program *p = program(malloc(sz),sz);
    imm  (p, 2.0f);
    load (p,    0);
    uni  (p,    2);
    mul  (p      );
    add  (p      );
    store(p,    1);
    done (p      );

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        execute(p, len(src), (void*[]){src,dst,&uni});
    }
    free(p);

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

// Regression test for a bug when n%K == 0.
static void test_all_body(int const loops) {
    size_t const sz = program_size(7);
    struct program *p = program(malloc(sz),sz);
    imm  (p, 2.0f);
    load (p,    0);
    uni  (p,    1);
    mul  (p      );
    add  (p      );
    store(p,    0);
    done (p      );

    float buf[] = {1,2,3,4,5,6,7,8,9,10,11,12},
          uni   = 3.0f;

    for (int i = 0; i < loops; i++) {
        execute(p, len(buf), (void*[]){buf,&uni});
    }
    free(p);

    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n>1 && n%K == 1.
static void test_one_head(int const loops) {
    size_t const sz = program_size(7);
    struct program *p = program(malloc(sz),sz);
    imm  (p, 2.0f);
    load (p,    0);
    uni  (p,    1);
    mul  (p      );
    add  (p      );
    store(p,    0);
    done (p      );

    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;

    for (int i = 0; i < loops; i++) {
        execute(p, len(buf), (void*[]){buf,&uni});
    }
    free(p);

    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// Regression test for a bug when n == 1.
static void test_just_one(int const loops) {
    size_t const sz = program_size(7);
    struct program *p = program(malloc(sz),sz);
    imm  (p, 2.0f);
    load (p,    0);
    uni  (p,    1);
    mul  (p      );
    add  (p      );
    store(p,    0);
    done (p      );

    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;

    for (int i = 0; i < loops; i++) {
        execute(p, 1, (void*[]){buf,&uni});
    }
    free(p);

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
    void* buf[128];
    struct program *p = program(buf, sizeof buf);
    {
        enum {R,G,B, InvW,Y};
        if (mode == 0) {
            id   (p     );  // x
            uni  (p,InvW);  // x InvW
            mul  (p     );  // x*InvW
            store(p,   R);  //

            imm  (p,0.5f);  // 0.5
            store(p,   G);  //

            uni  (p,Y);     // Y
            store(p,B);     //
        } else {
            // Pushing all the data onto the stack first gets better performance!
            // Probably due to better branch prediction, with more stage Fns used?
            id   (p     );  // x
            uni  (p,InvW);  // x InvW
            imm  (p,0.5f);  // x InvW 0.5
            uni  (p,   Y);  // x InvW 0.5 Y

            store(p,   B);  // x InvH 0.5
            store(p,   G);  // x InvH
            mul  (p     );  // x*InvH
            store(p,   R);  //
        }
        done(p);
    }

    int const w = 319,
              h = 240;

    float * const R = calloc((size_t)(w*h), sizeof *R),
          * const G = calloc((size_t)(w*h), sizeof *G),
          * const B = calloc((size_t)(w*h), sizeof *B);

    for (int i = 0; i < loops; i++) {
        for (int y = 0; y < h; y++) {
            float InvW =  1/(float)w,
                     Y = (1/(float)h) * (float)y;
            int const row = w*y;
            execute(p, w, (void*[]){R+row,G+row,B+row, &InvW,&Y});
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
    test(reuse);
    test(fixed);

    test(noop);
    test(3xp2);

    test(all_body);
    test(one_head);
    test(just_one);

    test(demo0);
    test(demo1);
    return 0;
}
