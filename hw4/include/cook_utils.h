#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "cookbook.h"
#include "debug.h"

// Auxillary functions
void validargs(int argc, char *argv[], char *argrecipe[], char *argfile[], int *argmaxcooks);

// Cooking functions
void master_chef(COOKBOOK *cbp, RECIPE *main_rp);
void sous_chef(RECIPE *rp);

// Cooking auxillary functions
void execute_step(STEP *step);
// int get_input_fd(TASK *task);
// int get_output_fd(TASK *task);

// Traversal functions
RECIPE *get_recipe(COOKBOOK *cbp, char *name);
void initialise_dependency_count(COOKBOOK *cbp);
void update_dependency_count(RECIPE *rp);
void queue_leaves(COOKBOOK *cbp, RECIPE_LINK **qp);
