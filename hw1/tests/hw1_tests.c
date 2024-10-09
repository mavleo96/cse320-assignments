#include <fcntl.h>
#include <criterion/criterion.h>
#include <criterion/logging.h>
#include "global.h"
#include "test_common.h"

#define PROGRAM_PATH "bin/transplant"

#define HELP_MODE (1)
#define SERIALIZE_MODE (2)
#define DESERIALIZE_MODE (4)
#define CLOBBER_FLAG (8)

#define ASSERT_GLOBAL_OPTIONS(exp_global_opt) do { \
    cr_assert_eq(global_options, exp_global_opt, \
		 "Wrong value for global_options: expected 0x%x, was 0x%x\n", \
		 exp_global_opt, global_options); \
  } while(0)

#define ARGC(argv) (sizeof(argv)/sizeof((argv)[0]) - 1)

#define INIT_PATH_BUF(path) do { \
    strcpy(path_buf, (path)); \
    path_length = strlen(path_buf); \
  } while(0)

// Deserialize result using reference code, so that we can compare the result
// to the original and avoid discrepancies resulting from order of directory entries.

#define REF_PROG REF_BIN_DIR"/transplant"
#define REF_DESERIALIZE REF_BIN_DIR"/deserialize"
#define REF_DESERIALIZE_DIRECTORY REF_BIN_DIR"/deserialize_directory"

#define DESERIALIZE_RESULT(ref_prog) do { \
    FILE *f; size_t s = 0; char *cmd = NULL; \
    NEWSTREAM(f, s, cmd); \
    fprintf(f, "%s -d -p %s < %s", ref_prog, alt_outfile, test_outfile); \
    fclose(f); \
    mkdir(alt_outfile, 0777); \
    int status = system(cmd); \
    assert_expected_status(EXIT_SUCCESS, status); \
  } while(0)

/**
 * ====================================
 * PART I
 * unit tests for valid_args()
 * 12 test cases: 7 positive, 5 negative
 * ====================================
 */

#define TEST_SUITE validargs_suite

/**
 * validargs_1
 * @brief -h -s -c  (positive test)
 */

#define TEST_NAME validargs_1
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-h", "-s", "-c", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(0);
    ASSERT_GLOBAL_OPTIONS(HELP_MODE);
}
#undef TEST_NAME

/**
 * validargs_2
 * @brief -s  (positive test)
 */

#define TEST_NAME validargs_2
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-s", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(0);
    ASSERT_GLOBAL_OPTIONS(SERIALIZE_MODE);
}
#undef TEST_NAME

/**
 * validargs_3
 * @brief -s -p ./testdir/  (positive test)
 */

#define TEST_NAME validargs_3
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-s", "-p", "./testdir/", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(0);
    ASSERT_GLOBAL_OPTIONS(SERIALIZE_MODE);
}
#undef TEST_NAME

/**
 * validargs_4
 * @brief -d  (positive test)
 */

#define TEST_NAME validargs_4
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-d", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(0);
    ASSERT_GLOBAL_OPTIONS(DESERIALIZE_MODE);
}
#undef TEST_NAME

/**
 * validargs_5
 * @brief -d -p ./out/  (positive test)
 */

#define TEST_NAME validargs_5
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-d", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(0);
    ASSERT_GLOBAL_OPTIONS(DESERIALIZE_MODE);
}
#undef TEST_NAME

/**
 * validargs_6
 * @brief -h -c -p ./out/  (positive test)
 */

#define TEST_NAME validargs_6
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-d", "-c", "-p", "./out/", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(0);
    ASSERT_GLOBAL_OPTIONS(DESERIALIZE_MODE | CLOBBER_FLAG);
}
#undef TEST_NAME

/**
 * validargs_7
 * @brief -s -c  (negative test)
 */

#define TEST_NAME validargs_7
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-s", "-c", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(-1);
    //ASSERT_GLOBAL_OPTIONS(0x0);
}
#undef TEST_NAME

/**
 * validargs_13
 * @brief -s -c  (check for global_options = 0x0)
 */

#define TEST_NAME validargs_13
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-s", "-c", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_GLOBAL_OPTIONS(0x0);
}
#undef TEST_NAME

/**
 * validargs_8
 * @brief -d -h  (negative test)
 */

#define TEST_NAME validargs_8
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-d", "-h", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(-1);
    //ASSERT_GLOBAL_OPTIONS(0x0);
}
#undef TEST_NAME

/**
 * validargs_9
 * @brief -d -p  (negative test)
 */

#define TEST_NAME validargs_9
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-d", "-p", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(-1);
    //ASSERT_GLOBAL_OPTIONS(0x0);
}
#undef TEST_NAME

/**
 * validargs_10
 * @brief -s -p  (negative test)
 */

#define TEST_NAME validargs_10
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-s", "-p", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(-1);
    //ASSERT_GLOBAL_OPTIONS(0x0);
}
#undef TEST_NAME

/**
 * validargs_11
 * @brief -s -p -c  (positive test)
 */

#define TEST_NAME validargs_11
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-s", "-p", "-c", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(0);
    ASSERT_GLOBAL_OPTIONS(SERIALIZE_MODE);
}
#undef TEST_NAME

/**
 * validargs_12
 * @brief -d -c -p  (negative test)
 */

#define TEST_NAME validargs_12
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *argv[] = {QUOTE(PROGRAM_NAME), "-d", "-c", "-p", NULL};
    int argc = ARGC(argv);
    int ret = validargs(argc, argv);
    ASSERT_RETURN(-1);
    //ASSERT_GLOBAL_OPTIONS(0x0);
}
#undef TEST_NAME

#undef TEST_SUITE

/**
 * ====================================
 * PART II
 * unit tests for path buffer functions
 * ====================================
 */

static void assert_path_eq(char *path, char *exp_path) {
    cr_assert(!strcmp(path, exp_path),
	      "Path buffer contents (%s) did not match expected (%s)\n",
	      path, exp_path);
}

static void assert_path_len_eq(size_t len, size_t exp_len) {
    cr_assert_eq(len, exp_len,
		 "The value of path_length (%ld), did not match expected (%ld)\n",
		 len, exp_len);
}

#define TEST_SUITE pathbuf_suite

/**
 * init_path_buf
 * @brief -h -s -c  (positive test)
 */

#define TEST_NAME path_init_normal
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char *path = "/foo/bar/xyzzy";
    size_t len = strlen(path);
    int exp_ret = 0;
    int ret = path_init(path);
    ASSERT_RETURN(exp_ret);
    assert_path_eq(path_buf, path);
    assert_path_len_eq(path_length, len);
}
#undef TEST_NAME

#define TEST_NAME path_init_too_long
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char path[PATH_MAX];
    memset(path, 'A', PATH_MAX);
    path[PATH_MAX] = '\0';
    int exp_ret = -1;
    int ret = path_init(path);
    ASSERT_RETURN(exp_ret);
}
#undef TEST_NAME

#define TEST_NAME path_push_empty_base
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char path[] = "/foobar";
    size_t len = strlen(path);
    int exp_ret = 0;
    int ret = path_push(&path[1]);
    ASSERT_RETURN(exp_ret);
    assert_path_eq(path_buf, path);
    assert_path_len_eq(path_length, len);
}
#undef TEST_NAME

#define TEST_NAME path_push_nonempty_base
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    char path[] = "/foo/bar/xyzzy";
    size_t len = strlen(path);
    int exp_ret = 0;
    char *sep = &path[8];
    *sep = '\0';
    path_init(path);
    *sep = '/';
    int ret = path_push(sep+1);
    ASSERT_RETURN(exp_ret);
    assert_path_eq(path_buf, path);
    assert_path_len_eq(path_length, len);
}
#undef TEST_NAME

#define TEST_NAME path_push_too_long
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    int exp_ret = -1;
    char path[] = "/foo/bar/xyzzy";
    char path1[PATH_MAX];
    memset(path1, 'A', PATH_MAX);
    path1[PATH_MAX] = '\0';
    path_init(path);
    int ret = path_push(path1);
    ASSERT_RETURN(exp_ret);
}
#undef TEST_NAME

#define TEST_NAME path_pop_empty
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    int exp_ret = -1;
    int ret = path_pop();
    ASSERT_RETURN(exp_ret);
}
#undef TEST_NAME

#define TEST_NAME path_pop_normal
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    int exp_ret = 0;
    char path[] = "/foo/bar/xyzzy";
    char comp[] = "xyzzy";
    size_t len = strlen(path) - strlen(comp) - 1;
    path_init(path);
    path[8] = '\0';
    int ret = path_pop();
    ASSERT_RETURN(exp_ret);
    assert_path_eq(path_buf, path);
    assert_path_len_eq(path_length, len);
}
#undef TEST_NAME

#undef TEST_SUITE


/**
 * ===================================================================
 * PART III
 * unit tests for serialization
 * 5 test cases
 * 3 on serialize_file(), 2 on serialize_directory(), 2 on serialize()
 * ===================================================================
 */

#define TEST_SUITE serialize_suite

/**
 * serialize_file_text
 * @brief serialize a text file
 */

#define TEST_NAME serialize_file_text
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(ref_infile);
    REDIRECT_STDOUT;
    int ret = serialize_file(1, 10);
    fflush(stdout);
    ASSERT_RETURN(0);
    assert_files_match(ref_outfile, test_outfile, NULL);
}
#undef TEST_NAME

/**
 * serialize_file_binary
 * @brief serialize a binary file
 */

#define TEST_NAME serialize_file_binary
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(ref_infile);
    REDIRECT_STDOUT;
    int ret = serialize_file(1, 299);
    fflush(stdout);
    ASSERT_RETURN(0);
    assert_files_match(ref_outfile, test_outfile, NULL);
}
#undef TEST_NAME

/**
 * serialize_file_empty
 * @brief serialize an empty file
 */

#define TEST_NAME serialize_file_empty
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(ref_infile);
    REDIRECT_STDOUT;
    int ret = serialize_file(1, 0);
    fflush(stdout);
    ASSERT_RETURN(0);
    assert_files_match(ref_outfile, test_outfile, NULL);
}
#undef TEST_NAME

/**
 * serialize_dir_no_subdir
 * @brief serialize directory without subdir
 */

#define TEST_NAME serialize_dir_no_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(ref_infile);
    REDIRECT_STDOUT;
    int ret = serialize_directory(1);
    fflush(stdout);
    ASSERT_RETURN(0);
    DESERIALIZE_RESULT(REF_DESERIALIZE_DIRECTORY);
    assert_dirs_match(ref_infile, alt_outfile);
}
#undef TEST_NAME

/**
 * serialize_dir_with_subdir
 * @brief serialize dir with subdir
 */

#define TEST_NAME serialize_dir_with_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(ref_infile);
    REDIRECT_STDOUT;
    int ret = serialize_directory(1);
    fflush(stdout);
    ASSERT_RETURN(0);
    DESERIALIZE_RESULT(REF_DESERIALIZE_DIRECTORY);
    assert_dirs_match(ref_infile, alt_outfile);
}
#undef TEST_NAME

/**
 * serialize_no_subdir
 * @brief serialize without subdir
 */

#define TEST_NAME serialize_no_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(ref_infile);
    REDIRECT_STDOUT;
    int ret = serialize();
    fflush(stdout);
    ASSERT_RETURN(0);
    DESERIALIZE_RESULT(REF_PROG);
    assert_dirs_match(ref_infile, alt_outfile);
}
#undef TEST_NAME

/**
 * serialize_with_subdir
 * @brief serialize with subdir
 */

#define TEST_NAME serialize_with_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(ref_infile);
    REDIRECT_STDOUT;
    int ret = serialize();
    fflush(stdout);
    ASSERT_RETURN(0);
    DESERIALIZE_RESULT(REF_PROG);
    assert_dirs_match(ref_infile, alt_outfile);
}
#undef TEST_NAME

#undef TEST_SUITE





/**
 * =========================================================================
 * PART IV
 * unit tests for deserialization
 * 8 test cases
 * 4 on deserialize_file(), 2 on deserialize_directory(), 2 on deserialize()
 * =========================================================================
 */

#define TEST_SUITE deserialize_suite

/**
 * deserialize_file_text_with_c
 * @brief deserialize a text file, -c set.
 */

#define TEST_NAME deserialize_file_text_with_c
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE | CLOBBER_FLAG;
    REDIRECT_STDIN;
    int ret = deserialize_file(1);
    ASSERT_RETURN(0);
    assert_files_match(ref_outfile, test_outfile, NULL);
}
#undef TEST_NAME


/**
 * deserialize_file_binary_with_c
 * @brief deserialize a binary file, -c set.
 */

#define TEST_NAME deserialize_file_binary_with_c
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE | CLOBBER_FLAG;
    REDIRECT_STDIN;
    int ret = deserialize_file(1);
    ASSERT_RETURN(0);
    assert_files_match(ref_outfile, test_outfile, NULL);
}
#undef TEST_NAME

/**
 * deserialize_file_empty_with_c
 * @brief deserialize an empty file, -c set.
 */

#define TEST_NAME deserialize_file_empty_with_c
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE | CLOBBER_FLAG;
    REDIRECT_STDIN;
    int ret = deserialize_file(1);
    ASSERT_RETURN(0);
    assert_files_match(ref_outfile, test_outfile, NULL);
}
#undef TEST_NAME

/**
 * deserialize_file_text_no_c
 * @brief deserialize a .txt file, -c not set, file already exists.
 */

#define TEST_NAME deserialize_file_text_no_c
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE;

    // create the file and set -c bit as 0
    int ret = creat(test_outfile, 0777);
    if(ret == -1)
	cr_assert_fail("Unable to create file: '%s'", test_outfile);

    REDIRECT_STDIN;
    ret = deserialize_file(1);
    ASSERT_RETURN(-1);
}
#undef TEST_NAME

/**
 * deserialize_directory_no_subdir
 * @brief deserialize dir without subdir
 */

#define TEST_NAME deserialize_directory_no_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE | CLOBBER_FLAG;
    CREATE_DIRECTORY(test_outfile);
    REDIRECT_STDIN;
    int ret = deserialize_directory(1);
    ASSERT_RETURN(0);
    assert_dirs_match(ref_outfile, test_outfile);
}
#undef TEST_NAME

/**
 * deserialize_directory_with_subdir
 * @brief deserialize dir with subdir
 */

#define TEST_NAME deserialize_directory_with_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE | CLOBBER_FLAG;
    CREATE_DIRECTORY(test_outfile);
    REDIRECT_STDIN;
    int ret = deserialize_directory(1);
    ASSERT_RETURN(0);
    assert_dirs_match(ref_outfile, test_outfile);
}
#undef TEST_NAME

/**
 * deserialize_no_subdir
 * @brief deserialize without subdir
 */

#define TEST_NAME deserialize_no_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE | CLOBBER_FLAG;
    CREATE_DIRECTORY(test_outfile);
    REDIRECT_STDIN;
    int ret = deserialize();
    ASSERT_RETURN(0);
    assert_dirs_match(ref_outfile, test_outfile);
}
#undef TEST_NAME

/**
 * deserialize_with_subdir
 * @brief deserialize with subdir
 */

#define TEST_NAME deserialize_with_subdir
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    INIT_PATH_BUF(test_outfile);
    global_options = DESERIALIZE_MODE | CLOBBER_FLAG;
    CREATE_DIRECTORY(test_outfile);
    REDIRECT_STDIN;
    int ret = deserialize();
    ASSERT_RETURN(0);
    assert_dirs_match(ref_outfile, test_outfile);
}
#undef TEST_NAME

#undef TEST_SUITE





/**
 * ==============================================================
 * PART V
 * test of the whole program as a black box
 * 11 test cases
 * ==============================================================
 */

#define TEST_SUITE blackbox_suite

/**
 * blackbox_1
 * @brief PROGRAM_PATH -h -s -d -c -p test_outfile  (positive test)
 */

#define TEST_NAME blackbox_1
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-h -s -d -c -p %s", test_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);
    assert_files_match(ref_errfile, test_errfile, NULL);
}
#undef TEST_NAME

/**
 * blackbox_2
 * @brief PROGRAM_PATH -p test_outfile  (negative test)
 */

#define TEST_NAME blackbox_2
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-p %s", test_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

/**
 * blackbox_3
 * @brief PROGRAM_PATH -s -p ref_infile (dir with subdir, positive test)
 */

#define TEST_NAME blackbox_3
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-s -p %s", ref_infile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);
    DESERIALIZE_RESULT(REF_PROG);
    assert_dirs_match(ref_infile, alt_outfile);
}
#undef TEST_NAME

/**
 * blackbox_4
 * @brief deserialize data of dir with subdir. -c set.
 *        PROGRAM_PATH -d -p alt_outfile -c  (positive test)
 */

#define TEST_NAME blackbox_4
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s -c", alt_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_SUCCESS, status);
    assert_dirs_match(ref_outfile, alt_outfile);
}
#undef TEST_NAME

/**
 * blackbox_5
 * @brief deserialize data of dir with subdir. -c not set.
 *        dest dir exists and has files with same name.
          PROGRAM_PATH -d -p alt_outfile -c  (negative test)
 */

#define TEST_NAME blackbox_5
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s", alt_outfile); fclose(f);

    // Create a conflicting file in directory alt_outfile.
    int ret = mkdir(alt_outfile, 0777);
    if(ret == -1)
      cr_assert_fail("Unable to create directory: '%s'", alt_outfile);
    char *cf = NULL; s = 0; NEWSTREAM(f, s, cf);
    fprintf(f, "%s/1.txt", alt_outfile); fclose(f);
    ret = creat(cf, 0777);
    if(ret == -1)
	cr_assert_fail("Unable to create file: '%s'", cf);

    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

/**
 * blackbox_6
 * @brief deserialize truncated data, 
 *        should detect this fault and return -1.
 *        PROGRAM_PATH -d -p test_outfile  (negative test)
 */

#define TEST_NAME blackbox_6
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s", test_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

/**
 * blackbox_7
 * @brief deserialize short FILE_DATA record
 *		  (i.e. wrong number of bytes until next record), 
 *		  should detect this fault and return -1.
 *        PROGRAM_PATH -d -p alt_outfile  (negative test)
 */

#define TEST_NAME blackbox_7
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s", alt_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

/**
 * blackbox_8
 * @brief deserialize empty input stream
 *		  should detect this fault and return -1. 
 *        PROGRAM_PATH -d -p ./etc/student_output/blackbox_test_8  (negative test)
 */

#define TEST_NAME blackbox_8
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s", alt_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

/**
 * blackbox_9
 * @brief deserialize bogus record type
 *		  should detect this fault and return -1. 
 *        PROGRAM_PATH -d -p ./etc/student_output/blackbox_test_9  (negative test)
 */

#define TEST_NAME blackbox_9
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s", alt_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

/**
 * blackbox_10
 * @brief deserialize complete garbage input 
 *        (e.g. stream OK to middle of a record, then random data follows for awhile)
 *		  should detect this fault and return -1. 
 *        PROGRAM_PATH -d -p ./etc/student_output/blackbox_test_10  (negative test)
 */

#define TEST_NAME blackbox_10
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s", alt_outfile); fclose(f);
    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

/**
 * blackbox_11
 * @brief deserialize data of dir with subdir. -c set.
 *        write permission of filein dest dir is locked. 
          PROGRAM_PATH -d -p ./etc/student_output/blackbox_test_11 -c  (negative test)
 */

#define TEST_NAME blackbox_11
Test(TEST_SUITE, TEST_NAME, .timeout=TEST_TIMEOUT)
{
    setup_test(QUOTE(TEST_NAME));
    FILE *f; size_t s = 0; char *args = NULL; NEWSTREAM(f, s, args);
    fprintf(f, "-d -p %s", alt_outfile); fclose(f);

    // Create a write-protected alt_outfile directory.
    int ret = mkdir(alt_outfile, 0000);
    if(ret == -1)
      cr_assert_fail("Unable to create directory: '%s'", alt_outfile);

    int status = run_using_system(PROGRAM_PATH, "", "", args, STANDARD_LIMITS);
    assert_expected_status(EXIT_FAILURE, status);
}
#undef TEST_NAME

#undef TEST_SUITE

// TODO: Add more test cases