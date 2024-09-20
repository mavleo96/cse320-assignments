#include "global.h"
#include "helper_functions.h"

void clear_input_buffer() {
    char ch;
    while ((ch = getchar()) != EOF) {
        // continue
    }
}

void clear_string(char *str) {
    int count = 0;
    while (*(str + count) != '\0') {
        *(str + count) = '\0';
        count++;
    }
}