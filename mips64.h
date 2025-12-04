#ifndef MIPS64_H
#define MIPS64_H

// Register functions
const char *get_register_code(const char *name);
const char *get_register_name(int reg_num);

// Instruction type functions
const char *get_i_type_code(const char *name);

// R-type instruction structure
typedef struct {
    const char *Name;
    const char *Op_code;
    const char *Shamt;
    const char *Funct;
} R_type;

extern const R_type r_type[];
extern const size_t r_type_count;

const R_type *get_r_type_code(const char *name);

#endif