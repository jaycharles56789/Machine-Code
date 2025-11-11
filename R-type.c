#include <stdio.h>
#include <string.h>
#include "MIPS64.h"

typedef struct {
    const char *name;
    const char *code;
} R_type;

static const R_type r_type[] = {
    {"DADDU", "000000"},
    {"DSUBU", "000000"},
    {"DMUL",  "000000"},
    {"DIV",   "000000"},
    {"NOP",   "000000"},
    {"BITSWAP", "011111"},
    {"DBITSWAP", "011111"}
};

int get_r_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]) {
    int i;
    for (i = 0; i < sizeof(r_type)/sizeof(r_type[0]); i++) {
        if (strcmp(r_type[i].name, mnemonic) == 0) {
            strcpy(out_code, r_type[i].code);
            return 1; // found
        }
    }
    return 0; // not found
}
