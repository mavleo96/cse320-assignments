#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cookbook.h"
#include "cook_utils.h"

int main(int argc, char *argv[]) {
    // Intialize variables
    char *cookbook;
    int max_cooks;
    char *main_recipe_name;
    RECIPE *main_rp;

    // Validate args and set variables
    validargs(argc, argv, &main_recipe_name, &cookbook, &max_cooks);
    debug("cooking %s from %s with %d cooks", main_recipe_name, cookbook, max_cooks);

    // Parse cookbook and get recipe to cook
    COOKBOOK *cbp;
    int err = 0;
    FILE *in;
    if((in = fopen(cookbook, "r")) == NULL) {
    	error("can't open cookbook %s: %s", cookbook, strerror(errno));
        exit(EXIT_FAILURE);
    }
    cbp = parse_cookbook(in, &err);
    if(err) {
    	error("error parsing cookbook %s", cookbook);
        exit(EXIT_FAILURE);
    }
    initialise_dependency_count(cbp);
    
    if ((main_rp = get_recipe(cbp, main_recipe_name)) == NULL) {
    	error("could not find %s in cookbook at %s", main_recipe_name, cookbook);
        exit(EXIT_FAILURE);
    }
    // Cook the recipe
    cook_program(cbp, main_rp);

    return EXIT_SUCCESS;
}
