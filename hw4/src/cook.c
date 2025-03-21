#include "cook_utils.h"
#include "globals.h"

void sous_chef(RECIPE *rp);
void sigchld_handler(int sig);

volatile int global_error_status = 0;

/*
 * Main cooking function
 */
int master_chef(RECIPE *main_rp) {
    // Installing signal handler for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        error("sigaction failed with error: %s", strerror(errno));
        return -1;
    }

    // Intializing signal masks
    sigset_t block_set, empty_set;
    sigemptyset(&empty_set);
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGCHLD);

    // Initialize queue
    if (queue_leaves(&queue, subset_rlp) == -1) {
        return -1;
    }
    // Master chef continues until queue is null or active cooks is zero
    while (queue.head || ACTIVE_COOKS > 0) {
        if (ACTIVE_COOKS > MAX_COOKS) error("active cooks exceed max allowed cooks!");

        // Enter sigsuspend if at full capacity or queue is null
        if (ACTIVE_COOKS == MAX_COOKS || !queue.head) {
            warn("Suspending main loop until recipes are cooked!");
            if (sigsuspend(&empty_set) == -1) {
                if (errno == EINTR) {
                    debug("sigsuspend returned with errno: %s", strerror(errno));
                } else {
                    error("sigsuspend failed with error: %s", strerror(errno));
                    queue.head = NULL;
                    global_error_status = 1;
                }
            }
        }
        // Skip if queue is null
        if (!queue.head) {
            continue;
        }
        
        // Block SIGCHLD
        if (sigprocmask(SIG_BLOCK, &block_set, NULL) == -1) {
            error("sigprocmask failed with error: %s", strerror(errno));
            global_error_status = 1;
            queue.head = NULL;
        }

        //-----CRITICAL SECTION-----//
        RECIPE* cooking_rp = queue.head->recipe;
        if (dequeue(&queue) == -1) {
            global_error_status = 1;
            continue;
        }
        ACTIVE_COOKS++;
        info("active cooks %d out of %d", ACTIVE_COOKS, MAX_COOKS);

        pid_t pid = fork();
        PID(cooking_rp) = pid;
        if (pid < 0) {
            error("fork failed with error in (pid %d): %s", getpid(), strerror(errno));
            global_error_status = 1;
            queue.head = NULL;
            ACTIVE_COOKS--;
            continue;
        }

        //-----CRITICAL SECTION-----//

        // Unblock SIGCHLD
        if (sigprocmask(SIG_UNBLOCK, &block_set, NULL) == -1) {
            error("sigprocmask failed with error: %s", strerror(errno));
            global_error_status = 1;
            queue.head = NULL;
        }

        // Child Process
        else if (!pid) {
            debug("start cook: %s by (pid %d)", cooking_rp->name, getpid());
            // Restore default SIGCHLD handler
            struct sigaction sa_child;
            sa_child.sa_handler = SIG_DFL;
            sa_child.sa_flags = 0;
            if (sigaction(SIGCHLD, &sa_child, NULL) == -1) {
                error("sigaction failed with error: %s", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            sous_chef(cooking_rp);
        }
    }

    // Exit as per main recipe status
    if (global_error_status) {
        error("exiting with error since global_error_status is set");
        return -1;
    }
    if (CSTATUS(main_rp) != 1) {
        if (!CSTATUS(main_rp)) {
            error("main recipe '%s' was not cooked", main_rp->name);
        } else {
            error("main recipe '%s' failed during cook process", main_rp->name);
        }
        return -1;
    }
    else {
        success("main recipe '%s' has been successfully cooked", main_rp->name);
        return 0;
    }
}

/*
 * Function to simulate task processing
 */
void sous_chef(RECIPE *rp) {
    if (!rp) {
        error("null pointer passed!");
        _exit(EXIT_FAILURE);
    }

    TASK *task = rp->tasks;
    int task_count = 1;

    // Sequential task processing 
    while (task != NULL) {
        debug("doing task %d of '%s'", task_count, rp->name);
        pid_t pid = fork();
        if (pid < 0) {
            error("fork failed with error in (pid %d, ppid %d): %s", getpid(), getppid(), strerror(errno));
            _exit(EXIT_FAILURE);
        }
        // Child process
        else if (!pid) {
            perform_tasks(task);
        }
        // Parent process
        else {
            // Reap the child
            debug("waiting (pid %d, ppid %d) for (child %d) to complete task %d of '%s'", getpid(), getppid(), pid, task_count, rp->name);

            // Exit if error
            int status;
            if (waitpid(pid, &status, 0) < 0) {
                error("error while waiting for (child %d): %s", pid, strerror(errno));
                _exit(EXIT_FAILURE);
            }
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status)) {
                    error("(child %d) exited with status %d", pid, WEXITSTATUS(status));
                    _exit(EXIT_FAILURE);
                }
                else {
                    success("task %d of '%s' done", task_count, rp->name);
                }
            }
            else if (WIFSIGNALED(status)) {
                error("(child %d) terminated by signal %d", pid, WTERMSIG(status));
                _exit(EXIT_FAILURE);
            }
            else {
                error("(child pid %d) terminated abnormally", pid);
                _exit(EXIT_FAILURE);
            }
        }
        // Update task and count
        task = task->next;
        task_count++;
    }
    success("(pid %d) exiting successfully", getpid());
    _exit(EXIT_SUCCESS);
}

/*
 * Signal handler to catch SIGCHLD; handler reaps the child and updates dependencies & queue
 */
void sigchld_handler(int sig) {
    (void) sig;

    int status;
    pid_t pid;

    // Reap sous chefs
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Retrive recipe pointer run in child
        RECIPE *rp = get_recipe_from_pid(pid);
        if (!rp) {
            error("cannot identify (pid %d)!", pid);
            global_error_status = 1;
            ACTIVE_COOKS--;
            return;
        }

        // Update state of recipe according to exit status
        if (WIFEXITED(status)) {
            if (!WEXITSTATUS(status)) {
                CSTATUS(rp) = 1;
                success("(pid %d) finished cooking recipe '%s'", pid, rp->name);
            }
            else {
                CSTATUS(rp) = 2;
                error("(pid %d) finished cooking recipe '%s' with exit status %d", pid, rp->name, WEXITSTATUS(status));
                global_error_status = 1;
            }
        }
        else if (WIFSIGNALED(status)) {
            CSTATUS(rp) = 2;
            error("(pid %d) terminated cooking recipe '%s' by signal %d", pid, rp->name, WTERMSIG(status));
            global_error_status = 1;
        }
        else {
            CSTATUS(rp) = 2;
            error("(pid %d) terminated cooking recipe '%s' abnormally", pid, rp->name);
            global_error_status = 1;
        }

        // Update dependencies and queue
        ACTIVE_COOKS--;
        if (!global_error_status) {
            update_dependency_count(rp);
            if (queue_leaves(&queue, subset_rlp) == -1) {
                global_error_status = 1;
            }
        } else {
            error("not updating dependencies since global_error_status is set");
        }
    }
    debug("exiting sigchld_handler");
    return;
}