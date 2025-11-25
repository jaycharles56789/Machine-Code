/*
 * 
 * This file is for testing purposes only.
 *   
 */
#include <stdio.h>
#include "MiPS64.h"

int main(void) {
    const char *registers = get_register_code("$zero");
    const char *i_type = get_i_type_code("DADDIU");
    const R_type *r_type = get_r_type_code("DDIV");
    
    if(i_type) {
        printf("I-Type code: %s\n", i_type);
    }
    
    if (r_type) {
        printf("R-Type code: %s | %s | %s \n", r_type->Op_code, r_type->Shamt, r_type->Funct);
    } 
    
    if (registers) {
        printf("Register code: %s\n", registers);
    } 
    



    return 0;
}