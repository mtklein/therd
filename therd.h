#pragma once

struct Program* program(void);
void start(struct Program const*, int n, void* ptr[]);

struct Program* store(struct Program*, int);
struct Program* load (struct Program*, int);
struct Program* uni  (struct Program*, int);
struct Program* imm  (struct Program*, float);

struct Program* mul(struct Program*);
struct Program* add(struct Program*);
