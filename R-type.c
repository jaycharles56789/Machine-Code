#include <stdio.h>

typedef struct {
    const char *name;
    const char *code;
} R_type;

static const R_type r_type[] = {
    {"DADDU"    , "000000"},
    {"DDIV"     , "000000"},
    {"DMOD"     , "000000"},
    {"DDIVU"    , "000000"},
    {"DMODU"    , "000000"},
    {"DSUBU"    , "000000"},
    {"DMUL"     , "000000"},
    {"DMUH"     , "000000"},
    {"DMULU"    , "000000"},
    {"DMUHU"    , "000000"},
    {"NOP"      , "000000"},  
    {"BITSWAP"  , "011111"},
    {"DBITSWAP" , "011111"}
};

int get_r_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]) {
    for (size_t i = 0; i < sizeof(r_type) / sizeof(r_type[0]); ++i) {
        if (strcmp(r_type[i].name, mnemonic) == 0) {
            strcpy(out_code, r_type[i].code);
            return 0;
        }
    }
    return 1; 
}