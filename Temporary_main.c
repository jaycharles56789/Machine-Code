/*
    TODO: Bai ikaw tiwas ug tarong ang uban wla nako ma huna2 an lain labad nas ulo 
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "MIPS64.h"

#define ESCAPE_SEQUENCE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define APHABETIC_CHARACTER(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define DIGIT_CHARACTER(c) ((c) >= '0' && (c) <= '9')
#define ALPHANUMERIC_CHARACTER(c) (APHABETIC_CHARACTER(c) || DIGIT_CHARACTER(c))

// Linked list algorithm approach
typedef struct {
    char line[1024];
    struct Node *next;
} Node;

/* ---------- Symbol table ---------- */
typedef struct {
    char name[64];
    int value;
    int initialized;
} Symbol;

/* ---------- Functions prototype ---------- */
char *open_source_file();
void compile_to_assemble(const char *source_code, const char *);
void lexical_analyzer(const char *source_code);
void parsing_statement(const char *statement);
char *skip_escape_sequences_and_comments(const char *source_code, int *i, int *line);
static void white_space_trim(char *s);

int main(int argc, char *argv[]) {

    (void)argc;
    (void)argv;

    // if(argc < 2) {
    //     fprintf(stderr, "Usage: %s\n", argv[0]);
    //     return 1;
    // }
    // char *source_coude = argv[1];

    char *source_code = open_source_file();
    if(source_code == NULL) {
        return 1;
    }

    printf("input source code:\n%s\n", source_code);
    
    char file_name[] = "assembly.asm";
    compile_to_assemble(source_code , file_name);

    free(source_code);
    source_code = NULL;

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
    char *duplecated_source_ = strdup(source_code ? source_code : "");
    if(duplecated_source_ == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(output_file);
        return;
    }

    lexical_analyzer(duplecated_source_);

    free(duplecated_source_);
    duplecated_source_ = NULL;

    fclose(output_file);
}

void lexical_analyzer(const char *source_code) {
    int i = 0, line = 1;

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

            parsing_statement(token);

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

void parsing_statement(const char *statement) {
    if (!statement) return;

    char temp[256];
    strncpy(temp, statement, sizeof(temp)-1);
    temp[sizeof(temp)-1] = '\0';
    white_space_trim(temp);

    if (temp[0] == '\0') return;

    // --- Variable declaration ---
    if (strncmp(temp, "int ", 4) == 0) {
        char var_name[64];
        int value = 0;
        int initialized = 0;

        if (sscanf(temp + 4, "%63[^=;] = %d", var_name, &value) == 2) {
            initialized = 1;
        } else if (sscanf(temp + 4, "%63[^;]", var_name) == 1) {
            initialized = 0;
        } else {
            printf("Syntax error in declaration: %s\n", temp);
            return;
        }

        white_space_trim(var_name);

        printf("Declare variable: %s, value=%d, initialized=%d\n",
               var_name, value, initialized);
        return;
    }

    // --- R-type instruction ---
    char instr[16];
    sscanf(temp, "%15s", instr);

    const R_type *r = get_r_type(instr);
    if (r) {
        char rd[8], rs[8], rt[8];
        if (sscanf(temp + strlen(instr), " %7[^,], %7[^,], %7s", rd, rs, rt) != 3) {
            printf("Invalid R-type format: %s\n", temp);
            return;
        }

        white_space_trim(rd);
        white_space_trim(rs);
        white_space_trim(rt);

        const char *rd_code = get_reg_code(rd);
        const char *rs_code = get_reg_code(rs);
        const char *rt_code = get_reg_code(rt);

        if (!rd_code || !rs_code || !rt_code) {
            printf("Unknown register in instruction: %s\n", temp);
            return;
        }

        printf("R-type binary: %s%s%s%s%s\n",
               r->Op_code, rs_code, rt_code, rd_code, r->Shamt, r->Funct);
        return;
    }

    printf("Unrecognized statement: %s\n", temp);
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