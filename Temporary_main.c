/*
 * Improved C to MIPS64 Compiler
 * Fixes: Expression parsing, operator precedence, register allocation
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "MIPS64.h"

#define ESCAPE_SEQUENCE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define ALPHABETIC_CHARACTER(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define DIGIT_CHARACTER(c) ((c) >= '0' && (c) <= '9')
#define ALPHANUMERIC_CHARACTER(c) (ALPHABETIC_CHARACTER(c) || DIGIT_CHARACTER(c))

/* ---------- Symbol table ---------- */
#define MAX_SYMBOLS 128
typedef struct {
    char name[64];
    int value;
    int initialized;
    int reg_num; // Assigned register number
} Symbol;

Symbol symbol_table[MAX_SYMBOLS];
size_t symbol_count = 0;

/* ---------- Expression Parsing ---------- */
typedef struct {
    char op;
    int precedence;
} Operator;

/* ---------- Functions prototype ---------- */
char *open_source_file(const char *filename);
void process_statements(const char *source_code, FILE *output_file);
void compile_to_assemble(const char *source_code, const char *filename);
void white_space_trim(char *s);
void skip_escape_sequences_and_comments(const char *source_code, int *i, int *line);
void add_variable(const char *name, int value, int initialized);
Symbol *find_symbol(const char *name);
void make_assembly_for_statement(FILE *output_file, const char *statement);
int evaluate_expression(const char *expr, FILE *output_file, int target_reg);
int get_operator_precedence(char op);
int is_operator(char c);

int main(void) {
    char *source_code = open_source_file("input.txt");
    if(source_code == NULL) {
        return 1;
    }

    printf("\nInput source code:\n%s\n", source_code);
    
    char file_name[] = "assembly.asm";
    compile_to_assemble(source_code, file_name);

    free(source_code);
    printf("\nAssembly code is generated successfully in '%s'\n", file_name);

    return 0;
}

// Read C-based variable from file
char *open_source_file(const char *filename) {
    FILE *source_code = fopen(filename, "r");
    if(source_code == NULL) {
        fprintf(stderr, "ERROR: File '%s' cannot be opened.\n", filename);
        return NULL;
    }

    // Allocate initial buffer
    size_t buffer_size = 1024;
    size_t total_length = 0;
    char *line_of_code = (char *)malloc(buffer_size * sizeof(char));
    
    if(line_of_code == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(source_code);
        return NULL;
    }
    
    line_of_code[0] = '\0';
    
    char lines[256];
    while(fgets(lines, sizeof(lines), source_code) != NULL) {
        size_t line_len = strlen(lines);
        
        // Resize buffer if needed
        if(total_length + line_len + 1 >= buffer_size) {
            buffer_size *= 2;
            char *new_buffer = (char *)realloc(line_of_code, buffer_size);
            if(new_buffer == NULL) {
                fprintf(stderr, "ERROR: Memory reallocation failed.\n");
                free(line_of_code);
                fclose(source_code);
                return NULL;
            }
            line_of_code = new_buffer;
        }
        
        strcat(line_of_code, lines);
        total_length += line_len;
    }

    fclose(source_code);
    return line_of_code;
}

/* ----------- Compiled to runnable EduMIPS64 ------------- */
void compile_to_assemble(const char *source_code, const char *file_name) {
    FILE *output_file = fopen(file_name, "w");
    if(output_file == NULL) {
        fprintf(stderr, "ERROR: '%s' can't be created\n", file_name);
        return;
    }

    fprintf(output_file, ".data\n\n");
    fprintf(output_file, ".code\n");
    fprintf(output_file, "main:\n");

    char *duplicated_source = strdup(source_code ? source_code : "");
    if(duplicated_source == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed.\n");
        fclose(output_file);
        return;
    }
    
    process_statements(duplicated_source, output_file);

    fprintf(output_file, "\n\thalt\n");

    free(duplicated_source);
    fclose(output_file);
}

void process_statements(const char *source_code, FILE *output_file) {
    int i = 0, line = 1;
    
    while (source_code[i] != '\0') {
        skip_escape_sequences_and_comments(source_code, &i, &line);
        if(source_code[i] == '\0') break;

        int start = i;
        // Find end of statement (semicolon)
        while (source_code[i] != '\0' && source_code[i] != ';') {
            if(source_code[i] == '\n') line++;
            i++;
        }
        
        if (i > start) {
            int len = i - start;
            char *statement = (char *)malloc(len + 2);
            if(statement) {
                strncpy(statement, &source_code[start], len);
                statement[len] = '\0';
                white_space_trim(statement);
                
                if (statement[0] != '\0' && statement[0] != ';') {
                    make_assembly_for_statement(output_file, statement);
                }
                free(statement);
            }
        }
        
        if (source_code[i] == ';') i++;
    }
}

// Evaluate expression with proper operator precedence
int evaluate_expression(const char *expr, FILE *output_file, int target_reg) {
    char tokens[64][64];
    int token_count = 0;
    int i = 0;
    
    // Tokenize expression
    while(expr[i] != '\0' && token_count < 64) {
        while(isspace(expr[i])) i++;
        if(expr[i] == '\0') break;
        
        if(ALPHABETIC_CHARACTER(expr[i])) {
            // Variable name
            int j = 0;
            while(ALPHANUMERIC_CHARACTER(expr[i]) && j < 63) {
                tokens[token_count][j++] = expr[i++];
            }
            tokens[token_count][j] = '\0';
            token_count++;
        } else if(DIGIT_CHARACTER(expr[i])) {
            // Number
            int j = 0;
            while(DIGIT_CHARACTER(expr[i]) && j < 63) {
                tokens[token_count][j++] = expr[i++];
            }
            tokens[token_count][j] = '\0';
            token_count++;
        } else if(is_operator(expr[i])) {
            tokens[token_count][0] = expr[i];
            tokens[token_count][1] = '\0';
            token_count++;
            i++;
        } else {
            i++;
        }
    }
    
    if(token_count == 0) return -1;
    
    // Handle single operand (just assignment)
    if(token_count == 1) {
        if(DIGIT_CHARACTER(tokens[0][0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", target_reg, tokens[0]);
            // fprintf(output_file, "# %s | %s | %s | %s");
        } else {
            Symbol *sym = find_symbol(tokens[0]);
            if(sym && sym->reg_num >= 0) {
                fprintf(output_file, "\tdaddu r%d, r0, r%d\n", target_reg, sym->reg_num);
            }
        }
        return target_reg;
    }
    
    // Simple two-operand expression with one operator
    if(token_count == 3) {
        int reg1 = target_reg + 1;
        int reg2 = target_reg + 2;
        
        // Load first operand
        if(DIGIT_CHARACTER(tokens[0][0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", reg1, tokens[0]);
        } else {
            Symbol *sym = find_symbol(tokens[0]);
            if(sym && sym->reg_num >= 0) {
                fprintf(output_file, "\tdaddu r%d, r0, r%d\n", reg1, sym->reg_num);
            }
        }
        
        // Load second operand
        if(DIGIT_CHARACTER(tokens[2][0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", reg2, tokens[2]);
        } else {
            Symbol *sym = find_symbol(tokens[2]);
            if(sym && sym->reg_num >= 0) {
                fprintf(output_file, "\tdaddu r%d, r0, r%d\n", reg2, sym->reg_num);
            }
        }
        
        // Perform operation
        char op = tokens[1][0];
        if(op == '+') {
            fprintf(output_file, "\tdaddu r%d, r%d, r%d\n", target_reg, reg1, reg2);
        } else if(op == '-') {
            fprintf(output_file, "\tdsubu r%d, r%d, r%d\n", target_reg, reg1, reg2);
        } else if(op == '*') {
            fprintf(output_file, "\tdmult r%d, r%d\n", reg1, reg2);
            fprintf(output_file, "\tmflo r%d\n", target_reg);
        } else if(op == '/') {
            fprintf(output_file, "\tddiv r%d, r%d\n", reg1, reg2);
            fprintf(output_file, "\tmflo r%d\n", target_reg);
        }
        
        return target_reg;
    }
    
    // Complex expression: evaluate with precedence (simplified)
    // For "x - y / z", we need to do division first
    int highest_prec_idx = -1;
    int highest_prec = -1;
    
    for(int j = 1; j < token_count; j += 2) {
        if(is_operator(tokens[j][0])) {
            int prec = get_operator_precedence(tokens[j][0]);
            if(prec > highest_prec) {
                highest_prec = prec;
                highest_prec_idx = j;
            }
        }
    }
    
    if(highest_prec_idx > 0) {
        // Evaluate high precedence operation first
        int reg1 = target_reg + 1;
        int reg2 = target_reg + 2;
        int result_reg = target_reg + 3;
        
        // Load operands for high precedence op
        char *left = tokens[highest_prec_idx - 1];
        char *right = tokens[highest_prec_idx + 1];
        char op = tokens[highest_prec_idx][0];
        
        if(DIGIT_CHARACTER(left[0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", reg1, left);
        } else {
            Symbol *sym = find_symbol(left);
            if(sym && sym->reg_num >= 0) {
                fprintf(output_file, "\tdaddu r%d, r0, r%d\n", reg1, sym->reg_num);
            }
        }
        
        if(DIGIT_CHARACTER(right[0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", reg2, right);
        } else {
            Symbol *sym = find_symbol(right);
            if(sym && sym->reg_num >= 0) {
                fprintf(output_file, "\tdaddu r%d, r0, r%d\n", reg2, sym->reg_num);
            }
        }
        
        // Perform high precedence operation
        if(op == '*') {
            fprintf(output_file, "\tdmult r%d, r%d\n", reg1, reg2);
            fprintf(output_file, "\tmflo r%d\n", result_reg);
        } else if(op == '/') {
            fprintf(output_file, "\tddiv r%d, r%d\n", reg1, reg2);
            fprintf(output_file, "\tmflo r%d\n", result_reg);
        }
        
        // Now handle remaining operations
        if(highest_prec_idx >= 2) {
            int reg_left = target_reg + 4;
            char *leftmost = tokens[0];
            
            if(DIGIT_CHARACTER(leftmost[0])) {
                fprintf(output_file, "\tdaddiu r%d, r0, %s\n", reg_left, leftmost);
            } else {
                Symbol *sym = find_symbol(leftmost);
                if(sym && sym->reg_num >= 0) {
                    fprintf(output_file, "\tdaddu r%d, r0, r%d\n", reg_left, sym->reg_num);
                }
            }
            
            char op2 = tokens[1][0];
            if(op2 == '+') {
                fprintf(output_file, "\tdaddu r%d, r%d, r%d\n", target_reg, reg_left, result_reg);
            } else if(op2 == '-') {
                fprintf(output_file, "\tdsubu r%d, r%d, r%d\n", target_reg, reg_left, result_reg);
            }
        }
    }
    
    return target_reg;
}

void make_assembly_for_statement(FILE *output_file, const char *statement) {
    if (!statement || !output_file) return;

    char temp[512];
    strncpy(temp, statement, sizeof(temp)-1);
    temp[sizeof(temp)-1] = '\0';
    white_space_trim(temp);

    if (temp[0] == '\0') return;

    // --- Variable declaration: int x; or int y = 10; ---
    if (strncmp(temp, "int ", 4) == 0) {
        char var_name[64];
        int value = 0;
        int initialized = 0;

        if (sscanf(temp + 4, "%63[^=;] = %d", var_name, &value) == 2) {
            initialized = 1;
            white_space_trim(var_name);
            add_variable(var_name, value, initialized);
            
            Symbol *sym = find_symbol(var_name);
            if(sym) {
                fprintf(output_file, "\tdaddiu r%d, r0, %d\n", sym->reg_num, value);
            }
        } else if (sscanf(temp + 4, "%63[^;]", var_name) == 1) {
            white_space_trim(var_name);
            add_variable(var_name, 0, 0);
        }
        return;
    }

    // --- Assignment: x = ...; ---
    char *eq_pos = strchr(temp, '=');
    if(eq_pos != NULL) {
        char lhs[64];
        char rhs[256];
        
        int lhs_len = eq_pos - temp;
        strncpy(lhs, temp, lhs_len);
        lhs[lhs_len] = '\0';
        white_space_trim(lhs);
        
        strcpy(rhs, eq_pos + 1);
        white_space_trim(rhs);
        
        Symbol *sym = find_symbol(lhs);
        if(sym) {
            // Mark as initialized
            sym->initialized = 1;
            
            // Evaluate RHS expression
            evaluate_expression(rhs, output_file, sym->reg_num);
        }
        return;
    }
}

/* ---------- Symbol table functions ---------- */
void add_variable(const char *name, int value, int initialized) {
    if (symbol_count >= MAX_SYMBOLS) return;

    // Check if already exists
    for(size_t i = 0; i < symbol_count; i++) {
        if(strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].value = value;
            symbol_table[i].initialized = initialized;
            return;
        }
    }

    strncpy(symbol_table[symbol_count].name, name, 63);
    symbol_table[symbol_count].name[63] = '\0';
    symbol_table[symbol_count].value = value;
    symbol_table[symbol_count].initialized = initialized;
    symbol_table[symbol_count].reg_num = 8 + symbol_count; // Start from r8
    
    symbol_count++;
}

int get_operator_precedence(char op) {
    switch(op) {
        case '*':
        case '/': return 2;
        case '+':
        case '-': return 1;
        default: return 0;
    }
}

int is_operator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

void generate_machine_code() {}

Symbol *find_symbol(const char *name) {
    for(size_t i = 0; i < symbol_count; i++) {
        if(strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL;
}

/* ---------- Trims white space ----------*/
void white_space_trim(char *s) {
    if(!s) return;
    
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }

    char *end = s + strlen(s) - 1;
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

/* ---------- Skip escape sequences and comments ---------- */
void skip_escape_sequences_and_comments(const char *source_code, int *i, int *line) {
    while(source_code[*i] != '\0') {
        if(ESCAPE_SEQUENCE(source_code[*i])) {
            if(source_code[*i] == '\n') {
                (*line)++;
            }
            (*i)++;
        } else if(source_code[*i] == '/' && source_code[(*i) + 1] == '/') {
            // Single-line comment
            while(source_code[*i] != '\0' && source_code[*i] != '\n') {
                (*i)++;
            }
        } else if(source_code[*i] == '/' && source_code[(*i) + 1] == '*') {
            // Multi-line comment
            (*i) += 2;
            while(source_code[*i] != '\0' && 
                  !(source_code[*i] == '*' && source_code[(*i) + 1] == '/')) {
                if(source_code[*i] == '\n') {
                    (*line)++;
                }
                (*i)++;
            }
            if(source_code[*i] != '\0') {
                (*i) += 2;
            }
        } else {
            break;
        }
    }
}