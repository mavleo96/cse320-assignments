#include <stdio.h>
#include <stdlib.h>
#include "string_functions.h"

#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    int ret;
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & 0x1)
        USAGE(*argv, EXIT_SUCCESS);

    // Catch the DIR argument
    int count = 1;
    while (string_compare(*(argv + count), "-p") != 0) {
        count += 1;
    }
    char **dir_argv = argv + count + 1;
    path_init(*dir_argv);

    // Code to test path function implementations
    debug("This is path of length %d: %s", path_length, path_buf);

    // TODO: Calling serialize regardless for now
    serialize();

    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
