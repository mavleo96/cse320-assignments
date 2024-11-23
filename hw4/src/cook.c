#include "cook_utils.h"

/*
 * Function to simulate task processing
 */
void sous_chef(RECIPE *rp) {
    if (!rp) {
        error("null pointer passed!");
        exit(EXIT_FAILURE);
    }

    TASK *task = rp->tasks;
    int task_count = 1;

    while (task != NULL) {
        pid_t pid = fork();
        if (pid < 0) {
            // TODO: program should exit after reaping child processes
            error("fork failed with error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (!pid) {
            debug("start task: task %d of %s by (pid %d, ppid %d)", task_count, rp->name, getpid(), getppid());
            perform_tasks(task);
        }
        else {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                if (!WEXITSTATUS(status)) {
                    debug("finish task: task %d of %s by (pid %d, ppid %d, status %d)", task_count, rp->name, getpid(), getppid(), WEXITSTATUS(status));
                }
                else {
                    error("finish task: task %d of %s by (pid %d, ppid %d, status %d)", task_count, rp->name, getpid(), getppid(), WEXITSTATUS(status));
                    exit(EXIT_FAILURE);
                }
            } else if (WIFSIGNALED(status)) {
                error("child terminated by signal %d", WTERMSIG(status));
            }
        }
        task = task->next;
        task_count++;
    }
    exit(EXIT_SUCCESS);
}


/*
 * Main cooking function
 */
void master_chef(RECIPE *main_rp, RECIPE_LINK *subset, int max_cooks) {
    QUEUE queue = {NULL, NULL};
    if (queue_leaves(&queue, subset) == -1) {
        // TODO: handle this gracefully
        abort();
    }

    while (queue.head != NULL) {
        RECIPE* cooking_rp = queue.head->recipe;
        if (dequeue(&queue) == -1) {
            // TODO: handle this gracefully
            abort();
        }

        pid_t pid = fork();
        if (pid < 0) {
            // TODO: program should exit after reaping child processes
            error("fork failed with error: %s", strerror(errno));
            abort();
        }
        else if (!pid) {
            debug("start cook: %s by (pid %d)", cooking_rp->name, getpid());
            sous_chef(cooking_rp);
            // exit(EXIT_SUCCESS);
        }
        else {
            // debug("wait cook: %s by (pid %d)", cooking_rp->name, pid);
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                if (!WEXITSTATUS(status)) {
                    // debug("finish cook: %s by (pid %d, status %d)", cooking_rp->name, pid, WEXITSTATUS(status));
                }
                else {
                    error("finish cook: %s by (pid %d, status %d)", cooking_rp->name, pid, WEXITSTATUS(status));
                    exit(EXIT_FAILURE);
                }
            } else if (WIFSIGNALED(status)) {
                error("error in cook recipe: child %d terminated by signal %d", pid, WTERMSIG(status));
            }
        }
        update_dependency_count(cooking_rp);
        if (queue_leaves(&queue, subset) == -1) {
            // TODO: handle this gracefully
            abort();
        }
    }
}