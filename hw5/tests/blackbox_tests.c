/*
 * Important: the Criterion tests in this file have to be run with -j1,
 * because they each start a separate server instance and if they are
 * run concurrently only one server will be able to bind the server port
 * and the others will fail.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include <criterion/criterion.h>
#include <pthread.h>

#include "__test_includes.h"
#include "excludes.h"

static int server_pid;

static void killall() {
    system("killall -v -s KILL pbx memcheck-amd64-linux valgrind.bin tester");
}

static void wait_for_server() {
    int ret;
    int i = 0;
    do {
        fprintf(stderr, "Waiting for server to start (i = %d)\n", i);
	ret = system("netstat -an | grep 'LISTEN[ ]*$' | grep ':"SERVER_PORT_STR"'");
	sleep(SERVER_STARTUP_SLEEP);
    } while(++i < 30 && WEXITSTATUS(ret));
}

static void wait_for_no_server() {
    int ret;
    do {
	ret = system("netstat -an | grep 'LISTEN[ ]*$' | grep ':"SERVER_PORT_STR"'");
	if(WEXITSTATUS(ret) == 0) {
	    fprintf(stderr, "Waiting for server port to clear...\n");
	    //system("killall -s KILL pbx > /dev/null");
	    killall();
	    sleep(1);
	} else {
	    break;
	}
    } while(1);
}

static void init() {
#ifdef NO_SERVER
  cr_assert_fail("The server module was not implemented\n");
#endif
    server_pid = 0;
    wait_for_no_server();
    fprintf(stderr, "***Starting server...");
    if((server_pid = fork()) == 0) {
        struct rlimit limit = { .rlim_cur=30, .rlim_max=30 };
	setrlimit(RLIMIT_CPU, &limit);
	execlp("bin/pbx", "pbx", "-p", SERVER_PORT_STR, NULL);
	fprintf(stderr, "Failed to exec server\n");
	abort();
    }
    fprintf(stderr, "pid = %d\n", server_pid);
    // Wait for server to start before returning
    wait_for_server();
}

static void fini(int chk) {
    int ret;
    cr_assert(server_pid != 0, "No server was started!\n");
    fprintf(stderr, "***Sending SIGHUP to server pid %d\n", server_pid);
    kill(server_pid, SIGHUP);
    sleep(SERVER_SHUTDOWN_SLEEP);
    kill(server_pid, SIGKILL);
    wait(&ret);
    fprintf(stderr, "***Server wait() returned = 0x%x\n", ret);
    if(chk) {
      if(WIFSIGNALED(ret))
	  cr_assert_fail("***Server terminated ungracefully with signal %d\n", WTERMSIG(ret));
      //cr_assert_eq(WEXITSTATUS(ret), 0, "Server exit status was not 0");
    }
}

/*
 * Versions of initialization and finalization that use valgrind.
 */
static void init_valgrind() {
#ifdef NO_SERVER
  cr_assert_fail("The server module was not implemented\n");
#endif
    server_pid = 0;
    wait_for_no_server();
    fprintf(stderr, "***Starting server...");
    if((server_pid = fork()) == 0) {
        struct rlimit limit = { .rlim_cur=30, .rlim_max=30 };
	setrlimit(RLIMIT_CPU, &limit);
        execlp("timeout", "timeout", "-v", "--preserve-status", "--signal=TERM", "--kill-after=5", "60",
	       "valgrind", "--leak-check=full", "--errors-for-leak-kinds=definite,indirect",
	       "--track-fds=yes", "--error-exitcode=37", "--log-file=valgrind.out",
	       "bin/pbx", "-p", SERVER_PORT_STR, NULL);
	fprintf(stderr, "Failed to exec server\n");
	abort();
    }
    fprintf(stderr, "pid = %d\n", server_pid);
    // Wait for server to start before returning
    wait_for_server();
}
static void fini_valgrind(int chk) {
    int ret;
    cr_assert(server_pid != 0, "No server was started!\n");
    fprintf(stderr, "***Sending SIGHUP to server pid %d\n", server_pid);
    kill(server_pid, SIGHUP);
    sleep(SERVER_SHUTDOWN_SLEEP);
    kill(server_pid, SIGKILL);
    wait(&ret);
    fprintf(stderr, "***Server wait() returned = 0x%x\n", ret);
    if(WIFSIGNALED(ret) || WEXITSTATUS(ret) == 37)
	system("cat valgrind.out");
    cr_assert_neq(WEXITSTATUS(ret), 37, "Valgrind reported errors");
    if(chk) {
      if(WIFSIGNALED(ret))
	    cr_assert_fail("***Server terminated ungracefully with signal %d\n", WTERMSIG(ret));
      //cr_assert_eq(WEXITSTATUS(ret), 0, "Server exit status was not 0");
    }
}


#define SUITE blackbox_suite

#define TEST_NAME connect_disconnect_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30)
{
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

#define TEST_NAME connect_disconnect2_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

#define TEST_NAME pickup_hangup_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     TEN_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

#define TEST_NAME dial_answer_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   0,  TU_DIAL_CMD,        1,           TU_RING_BACK,   TEN_MSEC },
    {   1,  TU_PICKUP_CMD,     -1,           TU_CONNECTED,   TEN_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

#define TEST_NAME dial_disconnect_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   0,  TU_DIAL_CMD,        1,           TU_RING_BACK,   TEN_MSEC },
    {   1,  TU_PICKUP_CMD,     -1,           TU_CONNECTED,   TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

#define TEST_NAME connect_disconnect_many_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   2,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   3,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   4,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   5,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   6,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   7,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   8,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   9,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   2,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   3,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   4,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   5,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   6,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   7,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   8,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   9,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(1);
}
#undef TEST_NAME

/*
 * Test what happens if a connection is left open at the end when the server
 * is sent SIGHUP.
 */
#define TEST_NAME open_connection_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT

    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    // Leave extension 1 connected to see if it is freed gracefully
    //{   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(1);
}
#undef TEST_NAME

/*
 * Run the same tests again, this time with valgrind.
 */

#define TEST_NAME connect_disconnect_valgrind_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     QTR_SEC  }, // QTR_SEC
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init_valgrind, .fini = killall, .timeout = 30)
{
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini_valgrind(0);
}
#undef TEST_NAME

#define TEST_NAME connect_disconnect2_valgrind_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     QTR_SEC  },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini_valgrind(0);
}
#undef TEST_NAME

#define TEST_NAME connect_disconnect_many_valgrind_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     QTR_SEC  },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   2,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   3,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   4,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   5,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   6,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   7,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   8,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   9,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   2,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   3,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   4,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   5,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   6,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   7,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   8,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   9,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini_valgrind(1);
}
#undef TEST_NAME

/*
 * Test what happens if a connection is left open at the end when the server
 * is sent SIGHUP.
 */
#define TEST_NAME open_connection_valgrind_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT

    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     QTR_SEC  },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    // Leave extension 1 connected to see if it is freed gracefully
    //{   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init_valgrind, .fini = killall, .timeout = 30) {
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini_valgrind(1);
}
#undef TEST_NAME

#define TEST_NAME pickup_hangup2_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   1,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     TEN_MSEC },
    {   1,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     TEN_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

#define TEST_NAME dial_self_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   0,  TU_DIAL_CMD,        0,           TU_BUSY_SIGNAL, TEN_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     TEN_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

#define TEST_NAME dial_bad_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   0,  TU_DIAL_CMD,        1,           TU_ERROR,       TEN_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     TEN_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

/*
 * Tests that primarily target the pbx module, and so are only used
 * in test configuration C.
 */
#define TEST_NAME dial_good_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   TEN_MSEC },
    {   0,  TU_DIAL_CMD,        1,           TU_RING_BACK,   TEN_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(0);
}
#undef TEST_NAME

/*
 * This is a more extensive test, used only in configuration C.
 * It exercises various situations that could potentially trigger bugs.
 */
#define TEST_NAME exercise_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT

    // Do some connects and disconnects
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   2,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   3,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   4,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   5,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   6,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   7,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   8,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   9,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    
    // Basic pickup and hangup
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Dial self
    {   1,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   1,  TU_DIAL_CMD,        1,           TU_BUSY_SIGNAL, FTY_MSEC },
    {   1,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Dial nonexistent extension
    {   2,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   2,  TU_DIAL_CMD,       19,           TU_ERROR,       FTY_MSEC },
    {   2,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Dial an existing extension
    {   3,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   3,  TU_DIAL_CMD,        0,           TU_RING_BACK,   FTY_MSEC },
    {   3,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Complete a call and hang up
    {   4,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   4,  TU_DIAL_CMD,        5,           TU_RING_BACK,   FTY_MSEC },
    {   5,  TU_PICKUP_CMD,     -1,           TU_CONNECTED,   FTY_MSEC },
    {   4,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Complete a call and peer hangs up
    {   6,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   6,  TU_DIAL_CMD,        7,           TU_RING_BACK,   FTY_MSEC },
    {   7,  TU_PICKUP_CMD,     -1,           TU_CONNECTED,   FTY_MSEC },
    {   7,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   2,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   3,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   4,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   5,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   6,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   7,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   8,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    // Leave extension 9 connected to see if it is freed gracefully
    //{   9,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini(1);
}
#undef TEST_NAME

/*
 * The same test, only this time with valgrind.
 */

#define TEST_NAME exercise_valgrind_test
static TEST_STEP SCRIPT(TEST_NAME)[] = {
    // ID,  COMMAND,          ID_TO_DIAL,    RESPONSE,       TIMEOUT

    // Do some connects and disconnects
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     QTR_SEC  },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    {   0,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   1,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   2,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   3,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   4,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   5,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   6,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   7,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   8,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    {   9,  TU_CONNECT_CMD,    -1,           TU_ON_HOOK,     HND_MSEC },
    
    // Basic pickup and hangup
    {   0,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   0,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Dial self
    {   1,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   1,  TU_DIAL_CMD,        1,           TU_BUSY_SIGNAL, FTY_MSEC },
    {   1,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Dial nonexistent extension
    {   2,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   2,  TU_DIAL_CMD,       19,           TU_ERROR,       FTY_MSEC },
    {   2,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Dial an existing extension
    {   3,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   3,  TU_DIAL_CMD,        0,           TU_RING_BACK,   FTY_MSEC },
    {   3,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Complete a call and hang up
    {   4,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   4,  TU_DIAL_CMD,        5,           TU_RING_BACK,   FTY_MSEC },
    {   5,  TU_PICKUP_CMD,     -1,           TU_CONNECTED,   FTY_MSEC },
    {   4,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    // Complete a call and peer hangs up
    {   6,  TU_PICKUP_CMD,     -1,           TU_DIAL_TONE,   FTY_MSEC },
    {   6,  TU_DIAL_CMD,        7,           TU_RING_BACK,   FTY_MSEC },
    {   7,  TU_PICKUP_CMD,     -1,           TU_CONNECTED,   FTY_MSEC },
    {   7,  TU_HANGUP_CMD,     -1,           TU_ON_HOOK,     FTY_MSEC },

    {   0,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   1,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   2,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   3,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   4,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   5,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   6,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   7,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },
    {   8,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    // Leave extension 9 connected to see if it is freed gracefully
    //{   9,  TU_DISCONNECT_CMD, -1,           -1,             TEN_MSEC },

    {   -1, -1,                -1,           -1,             ZERO_SEC }
};

Test(SUITE, TEST_NAME, .init = init, .fini = killall, .timeout = 30) {
#ifdef NO_PBX
  cr_assert_fail("The PBX module was not implemented\n");
#endif
#ifdef NO_TU
  cr_assert_fail("The TU module was not implemented\n");
#endif
    char *name = QUOTE(SUITE)"/"QUOTE(TEST_NAME);
    int ret = run_test_script(name, SCRIPT(TEST_NAME), SERVER_PORT);
    cr_assert_eq(ret, 0, "expected %d, was %d\n", 0, ret);
    fini_valgrind(1);
}
#undef TEST_NAME
