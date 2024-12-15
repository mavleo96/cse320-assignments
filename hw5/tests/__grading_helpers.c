#include <ctype.h>
#include "__grading_helpers.h"

int get_ms(struct timespec * ts){
    int ms = (int) (ts->tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        ts->tv_sec++;
        ms = 0;
    }
    return ms;
}

void redirect_stdin(void) {
  int fd = open("/dev/null", O_RDONLY);
  if(fd == -1) {
    cr_assert_fail("Could not open /dev/null");
  }
  if(dup2(fd, 0) == -1) {
    cr_assert_fail("Could not redirect stdin");
  }
}

void redirect_stdout(void) {
  int fd = open("/dev/null", O_RDONLY);
  if(fd == -1) {
    cr_assert_fail("Could not open /dev/null");
  }
  if(dup2(fd, 1) == -1) {
    cr_assert_fail("Could not redirect stdin");
  }
}

char *remove_nonascii(char *str)
{
    int i,j= 0;
    char *result = malloc(strlen(str));
    for (i = 0; str[i]; i++)
    {
        if(isalpha(str[i]))
            result[j++] = str[i];
    }
    result[j] = 0;
    return result;
}

void assert_values_equal(long act, long exp, char *msg) {
    cr_assert_eq(act, exp,
                 "%s, Failed to get expected value. Got: %ld | Expected: %ld.",
                 msg, act, exp);
}

void assert_null(void *buf, char *msg) {
    cr_assert_null(buf, "%s, Buffer is not null", msg);
}

void assert_not_null(void *buf, char *msg) {
    cr_assert_not_null(buf, "%s, Buffer is NULL.", msg);
}

void assert_normal_exit(int status, char *msg)
{
    cr_assert(!WIFSIGNALED(status),
              "%s, The program terminated with an unexpected signal (%d).\n",
              msg, WTERMSIG(status));
    cr_assert_eq(status, 0,
                 "%s, The program did not exit normally (status = 0x%x).\n",
                 msg, status);
}

void assert_success(int code, char *msg) {
    cr_assert_eq(code, EXIT_SUCCESS,
                 "%s, Program exited with %d instead of EXIT_SUCCESS",
                 msg, code);
}

void assert_not_fail(int code, char *msg) {
    cr_assert_neq(code, -1,
                 "%s, Program should not return -1.", msg);
}

void assert_failure(int code, char *msg) {
    cr_assert_eq(code, -1,
                 "%s, Program exited with %d instead of -1",
                 msg, code);
}

void assert_output_matches(int code, char *msg) {
    cr_assert_eq(code, EXIT_SUCCESS,
                 "%s, File output did not match reference output.\n", msg);
}

void assert_outfile_matches(char *actual_file, char *expected_file, char *msg)
{
    char cmd[500];
    sprintf(cmd,
            "diff --ignore-tab-expansion --ignore-trailing-space "
            "--ignore-space-change --ignore-blank-lines %s "
            "%s",
            actual_file, expected_file);
    int err = system(cmd);
    cr_assert_eq(err, 0,
                 "%s, The output was not what was expected (diff exited with "
                 "status %d).\n",
                 msg, WEXITSTATUS(err));
}

void assert_no_valgrind_errors(int status, char *err_file, char *msg)
{
    cr_assert_neq(WEXITSTATUS(status), 37,
                  "%s, Valgrind reported errors -- see %s",
                  msg, err_file);
}

void assert_string_matches(char *act, char *exp, char *msg) {
    cr_assert_str_eq(act, exp,
                     "%s, Output string did not match expected string. Got: %s | Expected: %s",
                     msg, act, exp);
}

void assert_alpha_string_matches(char *act, char *exp, char *msg) {
    cr_assert_str_eq(remove_nonascii(act), remove_nonascii(exp),
                     "%s, Output string did not match expected string. Got: %s | Expected: %s",
                     msg, act, exp);
}

void assert_timer_not_exceed_ms(int time, int limit, char *msg) {
    cr_assert_leq(time, limit,
                  "%s, Job took too long: %d ms | Limit: %d ms",
                  time, limit);
}
