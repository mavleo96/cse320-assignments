#include "cook_utils.h"

int validargs(int argc, char *argv[], char *argrecipe[], char *argfile[], int *argmaxcooks) {
    int i = 1;

    // Initialize variables for default values
    // TODO: Veridy that eggs_benedict need not be dynamically assigned
    *argrecipe = "eggs_benedict";
    *argfile = "rsrc/cookbook.ckb";
    *argmaxcooks = 1;  // Default to 1 cook if not specified

    while (i < argc) {
        // Handle '-c' option for max_cooks
        if (strcmp(argv[i], "-c") == 0) {
            // TODO: Check if you need to exit with failure for multiple -c
            if (i + 1 < argc) {
                // TODO: Fail for c <0 or invalid case
                *argmaxcooks = atoi(argv[i + 1]);
                i++;
            } else {
                error("missing value for '-c' option");
                return -1;
            }
        }
        // Handle '-f' option for cookbook filename
        else if (strcmp(argv[i], "-f") == 0) {
            // TODO: Check if you need to exit with failure for multiple -f
            if (i + 1 < argc) {
                // TODO: Fail for empty string
                *argfile = argv[i + 1];
                i++;
            } else {
                error("missing filename for '-f' option");
                return -1;
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
            return -1;
        }
        // Increment i
        i++;
    }

    return 0;
}