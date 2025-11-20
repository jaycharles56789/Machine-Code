#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "MIPS64.h"

#define ESCAPE_SEQUENCE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')

// Functions prototype
char *open_source_file();
void compile_to_assemble(const char *source_code, const char *);

int main(void) {
    char *source_coude = open_source_file();
    char file_name[] = "assembly.asm";
    if(source_coude == NULL) {
        return 1;
    }
    printf("input source code:\n%s\n", source_coude);
    compile_to_assemble(source_coude , file_name);
    fclose(source_coude);
    return 0;
}

// Read C-based variable from file
char *open_source_file() {
    FILE *source_code = fopen("input.txt","r");
    if(source_code == NULL) {
        fprintf(stderr, "ERROR: File cannot be opened.\n");
        return NULL;
    }

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
// Compile C-based source code to MIPS64 assembly
void compile_to_assemble(const char *source_code, const char *file_name) {



    FILE *output_file = fopen(file_name, "w");
    if(output_file == NULL) {
        fprintf(stderr, "ERROR: \"%s\"\n", file_name);
        return;
    }
}

