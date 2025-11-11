#include <stdio.h>
#include <string.h>
#include "MIPS64.h"

typedef struct {
    const char *name;
    const char *code;
} J_type;

static const J_type j_type[] = {
    {"BALC", "111010"},
    {"BC",   "110010"},
    {"J",    "000010"},
    {"JAL",  "000011"}
};

int get_j_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]) {
    int i;
    for (i = 0; i < sizeof(j_type)/sizeof(j_type[0]); i++) {
        if (strcmp(j_type[i].name, mnemonic) == 0) {
            strcpy(out_code, j_type[i].code);
            return 1; // found
        }
    }
    return 0; // not found
}
