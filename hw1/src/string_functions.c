#include <stdio.h>
#include "string_functions.h"

int string_compare(char *str1, char *str2) {
    int count = 0;
    if (string_length(str1) != string_length(str2)) {
        return -1;
    }
    while (*(str1 + count) != '\0') {
        if (*(str1 + count) == *(str2 + count)) {
            count += 1;
        } else {
            return -1;
        }
    }
    return 0;
}

int string_length(char *str1) {
    int count = 0;
    while (*(str1 + count) != '\0') {
        count += 1;
    }
    return count;
}