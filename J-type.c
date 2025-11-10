#include <stdio.h>

typedef struct {
    const char *name;
    const char *code;
} J_type;

static const J_type j_type[] = {
    {"BALC" , "111010"},
    {"BC"   , "110010"},
    {"J"    , "000010"},
    {"JAL"  , "000011"}
};