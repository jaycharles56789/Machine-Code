#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "MIPS64.h"

// Functions prototype
char *open_source_file();

int main(void) {
    char *source_coude = open_source_file();
    if(source_coude == NULL) {
        return 1;
    }
    printf("input source code:\n%s\n", source_coude);
    fclose(source_coude);
    return 0;
}

// Read source code from file
char *open_source_file() {
    FILE *source_code = fopen("input.txt","r");
    if(source_code == NULL) {
        fprintf(stderr, "ERROR: File cannot be opened.\n");
        return NULL;
    }

    // 
    char *line_of_code = (char *)malloc(1024 * sizeof(char));
    if(line_of_code == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(source_code);
        return NULL;
    }
    
    line_of_code[0] = '\0';
    
    char lines[1024];
    while(fgets(lines, sizeof(lines), source_code) != NULL) {
        strcat(line_of_code, lines);
    }

    fclose(source_code);
    return line_of_code;
}