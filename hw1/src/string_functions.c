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

void string_clear(char *str) {
    int count = 0;
    while (*(str + count) != '\0') {
        *(str + count) = '\0';
        count++;
    }
}

int string_copy(char *str1, char *str2) {

    int length_1 = string_length(str1);
    int length_2 = string_length(str2);

    int loop_length;
    if (length_1 < length_2) {
        loop_length = length_2;
    } else {
        loop_length = length_1;
    }

    for (int i = 0; i <= loop_length; i++) {
        if (i <= length_2) {
            *(str1 + i) = *(str2 + i);
        } else {
            *(str1 + i) = '\0';
        }
    }
    return 0;
}