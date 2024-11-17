#include "cook_utils.h"

/*
 * Function to simulate task processing
 */
static void task_processing(TASK *taskp) {
    STEP *stepp = taskp->steps;
    info(">processing_task");
    while (stepp != NULL) {
        info(">>processing step: %s %s", *stepp->words, *(stepp->words+1));
        stepp = stepp->next;
    }
    
    if (taskp->input_file != NULL) {
        info("Input file: %s", taskp->input_file);
    }

    if (taskp->output_file != NULL) {
        info("Output file: %s", taskp->output_file);
    }
}

/*
 * Main cooking function
 */
void cook_program(COOKBOOK *cbp, RECIPE *main_rp) {
    // RECIPE_LINK *recipe_subset;
    RECIPE_LINK *qp = NULL;
    queue_leaves(cbp, &qp);

    while (qp != NULL) {
        RECIPE* cooking_rp = qp->recipe;        
        qp = qp->next;
        info("cooking %s...", cooking_rp->name);

        TASK *taskp = cooking_rp->tasks;

        while (taskp != NULL) {
            task_processing(taskp);
            taskp = taskp->next;
        }

        update_dependency_count(cooking_rp);
        queue_leaves(cbp, &qp);
    }
}

