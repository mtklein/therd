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

static void test_empty(int const loops) {
    struct Program *p = compile(builder());

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        start(p, len(src), (void*[]){src,dst,&uni});
    }
    free(p);

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 0));
    }
}

static void test_3xp2(int const loops) {
    struct Program *p;
    {
        struct Builder *b = builder();
        imm  (b, 2.0f);
        load (b,    0);
        uni  (b,    2);
        mul  (b      );
        add  (b      );
        store(b,    1);
        p = compile(b);
    }

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          uni   = 3.0f,
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        start(p, len(src), (void*[]){src,dst,&uni});
    }
    free(p);

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3*src[i] + 2));
    }
}

#define test(fn) test_##fn(strcmp(bench, #fn) ? 1 : loops)

int main(int argc, char* argv[]) {
    int const   loops = argc > 1 ? atoi(argv[1]) : 1;
    char const *bench = argc > 2 ?      argv[2]  : "3xp2";

    test(empty);
    test(3xp2);

    return 0;
}
