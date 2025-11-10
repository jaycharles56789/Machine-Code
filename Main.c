#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mips64.h"

void transform_to_mips(char c_based[][256], int line_count);

int main(void) {
    char c_based_variable[128][256];
    int line_counter = 0;

    printf("Enter a simple C-based variable assignment\n(e.g., int a = 5; || int b; || int c = a + b; || a + b + c):\n");

    while(1) {

        fgets(c_based_variable[line_counter], sizeof(c_based_variable[line_counter]), stdin);
        c_based_variable[line_counter][strcspn(c_based_variable[line_counter], "\n")] = '\0';

        if (strcmp(c_based_variable[line_counter], "exit") == 0) {
            break;
        }

        // Check if the input starts with "int "
        if(strncmp(c_based_variable[line_counter], "int ", 4) == 0) {
            // skipping "int " and space
            char *var = c_based_variable[line_counter] + 4; 
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

        line_counter++;
    }

    transform_to_mips(c_based_variable, line_counter);

    // for(int i = 0; i < line_counter; i++) {
    //     printf("%s\n", c_based_variable[i]);
    // }

    // transform_to_mips(c_based_variable);

    return 0;
}
void transform_to_mips(char c_based[][256], int line_count) {
    for(int i = 0; i < line_count; i++) {
        printf("%s\n", c_based[i]);
    }
}