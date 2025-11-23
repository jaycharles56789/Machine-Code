#include <stdio.h>
#include <string.h>
#include "mips64.h"

typedef struct { const char *mn; const char *opcode; const char *funct; } Rentry;

static const Rentry rtab[] = {
    {"DADDU", "000000", "000000"},
    {"DSUBU", "000000", "000011"},
    {"DMULT", "000000", "000010"}, 
    {"DDIV",  "000000", "000110"},
    {"DADD",  "000000", "000100"}, 
    {"DSUB",  "000000", "000111"}  
};

int get_r_opcode(const char *mnemonic, char opcode_out[OPCODE_LEN], char funct_out[FUNCT_LEN]) {
    for (size_t i = 0; i < sizeof(rtab)/sizeof(rtab[0]); ++i) {
        if (strcmp(rtab[i].mn, mnemonic) == 0) {
            strncpy(opcode_out, rtab[i].opcode, OPCODE_LEN);
            opcode_out[OPCODE_LEN-1] = '\0';
            strncpy(funct_out, rtab[i].funct, FUNCT_LEN);
            funct_out[FUNCT_LEN-1] = '\0';
            return 1;
        }
    }
    return 0;
}
