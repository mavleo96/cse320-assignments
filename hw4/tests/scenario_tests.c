#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <criterion/criterion.h>

#include "test.h"

Test(scenario_suite, instant_single_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/instant_eggs_benedict.ckb";

    int return_code = WEXITSTATUS(system(cmd));
    assert_success(return_code);
}

Test(scenario_suite, instant_multi_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 8 -f tests/rsrc/instant_eggs_benedict.ckb";

    int return_code = WEXITSTATUS(system(cmd));
    assert_success(return_code);
}

Test(scenario_suite, circular_single_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/circular_eggs_benedict.ckb";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}

Test(scenario_suite, circular_multi_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 8 -f tests/rsrc/circular_eggs_benedict.ckb";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}

Test(scenario_suite, abort_single_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/test.ckb test_abort_fail";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}

Test(scenario_suite, abort_multi_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 8 -f tests/rsrc/test.ckb test_abort_fail";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}

Test(scenario_suite, abort_input_single_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/test.ckb test_input_abort_fail";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}

Test(scenario_suite, abort_input_multi_cook_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 8 -f tests/rsrc/test.ckb test_input_abort_fail";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}