#include <stdio.h>
#include <stdlib.h>
#include "string_functions.h"
#include "path_functions.h"

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

#define OPTION_HELP (1 << 0)
#define OPTION_SERIALIZE (1 << 1) 
#define OPTION_DESERIALIZE (1 << 2)

int main(int argc, char **argv)
{
    // Validate arguments
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & OPTION_HELP)
        USAGE(*argv, EXIT_SUCCESS);

    // Initialize path_buf
    if (path_init(".") == -1) return EXIT_FAILURE;
    debug("Path initialized with path length %d to %s", path_length, path_buf);
    for (int count = 0; count < argc; count++) {
        if (string_compare(*(argv + count), "-p") == 0) {
            char *dir_argv = *(argv + count + 1);
            if (path_init(dir_argv) == -1) return EXIT_FAILURE;
            debug("Path initialized with path length %d to %s", path_length, path_buf);
            break;
        }
    }

    // Enter function according to global_options
    int status;
    if ((global_options & OPTION_SERIALIZE) == OPTION_SERIALIZE) {
        debug("Entering serialization...");
        // Check if initialized path exists
        struct stat stat_buf;
        if (stat(path_buf, &stat_buf) == -1) {
            status = -1;
            error("Provided path does not exist");
        } else {
            status = serialize();
        }
        debug("Exiting serialization...");
    } else if ((global_options & OPTION_DESERIALIZE) == OPTION_DESERIALIZE) {
        debug("Entering deserialization...");
        if (string_compare(path_buf, ".") != 0) mkdir(path_buf, 0775);  // Make directory with 775 permission if not set to cwd
        status = deserialize();
        debug("Exiting deserialization...");
    }

    // Exit program as per status value
    if (status == 0) {
        debug("Exiting program successfully");
        return EXIT_SUCCESS;
    } else {
        debug("Exiting program with failure");
        return EXIT_FAILURE;
    }
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
