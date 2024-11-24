#ifndef COOK_UTILS_H
#define COOK_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "cookbook.h"
#include "debug.h"

// Cooking functions
int master_chef(RECIPE *main_rp);
void sous_chef(RECIPE *rp);

// Cooking task functions
void perform_tasks(TASK *task);

// Cooking auxillary functions
void setup_file_descriptors(TASK *task, int *input_fd, int *output_fd);
void setup_io_redirection(int input_fd, int output_fd, int pipe_fd[], int count, int n);
void initialize_pipes(int *pipe_fd, int pipe_size);
void close_pipes(int pipe_fd[], int n);
int step_count(STEP *step);
RECIPE *get_recipe_from_pid(pid_t pid);

// Queue functions
typedef struct queue {
    RECIPE_LINK *head;
    RECIPE_LINK *tail;
} QUEUE;

int enqueue(RECIPE *rp, QUEUE *qp);
int dequeue(QUEUE *qp);
int queue_leaves(QUEUE *qp, RECIPE_LINK *subset);

// Dependency & state functions / macros
typedef struct state {
    int dcount;
    int cstatus;      // not cooked (0), cooked (1), cook failed (2)
    int qstatus;      // not in queue (0), in queue(1), dequeued(2)
    pid_t pid;        // cooked by pid
} STATE;

#define DEP_COUNT(rp)    (((STATE *) rp->state)->dcount)
#define CSTATUS(rp)      (((STATE *) rp->state)->cstatus)
#define QSTATUS(rp)      (((STATE *) rp->state)->qstatus)
#define PID(rp)          (((STATE *) rp->state)->pid)

void update_dependency_count(RECIPE *rp);
RECIPE_LINK *dependency_analysis(RECIPE *rp, RECIPE_LINK *rlp);

// Auxillary functions
void validargs(int argc, char *argv[], char *argrecipe[], char *argfile[]);
RECIPE *get_main_recipe(COOKBOOK *cbp, char *name);
int recipe_link_length(RECIPE_LINK *rlp);

// Debugging functions
void print_recipe_link(RECIPE_LINK *rlp);
void print_queue(QUEUE *qp);

#endif