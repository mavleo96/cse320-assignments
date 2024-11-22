#include "cook_utils.h"

/*
 * Function to get recipe pointer for a given name from cookbook at given pointer
 */
RECIPE *get_recipe(COOKBOOK *cbp, char *name) {
    for(RECIPE *rp = cbp->recipes; rp != NULL; rp = rp->next) {
        if(!strcmp(rp->name, name))
            return rp;
    }
    return NULL;
}

/*
 * Function to initialise dependency count
 */
void initialise_dependency_count(COOKBOOK *cbp) {
    if (!cbp){
        error("null cookbook pointer passed!");
        exit(EXIT_FAILURE);
    }
    RECIPE *rp = cbp->recipes;
    while (rp != NULL) {
        void *dcount = 0;
        RECIPE_LINK *dlist = rp->this_depends_on;
        while (dlist != NULL) {
            dcount++;
            dlist = dlist->next;
        }
        if (rp->state){
            error("state field for recipe %s already initialised with %ld!", rp->name, (long int)rp->state);
            exit(EXIT_FAILURE);
        }
        rp->state = dcount;
        rp = rp->next;
    }
}

/*
 * Function to update dependency count
 */
void update_dependency_count(RECIPE *rp) {
    if (!rp) {
        error("null recipe pointer passed!");
        exit(EXIT_FAILURE);
    }
    rp->state = (void *) -1;
    RECIPE_LINK *dlist = rp->depend_on_this;
    while (dlist != NULL) {
        dlist->recipe->state--;
        dlist = dlist->next;
    }
}

/*
 * Function to queue leaves
 * TODO: change this function to work for given main_rp
 */
void queue_leaves(COOKBOOK *cbp, RECIPE_LINK **queue) {
    if (!cbp) {
        error("null cookbook pointer passed!");
        exit(EXIT_FAILURE);
    }
    if (!queue) {
        error("null queue pointer passed!");
        exit(EXIT_FAILURE);
    }

    RECIPE_LINK *tail = *queue;
    if (tail != NULL) {
        while (tail->next != NULL) {
            tail = tail->next;
        }
    }
    RECIPE *rp = cbp->recipes;
    while (rp != NULL) {
        if ((long int) rp->state == 0) {
            RECIPE_LINK *new_np = malloc(sizeof(RECIPE_LINK));
            if (new_np == NULL) {
                error("error in allocating memory for queue node");
                exit(EXIT_FAILURE);
            }

            new_np->name = rp->name;
            new_np->recipe = rp;
            new_np->next = NULL;
            rp->state = (void *) -1;

            debug("add to queue: %s", new_np->name);
            if (*queue == NULL) {
                *queue = new_np;
                tail = *queue;
            } else {
                tail->next = new_np;
            }
            tail = new_np;
        }
        rp = rp->next;
    }
}
