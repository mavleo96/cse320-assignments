#include "cook_utils.h"
#include "globals.h"

/*
 * Helper function to return length of task
 */
int step_count(STEP *step) {
    int len = 0;
    while (step) {
        len++;
        step = step->next;
    }
    return len;
}

/*
 * Helper function to retrive pointer of recipe run by a given process
 */
RECIPE *get_recipe_from_pid(pid_t pid) {
    RECIPE_LINK *rlp = subset_rlp;
    while (rlp) {
        RECIPE *rp = rlp->recipe;
        if (PID(rp) == pid) {
            return rp;
        }
        rlp = rlp->next;
    }
    return NULL;
}
