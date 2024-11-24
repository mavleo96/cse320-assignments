#include "cook_utils.h"

static void execute_step(STEP *step);

/*
 * Function to fork and execute step
 */
void perform_tasks(TASK *task) {
    if (!task) {
        error("null pointer passed!");
        _exit(EXIT_FAILURE);
    }

    STEP *step = task->steps;
    int n = step_count(step);
    if (!n) {
        warn("no steps to execute in (pid %d)", getpid());
        _exit(EXIT_SUCCESS);
    }

    // Setup input and output file descriptors
    int input_fd = -1;
    if (task->input_file) {
        if ((input_fd = open(task->input_file, O_RDONLY, 0)) == -1) {
            error("can't open input file %s: %s", task->input_file, strerror(errno));
            _exit(EXIT_FAILURE);
        }
    }

    int output_fd = -1;
    if (task->output_file) {
        if ((output_fd = open(task->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0755)) == -1) {
            error("can't open output file %s: %s", task->output_file, strerror(errno));
            _exit(EXIT_FAILURE);
        }
    }

    // Initialize pipes
    int pipe_size = 2 * (n - 1);
    int pipe_fd[pipe_size];
    // initialize_pipes(pipe_fd, pipe_size);
    memset(pipe_fd, 0, pipe_size);
    for (int i = 0; i < pipe_size; i += 2) {
        if (pipe(&pipe_fd[i]) == -1) {
            error("error in creating pipe: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
    }

    int rstatus = 0;  // Flag to mark error
    int count = 0;    // Step processing count
    while (step != NULL) {
        // error("DEBUGGING @ 1");
        pid_t pid = fork();
        if (pid < 0) {
            error("fork failed with error in (pid %d): %s", getpid(), strerror(errno));
            rstatus = 1;
            break;
        }
        else if (!pid) {
            if (input_fd != -1 && dup2(input_fd, STDIN_FILENO) == -1) {
                error("dup2 failed for input file: %s", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            else if (count > 0) {
                if (dup2(pipe_fd[2 * (count - 1)], STDIN_FILENO) == -1) {
                    error("dup2 failed for pipe input: %s", strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }

            if (count == n - 1) {
                if (output_fd != -1 && dup2(output_fd, STDOUT_FILENO) == -1) {
                    error("dup2 failed for output file: %s", strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            } 
            else {
                if (dup2(pipe_fd[2 * count + 1], STDOUT_FILENO) == -1) {
                    error("dup2 failed for stdout pipe: %s", strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }

            // close_pipes(pipe_fd, n);
            for (int i = 0; i < pipe_size; i++) {
                close(pipe_fd[i]);
            }
            execute_step(step);
        }
        // Close file descriptors
        if (input_fd != -1) {
            close(input_fd);
            input_fd = -1;
        }
        if ((count == n - 1) && (output_fd != -1)) close(output_fd);
        if (count > 0) {
            close(pipe_fd[2 * (count - 1) + 0]);
            close(pipe_fd[2 * (count - 1) + 1]);
        }
        step = step->next;
        count++;
    }

    // Reaping child processes
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, 0)) > 0) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status)) {
                error("(child %d) exited with status %d", pid, WEXITSTATUS(status));
                rstatus = 1;
            }
        }
        else if (WIFSIGNALED(status)) {
            error("(child %d) terminated by signal %d", pid, WTERMSIG(status));
            rstatus = 1;

        }
        else {
            error("(child %d) terminated abnormally", pid);
            rstatus  = 1;
        }
    }

    // Exit process
    if (pid == -1 && errno == ECHILD) {
        if (rstatus) {
            error("(pid %d) exiting with failure", getpid());
            _exit(EXIT_FAILURE);
        }
        else {
            success("(pid %d) exiting successfully", getpid());
            exit(EXIT_SUCCESS);
        }
    }
    else {
        error("error in (pid %d) while waiting for child processes: %s", getpid(), strerror(errno));
        _exit(EXIT_FAILURE);
    }
}

/*
 * Wrapper function for execvp to execute step
 */
static void execute_step(STEP *step) {
    if (!step || !step->words || !*step->words) {
        error("null pointer passed!");
        _exit(EXIT_FAILURE);
    }

    // Buffer to hold path
    size_t length = sizeof("util/") + strlen(*step->words) + 1;
    char path[length];
    snprintf(path, length, "util/%s", *step->words);

    // Execute command
    if (execvp(path, step->words) == -1) {
        warn("execvp for path %s failed with error: %s", path, strerror(errno));

        if (execvp(*step->words, step->words) == -1) {
            error("execvp failed with error: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
    }
}
