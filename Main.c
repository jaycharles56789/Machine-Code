#include <stdio.h>
#include <string.h>
#include "mips64.h"

int main(void) {
    char c_based_variable[128];
    printf("");
    printf("Enter a simple C-based variable assignment\n(e.g., int a = 5; || int b; || int c = a + b; || a + b + c):\n");

    fgets(c_based_variable, sizeof(c_based_variable), stdin);
    c_based_variable[strcspn(c_based_variable, "\n")] = '\0';

    trasnform_to_assembly(c_based_variable);

    
    return 0;
}