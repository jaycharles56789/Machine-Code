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

/* ---------- Symbol table ---------- */
#define MAX_SYMBOLS 128
typedef struct {
    char name[64];
    int value;
    int initialized;
} Symbol;

Symbol symbol_table[MAX_SYMBOLS];
size_t symbol_count = 0;

/* ---------- Functions prototype ---------- */
char *open_source_file();
void compile_to_assemble(const char *source_code, const char *);
void lexical_analyzer(const char *source_code);
void parsing_statement(const char *token);
void semantic_analyzer(const char *token);
static void white_space_trim(char *s);
char *skip_escape_sequences_and_comments(const char *source_code, int *i, int *line);
void add_variable(const char *name, int value, int initialized);

int main(void) {

    

    char *source_code = open_source_file();
    if(source_code == NULL) {
        return 1;
    }

    printf("\ninput source code:\n%s\n", source_code);
    
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
    char *duplecated_source = strdup(source_code ? source_code : "");
    if(duplecated_source == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(output_file);
        return;
    }

    const char *p = duplecated_source;
    while (*p) {
        const char *semi = strchr(p, ';'); // find next semicolon
        if (!semi) {
            printf("Syntax Error: Missing semicolon in statement '%s'\n", p);
            break;
        }
    
        size_t len = semi - p + 1; // include semicolon
        char stmt[512];
        strncpy(stmt, p, len);
        stmt[len] = '\0';
        white_space_trim(stmt);
    
        if (stmt[0] != '\0') {
            semantic_analyzer(stmt);   // statement still has semicolon
            parsing_statement(stmt);   // parse variable declarations
        }
    
        p = semi + 1; // move past this semicolon
    }

    lexical_analyzer(duplecated_source);

    free(duplecated_source);
    duplecated_source = NULL;

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
            char *tokenizer = (char *)malloc(length + 1);
            if(tokenizer == NULL) {
                fprintf(stderr, "ERROR: Memory allocation failed.\n");
                return;
            }

            strncpy(tokenizer, &source_code[start], length);

            tokenizer[length] = '\0';

            // printf("Line %d: Identifier/Keyword: %s\n", line, tokenizer);

            parsing_statement(tokenizer);

            free(tokenizer);
        } else if(DIGIT_CHARACTER(source_code[i])) {
            // Numeric literal
            int start = i;
            while(DIGIT_CHARACTER(source_code[i])) {
                i++;
            }

            int length = i - start;
            char *tokenizer = (char *)malloc(length + 1);
            if(tokenizer == NULL) {
                fprintf(stderr, "ERROR: Memory allocation failed.\n");
                return;
            }

            strncpy(tokenizer, &source_code[start], length);

            tokenizer[length] = '\0';

            // printf("Line %d: Numeric Literal: %s\n", line, tokenizer);

            free(tokenizer);
        } else {
            // Other characters (operators, punctuation, etc.)
            // printf("Line %d: Symbol: %c\n", line, source_code[i]);
            i++;
        }
    }
}

void parsing_statement(const char *token) {
    if (!token) return;

    char temp[256];
    strncpy(temp, token, sizeof(temp)-1);
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

        add_variable(var_name, value, initialized);

        // printf("Declare variable: %s, value=%d, initialized=%d\n",
        //        var_name, value, initialized);
        return;
    }

    // printf("Unrecognized statement: %s\n", temp);
}

void semantic_analyzer(const char *token) {
    if (!token) return;

    char temp[256];
    strncpy(temp, token, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    white_space_trim(temp);

    if (temp[0] == '\0') return;

    // --- 1) Check for missing semicolon ---
    size_t len = strlen(temp);
    if (temp[len - 1] != ';') {
        printf("Syntax Error: Missing semicolon in statement '%s'\n", temp);
        return;
    }

    // Remove semicolon for easier parsing
    temp[len - 1] = '\0';
    white_space_trim(temp);

    // --- 2) Skip variable declarations ---
    if (strncmp(temp, "int ", 4) == 0) return;

    // --- 3) Assignment statements ---
    char lhs[64];
    char rhs[256];
    if (sscanf(temp, "%63[^=] = %255[^\n]", lhs, rhs) == 2) {
        white_space_trim(lhs);
        white_space_trim(rhs);

        // Check LHS variable exists
        int lhs_found = 0;
        for (size_t i = 0; i < symbol_count; ++i) {
            if (strcmp(symbol_table[i].name, lhs) == 0) {
                lhs_found = 1;
                break;
            }
        }
        if (!lhs_found) {
            printf("Semantic Error: Variable '%s' used before declaration (LHS)\n", lhs);
        }

        // Check RHS variables
        char var[64];
        const char *p = rhs;
        while (*p) {
            if (APHABETIC_CHARACTER(*p)) {
                int j = 0;
                while (ALPHANUMERIC_CHARACTER(*p) && j < 63) {
                    var[j++] = *p++;
                }
                var[j] = '\0';

                // Is this variable declared?
                int found = 0;
                int initialized = 0;
                for (size_t k = 0; k < symbol_count; ++k) {
                    if (strcmp(symbol_table[k].name, var) == 0) {
                        found = 1;
                        initialized = symbol_table[k].initialized;
                        break;
                    }
                }
                if (!found) {
                    printf("Semantic Error: Variable '%s' used before declaration (RHS)\n", var);
                } else if (!initialized) {
                    printf("Semantic Warning: Variable '%s' may be used uninitialized\n", var);
                }
            } else {
                p++; // skip operators / numbers
            }
        }

        // Mark LHS as initialized if it exists
        if (lhs_found) {
            for (size_t i = 0; i < symbol_count; ++i) {
                if (strcmp(symbol_table[i].name, lhs) == 0) {
                    symbol_table[i].initialized = 1;
                    break;
                }
            }
        }

        return;
    }

    // --- 4) Check other variable usage in expressions ---
    const char *p = temp;
    char var[64];
    while (*p) {
        if (APHABETIC_CHARACTER(*p)) {
            int j = 0;
            while (ALPHANUMERIC_CHARACTER(*p) && j < 63) {
                var[j++] = *p++;
            }
            var[j] = '\0';

            int found = 0;
            int initialized = 0;
            for (size_t k = 0; k < symbol_count; ++k) {
                if (strcmp(symbol_table[k].name, var) == 0) {
                    found = 1;
                    initialized = symbol_table[k].initialized;
                    break;
                }
            }
            if (!found) {
                printf("Semantic Error: Variable '%s' used before declaration\n", var);
            } else if (!initialized) {
                printf("Semantic Warning: Variable '%s' may be used uninitialized\n", var);
            }
        } else {
            p++;
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

/* ---------- Skip escape sequences and comments ---------- */
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

/* ---------- Symbol table functions ---------- */
void add_variable(const char *name, int value, int initialized) {
    if (symbol_count >= MAX_SYMBOLS) return;
    strncpy(symbol_table[symbol_count].name, name, 63);
    symbol_table[symbol_count].value = value;
    symbol_table[symbol_count].initialized = initialized;
    symbol_count++;
}