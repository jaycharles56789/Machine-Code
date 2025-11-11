#include <stdio.h>
#include <string.h>
#include "MIPS64.h"

typedef struct {
    const char *name;
    const char *code;
} I_type;

static const I_type i_type[] = {
    {"DADDIU", "011001"},
    {"ANDI",   "001100"},
    {"ORI",    "001101"},
    {"XORI",   "001110"},
    {"SLTI",   "001010"},
    {"SLTIU",  "001011"},
    {"AUI",    "001111"},
    {"DAUI",   "011101"},
    {"LB",     "100000"},
    {"LBU",    "100100"},
    {"LD",     "110111"},
    {"LDC1",   "110101"},
    {"LH",     "100001"},
    {"LHU",    "100101"},
    {"LW",     "100011"},
    {"LWC1",   "110001"},
    {"LWU",    "100111"},
    {"LUI",    "001111"},
    {"SB",     "101000"},
    {"SD",     "111111"},
    {"SDC1",   "111101"},
    {"SH",     "101001"},
    {"SW",     "101011"},
    {"SWC1",   "111001"},
    {"B",      "000100"},
    {"BAL",    "000001"},
    {"BEQ",    "000100"},
    {"BNE",    "000101"}
};

int get_i_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]) {
    int i;
    for (i = 0; i < sizeof(i_type)/sizeof(i_type[0]); i++) {
        if (strcmp(i_type[i].name, mnemonic) == 0) {
            strcpy(out_code, i_type[i].code);
            return 1; // found
        }
    }
    return 0; // not found
}
