#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "mips64.h"

#define ESCAPE_SEQUENCE(c) ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define ALPHABETIC_CHARACTER(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_')
#define DIGIT_CHARACTER(c) ((c) >= '0' && (c) <= '9')
#define ALPHANUMERIC_CHARACTER(c) (ALPHABETIC_CHARACTER(c) || DIGIT_CHARACTER(c))

// Maximum number of symbols (variables) that can be stored
#define MAX_SYMBOLS 128

// Structure to represent a variable/symbol in the symbol table
typedef struct {
    char name[64];          // Variable name (up to 63 characters + null terminator)
    int value;              // Variable's value
    int initialized;        // Flag: 1 if variable has been assigned a value, 0 otherwise
    int mem_offset;         // Memory offset for the variable (used in assembly generation)
    int reg_num;            // Allocated register number for this variable (-1 if not allocated)
    int in_register;        // Flag: 1 if current value is in register, 0 if needs to be loaded
} Symbol;

// Global symbol table array to store all variables
Symbol symbol_table[MAX_SYMBOLS];

// Counter to track how many symbols are currently in the table
size_t symbol_count = 0;

// Next available register for variable allocation (starts at r1, r0 is reserved)
int next_available_register = 1;

// function prototypes
char *open_source_file(const char *filename);
void compile_to_assemble(const char *source_code, const char *filename);
int lexical_analyzer(const char *expr, FILE *output_file, int target_reg);
int syntax_analyzer(const char *source_code, FILE *output_file);
void make_assembly_for_statement(FILE *output_file, const char *statement);
void add_variable(const char *name, int value, int initialized);
int is_operator(char c);
Symbol *find_symbol(const char *name);
void white_space_trim(char *s);
void skip_escape_sequences_and_comments(const char *source_code, int *i, int *line);

int main(void) {
    // Open and read the source file
    char *source_code = open_source_file("input.txt");
    if(source_code == NULL) {
        return 1;
    }

    // Display the input source code for debugging
    printf("\nInput source code:\n%s\n", source_code);
    
    // Define the output assembly file name
    char file_name[] = "assembly.asm";

    // Compile the source code to assembly language
    compile_to_assemble(source_code, file_name);

    // Free the dynamically allocated memory for source code
    free(source_code);

    // Inform user that compilation was successful
    printf("\nAssembly code is generated successfully in '%s'\n", file_name);

    return 0;
}

/*
 *============================================
 * Function: open_source_file
 * Purpose: Opens a source file and reads its entire contents into a dynamically allocated string
 * Parameters: filename - path to the source file to read
 * Returns: Pointer to dynamically allocated string containing file contents, or NULL on error
 * ============================================
*/
char *open_source_file(const char *filename) {
    // Attempt to open the file in read mode
    FILE *source_code = fopen(filename, "r");
    if(source_code == NULL) {
        // Print error message if file cannot be opened
        fprintf(stderr, "File '%s' cannot be opened.\n", filename);
        return NULL;
    }

    // Initial buffer size for storing file contents (will grow dynamically if needed)
    size_t buffer_size = 1024;
    
    // Track the total length of content read so far
    size_t total_length = 0;
    
    // Allocate initial buffer to store the file contents
    char *line_of_code = (char *)malloc(buffer_size * sizeof(char));
    if(line_of_code == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(source_code);  // Close file before returning
        return NULL;
    }
    
    // Initialize the buffer with an empty string
    line_of_code[0] = '\0';
    
    // Temporary buffer to read one line at a time (up to 255 characters + null terminator)
    char lines[256];
    
    // Read the file line by line until end of file
    while(fgets(lines, sizeof(lines), source_code) != NULL) {
        // Get the length of the current line
        size_t line_len = strlen(lines);
        
        // Check if adding this line would exceed current buffer capacity
        if(total_length + line_len + 1 >= buffer_size) {
            // Double the buffer size to accommodate more content
            buffer_size *= 2;
            
            // Reallocate the buffer with the new size
            char *new_buffer = (char *)realloc(line_of_code, buffer_size);
            
            // Check if reallocation succeeded
            if(new_buffer == NULL) {
                fprintf(stderr, "Memory reallocation failed.\n");
                free(line_of_code);       // Free the original buffer
                fclose(source_code);       // Close the file
                return NULL;
            }
            
            // Update pointer to the newly allocated buffer
            line_of_code = new_buffer;
        }
        
        // Append the current line to the accumulated content
        strcat(line_of_code, lines);
        
        // Update the total length counter
        total_length += line_len;
    }

    // Close the file after reading all content
    fclose(source_code);
    
    // Return pointer to the complete file contents
    return line_of_code;
}

/* ============================================
 * Function: compile_to_assemble
 * Purpose: Compiles source code into assembly language using a two-pass approach
 * Parameters: 
 *            source_code - the input source code to compile
 *            file_name - name of the output assembly file
 * ============================================
*/
void compile_to_assemble(const char *source_code, const char *file_name) {
    // Open the output file for writing assembly code
    FILE *output_file = fopen(file_name, "w");
    if(output_file == NULL) {
        // Print error if file creation fails
        fprintf(stderr, "'%s' can't be created\n", file_name);
        return;
    }

    // Write the .data section header (where variables are declared)
    fprintf(output_file, ".data\n");
    
    /*
     * Create a duplicate of the source code for processing
     * This allows us to parse it multiple times without modifying the original
    */
    char *duplicated_source = strdup(source_code ? source_code : "");
    if(duplicated_source == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(output_file);
        return;
    }
    
    /* 
     * ============================================
     * FIRST PASS: Symbol Table Construction
     * ============================================
     * Scan through the source code to:
     * - Collect all variable declarations
     * - Build the symbol table
     * - Check for syntax/semantic errors
    */
    if(syntax_analyzer(duplicated_source, NULL) != 0) {
        // If errors were found, clean up and exit
        free(duplicated_source);
        fclose(output_file);
        return;
    }
    
    // Write all variable declarations to the .data section
    for(size_t i = 0; i < symbol_count; i++) {
        // Format: variable_name: .byte initial_value
        fprintf(output_file, "\t%s:\t.byte %d\n", symbol_table[i].name, symbol_table[i].value);
    }
    
    // Write the .code section header (where executable instructions go)
    fprintf(output_file, ".code\n");
    
    // Write the main entry point label
    fprintf(output_file, "main:\n");
    
    /*
     * ============================================
     * SECOND PASS: Code Generation
     * ============================================
     * Free the old duplicate and create a fresh copy of the source code
     * This resets our position in the source for the second pass
    */
    free(duplicated_source);

    duplicated_source = strdup(source_code ? source_code : "");
    if(duplicated_source == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        fclose(output_file);
        return;
    }
    
    // Process statements again, this time generating actual assembly code
    // The output_file is passed so assembly instructions are written
    if(syntax_analyzer(duplicated_source, output_file) != 0) {
        // If errors occurred during code generation, clean up and exit
        free(duplicated_source);
        fclose(output_file);
        return;
    }

    // Clean up: free allocated memory and close the output file
    free(duplicated_source);
    fclose(output_file);
}

/*
 * ============================================
 * Function: lexical_analyzer
 * Purpose: Performs lexical analysis (tokenization) on an expression and generates assembly code
 *          Now uses register allocation - each variable gets its own register
 * Parameters:
 *            expr - the expression string to analyze (e.g., "a + b * 2")
 *            output_file - file pointer to write assembly instructions
 *            target_reg - the target register number to store the result
 * Returns: Register number containing the result, or -1 on error
 * ============================================
*/
int lexical_analyzer(const char *expr, FILE *output_file, int target_reg) {
    // Validate output file pointer
    if(!output_file) return -1;
    
    // Array to store tokens (max 64 tokens, each up to 63 chars + null terminator)
    char tokens[64][64];
    int token_count = 0;  // Counter for number of tokens found
    int i = 0;            // Index for traversing the expression string
    
    // ============================================
    // TOKENIZATION PHASE
    // ============================================
    // Break down the expression into individual tokens (identifiers, numbers, operators)
    while(expr[i] != '\0' && token_count < 64) {
        // Skip any whitespace characters
        while(isspace(expr[i])) i++;
        if(expr[i] == '\0') break;
        
        // Case 1: Alphabetic character (identifier/variable name)
        if(ALPHABETIC_CHARACTER(expr[i])) {
            int j = 0;
            // Extract the complete identifier (letters, digits, underscores)
            while(ALPHANUMERIC_CHARACTER(expr[i]) && j < 63) {
                tokens[token_count][j++] = expr[i++];
            }
            tokens[token_count][j] = '\0';  // Null-terminate the token
            token_count++;
        } 
        // Case 2: Digit character (numeric literal)
        else if(DIGIT_CHARACTER(expr[i])) {
            int j = 0;
            // Extract the complete number
            while(DIGIT_CHARACTER(expr[i]) && j < 63) {
                tokens[token_count][j++] = expr[i++];
            }
            tokens[token_count][j] = '\0';  // Null-terminate the token
            token_count++;
        } 
        // Case 3: Operator character (+, -, *, /)
        else if(is_operator(expr[i])) {
            tokens[token_count][0] = expr[i];
            tokens[token_count][1] = '\0';  // Null-terminate the single-char token
            token_count++;
            i++;
        } 
        // Case 4: Unknown character - skip it
        else {
            i++;
        }
    }
    
    // Check if any tokens were found
    if(token_count == 0) return -1;
    
    const char *r0_name = get_register_name(0);
    
    // ============================================
    // CODE GENERATION PHASE - Using Register Allocation
    // ============================================
    
    // Case 1: Single operand (no operations)
    // Example: "x" or "5"
    if(token_count == 1) {
        // If it's a number, load it directly into the target register
        if(DIGIT_CHARACTER(tokens[0][0])) {
            const char *target_reg_name = get_register_name(target_reg);
            // daddiu: Add immediate unsigned (load constant)
            fprintf(output_file, "\tdaddiu %s, %s, %s\n", target_reg_name, r0_name, tokens[0]);
        } 
        // If it's a variable, use its allocated register
        else {
            Symbol *sym = find_symbol(tokens[0]);
            if(sym && sym->reg_num >= 0) {
                // Variable has an allocated register, no load needed if target matches
                if(sym->reg_num != target_reg) {
                    const char *target_reg_name = get_register_name(target_reg);
                    const char *src_reg_name = get_register_name(sym->reg_num);
                    // Move from variable's register to target
                    fprintf(output_file, "\tdaddu %s, %s, %s\n", target_reg_name, src_reg_name, r0_name);
                }
                return sym->reg_num;
            }
        }
        return target_reg;
    }
    
    // Case 2: Simple binary operation (operand operator operand)
    // Example: "a + b" or "5 * x"
    if(token_count == 3) {
        int reg1 = -1;  // Register for first operand
        int reg2 = -1;  // Register for second operand
        
        // Get register for first operand
        if(DIGIT_CHARACTER(tokens[0][0])) {
            // Use a temporary register for constants
            reg1 = target_reg + 10;  // Use higher registers for temps
            const char *reg1_name = get_register_name(reg1);
            fprintf(output_file, "\tdaddiu %s, %s, %s\n", reg1_name, r0_name, tokens[0]);
        } else {
            Symbol *sym = find_symbol(tokens[0]);
            if(sym && sym->reg_num >= 0) {
                reg1 = sym->reg_num;  // Use the variable's allocated register
                // If value not in register, load it
                if(!sym->in_register) {
                    const char *reg1_name = get_register_name(reg1);
                    fprintf(output_file, "\tlb %s, %s(%s)\n", reg1_name, sym->name, r0_name);
                    sym->in_register = 1;
                }
            }
        }
        
        // Get register for second operand
        if(DIGIT_CHARACTER(tokens[2][0])) {
            // Use a temporary register for constants
            reg2 = target_reg + 11;  // Use higher registers for temps
            const char *reg2_name = get_register_name(reg2);
            fprintf(output_file, "\tdaddiu %s, %s, %s\n", reg2_name, r0_name, tokens[2]);
        } else {
            Symbol *sym = find_symbol(tokens[2]);
            if(sym && sym->reg_num >= 0) {
                reg2 = sym->reg_num;  // Use the variable's allocated register
                // If value not in register, load it
                if(!sym->in_register) {
                    const char *reg2_name = get_register_name(reg2);
                    fprintf(output_file, "\tlb %s, %s(%s)\n", reg2_name, sym->name, r0_name);
                    sym->in_register = 1;
                }
            }
        }
        
        if(reg1 < 0 || reg2 < 0) return -1;
        
        const char *target_reg_name = get_register_name(target_reg);
        const char *reg1_name = get_register_name(reg1);
        const char *reg2_name = get_register_name(reg2);
        
        // Perform the operation based on the operator
        char op = tokens[1][0];
        if(op == '+') {
            // daddu: Add unsigned
            fprintf(output_file, "\tdaddu %s, %s, %s\n", target_reg_name, reg1_name, reg2_name);
        } else if(op == '-') {
            // dsubu: Subtract unsigned
            fprintf(output_file, "\tdsubu %s, %s, %s\n", target_reg_name, reg1_name, reg2_name);
        } else if(op == '*') {
            // dmult: Multiply (result goes to HI/LO registers)
            fprintf(output_file, "\tdmult %s, %s\n", reg1_name, reg2_name);
            // mflo: Move from LO register to target register
            fprintf(output_file, "\tmflo %s\n", target_reg_name);
        } else if(op == '/') {
            // ddiv: Divide (quotient to LO, remainder to HI)
            fprintf(output_file, "\tddiv %s, %s\n", reg1_name, reg2_name);
            // mflo: Move quotient from LO register to target register
            fprintf(output_file, "\tmflo %s\n", target_reg_name);
        }
        
        return target_reg;
    }
    
    // Case 3: Multiple operations (left-to-right evaluation)
    // Example: "a + b - c * d"
    // Note: This evaluates strictly left-to-right, ignoring operator precedence
    int current_reg = target_reg;
    
    // Load the first operand
    if(DIGIT_CHARACTER(tokens[0][0])) {
        const char *current_reg_name = get_register_name(current_reg);
        fprintf(output_file, "\tdaddiu %s, %s, %s\n", current_reg_name, r0_name, tokens[0]);
    } else {
        Symbol *sym = find_symbol(tokens[0]);
        if(sym && sym->reg_num >= 0) {
            current_reg = sym->reg_num;
        }
    }
    
    // Process each operator-operand pair sequentially
    for(int j = 1; j < token_count - 1; j += 2) {
        char op = tokens[j][0];
        char *next_operand = tokens[j + 1];
        
        int operand_reg = -1;
        
        // Get register for next operand
        if(DIGIT_CHARACTER(next_operand[0])) {
            operand_reg = target_reg + 1;
            const char *operand_reg_name = get_register_name(operand_reg);
            fprintf(output_file, "\tdaddiu %s, %s, %s\n", operand_reg_name, r0_name, next_operand);
        } else {
            Symbol *sym = find_symbol(next_operand);
            if(sym && sym->reg_num >= 0) {
                operand_reg = sym->reg_num;
            }
        }
        
        if(operand_reg < 0) continue;
        
        const char *current_reg_name = get_register_name(current_reg);
        const char *operand_reg_name = get_register_name(operand_reg);
        
        // Perform the operation
        if(op == '+') {
            fprintf(output_file, "\tdaddu %s, %s, %s\n", current_reg_name, current_reg_name, operand_reg_name);
        } else if(op == '-') {
            fprintf(output_file, "\tdsubu %s, %s, %s\n", current_reg_name, current_reg_name, operand_reg_name);
        } else if(op == '*') {
            fprintf(output_file, "\tdmult %s, %s\n", current_reg_name, operand_reg_name);
            fprintf(output_file, "\tmflo %s\n", current_reg_name);
        } else if(op == '/') {
            fprintf(output_file, "\tddiv %s, %s\n", current_reg_name, operand_reg_name);
            fprintf(output_file, "\tmflo %s\n", current_reg_name);
        }
    }
    
    // If result is not in target register, move it there
    if(current_reg != target_reg) {
        const char *target_reg_name = get_register_name(target_reg);
        const char *current_reg_name = get_register_name(current_reg);
        fprintf(output_file, "\tdaddu %s, %s, %s\n", target_reg_name, current_reg_name, r0_name);
    }
    
    return target_reg;
}

/*
 * ============================================
 * Function: syntax_analyzer
 * Purpose: Performs syntax analysis on source code by validating statement structure
 *          and processing statements for assembly code generation
 * Parameters:
 *          source_code - the complete source code to analyze
 *          output_file - file pointer to write assembly instructions (NULL during first pass)
 * Returns: 0 on success, exits on syntax error
 * ============================================
*/
int syntax_analyzer(const char *source_code, FILE *output_file) {
    int i = 0, line = 1;  // Main processing position and line counter
    
    // ============================================
    // FIRST PASS: SYNTAX VALIDATION
    // ============================================
    // Validate that each non-empty line ends with a semicolon
    int validation_i = 0;       // Position index for validation pass
    int validation_line = 1;    // Line number for validation pass
    
    // Scan through entire source code to check for semicolons
    while (source_code[validation_i] != '\0') {
        // Skip over whitespace and comments
        skip_escape_sequences_and_comments(source_code, &validation_i, &validation_line);
        if(source_code[validation_i] == '\0') break;
        
        int line_start = validation_i;      // Remember where this line started
        int line_num = validation_line;     // Remember the line number for error reporting
        int has_content = 0;                // Flag to track if line has non-whitespace content
        
        // Read characters until we hit end of line, end of file, or semicolon
        while(source_code[validation_i] != '\0' && 
              source_code[validation_i] != '\n' && 
              source_code[validation_i] != ';') {
            // Check if this character is not whitespace
            if(!isspace((unsigned char)source_code[validation_i])) {
                has_content = 1;  // Mark that we found actual content
            }
            validation_i++;
        }
        
        // SYNTAX ERROR: Line has content but doesn't end with semicolon
        if(has_content && source_code[validation_i] != ';') {
            // Report the error with line number
            fprintf(stderr, "\nMissing semicolon at line %d\n", line_num);
            fprintf(stderr, "Line content: ");
            
            // Print the problematic line for user reference
            for(int j = line_start; source_code[j] != '\0' && source_code[j] != '\n'; j++) {
                fputc(source_code[j], stderr);
            }
            fprintf(stderr, "\n");
            
            // Exit program due to syntax error
            exit(1);
        }
        
        // Move past the semicolon or newline
        if(source_code[validation_i] == ';') {
            validation_i++;
        } else if(source_code[validation_i] == '\n') {
            validation_line++;  // Increment line counter
            validation_i++;
        }
    }
    
    // ============================================
    // SECOND PASS: STATEMENT PROCESSING
    // ============================================
    // Now that syntax is validated, process each statement
    while (source_code[i] != '\0') {
        // Skip whitespace and comments
        skip_escape_sequences_and_comments(source_code, &i, &line);
        if(source_code[i] == '\0') break;

        int start = i;  // Mark the beginning of this statement
        
        // Read until we find a semicolon (statement terminator)
        while (source_code[i] != '\0' && source_code[i] != ';') {
            // Track line numbers for error reporting
            if(source_code[i] == '\n') line++;
            i++;
        }
        
        // Process the statement if it's not empty
        if (i > start) {
            int len = i - start;  // Calculate statement length
            
            // Allocate memory for the statement (+2 for null terminator and safety)
            char *statement = (char *)malloc(len + 2);
            if(statement) {
                // Copy the statement from source code
                strncpy(statement, &source_code[start], len);
                statement[len] = '\0';  // Null-terminate the string
                
                // Remove leading and trailing whitespace
                white_space_trim(statement);
                
                // Process non-empty statements (ignore empty lines and lone semicolons)
                if (statement[0] != '\0' && statement[0] != ';') {
                    // Generate assembly code for this statement
                    make_assembly_for_statement(output_file, statement);
                }
                
                // Free the allocated memory for this statement
                free(statement);
            }
        }
        
        // Move past the semicolon to start processing next statement
        if (source_code[i] == ';') i++;
    }
    
    // Return success (0 means no errors)
    return 0;
}

/*
 * ============================================
 * Function: make_assembly_for_statement
 * Purpose: Performs semantic analysis and generates assembly code for a single statement
 *          Handles variable declarations and assignment statements
 * Parameters:
 *           output_file - file pointer to write assembly instructions (NULL during first pass)
 *           statement - the statement to process (e.g., "int x = 5;" or "y = x + 2;")
 * ============================================
*/
void make_assembly_for_statement(FILE *output_file, const char *statement) {
    // Validate input statement
    if (!statement) return;

    // Create a local copy of the statement for processing
    char temp[512];
    strncpy(temp, statement, sizeof(temp)-1);
    temp[sizeof(temp)-1] = '\0';  // Ensure null termination
    white_space_trim(temp);       // Remove leading/trailing whitespace

    // Skip empty statements
    if (temp[0] == '\0') return;

    // ============================================
    // CASE 1: VARIABLE DECLARATION
    // ============================================
    // Check if statement starts with "int " (variable declaration)
    if (strncmp(temp, "int ", 4) == 0) {
        char var_name[64];      // Buffer for variable name
        int value = 0;          // Initial value (default 0)
        int initialized = 0;    // Flag: 1 if initialized with value, 0 otherwise

        // Try to parse: "int var_name = value"
        // %63[^=;] reads up to 63 chars that are not '=' or ';'
        if (sscanf(temp + 4, "%63[^=;] = %d", var_name, &value) == 2) {
            // Variable is initialized with a constant value
            initialized = 1;
            white_space_trim(var_name);  // Clean up variable name
            
            // Add variable to symbol table
            add_variable(var_name, value, initialized);
            
            // Generate assembly code (only during second pass when output_file is not NULL)
            if(output_file) {
                Symbol *sym = find_symbol(var_name);
                if(sym) {
                    const char *var_reg_name = get_register_name(sym->reg_num);
                    const char *r0_name = get_register_name(0);
                    
                    // Load the initial value into the variable's allocated register
                    fprintf(output_file, "\tdaddiu %s, %s, %d\n", var_reg_name, r0_name, value);
                    // Store the value to memory
                    fprintf(output_file, "\tsb %s, %s(%s)\n", var_reg_name, sym->name, r0_name);
                    // Mark that value is now in register
                    sym->in_register = 1;
                }
            }
        }
        // Try to parse: "int var_name = expression" (e.g., int d = a)
        else if (strchr(temp + 4, '=') != NULL) {
            char rhs[256];
            // Find the '=' sign
            char *eq_pos = strchr(temp + 4, '=');
            
            // Extract variable name (everything before '=')
            int name_len = eq_pos - (temp + 4);
            strncpy(var_name, temp + 4, name_len);
            var_name[name_len] = '\0';
            white_space_trim(var_name);
            
            // Extract right-hand side (everything after '=')
            strcpy(rhs, eq_pos + 1);
            white_space_trim(rhs);
            
            // Add variable to symbol table first (before semantic checks)
            add_variable(var_name, 0, 0);
            
            // Now do semantic checks on the RHS expression
            if(output_file) {
                // Verify all RHS variables are declared
                char rhs_copy[256];
                strncpy(rhs_copy, rhs, sizeof(rhs_copy)-1);
                rhs_copy[sizeof(rhs_copy)-1] = '\0';
                
                int i = 0;
                while(rhs_copy[i] != '\0') {
                    while(isspace(rhs_copy[i])) i++;
                    if(rhs_copy[i] == '\0') break;
                    
                    if(ALPHABETIC_CHARACTER(rhs_copy[i])) {
                        char rhs_var_name[64];
                        int j = 0;
                        while(ALPHANUMERIC_CHARACTER(rhs_copy[i]) && j < 63) {
                            rhs_var_name[j++] = rhs_copy[i++];
                        }
                        rhs_var_name[j] = '\0';
                        
                        if(!find_symbol(rhs_var_name)) {
                            fprintf(stderr, "\nVariable '%s' used without declaration\n", rhs_var_name);
                            fprintf(stderr, "Statement: %s\n", temp);
                            exit(1);
                        }
                    } 
                    else if(DIGIT_CHARACTER(rhs_copy[i])) {
                        while(DIGIT_CHARACTER(rhs_copy[i])) i++;
                    } 
                    else if(is_operator(rhs_copy[i])) {
                        i++;
                    } 
                    else {
                        i++;
                    }
                }
                
                // Generate assembly for the initialization
                Symbol *sym = find_symbol(var_name);
                if(sym) {
                    // Evaluate RHS and store in variable's register
                    lexical_analyzer(rhs, output_file, sym->reg_num);
                    
                    const char *var_reg_name = get_register_name(sym->reg_num);
                    const char *r0_name = get_register_name(0);
                    
                    // Store to memory
                    fprintf(output_file, "\tsb %s, %s(%s)\n", var_reg_name, sym->name, r0_name);
                    sym->in_register = 1;
                    sym->initialized = 1;
                }
            }
        }
        // Try to parse: "int var_name" (no initialization)
        else if (sscanf(temp + 4, "%63[^;]", var_name) == 1) {
            white_space_trim(var_name);
            // Add uninitialized variable to symbol table (value defaults to 0)
            add_variable(var_name, 0, 0);
            
            // Generate assembly to load initial value from memory
            if(output_file) {
                Symbol *sym = find_symbol(var_name);
                if(sym) {
                    const char *var_reg_name = get_register_name(sym->reg_num);
                    const char *r0_name = get_register_name(0);
                    // Load from memory into the allocated register
                    fprintf(output_file, "\tlb %s, %s(%s)\n", var_reg_name, sym->name, r0_name);
                }
            }
        }
        return;
    }

    // ============================================
    // CASE 2: ASSIGNMENT STATEMENT
    // ============================================
    // Look for '=' character to identify assignment
    char *eq_pos = strchr(temp, '=');
    if(eq_pos != NULL) {
        char lhs[64];   // Left-hand side (variable name)
        char rhs[256];  // Right-hand side (expression)
        
        // Extract left-hand side (everything before '=')
        int lhs_len = eq_pos - temp;
        strncpy(lhs, temp, lhs_len);
        lhs[lhs_len] = '\0';
        white_space_trim(lhs);
        
        // Extract right-hand side (everything after '=')
        strcpy(rhs, eq_pos + 1);
        white_space_trim(rhs);
        
        // ============================================
        // SEMANTIC CHECK 1: Verify LHS variable is declared
        // ============================================
        Symbol *sym = find_symbol(lhs);
        if(!sym) {
            // ERROR: Variable used without declaration
            fprintf(stderr, "\nVariable '%s' used without declaration\n", lhs);
            fprintf(stderr, "Statement: %s\n", temp);
            exit(1);
        }
        
        // ============================================
        // SEMANTIC CHECK 2: Verify all RHS variables are declared
        // ============================================
        char rhs_copy[256];
        strncpy(rhs_copy, rhs, sizeof(rhs_copy)-1);
        rhs_copy[sizeof(rhs_copy)-1] = '\0';
        
        int i = 0;
        // Parse through the right-hand side expression
        while(rhs_copy[i] != '\0') {
            // Skip whitespace
            while(isspace(rhs_copy[i])) i++;
            if(rhs_copy[i] == '\0') break;
            
            // Check if we found a variable name (starts with letter or underscore)
            if(ALPHABETIC_CHARACTER(rhs_copy[i])) {
                char var_name[64];
                int j = 0;
                // Extract the complete variable name
                while(ALPHANUMERIC_CHARACTER(rhs_copy[i]) && j < 63) {
                    var_name[j++] = rhs_copy[i++];
                }
                var_name[j] = '\0';
                
                // Verify this variable exists in symbol table
                if(!find_symbol(var_name)) {
                    // ERROR: Variable used without declaration
                    fprintf(stderr, "\nVariable '%s' used without declaration\n", var_name);
                    fprintf(stderr, "Statement: %s\n", temp);
                    exit(1);
                }
            } 
            // Skip over numeric literals
            else if(DIGIT_CHARACTER(rhs_copy[i])) {
                while(DIGIT_CHARACTER(rhs_copy[i])) i++;
            } 
            // Skip over operators
            else if(is_operator(rhs_copy[i])) {
                i++;
            } 
            // Skip over any other character
            else {
                i++;
            }
        }
        
        // ============================================
        // CODE GENERATION: Generate assembly for assignment
        // ============================================
        if(output_file) {
            // Mark variable as initialized and in register after assignment
            sym->initialized = 1;
            
            // Evaluate the right-hand side expression and store result in variable's register
            lexical_analyzer(rhs, output_file, sym->reg_num);
            
            const char *var_reg_name = get_register_name(sym->reg_num);
            const char *r0_name = get_register_name(0);
            
            // Store the result from the variable's register to memory
            fprintf(output_file, "\tsb %s, %s(%s)\n", var_reg_name, sym->name, r0_name);
            
            // Mark that the variable's value is now in its register
            sym->in_register = 1;
        }
        return;
    }
}
/*
 * ============================================
 * Function: add_variable
 * Purpose: Adds a new variable to the symbol table with register allocation
 * Parameters:
 *          name - variable name
 *          value - initial value
 *          initialized - flag indicating if variable has been initialized (1) or not (0)
 * ============================================
*/
void add_variable(const char *name, int value, int initialized) {
    // Check if symbol table is full
    if (symbol_count >= MAX_SYMBOLS) return;

    // Search for existing variable with the same name
    for(size_t i = 0; i < symbol_count; i++) {
        if(strcmp(symbol_table[i].name, name) == 0) {
            // Variable already exists - update its value and initialization status
            symbol_table[i].value = value;
            symbol_table[i].initialized = initialized;
            return;
        }
    }

    // Variable doesn't exist - add new entry to symbol table
    // Copy variable name (max 63 chars + null terminator)
    strncpy(symbol_table[symbol_count].name, name, 63);
    symbol_table[symbol_count].name[63] = '\0';  // Ensure null termination
    
    // Set variable properties
    symbol_table[symbol_count].value = value;
    symbol_table[symbol_count].initialized = initialized;
    symbol_table[symbol_count].mem_offset = symbol_count;  // Memory offset for assembly
    
    // Allocate a register for this variable
    symbol_table[symbol_count].reg_num = next_available_register;
    symbol_table[symbol_count].in_register = 0;  // Initially not in register
    next_available_register++;
    
    // Increment the symbol counter
    symbol_count++;
}

/* 
 * ============================================
 * Function: is_operator
 * Purpose: Checks if a character is an arithmetic operator
 * Parameters: c - character to check
 * Returns: 1 (true) if operator, 0 (false) otherwise
 * ============================================
*/
int is_operator(char c) {
    // Check for addition, subtraction, multiplication, or division
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

/* 
 * ============================================
 * Function: find_symbol
 * Purpose: Searches for a variable in the symbol table by name
 * Parameters: name - variable name to search for
 * Returns: Pointer to Symbol if found, NULL if not found
 * ============================================
*/
Symbol *find_symbol(const char *name) {
    // Linear search through symbol table
    for(size_t i = 0; i < symbol_count; i++) {
        if(strcmp(symbol_table[i].name, name) == 0) {
            // Found the variable - return pointer to its symbol table entry
            return &symbol_table[i];
        }
    }
    // Variable not found
    return NULL;
}

/*
 * ============================================
 * Function: white_space_trim
 * Purpose: Removes leading and trailing whitespace from a string (in-place)
 * Parameters: s - string to trim (modified in-place)
 * ============================================
*/
void white_space_trim(char *s) {
    // Validate input
    if(!s) return;
    
    // ============================================
    // STEP 1: Remove leading whitespace
    // ============================================
    // Find first non-whitespace character
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    
    // If there was leading whitespace, shift the string left
    if (p != s) {
        memmove(s, p, strlen(p) + 1);  // +1 to include null terminator
    }

    // ============================================
    // STEP 2: Remove trailing whitespace
    // ============================================
    // Find the last character of the string
    char *end = s + strlen(s) - 1;
    
    // Walk backwards from end, replacing whitespace with null terminators
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';  // Truncate at this position
        end--;
    }
}

/*
 * ============================================
 * Function: skip_escape_sequences_and_comments
 * Purpose: Advances the position index past whitespace and comments
 * Parameters:
 *   source_code - the source code string
 *   i - pointer to current position index (modified)
 *   line - pointer to current line number (modified)
 * ============================================
*/
void skip_escape_sequences_and_comments(const char *source_code, int *i, int *line) {
    // Continue looping until we hit non-whitespace, non-comment content
    while(source_code[*i] != '\0') {
        
        // ============================================
        // CASE 1: Skip whitespace (space, newline, tab, carriage return)
        // ============================================
        if(ESCAPE_SEQUENCE(source_code[*i])) {
            // Track line numbers when encountering newlines
            if(source_code[*i] == '\n') {
                (*line)++;
            }
            (*i)++;
        } 
        
        // ============================================
        // CASE 2: Skip single-line comments (//)
        // ============================================
        else if(source_code[*i] == '/' && source_code[(*i) + 1] == '/') {
            // Skip everything until end of line or end of file
            while(source_code[*i] != '\0' && source_code[*i] != '\n') {
                (*i)++;
            }
        } 
        
        // ============================================
        // CASE 3: Skip multi-line comments (/* ... */)
        // ============================================
        else if(source_code[*i] == '/' && source_code[(*i) + 1] == '*') {
            // Skip the opening "/*"
            (*i) += 2;
            
            // Continue until we find the closing "*/" or reach end of file
            while(source_code[*i] != '\0' && 
                  !(source_code[*i] == '*' && source_code[(*i) + 1] == '/')) {
                // Track line numbers within the comment block
                if(source_code[*i] == '\n') {
                    (*line)++;
                }
                (*i)++;
            }
            
            // Skip the closing "*/" if we found it
            if(source_code[*i] != '\0') {
                (*i) += 2;
            }
        } 
        
        // ============================================
        // Found actual code - stop skipping
        // ============================================
        else {
            break;
        }
    }
}