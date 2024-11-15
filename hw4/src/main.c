#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cookbook.h"
#include "cook_utils.h"

int main(int argc, char *argv[]) {

    char *cookbook;
    int max_cooks;
    char *recipe_name;

    if (validargs(argc, argv, &recipe_name, &cookbook, &max_cooks) == -1) {
        return EXIT_FAILURE;
    }
    debug("cooking %s from %s with %d cooks", recipe_name, cookbook, max_cooks);

    // COOKBOOK *cbp;
    // int err = 0;
    // FILE *in;
    // if((in = fopen(cookbook, "r")) == NULL) {
    // 	fprintf(stderr, "Can't open cookbook '%s': %s\n", cookbook, strerror(errno));
    //     return EXIT_FAILURE;
    // }
    // cbp = parse_cookbook(in, &err);
    // if(err) {
    // 	fprintf(stderr, "Error parsing cookbook '%s'\n", cookbook);
    //     return EXIT_FAILURE;
    // }

    return EXIT_SUCCESS;
}
