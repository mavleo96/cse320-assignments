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
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & 0x1)
        USAGE(*argv, EXIT_SUCCESS);

    // Catch the DIR argument
    // TODO: Below logic doesn't work if p not present 
    int count = 1;
    while (string_compare(*(argv + count), "-p") != 0) {
        count += 1;
    }
    char **dir_argv = argv + count + 1;
    path_init(*dir_argv);

    // Code to test path function implementations
    debug("This is path of length %d: %s", path_length, path_buf);

    int status;
    if ((global_options & (1 << 1)) == (1 << 1)){
        debug("ENTERING SERIALIZATION");
        status = serialize();
    } else if ((global_options & (1 << 2)) == (1 << 2)) {
        debug("ENTERING DESERIALIZATION");
        mkdir(path_buf, 0775);
        status = deserialize();
    }

    if (status == 0) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
