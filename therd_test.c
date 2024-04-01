#include "therd.h"
#include <stdio.h>
#include <stdlib.h>

#define len(x) (int)( sizeof(x) / sizeof(*x) )
#define want(x) if (!(x)) dprintf(2, "%s:%d want(%s)\n", __FILE__, __LINE__, #x), __builtin_trap()

static _Bool equiv(float x, float y) {
    return (x <= y && y <= x)
        || (x != x && y != y);
}

int main(int argc, char* argv[]) {
    int const loops = argc > 1 ? atoi(argv[1]) : 1;

    struct Program *p;
    {
        struct Builder *b = builder();
        load  (b,    0);
        splat (b, 3.0f);
        mul   (b      );
        store (b,    1);
        p = compile(b);
    }

    float src[] = {1,2,3,4,5,6,7,8,9,10,11},
          dst[len(src)] = {0};

    for (int i = 0; i < loops; i++) {
        execute(p, len(src), (void*[]){src,dst});
    }
    free(p);

    for (int i = 0; i < len(src); i++) {
        want(equiv(dst[i], 3 * src[i]));
    }
    return 0;
}
