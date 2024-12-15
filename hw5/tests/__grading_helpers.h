#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <criterion/criterion.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "excludes.h"

#define MAX_BUFFER_LEN 200

extern void push_input(FILE *in);

int get_ms(struct timespec *);
void redirect_stdin(void);
void redirect_stdout(void);
void assert_values_equal(long, long, char *);
void assert_null(void *, char *);
void assert_not_null(void *, char *);
void assert_normal_exit(int, char *);
void assert_success(int, char *);
void assert_not_fail(int, char *);
void assert_failure(int, char *);
void assert_output_matches(int, char *);
void assert_outfile_matches(char *, char *, char *);
void assert_no_valgrind_errors(int status, char *, char *);
void assert_string_matches(char *, char *, char *);
void assert_alpha_string_matches(char *, char *, char *);
void assert_timer_not_exceed_ms(int, int, char *);
