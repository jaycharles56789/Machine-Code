#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function: converts a hex string (like "0x3C000000") to binary
void hex_to_binary(const char *hex) {
    unsigned int value;
    sscanf(hex, "%x", &value);  // Convert hex string to integer

    // Print as 32-bit binary
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
    }
    printf("\n");
}

void print_machine_code(const char *mips_code) {
    if (strcmp(mips_code, "li $t0, 5") == 0) {
        printf("Machine Code (binary): ");
        hex_to_binary("0x3C000000");
        printf("Machine Code (binary): ");
        hex_to_binary("0x35080005");
    } else if (strcmp(mips_code, "sw $t0, a") == 0) {
        printf("Machine Code (binary): ");
        hex_to_binary("0xAD200000");
    } else if (strcmp(mips_code, "addi $t0, $t0, 3") == 0) {
        printf("Machine Code (binary): ");
        hex_to_binary("0x21310003");
    } else if (strcmp(mips_code, "lw $t0, b") == 0) {
        printf("Machine Code (binary): ");
        hex_to_binary("0x8D200000");
    } else if (strcmp(mips_code, "add $t2, $t0, $t1") == 0) {
        printf("Machine Code (binary): ");
        hex_to_binary("0x00000020");
    } else if (strcmp(mips_code, "sw $t2, result") == 0) {
        printf("Machine Code (binary): ");
        hex_to_binary("0xAD230000");
    } else {
        printf("Machine Code: Unknown\n");
    }
}

void convert_to_mips(const char *c_code) {
    if (strstr(c_code, "int a = ") != NULL) {
        int value;
        sscanf(c_code, "int a = %d;", &value);
        
        printf("MIPS Assembly: li $t0, %d\n", value);
        printf("MIPS Assembly: sw $t0, a\n");

        print_machine_code("li $t0, 5");
        print_machine_code("sw $t0, a");
    } else if (strstr(c_code, "int a = b + ") != NULL) {
        int value;
        sscanf(c_code, "int a = b + %d;", &value);
        
        printf("MIPS Assembly: lw $t0, b\n");
        printf("MIPS Assembly: addi $t0, $t0, %d\n", value);
        printf("MIPS Assembly: sw $t0, a\n");

        print_machine_code("lw $t0, b");
        print_machine_code("addi $t0, $t0, 3");
        print_machine_code("sw $t0, a");
    } else if (strstr(c_code, "int result = a + b;") != NULL) {
        printf("MIPS Assembly: lw $t0, a\n");
        printf("MIPS Assembly: lw $t1, b\n");
        printf("MIPS Assembly: add $t2, $t0, $t1\n");
        printf("MIPS Assembly: sw $t2, result\n");

        print_machine_code("lw $t0, a");
        print_machine_code("lw $t1, b");
        print_machine_code("add $t2, $t0, $t1");
        print_machine_code("sw $t2, result");
    } else {
        printf("Unsupported C code\n");
    }
}

int main() {
    char c_code[256];

    printf("Enter C code (examples: 'int a = 5;', 'int a = b + 3;', 'int result = a + b;'):\n");
    fgets(c_code, sizeof(c_code), stdin);

    size_t len = strlen(c_code);
    if (len > 0 && c_code[len - 1] == '\n') {
        c_code[len - 1] = '\0';
    }

    convert_to_mips(c_code);
    return 0;
}
