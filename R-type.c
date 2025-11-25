#include <string.h>
#include "mips64.h"

const R_type r_type[] = {
    {"DMULT", "000000", "00010", "011100"}, 
    {"DDIV",  "000000", "00010", "011110"},
    {"DADDU", "000000", "00000", "101101"},
    {"DSUBU", "000000", "00000", "101111"},
};

const size_t r_type_count = sizeof(r_type) / sizeof(r_type[0]);

const R_type *get_r_type_code(const char *name) {
    for (size_t i = 0; i < r_type_count; ++i) {
        if (strcmp(r_type[i].Name, name) == 0) {
            return &r_type[i];
        }
    }
    return NULL; // Not found
}

