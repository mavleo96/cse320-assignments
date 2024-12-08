#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <criterion/criterion.h>

#include "__grading_helpers.h"

void mkdir_tmp(void) {
    int err = mkdir("tmp", 0777);
    if(err == -1 && errno != EEXIST) {
	perror("Could not make tmp directory");
	cr_assert_fail("no tmp directory");
    }
}

void assert_success(int code) {
    cr_assert_eq(code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 code);
}

void assert_failure(int code) {
    cr_assert_eq(code, EXIT_FAILURE,
                 "Program exited with %d instead of EXIT_FAILURE",
		 code);
}

void assert_output_matches(int code) {
    cr_assert_eq(code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}
