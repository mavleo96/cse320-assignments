#include "cook_utils.h"

void perform_tasks(TASK *task) {
    if (!task) {
        error("null pointer passed!");
        exit(EXIT_FAILURE);
    }

    int input_fd = -1;
    if (task->input_file != NULL) {
        int input_fd = open(task->input_file, O_RDONLY);
        if (input_fd == -1) {
            error("can't open input file %s: %s", task->input_file, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    int output_fd = -1;
    if (task->output_file != NULL) {
        // TODO: check what permissions are needed for new file
        int output_fd = open(task->output_file, O_WRONLY | O_CREAT);
        if (output_fd == -1) {
            error("can't open output file %s: %s", task->output_file, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    int pipefd[2];
    int prev_pipe = -1;
    STEP *step = task->steps;

    while (step != NULL) {
        if (step->next != NULL) {
            if (pipe(pipefd) == -1) {
                error("error in creating pipe: %s", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            // TODO: program should exit after reaping child processes
            error("fork failed with error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (!pid) {
            if (prev_pipe != -1) {
                if (dup2(prev_pipe, STDIN_FILENO) == -1) {
                    error("error in dup2 (stdin): %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                close(prev_pipe);
            }
            else if (input_fd != -1) {
                if (dup2(input_fd, STDIN_FILENO) == -1) {
                    error("dup2 failed for input file: %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                close(input_fd);
            }

            if (step->next != NULL) {
                close(pipefd[0]);
                if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                    error("error in dup2 (stdout): %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                close(pipefd[1]);
            }
            else if (output_fd != -1) {
                if (dup2(output_fd, STDOUT_FILENO) == -1) {
                    error("dup2 failed for input file: %s", strerror(errno));
                    exit(EXIT_FAILURE);
                }
                close(output_fd);
            }

            execute_step(step);
            // exit(EXIT_FAILURE);
        }

        if (prev_pipe != -1) {
            close(prev_pipe);
        }

        if (step->next != NULL) {
            close(pipefd[1]);
            prev_pipe = pipefd[0];
        }
        step = step->next;
    }

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, 0)) > 0) { // Wait for any child process
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status)) {
                debug("error in step!");
            }
        } else if (WIFSIGNALED(status)) {
            debug("Child process %d was terminated by signal %d", pid, WTERMSIG(status));
        } else {
            error("Child process %d terminated abnormally", pid);
        }
    }


    if (input_fd != -1) close(input_fd);
    if (output_fd != -1) close(output_fd);

    if (pid == -1 && errno == ECHILD) {
        debug("all steps completed");
        exit(EXIT_SUCCESS);
    }
    else if (errno != ECHILD) {
        error("Error while waiting for child processes: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    else if (pid == -1) {
        error("Error while waiting for child processes: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
}


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
void master_chef(COOKBOOK *cbp, RECIPE *main_rp) {
    // TODO: change to cook main_rp
    // RECIPE_LINK *recipe_subset;
    RECIPE_LINK *queue = NULL;
    queue_leaves(cbp, &queue);

    while (queue != NULL) {
        RECIPE* cooking_rp = queue->recipe;
        queue = queue->next;

        pid_t pid = fork();
        if (pid < 0) {
            // TODO: program should exit after reaping child processes
            error("fork failed with error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if (!pid) {
            debug("start cook: %s by (pid %d)", cooking_rp->name, getpid());
            sous_chef(cooking_rp);
        }
        else {
            debug("wait cook: %s by (pid %d)", cooking_rp->name, pid);
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                if (!WEXITSTATUS(status)) {
                    debug("finish cook: %s by (pid %d, status %d)", cooking_rp->name, pid, WEXITSTATUS(status));
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
        queue_leaves(cbp, &queue);
    }
}