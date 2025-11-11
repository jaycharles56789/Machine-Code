#include <stdio.h>
#include <string.h>
#include "MIPS64.h"

int main(void) {
    char input[128];
    char assembly[256];

    printf("Enter a simple C-based variable assignment\n");
    printf("(Examples: int a = 5; | int c = a + b; | a + b + c)\n\n");

    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0';  // remove newline

    if (strcmp(input, "int a = 5;") == 0) {
        strcpy(assembly, "DADDIU $t0, $zero, 5   # a = 5");
    }
    else if (strcmp(input, "int b = 10;") == 0) {
        strcpy(assembly, "DADDIU $t1, $zero, 10   # b = 10");
    }
    else if (strcmp(input, "int c = a + b;") == 0) {
        strcpy(assembly, "DADDU $t2, $t0, $t1   # c = a + b");
    }
    else if (strcmp(input, "int c = a - b;") == 0) {
        strcpy(assembly, "DSUBU $t2, $t0, $t1   # c = a - b");
    }
    else if (strcmp(input, "a + b + c") == 0) {
        strcpy(assembly, "DADDU $t3, $t0, $t1\nDADDU $t3, $t3, $t2   # a + b + c");
    }
    else {
        strcpy(assembly, "Input not recognized or supported.");
    }

    printf("\nMIPS64 Assembly:\n%s\n", assembly);
    return 0;
}
