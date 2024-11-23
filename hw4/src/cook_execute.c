#include "cook_utils.h"

static void execute_step(STEP *step);

/*
 * Function to fork and execute step
 */
void perform_tasks(TASK *task) {
    if (!task) {
        error("null pointer passed!");
        exit(EXIT_FAILURE);
    }

    STEP *step = task->steps;
    int n = step_count(step);
    if (!n) {
        warn("no steps to execute in (pid %d)", getpid());
        exit(EXIT_SUCCESS);
    }

    // Setup input and output file descriptors
    int input_fd = -1, output_fd = -1;
    setup_file_descriptors(task, &input_fd, &output_fd);

    // Initialize pipes
    int pipe_size = 2 * (n - 1);
    int pipe_fd[pipe_size];
    initialize_pipes(pipe_fd, pipe_size);

    int rstatus = 0;  // Flag to mark error
    int count = 0;    // Step processing count
    while (step != NULL) {
        pid_t pid = fork();
        if (pid < 0) {
            // TODO: program should exit after reaping child processes
            error("fork failed with error in (pid %d): %s", getpid(), strerror(errno));
            rstatus = 1;
            break;
        }
        else if (!pid) {
            setup_io_redirection(input_fd, output_fd, pipe_fd, count, n);
            close_pipes(pipe_fd, n);
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
            exit(EXIT_FAILURE);
        }
        else {
            success("(pid %d) exiting successfully", getpid());
            exit(EXIT_SUCCESS);
        }
    }
    else {
        error("error in (pid %d) while waiting for child processes: %s", getpid(), strerror(errno));
        exit(EXIT_FAILURE);
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
