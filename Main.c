#include <stdio.h>
#include <string.h>
#include "MIPS64.h"

int main(void) {
    char c_based_variable[128];
    char assembly_output[256];

    printf("Enter a simple C-based variable assignment\n");
    printf("(e.g., int a = 5; | int c = a + b; | a + b + c):\n");

    fgets(c_based_variable, sizeof(c_based_variable), stdin);
    c_based_variable[strcspn(c_based_variable, "\n")] = '\0';

    if (transform_to_assembly(c_based_variable, assembly_output, sizeof(assembly_output)) == 0)
        printf("MIPS64 Assembly:\n%s\n", assembly_output);
    else
        printf("Error: could not translate input.\n");

    return 0;
}
