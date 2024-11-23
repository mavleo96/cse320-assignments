#include "cook_utils.h"

static int linked_list_search(RECIPE *rp, RECIPE_LINK *rlp);
static STATE *initialize_state(RECIPE *rp);

/*
 * Function to recursively traverse and update dependency and subset recipe linked list
 */
RECIPE_LINK *dependency_analysis(RECIPE *rp, RECIPE_LINK *subset) {
    if (!rp) {
        error("null recipe pointer passed!");
        exit(EXIT_FAILURE);
    }
    if (linked_list_search(rp, subset)) {
        return subset;
    }
    
    // Initialize state / dependency count
    debug("dependency intialize: %s", rp->name);
    if (rp->state){
        error("state field for recipe %s already initialized!", rp->name);
        exit(EXIT_FAILURE);
    }
    rp->state = (void *) initialize_state(rp);

    // Add recipe to subset recipe linked list
    RECIPE_LINK *new_np = malloc(sizeof(RECIPE_LINK));
    if (new_np == NULL) {
        error("error in allocating memory for subset node");
        exit(EXIT_FAILURE);
    }
    new_np->name = rp->name;
    new_np->recipe = rp;
    new_np->next = subset;
    subset = new_np;

    // Update state / dependency count for dependencies link
    RECIPE_LINK *rlp = rp->this_depends_on;
    while (rlp) {
        subset = dependency_analysis(rlp->recipe, subset);
        rlp = rlp->next;
    }
    return subset;
}

/*
 * Static function to search a recipe in given recipe link; returns 1 if found else 0
 */
static int linked_list_search(RECIPE *rp, RECIPE_LINK *rlp) {
    if (!rp) {
        error("null recipe pointer passed!");
        exit(EXIT_FAILURE);
    }
    // if (!rlp) {
    //     error("null recipe link pointer passed!");
    //     exit(EXIT_FAILURE);
    // }

    while (rlp) {
        if (rlp->recipe == rp) {
            return 1;
        }
        rlp = rlp->next;
    }
    return 0;
}

/*
 * Static function to initalize state for a recipe
 */
static STATE *initialize_state(RECIPE *rp) {
    STATE *sp = malloc(sizeof(STATE));
    sp->dcount = recipe_link_length(rp->this_depends_on);
    sp->qstatus = 0;
    sp->cstatus = 0;
    return sp;
}

/*
 * Function to update dependency count
 */
void update_dependency_count(RECIPE *rp) {
    if (!rp) {
        error("null recipe pointer passed!");
        exit(EXIT_FAILURE);
    }
    RECIPE_LINK *dlist = rp->depend_on_this;
    while (dlist != NULL) {
        // Update dependency count if state is initialized
        if (dlist->recipe->state) {
            DEP_COUNT(dlist->recipe)--;
        }
        dlist = dlist->next;
    }
}
