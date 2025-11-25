/*
 * 
 * This file is for testing purposes only.
 *   
 */
#include <stdio.h>
#include "MiPS64.h"

int main(void) {
    const char *rig = get_register_code("r1");

    if(rig) {
        printf("Register code: %s\n", rig);
    } else {
        printf("Register not found.\n");
    }

    return 0;
}