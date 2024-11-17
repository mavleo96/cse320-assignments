#include "cook_utils.h"

/*
 * Function to validate args and set parameters; return -1 on error
 * TODO: more edge case error handling needed
 */
void validargs(int argc, char *argv[], char *argrecipe[], char *argfile[], int *argmaxcooks) {
    int i = 1;

    // Initialize variables for default values
    // TODO: Veridy that eggs_benedict need not be dynamically assigned
    *argrecipe = "eggs_benedict";
    *argfile = "rsrc/cookbook.ckb";
    *argmaxcooks = 1;  // Default to 1 cook if not specified

    while (i < argc) {
        // Handle '-c' option for max_cooks
        if (!strcmp(argv[i], "-c")) {
            // TODO: Check if you need to exit with failure for multiple -c
            if (i + 1 < argc) {
                // TODO: Fail for c <0 or invalid case
                *argmaxcooks = atoi(argv[i + 1]);
                i++;
            } else {
                error("missing value for '-c' option");
                exit(EXIT_FAILURE);
            }
        }
        // Handle '-f' option for cookbook filename
        else if (!strcmp(argv[i], "-f")) {
            // TODO: Check if you need to exit with failure for multiple -f
            if (i + 1 < argc) {
                // TODO: Fail for empty string
                *argfile = argv[i + 1];
                i++;
            } else {
                error("missing filename for '-f' option");
                exit(EXIT_FAILURE);
            }
        } 
        // TODO: Below code if you can't defaualt main recipe with static assignment
        // Assign recipe if not already assigned
        // else if (*argrecipe == NULL) {
        //     *argrecipe = argv[i];
        // }
        else if (argc == i + 1) {
            *argrecipe = argv[i];
        }
        // If no valid argument is found
        else {
            error("invalid argument: %s", argv[i + 1]);
            exit(EXIT_FAILURE);
        }
        // Increment i
        i++;
    }
}