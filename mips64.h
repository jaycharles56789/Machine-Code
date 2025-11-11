#ifndef _MIPS64_H_
#define _MIPS64_H_

#define OPCODE_STR_LEN 8   // Enough space for 6-bit opcode + null terminator

// Function prototypes for instruction lookups
int get_i_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);
int get_j_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);
int get_r_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

#endif


