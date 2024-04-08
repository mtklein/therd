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
        struct Program *p = program(NULL);
        p = imm  (p, 2.0f);
        p = load (p,    0);
        p = uni  (p,    2);
        p = mul  (p      );
        p = add  (p      );
        p = store(p,    1);
        free(p);
    }
}

static void test_reuse(int const loops) {
    void *buf = NULL;
    for (int i = 0; i < loops; i++) {
        struct Program *p = program(buf);
        p = imm  (p, 2.0f);
        p = load (p,    0);
        p = uni  (p,    2);
        p = mul  (p      );
        p = add  (p      );
        p = store(p,    1);
        buf = p;
    }
    free(buf);
}

static void test_empty(int const loops) {
    struct Program *p = program(NULL);

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
    struct Program *p = program(NULL);
    p = imm  (p, 2.0f);
    p = load (p,    0);
    p = uni  (p,    2);
    p = mul  (p      );
    p = add  (p      );
    p = store(p,    1);

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

#define test(fn) test_##fn(strcmp(bench, #fn) ? 1 : loops)

int main(int argc, char* argv[]) {
    int  const  loops = argc > 1 ? atoi(argv[1]) : 1;
    char const *bench = argc > 2 ?      argv[2]  : "3xp2";

    test(build);
    test(reuse);
    test(empty);
    test(3xp2);

    return 0;
}
