#include <string.h>
#include "mips64.h"

typedef struct {
    const char *Name;
    const char *Op_code;
} Registers;

static const Registers registers[] = {
    {"r0", "00000"},
    {"r1", "00001"},
    {"r2", "00010"},
    {"r3", "00011"},
    {"r4", "00100"},
    {"r5", "00101"},
    {"r6", "00110"},
    {"r7", "00111"},
    {"r8", "01000"},
    {"r9", "01001"},
    {"r10", "01010"},
    {"r11", "01011"},
    {"r12", "01100"},
    {"r13", "01101"},
    {"r14", "01110"},
    {"r15", "01111"},
    {"r16", "10000"},
    {"r17", "10001"},
    {"r18", "10010"},
    {"r19", "10011"},
    {"r20", "10100"},
    {"r21", "10101"},
    {"r22", "10110"},
    {"r23", "10111"},
    {"r24", "11000"},
    {"r25", "11001"},
    {"r26", "11010"},
    {"r27", "11011"},
    {"r28", "11100"},
    {"r29", "11101"},
    {"r30", "11110"},
    {"r31", "11111"}
};

const size_t regs_count = sizeof(registers) / sizeof(registers[0]);

/*
 * ============================================
 * Function: get_register_code
 * Purpose: Gets the binary opcode for a register given its name
 * Parameters: name - register name (e.g., "r0", "r1", etc.)
 * Returns: Pointer to binary opcode string, or NULL if not found
 * ============================================
 */
const char *get_register_code(const char *name) {
    for (size_t i = 0; i < regs_count; ++i) {
        if (strcmp(registers[i].Name, name) == 0) {
            return registers[i].Op_code;
        }
    }
    return NULL; // Not found
}

/*
 * ============================================
 * Function: get_register_name
 * Purpose: Gets the register name given its number (0-31)
 * Parameters: reg_num - register number (0 to 31)
 * Returns: Pointer to register name string, or NULL if out of range
 * ============================================
 */
const char *get_register_name(int reg_num) {
    if (reg_num >= 0 && reg_num < (int)regs_count) {
        return registers[reg_num].Name;
    }
    return NULL; // Out of range
}