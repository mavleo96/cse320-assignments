#include "globals.h"

int MAX_COOKS = 1;
int ACTIVE_COOKS = 0;

QUEUE queue = {NULL, NULL};
RECIPE_LINK *subset_rlp = NULL;