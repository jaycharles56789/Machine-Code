#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "mips64.h"

#define ESCAPE_SEQUENCE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define ALPHABETIC_CHARACTER(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define DIGIT_CHARACTER(c) ((c) >= '0' && (c) <= '9')
#define ALPHANUMERIC_CHARACTER(c) (ALPHABETIC_CHARACTER(c) || DIGIT_CHARACTER(c))

// symbol table 
#define MAX_SYMBOLS 128
typedef struct {
    char name[64];
    int value;
    int initialized;
    int mem_offset;
} Symbol;

Symbol symbol_table[MAX_SYMBOLS];
size_t symbol_count = 0;

// function prototypes
char *open_source_file(const char *filename);
int process_statements(const char *source_code, FILE *output_file);
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

char *open_source_file(const char *filename) {
    FILE *source_code = fopen(filename, "r");
    if(source_code == NULL) {
        fprintf(stderr, "File '%s' cannot be opened.\n", filename);
        return NULL;
    }

    size_t buffer_size = 1024;
    size_t total_length = 0;
    char *line_of_code = (char *)malloc(buffer_size * sizeof(char));
    
    if(line_of_code == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(source_code);
        return NULL;
    }
    
    line_of_code[0] = '\0';
    
    char lines[256];
    while(fgets(lines, sizeof(lines), source_code) != NULL) {
        size_t line_len = strlen(lines);
        
        if(total_length + line_len + 1 >= buffer_size) {
            buffer_size *= 2;
            char *new_buffer = (char *)realloc(line_of_code, buffer_size);
            if(new_buffer == NULL) {
                fprintf(stderr, "Memory reallocation failed.\n");
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

void compile_to_assemble(const char *source_code, const char *file_name) {
    FILE *output_file = fopen(file_name, "w");
    if(output_file == NULL) {
        fprintf(stderr, "'%s' can't be created\n", file_name);
        return;
    }

    fprintf(output_file, ".data\n");
    
    char *duplicated_source = strdup(source_code ? source_code : "");
    if(duplicated_source == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(output_file);
        return;
    }
    
    // First pass: collect all variable declarations and check for errors
    if(process_statements(duplicated_source, NULL) != 0) {
        free(duplicated_source);
        fclose(output_file);
        return;
    }
    
    // Write variable declarations in .data section
    for(size_t i = 0; i < symbol_count; i++) {
        fprintf(output_file, "\t%s:\t.byte %d\n", symbol_table[i].name, symbol_table[i].value);
    }
    
    fprintf(output_file, ".code\n");
    fprintf(output_file, "main:\n");
    
    // Reset source for second pass
    free(duplicated_source);
    duplicated_source = strdup(source_code ? source_code : "");
    
    // Second pass: generate code
    if(process_statements(duplicated_source, output_file) != 0) {
        free(duplicated_source);
        fclose(output_file);
        return;
    }

    // fprintf(output_file, "\n\thalt\n");

    free(duplicated_source);
    fclose(output_file);
}

int process_statements(const char *source_code, FILE *output_file) {
    int i = 0, line = 1;
    
    // First, validate that each line with content ends with semicolon
    int validation_i = 0;
    int validation_line = 1;
    
    while (source_code[validation_i] != '\0') {
        skip_escape_sequences_and_comments(source_code, &validation_i, &validation_line);
        if(source_code[validation_i] == '\0') break;
        
        int line_start = validation_i;
        int line_num = validation_line;
        int has_content = 0;
        
        // Read until end of line or semicolon
        while(source_code[validation_i] != '\0' && 
              source_code[validation_i] != '\n' && 
              source_code[validation_i] != ';') {
            if(!isspace((unsigned char)source_code[validation_i])) {
                has_content = 1;
            }
            validation_i++;
        }
        
        // Check if line has content but no semicolon
        if(has_content && source_code[validation_i] != ';') {
            fprintf(stderr, "\nMissing semicolon at line %d\n", line_num);
            fprintf(stderr, "Line content: ");
            for(int j = line_start; source_code[j] != '\0' && source_code[j] != '\n'; j++) {
                fputc(source_code[j], stderr);
            }
            fprintf(stderr, "\n");
            exit(1);
        }
        
        if(source_code[validation_i] == ';') {
            validation_i++;
        } else if(source_code[validation_i] == '\n') {
            validation_line++;
            validation_i++;
        }
    }
    
    // Now process statements normally
    while (source_code[i] != '\0') {
        skip_escape_sequences_and_comments(source_code, &i, &line);
        if(source_code[i] == '\0') break;

        int start = i;
        
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
    
    return 0;
}

int evaluate_expression(const char *expr, FILE *output_file, int target_reg) {
    if(!output_file) return -1;
    
    char tokens[64][64];
    int token_count = 0;
    int i = 0;
    
    // Tokenize
    while(expr[i] != '\0' && token_count < 64) {
        while(isspace(expr[i])) i++;
        if(expr[i] == '\0') break;
        
        if(ALPHABETIC_CHARACTER(expr[i])) {
            int j = 0;
            while(ALPHANUMERIC_CHARACTER(expr[i]) && j < 63) {
                tokens[token_count][j++] = expr[i++];
            }
            tokens[token_count][j] = '\0';
            token_count++;
        } else if(DIGIT_CHARACTER(expr[i])) {
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
    
    // Single operand
    if(token_count == 1) {
        if(DIGIT_CHARACTER(tokens[0][0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", target_reg, tokens[0]);
        } else {
            Symbol *sym = find_symbol(tokens[0]);
            if(sym) {
                fprintf(output_file, "\tlb r%d, %s(r0)\n", target_reg, sym->name);
            }
        }
        return target_reg;
    }
    
    // Two operands with one operator
    if(token_count == 3) {
        int reg1 = target_reg;
        int reg2 = target_reg + 1;
        
        if(DIGIT_CHARACTER(tokens[0][0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", reg1, tokens[0]);
        } else {
            Symbol *sym = find_symbol(tokens[0]);
            if(sym) {
                fprintf(output_file, "\tlb r%d, %s(r0)\n", reg1, sym->name);
            }
        }
        
        if(DIGIT_CHARACTER(tokens[2][0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", reg2, tokens[2]);
        } else {
            Symbol *sym = find_symbol(tokens[2]);
            if(sym) {
                fprintf(output_file, "\tlb r%d, %s(r0)\n", reg2, sym->name);
            }
        }
        
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
    
    // Multiple operators - evaluate left to right
    int current_reg = target_reg;
    int temp_reg = target_reg + 1;
    
    // Load first operand
    if(DIGIT_CHARACTER(tokens[0][0])) {
        fprintf(output_file, "\tdaddiu r%d, r0, %s\n", current_reg, tokens[0]);
    } else {
        Symbol *sym = find_symbol(tokens[0]);
        if(sym) {
            fprintf(output_file, "\tlb r%d, %s(r0)\n", current_reg, sym->name);
        }
    }
    
    // Process each operation
    for(int j = 1; j < token_count - 1; j += 2) {
        char op = tokens[j][0];
        char *next_operand = tokens[j + 1];
        
        // Load next operand
        if(DIGIT_CHARACTER(next_operand[0])) {
            fprintf(output_file, "\tdaddiu r%d, r0, %s\n", temp_reg, next_operand);
        } else {
            Symbol *sym = find_symbol(next_operand);
            if(sym) {
                fprintf(output_file, "\tlb r%d, %s(r0)\n", temp_reg, sym->name);
            }
        }
        
        // Perform operation
        if(op == '+') {
            fprintf(output_file, "\tdaddu r%d, r%d, r%d\n", current_reg, current_reg, temp_reg);
        } else if(op == '-') {
            fprintf(output_file, "\tdsubu r%d, r%d, r%d\n", current_reg, current_reg, temp_reg);
        } else if(op == '*') {
            fprintf(output_file, "\tdmult r%d, r%d\n", current_reg, temp_reg);
            fprintf(output_file, "\tmflo r%d\n", current_reg);
        } else if(op == '/') {
            fprintf(output_file, "\tddiv r%d, r%d\n", current_reg, temp_reg);
            fprintf(output_file, "\tmflo r%d\n", current_reg);
        }
    }
    
    return target_reg;
}

void make_assembly_for_statement(FILE *output_file, const char *statement) {
    if (!statement) return;

    char temp[512];
    strncpy(temp, statement, sizeof(temp)-1);
    temp[sizeof(temp)-1] = '\0';
    white_space_trim(temp);

    if (temp[0] == '\0') return;

    // Variable declaration
    if (strncmp(temp, "int ", 4) == 0) {
        char var_name[64];
        int value = 0;
        int initialized = 0;

        if (sscanf(temp + 4, "%63[^=;] = %d", var_name, &value) == 2) {
            initialized = 1;
            white_space_trim(var_name);
            add_variable(var_name, value, initialized);
            
            if(output_file) {
                Symbol *sym = find_symbol(var_name);
                if(sym) {
                    fprintf(output_file, "\tdaddiu r1, r0, %d\n", value);
                    fprintf(output_file, "\tsb r1, %s(r0)\n", sym->name);
                }
            }
        } else if (sscanf(temp + 4, "%63[^;]", var_name) == 1) {
            white_space_trim(var_name);
            add_variable(var_name, 0, 0);
        }
        return;
    }

    // Assignment
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
        
        // Check if LHS variable is declared
        Symbol *sym = find_symbol(lhs);
        if(!sym) {
            fprintf(stderr, "\nVariable '%s' used without declaration\n", lhs);
            fprintf(stderr, "Statement: %s\n", temp);
            exit(1);
        }
        
        // Check RHS variables
        char rhs_copy[256];
        strncpy(rhs_copy, rhs, sizeof(rhs_copy)-1);
        rhs_copy[sizeof(rhs_copy)-1] = '\0';
        
        int i = 0;
        while(rhs_copy[i] != '\0') {
            while(isspace(rhs_copy[i])) i++;
            if(rhs_copy[i] == '\0') break;
            
            if(ALPHABETIC_CHARACTER(rhs_copy[i])) {
                char var_name[64];
                int j = 0;
                while(ALPHANUMERIC_CHARACTER(rhs_copy[i]) && j < 63) {
                    var_name[j++] = rhs_copy[i++];
                }
                var_name[j] = '\0';
                
                if(!find_symbol(var_name)) {
                    fprintf(stderr, "\nVariable '%s' used without declaration\n", var_name);
                    fprintf(stderr, "Statement: %s\n", temp);
                    exit(1);
                }
            } else if(DIGIT_CHARACTER(rhs_copy[i])) {
                while(DIGIT_CHARACTER(rhs_copy[i])) i++;
            } else if(is_operator(rhs_copy[i])) {
                i++;
            } else {
                i++;
            }
        }
        
        if(output_file) {
            sym->initialized = 1;
            evaluate_expression(rhs, output_file, 1);
            fprintf(output_file, "\tsb r1, %s(r0)\n", sym->name);
        }
        return;
    }
}

void add_variable(const char *name, int value, int initialized) {
    if (symbol_count >= MAX_SYMBOLS) return;

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
    symbol_table[symbol_count].mem_offset = symbol_count;
    
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

Symbol *find_symbol(const char *name) {
    for(size_t i = 0; i < symbol_count; i++) {
        if(strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL;
}

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

void skip_escape_sequences_and_comments(const char *source_code, int *i, int *line) {
    while(source_code[*i] != '\0') {
        if(ESCAPE_SEQUENCE(source_code[*i])) {
            if(source_code[*i] == '\n') {
                (*line)++;
            }
            (*i)++;
        } else if(source_code[*i] == '/' && source_code[(*i) + 1] == '/') {
            while(source_code[*i] != '\0' && source_code[*i] != '\n') {
                (*i)++;
            }
        } else if(source_code[*i] == '/' && source_code[(*i) + 1] == '*') {
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