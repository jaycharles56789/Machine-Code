#ifndef _MIPS64_H_
#define _MIPS64_H_

#include <stddef.h> //for size_t

#define REG_CODE_STR_LEN 6
#define OPCODE_STR_LEN 7

#ifdef __cplusplus
extern "C" {
#endif

int get_register_code(const char *name, char out_code[REG_CODE_STR_LEN]);

int get_register_name(const char code[REG_CODE_STR_LEN - 1],
                      char *out_name, size_t out_name_sz);

int get_i_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);
int get_j_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

int get_r_opcode(const char *mnemonic, char out_code[OPCODE_STR_LEN]);

int transform_to_assembly(const char *input_line,
                          char *out_buf, size_t out_buf_sz);

#ifdef __cplusplus
}
#endif

#endif // _MIPS64_H_

