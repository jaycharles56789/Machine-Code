#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mips64.h"

void transform_to_mips(const char *c_based);

int main(void) {
    char c_based_variable[128][256];

    printf("Enter a simple C-based variable assignment\n(e.g., int a = 5; || int b; || int c = a + b; || a + b + c):\n");

    while(1) {

        fgets(c_based_variable, sizeof(c_based_variable), stdin);
        c_based_variable[strcspn(c_based_variable, "\n")] = '\0';

        if (strcmp(c_based_variable, "exit") == 0) {
            break;
        }

        // Check if the input starts with "int "
        if(strncmp(c_based_variable, "int ", 4) == 0) {
            // skipping "int " and space
            char *var = c_based_variable + 4; 
            while (*var == ' ') var++; 

            if(strcmp(var, "int") == 0) {
                printf("Invalid: Data type \n");
                continue;
            }

            // Reject if empty or only semicolon
            if (*var == ';' || *var == '\0') {
                printf("Invalid: missing variable name\n");
                continue;
            }

            // Reject if first char of var is not a letter or underscore
            if (!isalpha(*var)) {
                printf("Invalid: variable must start with a letter\n");
                continue;
            }
        }
    }

    for(int i = 0; i < strlen(c_based_variable); i++) {
        printf("%c", c_based_variable[i]);
    }

    // transform_to_mips(c_based_variable);

    return 0;
}
void transform_to_mips(const char *c_based) {
    
}