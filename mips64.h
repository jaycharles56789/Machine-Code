#ifndef _MIPS64_H_
#define _MIPS64_H_

#define OPCODE_STR_LEN 8   // Enough space for 6-bit opcode + null terminator

// Look through register names and return corresponding binary code
int get_register_code(const char *reg_name, char out_code[OPCODE_STR_LEN]);

// Look through R-type instruction mnemonics and return corresponding binary opcode
int get_r_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

// Look through I-type instruction mnemonics and return corresponding binary opcode
int get_i_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

// Look through J-type instruction mnemonics and return corresponding binary opcode
int get_j_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

#endif


