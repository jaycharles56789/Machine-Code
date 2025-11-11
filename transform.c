#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "MIPS64.h"

int transform_to_assembly(const char *input, char *out_buf, size_t out_buf_sz) {
    char var1[32], var2[32], var3[32], op;
    int value;

    // Case 1: "int a = 5;"
    if (sscanf(input, "int %31s = %d;", var1, &value) == 2) {
        snprintf(out_buf, out_buf_sz,
                 "DADDIU $t0, $zero, %d   # load immediate %s = %d", value, var1, value);
        return 0;
    }

    // Case 2: "int c = a + b;"
    if (sscanf(input, "int %31s = %31s %c %31s;", var1, var2, &op, var3) == 4) {
        if (op == '+')
            snprintf(out_buf, out_buf_sz,
                     "DADDU $t2, $t0, $t1   # %s = %s + %s", var1, var2, var3);
        else if (op == '-')
            snprintf(out_buf, out_buf_sz,
                     "DSUBU $t2, $t0, $t1   # %s = %s - %s", var1, var2, var3);
        else
            snprintf(out_buf, out_buf_sz, "Unsupported operation");
        return 0;
    }

    // Case 3: "a + b + c"
    if (sscanf(input, "%31s + %31s + %31s", var1, var2, var3) == 3) {
        snprintf(out_buf, out_buf_sz,
                 "DADDU $t3, $t0, $t1\nDADDU $t3, $t3, $t2   # %s + %s + %s",
                 var1, var2, var3);
        return 0;
    }

    snprintf(out_buf, out_buf_sz, "Could not parse input.");
    return 1;
}
