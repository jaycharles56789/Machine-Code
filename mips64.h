#ifndef MIPS64_H
#define MIPS64_H


#define REG_CODE_LEN 6    
#define OPCODE_LEN   8    
#define FUNCT_LEN    8    

#include <stddef.h>


int get_register_code(const char *name, char out_code[REG_CODE_LEN]);


int get_i_opcode(const char *mnemonic, char out_code[OPCODE_LEN]);


int get_r_opcode(const char *mnemonic, char opcode_out[OPCODE_LEN], char funct_out[FUNCT_LEN]);

#endif 



