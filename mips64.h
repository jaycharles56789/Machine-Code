#ifndef MIPS64_H
#define MIPS64_H

#include <stddef.h>

typedef struct {
    const char *name;
    const char *code;
} Reg;

/* Extern declaration */
extern const Reg regs[];
extern const size_t regs_count;

const char *get_register_code(const char *name);

#endif 



