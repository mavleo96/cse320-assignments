#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <criterion/criterion.h>

#include "test.h"


Test(stress_suite, test_flaky_behaviour) {
    int failure_count = 0;
    int max_runs = 50;
    char *cmd = "ulimit -t 10; bin/cook -c 8 -f tests/rsrc/instant_eggs_benedict.ckb";

    for (int i = 0; i < max_runs; i++) {
        int return_code = WEXITSTATUS(system(cmd));
        if (return_code != EXIT_SUCCESS) {
            failure_count++;
        }
    }

    cr_assert_eq(failure_count, 0, "The program failed %d times.", failure_count);
}
