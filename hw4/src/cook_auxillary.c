#include "cook_utils.h"

/*
 * Helper function to open file and return file descriptor
 * Used in perform task parent (before fork + execvp); can exit
 */
static int open_file(const char *file, int flags, mode_t mode) {
    if (!file) {
        return -1;
    }
    int fd = open(file, flags, mode);
    if (fd == -1) {
        error("can't open file %s: %s", file, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return fd;
}

/*
 * Helper function to setup file descriptors
 * Used in perform task parent (before fork + execvp); can exit
 */
void setup_file_descriptors(TASK *task, int *input_fd, int *output_fd) {
    if (!task) {
        error("null task pointer passed!");
        exit(EXIT_FAILURE);
    }
    *input_fd = open_file(task->input_file, O_RDONLY, 0);
    *output_fd = open_file(task->output_file, O_WRONLY | O_CREAT, 0744);
}

/*
 * Helper function to setup io redirection using dup2
 * Used in perform task child; exit without cleanup
 */
void setup_io_redirection(int input_fd, int output_fd, int pipe_fd[], int count, int n) {
    // Redirection of stdin
    if (input_fd != -1) {
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            error("dup2 failed for input file: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
        close(input_fd);
    } else if (count > 0) {
        if (dup2(pipe_fd[2 * (count - 1)], STDIN_FILENO) == -1) {
            error("dup2 failed for stdin: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
    }
    // Redirection of stdout
    if (count == n - 1) {
        if (output_fd != -1) {
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                error("dup2 failed for output file: %s", strerror(errno));
                _exit(EXIT_FAILURE);
            }
            close(output_fd);
        }
    } else {
        if (output_fd != -1) close(output_fd);
        if (dup2(pipe_fd[2 * count + 1], STDOUT_FILENO) == -1) {
            error("dup2 failed for stdout: %s", strerror(errno));
            _exit(EXIT_FAILURE);
        }
    }
}

/*
 * Helper function to initalize pipes
 * Used in perform task parent (before fork + execvp); can exit
 */
void initialize_pipes(int *pipe_fd, int pipe_size) {
    memset(pipe_fd, 0, pipe_size);
    for (int i = 0; i < pipe_size; i += 2) {
        if (pipe(&pipe_fd[i]) == -1) {
            error("error in creating pipe: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

/*
 * Helper function to close pipes
 */
void close_pipes(int pipe_fd[], int n) {
    for (int i = 0; i < 2 * n; i++) {
        close(pipe_fd[i]);
    }
}

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
