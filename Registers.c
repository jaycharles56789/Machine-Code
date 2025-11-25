#include <string.h>
#include "mips64.h"

const Reg regs[] = {
    {"$zero","00000"}, {"r0","00000"},
    {"$t0","01000"}, {"r8","01000"},
    {"$t1","01001"}, {"r9","01001"},
    {"$t2","01010"}, {"r10","01010"},
    {"$t3","01011"}, {"r11","01011"},
    {"r1","00001"}, {"r2","00010"}, {"r3","00011"},
    {"r4","00100"}, {"r5","00101"}, {"r6","00110"}, {"r7","00111"},
    {"r12","01100"}, {"r13","01101"}, {"r14","01110"}, {"r15","01111"}
};

const size_t regs_count = sizeof(regs) / sizeof(regs[0]);

const char *get_register_code(const char *name) {
    for (size_t i = 0; i < regs_count; ++i) {
        if (strcmp(regs[i].name, name) == 0) {
            return regs[i].code;
        }
    }
    return NULL; // Not found
}