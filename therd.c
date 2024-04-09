#include "therd.h"

// This code uses GCC vector extensions, so it needs GCC or Clang,
// but should otherwise be pretty portable.
#define vec(T) T __attribute__(( vector_size(K * sizeof(T)), aligned(sizeof(T)) ))

struct Inst;
struct Program;

// The F and Fn types are geared for my primary dev machine, an M1 Mac.
// Its calling convention lets us pass around 8 4-float vector registers.
// x86-64 machines with System V ABI (Mac, Linux) should also perform well,
// though they could probably benefit from kicking K up to 8 (AVX) or 16 (512).
#define K 4
typedef vec(float) F;
typedef void Fn(struct Inst const*, int, int, void*[], F,F,F,F, F,F,F,F);

// An instruction has two parts: code to run, and some small data payload.
// Different instructions use that data payload in different ways.
struct Inst {
    Fn *fn;
    union { float imm; int ix; void *ptr; };
};

// A program is structured as a small header and two contiguous arrays of
// Insts, maybe with a gap between, each terminated by a special head_loop or
// body_loop instruction.
//    [  header   ]
//    [   inst    ]    (p->head)
//    [   inst    ]
//    [   inst    ]
//    [   inst    ]
//    [ head_loop ]
//    (maybe a gap)
//    [   inst    ]    (p->body)
//    [   inst    ]
//    [   inst    ]
//    [   inst    ]
//    [ body_loop ]
//
// The two sequences of Insts conceptually do the same thing, except the head
// instructions operate on 1 value at a time, while the body instructions
// operate on full vectors of K values at once.  run() starts things off at the
// right place, and from there on the head_loop and body_loop special
// instructions handle all control flow, including eventually returning back to
// run().
//
// Other instructions have more regular control flow.  They just do their thing
// then tail-call into the next instruction.  You'll see them use this next
// macro to do this:
#define next ip[1].fn(ip+1,i,N,ptr, v0,v1,v2,v3,v4,v5,v6,v7); return

// To run() a program we just need to know where the instructions are.
// These pointers are marked in the diagram above.
struct Program {
    struct Inst *body,*head;
};

// While building a program we also need to track the stack depth (depth) and
// how many instructions we've added so far (last+1).
//
// This struct layout may seem a little weird... it's mostly me being cute to
// pack everything into a compact header, and the union lets done() convert a
// Builder into a Program in-place.
struct Builder {
    union {
        struct {
            struct Inst *body;
            int          last,depth;
        };
        struct Program p;
    };
    struct Inst head[];
};

// So now builder_size() should make some sense reading the comments and diagram above.
// Each program needs a header, two copies of each user instruction, plus head_loop and body_loop.
size_t builder_size(int insts) {
    insts *= 2;
    insts += 2;
    return sizeof(struct Builder) + sizeof(struct Inst) * (size_t)insts;
}

// Stamp our Builder header onto the user-provided memory.
struct Builder* builder(void *buf, size_t sz) {
    struct Builder *b = buf;
    b->last  = -1;   // i.e. we have last+1 == 0 Insts so far
    b->depth =  0;
    // We use all the space after the Builder header for instructions,
    // so b->head now marks the beginning of that Inst array, and b->body points halfway in.
    b->body  =  b->head + (sz - sizeof *b)/2 / sizeof(struct Inst);

    // Notice, this is the same as "return buf".
    // Kind of convenient, as it means we can free(builder(malloc(...), ...)).
    // We pull this trick in done() too so the Program* is also the same address.
    return b;
}

// Append new head/body instructions and adjust the stack depth,
// or rewind the last and undo that stack depth adjustment.
// These helpers are the only plces b->last and b->depth change.
static void append(struct Builder *b, int delta, struct Inst head, struct Inst body) {
    b->head[++b->last] = head;
    b->body[  b->last] = body;
    b->depth += delta;
}
static void rewind(struct Builder *b, int delta) {
    b->depth -= delta;
    b->last--;
}

// We can quite easily track the depth of the stack as we build our program:
// load() pushes a value, increasing the stack depth by 1;  store() pops,
// decreasing by 1;  mul() pops twice and pushes once, a net decrease of 1.
//
// We use that knowledge of the stack depth to select one of several concrete
// implementations of each instruction, suited for that exact stack depth.
// Conceptually there could be up to 9 of these, for every stack depth between
// 0 and 8 both inclusive, but in practice most instructions have some
// constraint that means there are a few less than that.  (You can't multiply
// when the stack is empty or only has one value.)
//
// This lets us keep our stack in registers.  This is the most interesting
// design decision Therd makes!  When the stack only has one value in it,
// it's in v0.  When there are two, they're in v0 and v1, with v1 the top
// of the stack.  Five?  v0,v1,v2,v3,v4, with v4 as the top.
//
// We'll use a naming convention foo_n to indicate the foo instruction variant
// appropriate for a stack depth n.  This defn() macro just cuts down some of
// the boilerplate needed to define these instruction variants.
#define defn(name) static void name(struct Inst const *ip, int i, int N, void* ptr[], \
                                    F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7)

defn(mul_2) { v0 *= v1; next; }  // stack depth 2->1, top of stack v1->v0
defn(mul_3) { v1 *= v2; next; }
defn(mul_4) { v2 *= v3; next; }
defn(mul_5) { v3 *= v4; next; }
defn(mul_6) { v4 *= v5; next; }
defn(mul_7) { v5 *= v6; next; }
defn(mul_8) { v6 *= v7; next; }  // stack depth 8->7, top of stack v7->v6

void mul(struct Builder *b) {
    static Fn *fn[9] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8};
    struct Inst inst = { .fn=fn[b->depth] };
    append(b,-1,inst,inst);
}

// If you see a multiply instruction followed immediately by an add instruction,
// can fuse that into a multiply-add instruction, and that often executes faster than
// the two in sequence.  add() is a little fancier than mul() only to show this off.

defn(add_2) { v0 += v1; next; }     // depth 2->1, top v1->v0
defn(add_3) { v1 += v2; next; }
defn(add_4) { v2 += v3; next; }
defn(add_5) { v3 += v4; next; }
defn(add_6) { v4 += v5; next; }
defn(add_7) { v5 += v6; next; }
defn(add_8) { v6 += v7; next; }     // depth 8->7, top v7->v6

defn(mad_3) { v0 += v1*v2; next; }  // depth 3->1, top v2->v0
defn(mad_4) { v1 += v2*v3; next; }
defn(mad_5) { v2 += v3*v4; next; }
defn(mad_6) { v3 += v4*v5; next; }
defn(mad_7) { v4 += v5*v6; next; }
defn(mad_8) { v5 += v6*v7; next; }  // depth 8->6, top v7->v5

void add(struct Builder *b) {
    static Fn *mul[9] = {0,0,mul_2,mul_3,mul_4,mul_5,mul_6,mul_7,mul_8};
    int const mul_delta = -1;
    if (b->head[b->last].fn == mul[b->depth - mul_delta]) {  // if the last instruction was a mul...
        rewind(b, mul_delta);                                // ... rewind it

        static Fn *mad[9] = {0,0,0,mad_3,mad_4,mad_5,mad_6,mad_7,mad_8};
        struct Inst inst = { .fn=mad[b->depth] };
        append(b,-2,inst,inst);                              // ... and replace with a fused mul-add
        return;
    }

    // Not fusing with a previous mul, just append a normal add.
    static Fn *fn[9] = {0,0,add_2,add_3,add_4,add_5,add_6,add_7,add_8};
    struct Inst inst = { .fn=fn[b->depth] };
    append(b,-1,inst,inst);
}

// Generally head instructions should operate on one value at a time, while
// body instructions operate on a full K values at a time.  We haven't had to
// care about this yet; the math instructions above always just create one Inst
// and pass it as both head and body.  It's no big deal if we do some redundant
// math and throw it away, and generally vector math is as fast as scalar math.
//
// But instructions that touch memory need to be much more careful.  We don't
// want to write more memory than we're supposed to, and it's often not safe to
// even read too much.  That's segfault city, population us.
//
// So any instruction that reads or writes memory may be split on one further axis,
// into head instructions that operate on one value and body that operate on K.

defn(head_store_1) { float *dst = ptr[ip->ix]; dst[i] = v0[0]; next; }
defn(head_store_2) { float *dst = ptr[ip->ix]; dst[i] = v1[0]; next; }
defn(head_store_3) { float *dst = ptr[ip->ix]; dst[i] = v2[0]; next; }
defn(head_store_4) { float *dst = ptr[ip->ix]; dst[i] = v3[0]; next; }
defn(head_store_5) { float *dst = ptr[ip->ix]; dst[i] = v4[0]; next; }
defn(head_store_6) { float *dst = ptr[ip->ix]; dst[i] = v5[0]; next; }
defn(head_store_7) { float *dst = ptr[ip->ix]; dst[i] = v6[0]; next; }
defn(head_store_8) { float *dst = ptr[ip->ix]; dst[i] = v7[0]; next; }

defn(body_store_1) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v0; next; }
defn(body_store_2) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v1; next; }
defn(body_store_3) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v2; next; }
defn(body_store_4) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v3; next; }
defn(body_store_5) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v4; next; }
defn(body_store_6) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v5; next; }
defn(body_store_7) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v6; next; }
defn(body_store_8) { float *dst = ptr[ip->ix]; *(F*)(dst+i) = v7; next; }

void store(struct Builder *b, int ix) {
    static Fn *head_fn[9] = {0, head_store_1, head_store_2, head_store_3, head_store_4
                              , head_store_5, head_store_6, head_store_7, head_store_8},
              *body_fn[9] = {0, body_store_1, body_store_2, body_store_3, body_store_4
                              , body_store_5, body_store_6, body_store_7, body_store_8};
    append(b,-1, (struct Inst){.fn=head_fn[b->depth], .ix=ix}
               , (struct Inst){.fn=body_fn[b->depth], .ix=ix});
}

defn(head_load_0) { float const *src = ptr[ip->ix]; v0[0] = src[i]; next; }
defn(head_load_1) { float const *src = ptr[ip->ix]; v1[0] = src[i]; next; }
defn(head_load_2) { float const *src = ptr[ip->ix]; v2[0] = src[i]; next; }
defn(head_load_3) { float const *src = ptr[ip->ix]; v3[0] = src[i]; next; }
defn(head_load_4) { float const *src = ptr[ip->ix]; v4[0] = src[i]; next; }
defn(head_load_5) { float const *src = ptr[ip->ix]; v5[0] = src[i]; next; }
defn(head_load_6) { float const *src = ptr[ip->ix]; v6[0] = src[i]; next; }
defn(head_load_7) { float const *src = ptr[ip->ix]; v7[0] = src[i]; next; }

defn(body_load_0) { float const *src = ptr[ip->ix]; v0 = *(F const*)(src+i); next; }
defn(body_load_1) { float const *src = ptr[ip->ix]; v1 = *(F const*)(src+i); next; }
defn(body_load_2) { float const *src = ptr[ip->ix]; v2 = *(F const*)(src+i); next; }
defn(body_load_3) { float const *src = ptr[ip->ix]; v3 = *(F const*)(src+i); next; }
defn(body_load_4) { float const *src = ptr[ip->ix]; v4 = *(F const*)(src+i); next; }
defn(body_load_5) { float const *src = ptr[ip->ix]; v5 = *(F const*)(src+i); next; }
defn(body_load_6) { float const *src = ptr[ip->ix]; v6 = *(F const*)(src+i); next; }
defn(body_load_7) { float const *src = ptr[ip->ix]; v7 = *(F const*)(src+i); next; }

void load(struct Builder *b, int ix) {
    static Fn *head_fn[9] = {head_load_0, head_load_1, head_load_2, head_load_3,
                             head_load_4, head_load_5, head_load_6, head_load_7, 0},
              *body_fn[9] = {body_load_0, body_load_1, body_load_2, body_load_3,
                             body_load_4, body_load_5, body_load_6, body_load_7, 0};
    append(b,+1, (struct Inst){.fn=head_fn[b->depth], .ix=ix}
               , (struct Inst){.fn=body_fn[b->depth], .ix=ix});
}

// splat(T,v) broadcasts one scalar v to all lanes of a vector of type T.
// This funny formulation works efficiently for all vector types (int,float) and depths (4,8,etc.)
#define splat(T,v) (((T){0} + 1) * (v))

// No need for head/body split for uni().  It always just reads one float from memory.
defn(uni_0) { float const *uni = ptr[ip->ix]; v0 = splat(F, *uni); next; }
defn(uni_1) { float const *uni = ptr[ip->ix]; v1 = splat(F, *uni); next; }
defn(uni_2) { float const *uni = ptr[ip->ix]; v2 = splat(F, *uni); next; }
defn(uni_3) { float const *uni = ptr[ip->ix]; v3 = splat(F, *uni); next; }
defn(uni_4) { float const *uni = ptr[ip->ix]; v4 = splat(F, *uni); next; }
defn(uni_5) { float const *uni = ptr[ip->ix]; v5 = splat(F, *uni); next; }
defn(uni_6) { float const *uni = ptr[ip->ix]; v6 = splat(F, *uni); next; }
defn(uni_7) { float const *uni = ptr[ip->ix]; v7 = splat(F, *uni); next; }

void uni(struct Builder *b, int ix) {
    static Fn *fn[9] = {uni_0,uni_1,uni_2,uni_3,uni_4,uni_5,uni_6,uni_7,0};
    struct Inst inst = {.fn=fn[b->depth], .ix=ix};
    append(b,+1,inst,inst);
}

defn(imm_0) { v0 = splat(F, ip->imm); next; }
defn(imm_1) { v1 = splat(F, ip->imm); next; }
defn(imm_2) { v2 = splat(F, ip->imm); next; }
defn(imm_3) { v3 = splat(F, ip->imm); next; }
defn(imm_4) { v4 = splat(F, ip->imm); next; }
defn(imm_5) { v5 = splat(F, ip->imm); next; }
defn(imm_6) { v6 = splat(F, ip->imm); next; }
defn(imm_7) { v7 = splat(F, ip->imm); next; }

void imm(struct Builder *b, float imm) {
    static Fn *fn[9] = {imm_0,imm_1,imm_2,imm_3,imm_4,imm_5,imm_6,imm_7,0};
    struct Inst inst = {.fn=fn[b->depth], .imm=imm};
    append(b,+1,inst,inst);
}

// The functions run(), head_loop() and body_loop() together are the control flow for our Program.
void run(struct Program const *p, int N, void* ptr[]) {
    if (N > 0) {
        // If there are any N%K head program instances to run, start with head instructions,
        // otherwise just jump straight to the body to do N/K loops of K-sized gangs.
        struct Inst const *start = N%K ? p->head
                                       : p->body;
        F z = {0};
        start->fn(start,0,N,ptr, z,z,z,z, z,z,z,z);
    }
}

static void head_loop(struct Inst const *ip, int i, int N, void* ptr[],
                      F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    // When we arrive at head_loop we've just finished one program instance,
    // one pass of head instructions:
    //    [   inst    ]    (p->head)
    //    [   inst    ]
    //    [   inst    ]
    //    [   inst    ]
    //    [ head_loop ]
    i += 1;
    N -= 1;
    if (N > 0) {
        // If we're still not aligned with K, loop back to head, otherwise continue on to the body.
        struct Program const *p = ip->ptr;
        struct Inst const *jump = N%K ? p->head
                                      : p->body;
        jump->fn(jump,i,N,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}
static void body_loop(struct Inst const *ip, int i, int N, void* ptr[],
                      F v0, F v1, F v2, F v3, F v4, F v5, F v6, F v7) {
    // Just as in head_loop(), except now we've just finished K program instances.
    //    [   inst    ]    (p->body)
    //    [   inst    ]
    //    [   inst    ]
    //    [   inst    ]
    //    [ body_loop ]
    i += K;
    N -= K;
    if (N > 0) {
        // Can't have come unaligned to K now, so always just jump back to the body pointer.
        struct Inst const *body = ip->ptr;
        body->fn(body,i,N,ptr, v0,v1,v2,v3,v4,v5,v6,v7);
    }
}

struct Program* done(struct Builder *b) {
    // A final dusting that connects it all together.
    // Turn our Builder into a Program by...
    struct Program *p = &b->p;

    // ... appending the head_loop and body_loop instructions (with the pointers they need)
    append(b, 0, (struct Inst){.fn=head_loop, .ptr=p}
               , (struct Inst){.fn=body_loop, .ptr=p->body});
    // ... and filling in p->head pointer.  We could recalculate this every time but this is easier.
    p->head = b->head;
    return p;
}
