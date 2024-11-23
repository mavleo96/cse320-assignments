#include "cook_utils.h"

/*
 * Function to validate args and set parameters; return -1 on error
 * TODO: more edge case error handling needed
 # TODO: change behaviour kwargs can appear anywhere
 */
void validargs(int argc, char *argv[], char *argrecipe[], char *argfile[], int *argmaxcooks) {
    int i = 1;

    // Initialize variables for default values
    *argfile = "rsrc/cookbook.ckb";
    *argmaxcooks = 1;  // Default to 1 cook if not specified

    while (i < argc) {
        // Handle '-c' option for max_cooks
        if (!strcmp(argv[i], "-c")) {
            if (i + 1 < argc) {
                i++;
                *argmaxcooks = atoi(argv[i]);
                if (*argmaxcooks < 1) {
                    error("invalid value for '-c' option: %d", *argmaxcooks);
                    exit(EXIT_FAILURE);
                }
            } else {
                error("missing value for '-c' option");
                exit(EXIT_FAILURE);
            }
        }
        // Handle '-f' option for cookbook filename
        else if (!strcmp(argv[i], "-f")) {
            if (i + 1 < argc) {
                i++;
                *argfile = argv[i];
            } else {
                error("missing filename for '-f' option");
                exit(EXIT_FAILURE);
            }
        }
        // Assign recipe if not already assigned
        else if (*argrecipe == NULL) {
            *argrecipe = argv[i];
            // All arguments after recipe name are ignore
            return;
        }
        else {
            error("invalid argument: %s", argv[i + 1]);
            exit(EXIT_FAILURE);
        }
        // Increment i
        i++;
    }
}

/*
 * Function to get recipe pointer for a given name from cookbook at given pointer
 */
RECIPE *get_main_recipe(COOKBOOK *cbp, char *name) {
    if (!name) {
        name = cbp->recipes->name;
        return cbp->recipes;
    }

    for(RECIPE *rp = cbp->recipes; rp != NULL; rp = rp->next) {
        if(!strcmp(rp->name, name))
            return rp;
    }
    return NULL;
}

/*
 * Function to length of recipe link recipe at given pointer
 */
int recipe_link_length(RECIPE_LINK *rlp) {
    int len = 0;
    while (rlp) {
        len++;
        rlp = rlp->next;
    }
    return len;
}