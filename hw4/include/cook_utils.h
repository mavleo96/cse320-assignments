#include <stdlib.h>
#include <string.h>

#include "cookbook.h"
#include "debug.h"

/*
 * Function to validate args and set parameters; return -1 on error
 * TODO: more edge case error handling needed
 */
int validargs(int argc, char *argv[], char *argrecipe[], char *argfile[], int *argmaxcooks);