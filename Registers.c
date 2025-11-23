#include <stdio.h>
#include <string.h>
#include "mips64.h"


typedef struct { const char *name; const char *code; } Reg;

static const Reg regs[] = {
    {"$zero","00000"}, {"r0","00000"},
    {"$t0","01000"}, {"r8","01000"},
    {"$t1","01001"}, {"r9","01001"},
    {"$t2","01010"}, {"r10","01010"},
    {"$t3","01011"}, {"r11","01011"},
    {"r1","00001"}, {"r2","00010"}, {"r3","00011"},
    {"r4","00100"}, {"r5","00101"}, {"r6","00110"}, {"r7","00111"},
    {"r12","01100"}, {"r13","01101"}, {"r14","01110"}, {"r15","01111"}
};

int get_register_code(const char *name, char out_code[REG_CODE_LEN]) {
    for (size_t i = 0; i < sizeof(regs)/sizeof(regs[0]); ++i) {
        if (strcmp(regs[i].name, name) == 0) {
            strncpy(out_code, regs[i].code, REG_CODE_LEN);
            out_code[REG_CODE_LEN-1] = '\0';
            return 1;
        }
    }
    return 0; 
}
