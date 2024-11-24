#include "cook_utils.h"

/*
 * Debug function to list elements of recipe link
 */
void print_recipe_link(RECIPE_LINK *rlp) {
    debug("linked list at %p", rlp);
    while (rlp) {
        debug("[%s]", rlp->recipe->name);
        rlp = rlp->next;
    }
}

/*
 * Debug function to list elements of queue
 */
void print_queue(QUEUE *qp) {
    debug("queue at %p", qp);
    RECIPE_LINK *head = qp->head;
    while (head) {
        debug("[%s]", head->recipe->name);
        head = head->next;
    }    
}