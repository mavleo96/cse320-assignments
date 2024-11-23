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
    RECIPE_LINK *subset_rlp;

    // Validate args and set variables
    validargs(argc, argv, &main_recipe_name, &cookbook, &max_cooks);

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

    // Get main recipe to cook
    if (!main_recipe_name) {
        main_recipe_name = cbp->recipes->name;
        main_rp = cbp->recipes;
    }
    else {
        if ((main_rp = get_main_recipe(cbp, main_recipe_name)) == NULL) {
            error("could not find %s in cookbook", main_recipe_name);
            exit(EXIT_FAILURE);
        }
    }
    debug("cooking %s from %s with %d cooks", main_recipe_name, cookbook, max_cooks);

    // Dependency analysis
    subset_rlp = dependency_analysis(main_rp, NULL);

    // Cook the recipe
    master_chef(main_rp, subset_rlp, max_cooks);

    // TODO: Need to free the cookbook
    // free(cbp);
    // free(subset);

    return EXIT_SUCCESS;
}
