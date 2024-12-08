#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <criterion/criterion.h>
#include "test.h"

#include "__grading_helpers.h"

Test(basecode_suite, cook_basic_test, .timeout=20)
{
    char *cmd = "killall -q -KILL cook; ulimit -t 10; ulimit -p 200; python3 tests/test_cook.py -c 2 -f rsrc/eggs_benedict.ckb < /dev/null";

    int return_code = WEXITSTATUS(system(cmd));
    assert_success(return_code);
}

Test(basecode_suite, hello_world_test, .init=mkdir_tmp, .timeout=20) {
    char *cmd = "killall -q -KILL cook; ulimit -t 10; ulimit -p 200; bin/cook -c 1 -f rsrc/hello_world.ckb > tmp/hello_world.out < /dev/null";
    char *cmp = "cmp tmp/hello_world.out tests/rsrc/hello_world.out";

    int return_code = WEXITSTATUS(system(cmd));
    assert_success(return_code);
    return_code = WEXITSTATUS(system(cmp));
    assert_output_matches(return_code);
}
