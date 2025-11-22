
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "MIPS64.h"

#define ESCAPE_SEQUENCE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define COMMENT_CHARACTER(c) ((c) == '/' || (c) == '*')
#define APHABETIC_CHARACTER(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define DIGIT_CHARACTER(c) ((c) >= '0' && (c) <= '9')
#define ALPHANUMERIC_CHARACTER(c) (APHABETIC_CHARACTER(c) || DIGIT_CHARACTER(c))

// Linked list algorithm approach
typedef struct {
    char line[1024];

    struct Node *next;
} Node;


// Functions prototype
char *open_source_file();
void compile_to_assemble(const char *source_code, const char *);
char *skip_escape_sequences_and_comments(const char *source_code, int *i, int *line);
void lexical_analyzer(const char *source_code);
static void white_space_trim(char *s);

int main(int argc, char *argv[]) {

    // if(argc < 2) {
    //     fprintf(stderr, "Usage: %s\n", argv[0]);
    //     return 1;
    // }
    // char *source_coude = argv[1];

    char *source_coude = open_source_file();
    if(source_coude == NULL) {
        return 1;
    }

    printf("input source code:\n%s\n", source_coude);
    
    char file_name[] = "assembly.asm";
    compile_to_assemble(source_coude , file_name);

    free(source_coude);
    source_coude = NULL;

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

/* ----------- Compiled to runnable EduMIPS64 ------------- */
void compile_to_assemble(const char *source_code, const char *file_name) {
    // Creates and opens the output file
    FILE *output_file = fopen(file_name, "w");
    if(output_file == NULL) {
        fprintf(stderr, "ERROR: \"%s\" can't be made\n", file_name);
        return;
    }

    // Duplicate source_code to mutable buffer
    char *source_buff = strdup(source_code ? source_code : "");
    if(source_buff == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(output_file);
        return;
    }

    lexical_analyzer(source_buff);


    free(source_buff);
    source_buff = NULL;

    fclose(output_file);
}

void lexical_analyzer(const char *source_code) {
    int i = 0;
    int line = 1;

    while(source_code[i] != '\0') {
        skip_escape_sequences_and_comments(source_code, &i, &line);
        if(source_code[i] == '\0') break;

        if(APHABETIC_CHARACTER(source_code[i])) {
            // Identifier or keyword
            int start = i;
            while(ALPHANUMERIC_CHARACTER(source_code[i])) {
                i++;
            }
            int length = i - start;
            char *token = (char *)malloc(length + 1);
            strncpy(token, &source_code[start], length);
            token[length] = '\0';
            printf("Line %d: Identifier/Keyword: %s\n", line, token);
            free(token);
        } else if(DIGIT_CHARACTER(source_code[i])) {
            // Numeric literal
            int start = i;
            while(DIGIT_CHARACTER(source_code[i])) {
                i++;
            }
            int length = i - start;
            char *token = (char *)malloc(length + 1);
            strncpy(token, &source_code[start], length);
            token[length] = '\0';
            printf("Line %d: Numeric Literal: %s\n", line, token);
            free(token);
        } else {
            // Other characters (operators, punctuation, etc.)
            printf("Line %d: Symbol: %c\n", line, source_code[i]);
            i++;
        }
    }
}

/* ---------- trims white space ----------*/
static void white_space_trim(char *s) {
    char *p = s;
    
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }

    char *end = s + strlen(s) - 1;

    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0'; end--; 
    }
}

char *skip_escape_sequences_and_comments(const char *source_code, int *i, int *line) {
    while(source_code[*i] != '\0') {
        if(ESCAPE_SEQUENCE(source_code[*i])) {
            if(source_code[*i] == '\n') {
                (*line)++;
            }
            (*i)++;
        } else if(source_code[*i] == '/' && source_code[*i + 1] == '/') {
            // Single-line comment
            while(source_code[*i] != '\0' && source_code[*i] != '\n') (*i)++;
        } else if(source_code[*i] == '/' && source_code[*i + 1] == '*') {
            // Multi-line comment
            (*i) += 2; // Skip the /*
            while(source_code[*i] != '\0' && !(source_code[*i] == '*' && source_code[*i + 1] == '/')) {
                if(source_code[*i] == '\n') {
                    (*line)++;
                }
                (*i)++;
            }
            if(source_code[*i] != '\0') {
                (*i) += 2; // Skip the */
            }
        } else {
            break;
        }
    }
}