#include "global.h"
#include "helper_functions.h"

void clear_input_buffer() {
    char ch;
    while ((ch = getchar()) != EOF) {
        // continue
    }
}

int file_exists(const char *name) {
    FILE *f = fopen(name, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}