#include "cook_utils.h"

void execute_step(STEP *step) {
    if (!step || !step->words || !*step->words) {
        error("null pointer passed!");
        exit(EXIT_FAILURE);
    }

    size_t length = sizeof("util/") + strlen(*step->words) + 1;
    char path[length];
    snprintf(path, length, "util/%s", *step->words);

    if (execvp(path, step->words) == -1) {
        error("execvp for path %s failed with error: %s", path, strerror(errno));

        if (execvp(*step->words, step->words) == -1) {
            error("execvp failed with error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}


// int get_input_fd(TASK *task) {
//     int input_fd = -1;
//     if (task->input_file != NULL) {
//         int input_fd = open(task->input_file, O_RDONLY);
//         if (input_fd == -1) {
//             error("can't open input file %s: %s", task->input_file, strerror(errno));
//             return -1;
//         }
//     }
//     return input_fd;
// }

// int get_output_fd(TASK *task) {
//     int output_fd = -1;
//     if (task->output_file != NULL) {
//         // TODO: check what permissions are needed for new file
//         int output_fd = open(task->output_file, O_WRONLY | O_CREAT);
//         if (output_fd == -1) {
//             error("can't open output file %s: %s", task->output_file, strerror(errno));
//             exit(EXIT_FAILURE);
//         }
//     }
//     return output_fd;
// }