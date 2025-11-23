#include <stdio.h>
#include <string.h>
#include "mips64.h"

typedef struct { const char *mn; const char *op; } Ientry;

static const Ientry itab[] = {
    {"DADDIU", "011001"},
    {"SB",     "101000"},
    {"LB",     "100000"}
};

int get_i_opcode(const char *mnemonic, char out_code[OPCODE_LEN]) {
    for (size_t i = 0; i < sizeof(itab)/sizeof(itab[0]); ++i) {
        if (strcmp(itab[i].mn, mnemonic) == 0) {
            strncpy(out_code, itab[i].op, OPCODE_LEN);
            out_code[OPCODE_LEN-1] = '\0';
            return 1;
        }
    }
    return 0;
}
