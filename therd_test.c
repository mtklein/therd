#include "therd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define len(x) (int)( sizeof(x) / sizeof(*x) )
#define want(x) if (!(x)) dprintf(2, "%s:%d want(%s)\n", __FILE__, __LINE__, #x), __builtin_trap()

// This is basically x == y, but also works with NaN.
static _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

// test_build, test_reuse, test_fixed test a couple different ways to build a program.
// They're mostly benchmarks rather than tests, but nice to run in any case.
//   - test_build is the most naive, with a malloc()/free() in the hot loop
//   - test_reuse also uses malloc()/free(), but once only outside the loop
//   - test_fixed uses the stack
// test_build should be the slowest, with test_reuse and test_fixed tied for fastest.

static void test_build(int const loops) {
    for (int i = 0; i < loops; i++) {
        size_t const sz = builder_size(6);
        struct Builder *b = builder(malloc(sz), sz);
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    2);
        mul  (b      );
        add  (b      );
        store(b,    1);

        free(done(b));
    }
}

static void test_reuse(int const loops) {
    size_t const sz = builder_size(6);
    void *buf = malloc(sz);
    for (int i = 0; i < loops; i++) {
        struct Builder *b = builder(buf,sz);
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    2);
        mul  (b      );
        add  (b      );
        store(b,    1);
        (void)done(b);
    }
    free(buf);
}

static void test_fixed(int const loops) {
    void *buf[128];
    want(sizeof buf >= builder_size(6));
    for (int i = 0; i < loops; i++) {
        struct Builder *b = builder(buf, sizeof buf);
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    2);
        mul  (b      );
        add  (b      );
        store(b,    1);
        (void)done(b);
    }
}

// test_empty makes sure it's safe to run an empty program,
// and as a benchmark serves as the floor for how fast any other program can run.
static void test_empty(int const loops) {
    size_t const sz = builder_size(0);
    struct Program *p = done(builder(malloc(sz),sz));

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        run(p, len(src), (void*[]){src,dst,&uni});
    }
    free(p);

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 0));
    }
}

// test_3xp2 (3*x+2) runs a program with some simple math.
// This is the most "real" of any of the tests.
static void test_3xp2(int const loops) {
    size_t const sz = builder_size(6);
    struct Program *p;
    {
        struct Builder *b = builder(malloc(sz),sz);
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    2);
        mul  (b      );
        add  (b      );
        store(b,    1);
        p = done(b);
    }

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        run(p, len(src), (void*[]){src,dst,&uni});
    }
    free(p);

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

// Same as 3xp2, but sized to make sure it runs only body and no head.
//
// As a benchmark it's best compared to 3xp2.  Though this does a little more
// work, it should run considerably faster, as N=12 needs just 3 body loops
// instead of N=11 3 head loops and 2 body loops.
//
// Also a regression test for when N%K==0.
static void test_all_body(int const loops) {
    size_t const sz = builder_size(6);
    struct Program *p;
    {
        struct Builder *b = builder(malloc(sz),sz);
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    1);
        mul  (b      );
        add  (b      );
        store(b,    0);
        p = done(b);
    }

    float buf[] = {1,2,3,4,5,6,7,8,9,10,11,12},
          uni   = 3.0f;

    for (int i = 0; i < loops; i++) {
        run(p, len(buf), (void*[]){buf,&uni});
    }
    free(p);

    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// A regression test for a bug when N>1 && N%K==1.
static void test_one_head(int const loops) {
    size_t const sz = builder_size(6);
    struct Program *p;
    {
        struct Builder *b = builder(malloc(sz),sz);
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    1);
        mul  (b      );
        add  (b      );
        store(b,    0);
        p = done(b);
    }

    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;

    for (int i = 0; i < loops; i++) {
        run(p, len(buf), (void*[]){buf,&uni});
    }
    free(p);

    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

// A regression test for a bug when N==1.
static void test_just_one(int const loops) {
    size_t const sz = builder_size(6);
    struct Program *p;
    {
        struct Builder *b = builder(malloc(sz),sz);
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    1);
        mul  (b      );
        add  (b      );
        store(b,    0);
        p = done(b);
    }

    float buf[] = {1,2,3,4,5},
          uni   = 3.0f;

    for (int i = 0; i < loops; i++) {
        run(p, 1, (void*[]){buf,&uni});
    }
    free(p);

    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], i == 0 ? 3*(float)(i+1) + 2
                                  :   (float)(i+1)    ));
    }
}

#define test(fn) test_##fn(strcmp(bench, #fn) ? 1 : loops)

// I typically benchmark with hyperfine, something like
//    $ hyperfine -N -w 3 -L bench build,reuse,fixed "therd_test 1000000 {bench}"
// Using 1,000,000 loops like this lets you squint and read its ms results as ns/loop.
int main(int argc, char* argv[]) {
    int  const  loops = argc > 1 ? atoi(argv[1]) : 1;
    char const *bench = argc > 2 ?      argv[2]  : "3xp2";

    test(build);
    test(reuse);
    test(fixed);

    test(empty);
    test(3xp2);
    test(all_body);
    test(one_head);
    test(just_one);

    return 0;
}
