#include <stdio.h>
#include <string.h>
#include "mips64.h"

void whitespace_tream(char *str);

int main(void) {
    char string[128];
    return 0;
}

void whitespace_tream(char *str) {
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}