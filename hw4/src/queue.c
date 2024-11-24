#include "cook_utils.h"

/*
 * Function to enqueue a recipe; return -1 if error else 0
 */
int enqueue(RECIPE *rp, QUEUE *qp) {
    if (!rp) {
        error("null recipe pointer passed!");
        return -1;
    }
    if (!qp) {
        error("null queue pointer passed!");
        return -1;
    }

    // Create a recipe link node and initialize with rp
    RECIPE_LINK *new_np = malloc(sizeof(RECIPE_LINK));
    if (new_np == NULL) {
        error("error in allocating memory for queue node");
        return -1;
    }
    new_np->name = rp->name;
    new_np->recipe = rp;
    new_np->next = NULL;
    QSTATUS(rp) = 1;

    // Add to queue
    if (!qp->tail) {
        qp->head = qp->tail = new_np;
    }
    else {
        qp->tail->next = new_np;
        qp->tail = new_np;
    }
    return 0;
}

/*
 * Function to dequeue head; return -1 if error else 0
 */
int dequeue(QUEUE *qp) {
    if (!qp) {
        error("null queue pointer passed!");
        return -1;
    }
    if (!qp->head) {
        error("queue is empty!");
        return -1;
    }

    // Dequeue the head and free the pointer
    RECIPE_LINK *old_head = qp->head;
    RECIPE_LINK *new_head = qp->head->next;
    QSTATUS(old_head->recipe) = 2;
    free(old_head);

    // Pop from queue
    if (!new_head) {
        qp->head = qp->tail = NULL;
    }
    else {
        qp->head = new_head;
    }
    return 0;
}

/*
 * Function to enqueue ready recipes
 */
int queue_leaves(QUEUE *qp, RECIPE_LINK *subset) {
    if (!subset) {
        error("null subset pointer passed!");
        return -1;
    }
    if (!qp) {
        error("null queue pointer passed!");
        return -1;
    }

    // Loop over subset recipe link and add ready recipes
    while (subset) {
        RECIPE *rp = subset->recipe;
        if (!rp) {
            error("null recipe in queue!");
            return -1;
        }
        // Add to queue if qstatus is 0 and dependency count is 0
        if (!QSTATUS(rp) && !DEP_COUNT(rp)) {
            debug("add to queue: %s", rp->name);
            if (enqueue(rp, qp) == -1) {
                return -1;
            }
        }
        subset = subset->next;
    }
    return 0;
}