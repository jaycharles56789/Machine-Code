#ifndef MIPS64_H
#define MIPS64_H

#include <stddef.h>

#define REG_CODE_STR_LEN 6
#define OPCODE_STR_LEN 7

// ===== Registers.c =====
int get_register_code(const char *name, char out_code[REG_CODE_STR_LEN]);

// ===== I-type.c =====
int get_i_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

// ===== J-type.c =====
int get_j_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

// ===== R-type.c =====
int get_r_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

// ===== main translator =====
int transform_to_assembly(const char *input_line, char *out_buf, size_t out_buf_sz);

#endif

