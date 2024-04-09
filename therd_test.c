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

    float buf[] = {1,2,3,4,5,6,7,8},
          uni   = 3.0f;

    for (int i = 0; i < loops; i++) {
        run(p, len(buf), (void*[]){buf,&uni});
    }
    free(p);

    for (int i = 0; loops == 1 && i < len(buf); i++) {
        want(equiv(buf[i], 3*(float)(i+1) + 2));
    }
}

#define test(fn) test_##fn(strcmp(bench, #fn) ? 1 : loops)

int main(int argc, char* argv[]) {
    int  const  loops = argc > 1 ? atoi(argv[1]) : 1;
    char const *bench = argc > 2 ?      argv[2]  : "3xp2";

    test(build);
    test(reuse);
    test(fixed);

    test(empty);
    test(3xp2);
    test(all_body);

    return 0;
}
