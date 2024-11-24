#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <criterion/criterion.h>

#include "test.h"

Test(io_suite, input_pass_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/test.ckb in_pass > tmp/input_only_test.out";
    char *cmp = "cmp tmp/input_only_test.out tests/rsrc/io_test2.out";

    int return_code = WEXITSTATUS(system(cmd));
    assert_success(return_code);
    return_code = WEXITSTATUS(system(cmp));
    assert_output_matches(return_code);
}

Test(io_suite, output_pass_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/test.ckb out_pass";
    char *cmp = "cmp tmp/fortytwo.txt tests/rsrc/io_test1.out";

    int return_code = WEXITSTATUS(system(cmd));
    assert_success(return_code);
    return_code = WEXITSTATUS(system(cmp));
    assert_output_matches(return_code);
}

Test(io_suite, input_fail_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/test.ckb in_fail";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}

Test(io_suite, output_fail_test, .timeout=20)
{
    char *cmd = "ulimit -t 10; bin/cook -c 1 -f tests/rsrc/test.ckb out_fail";

    int return_code = WEXITSTATUS(system(cmd));
    assert_failure(return_code);
}
