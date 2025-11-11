#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MIPS64.h"

// Remove spaces from a string
void remove_spaces(char *str) {
    int i, j = 0;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] != ' ' && str[i] != '\t') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

int main(void) {
    char input[128];
    char clean_input[128];
    char var1[32], var2[32], var3[32];
    char op;
    int value;
    char assembly[256];

    printf("Enter a simple C-based variable assignment or arithmetic operation:\n");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';  // remove newline

    strcpy(clean_input, input);
    remove_spaces(clean_input);

    assembly[0] = '\0'; // clear assembly output

    // Check for assignment: int var = value;
    if (strncmp(clean_input, "int", 3) == 0) {
        // Remove "int" keyword
        char *equalSign = strchr(clean_input, '=');
        if (equalSign != NULL) {
            // Split variable and value
            int len = equalSign - (clean_input + 3); // skip "int"
            strncpy(var1, clean_input + 3, len);
            var1[len] = '\0'; // terminate string

            // value after '='
            value = atoi(equalSign + 1); // convert string to integer

            // Decide which register to use (t0, t1, t2, ...)
            // Here we just assign based on first letter for demo
            char reg[5];
            if (var1[0] == 'a') strcpy(reg, "$t0");
            else if (var1[0] == 'b') strcpy(reg, "$t1");
            else if (var1[0] == 'c') strcpy(reg, "$t2");
            else strcpy(reg, "$t3");

            // Build assembly string
            strcpy(assembly, "DADDIU ");
            strcat(assembly, reg);
            strcat(assembly, ", $zero, ");
            char buffer[10];
            sprintf(buffer, "%d", value);
            strcat(assembly, buffer);
        }
    }
    // Check for addition/subtraction: var = var + var
    else {
        // Find '='
        char *equalSign = strchr(clean_input, '=');
        if (equalSign != NULL) {
            strncpy(var1, clean_input, equalSign - clean_input);
            var1[equalSign - clean_input] = '\0';
            char *expr = equalSign + 1;

            // Find operator
            if (strchr(expr, '+') != NULL) op = '+';
            else if (strchr(expr, '-') != NULL) op = '-';
            else op = '?';

            // Split variables
            char *token = strtok(expr, "+-");
            strcpy(var2, token);
            token = strtok(NULL, "+-");
            if (token != NULL) strcpy(var3, token);

            // Map registers
            char reg1[5], reg2[5], reg3[5];
            if (var1[0] == 'a') strcpy(reg1, "$t0");
            else if (var1[0] == 'b') strcpy(reg1, "$t1");
            else if (var1[0] == 'c') strcpy(reg1, "$t2");
            else strcpy(reg1, "$t3");

            if (var2[0] == 'a') strcpy(reg2, "$t0");
            else if (var2[0] == 'b') strcpy(reg2, "$t1");
            else if (var2[0] == 'c') strcpy(reg2, "$t2");
            else strcpy(reg2, "$t3");

            if (var3[0] == 'a') strcpy(reg3, "$t0");
            else if (var3[0] == 'b') strcpy(reg3, "$t1");
            else if (var3[0] == 'c') strcpy(reg3, "$t2");
            else strcpy(reg3, "$t3");

            // Build assembly
            if (op == '+') sprintf(assembly, "DADDU %s, %s, %s", reg1, reg2, reg3);
            else if (op == '-') sprintf(assembly, "DSUBU %s, %s, %s", reg1, reg2, reg3);
            else strcpy(assembly, "Unsupported operation");
        }
    }

    printf("\nMIPS64 Assembly:\n%s\n", assembly);
    return 0;
}

