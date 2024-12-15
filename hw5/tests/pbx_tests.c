#include <pthread.h>
#include <sys/time.h>
#include "tu.h"
#include "pbx.h"
#include "__grading_helpers.h"

/* The maximum number of "file descriptors" we will use. */
#define NFD (1024)

/* The maximum number of clients we will register. */
#define NCLIENT (128)

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (5)

/* Number of iterations we use in several tests. */
#define NITER (200)

#define CLIENT_UP(pbx, fds, tus, ext) \
   do { \
     int fd = fileno(tmpfile()); \
     if(fd != -1) { \
       fds[ext] = fd; \
       tus[ext] = tu_init(fd); \
       pbx_register(pbx, tus[ext], fd); \
     } else { \
       fds[ext] = -1; \
     } \
     ext++; \
   } while(0)

#define CLIENT_DOWN(pbx, fds, tus, ext) \
   do { \
     ext--; \
     int fd = fds[ext]; \
     TU *tu = tus[ext]; \
     if(fd != -1) { \
       pbx_unregister(pbx, tu); \
       tu_unref(tu, "for TU being unregistered"); \
     } \
   } while(0)

/*
 * Randomly register and unregister clients, then unregister
 * all remaining registered at the end.
 */
void random_reg_unreg(PBX *pbx, int n) {
    int ext = 0;
    unsigned int seed = 1; //pthread_self();
    // Array mapping client IDs to file descriptors.
    int fds[NCLIENT];
    TU *tus[NCLIENT];
    for(int i = 0; i < NCLIENT; i++) {
        fds[i] = -1;
        tus[i] = NULL;
    }
    for(int i = 0; i < n; i++) {
        if(ext == 0) {
            // No clients: only way to go is up!
            CLIENT_UP(pbx, fds, tus, ext);
        } else if(ext == NCLIENT) {
            // Clients maxxed out: only way to go is down!
            CLIENT_DOWN(pbx, fds, tus, ext);
        } else {
            if(rand_r(&seed) % 2) {
                CLIENT_UP(pbx, fds, tus, ext);
            } else {
                CLIENT_DOWN(pbx, fds, tus, ext);
            }
        }
    }
    // Unregister any remaining file descriptors at the end.
    while(ext > 0)
        CLIENT_DOWN(pbx, fds, tus, ext);
}

/*
 * Thread that runs random register/unregister, then sets a flag.
 * The thread delays at the start of the test, to make it more likely
 * that other threads started at about the same time are active.
 */
struct reg_unreg_args {
    PBX *pbx;
    volatile int *done_flag;
    int iters;
    int start_delay;
};

void *random_reg_unreg_thread(void *arg) {
    struct reg_unreg_args *ap = arg;
    if(ap->start_delay)
        sleep(ap->start_delay);
    random_reg_unreg(ap->pbx, ap->iters);
    if(ap->done_flag != NULL)
        *ap->done_flag = 1;
    return NULL;
}

/*
 * Thread that calls shutdown on pbx, then checks a set of flags.
 * If all flags are nonzero, the test succeeds, otherwise return from shutdown
 * was premature and the test fails.
 */
struct shutdown_args {
    PBX *pbx;
    volatile int *flags;
    long elapsed_time;
    int nflags;
    int ret;
};

void *shutdown_thread(void *arg) {
    struct shutdown_args *ap = arg;
    struct timeval start, end;
    gettimeofday(&start, NULL);
    pbx_shutdown(ap->pbx);
    ap->ret = 1;
    for(int i = 0; i < ap->nflags; i++) {
        if(ap->flags[i] == 0) {
            ap->ret = 0;
        }
    }
    gettimeofday(&end, NULL);
    ap->elapsed_time = end.tv_sec - start.tv_sec;
    return NULL;
}

void *register_thread(void *arg) {
    PBX *pbx = (PBX *)arg;
    int fd = fileno(tmpfile());
    TU *tu = tu_init(fd);
    pbx_register(pbx, tu, fd);
    return NULL;
}

struct dial_args {
    PBX *pbx;
    TU *tu;
    int peer;
};

void *dial_thread(void *arg) {
    struct dial_args *ap = (struct dial_args *)arg;
    PBX *pbx = ap->pbx;
    TU *tu = ap->tu;
    int peer = ap->peer;

    pbx_dial(pbx, tu, peer);
    return NULL;
}

void *reg_unreg_thread(void *arg) {
    PBX *pbx = (PBX *)arg;
    int fd = fileno(tmpfile());
    TU *tu = tu_init(fd);
    pbx_register(pbx, tu, fd);
    pbx_unregister(pbx, tu);
    return NULL;
}

/* The following tests have been commented off since they are almost similar */
#if 0
Test(pbx_suite, only_reg_then_shutdown, .timeout = 10) {
#ifdef NO_PBX
    cr_assert_fail("PBX was not implemented");
#endif
    int ret;
    PBX *pbx = pbx_init();
    assert_not_null(pbx, "pbx_init");

    // Register a client to temporarily prevent an empty situation.
    int fd = fileno(tmpfile());
    TU *tu = tu_init(fd);
    ret = pbx_register(pbx, tu, fd);
    assert_success(ret, "pbx_register");

    pthread_t tid;
    struct reg_unreg_args *ap = calloc(1, sizeof(struct reg_unreg_args));
    ap->pbx = pbx;
    pthread_create(&tid, NULL, register_thread, ap);

    pthread_t tid1;
    struct shutdown_args *ap1 = calloc(1, sizeof(struct shutdown_args));
    ap1->pbx = pbx;
    ap1->elapsed_time = 0;
    pthread_create(&tid1, NULL, shutdown_thread, ap1);

    pthread_join(tid, NULL);
    ret = pbx_unregister(pbx, tu);
    assert_success(ret, "pbx_unregister");

    // Sleep for 5 seconds, check shutdown_thread's elapsed time.
    // shutdown_thread is not supposed to finish, since the other thread did not
    // call unregister.
    sleep(5);
    cr_assert(!ap1->elapsed_time, "pbx_shutdown did not wait for client threads to unregister");
    pthread_cancel(tid1);
}

Test(pbx_suite, reg_unreg_immediate, .timeout = 5) {
#ifdef NO_PBX
    cr_assert_fail("PBX was not implemented");
#endif
    int ret;
    PBX *pbx = pbx_init();
    assert_not_null(pbx, "pbx_init");

    // Register a client to temporarily prevent an empty situation.
    int fd = fileno(tmpfile());
    TU *tu = tu_init(fd);
    ret = pbx_register(pbx, tu, fd);
    assert_success(ret, "pbx_register");

    pthread_t tid;
    struct reg_unreg_args *ap = calloc(1, sizeof(struct reg_unreg_args));
    ap->pbx = pbx;
    pthread_create(&tid, NULL, reg_unreg_thread, ap);

    pthread_t tid1;
    struct shutdown_args *ap1 = calloc(1, sizeof(struct shutdown_args));
    ap1->pbx = pbx;
    ap1->elapsed_time = 0;
    pthread_create(&tid1, NULL, shutdown_thread, ap1);

    pthread_join(tid, NULL);
    ret = pbx_unregister(pbx, tu);
    assert_success(ret, "pbx_unregister");
    pthread_join(tid1, NULL); // shutdown thread should join
}

/*
 * Test one pbx, one thread doing random register/unregister,
 * and that thread calling pbx_shutdown does not block forever.
 */
Test(pbx_suite, random_reg_unreg, .timeout = 5) {
#ifdef NO_PBX
    cr_assert_fail("PBX was not implemented");
#endif
    PBX *pbx = pbx_init();
    assert_not_null(pbx, "pbx_init");

    // Spawn a thread to run random increment/decrement.
    pthread_t tid;
    struct reg_unreg_args *ap = calloc(1, sizeof(struct reg_unreg_args));
    ap->pbx = pbx;
    ap->iters = NITER;
    pthread_create(&tid, NULL, random_reg_unreg_thread, ap);

    // Wait for the increment/decrement to complete.
    pthread_join(tid, NULL);

    // Call shutdown -- should not time out.
    pbx_shutdown(pbx);
    cr_assert(1, "Timed out waiting for zero");
}

/*
 * One thread doing random register/unregister, one thread calling shutdown.
 * Check that thread calling shutdown does not return prematurely.
 */
Test(pbx_suite, random_reg_unreg_premature, .timeout = 5) {
#ifdef NO_PBX
    cr_assert_fail("PBX was not implemented");
#endif
    int ret;
    PBX *pbx = pbx_init();
    assert_not_null(pbx, "pbx_init");

    // Register a client to temporarily prevent an empty situation.
    int fd = fileno(tmpfile());
    TU *tu = tu_init(fd);
    ret = pbx_register(pbx, tu, fd);
    assert_success(ret, "pbx_register");

    // Create a flag to be set when random increment/decrement is finished.
    // This probably should be done more properly.
    volatile int flags[1] = { 0 };

    // Spawn a thread to shutdown and then check the flag.
    pthread_t tid1;
    struct shutdown_args *ap1 = calloc(1, sizeof(struct shutdown_args));
    ap1->pbx = pbx;
    ap1->flags = flags;
    ap1->nflags = 1;
    pthread_create(&tid1, NULL, shutdown_thread, ap1);

    // Spawn a thread to run a long random register/unregister test and set flag.
    pthread_t tid2;
    struct reg_unreg_args *ap2 = calloc(1, sizeof(struct reg_unreg_args));
    ap2->pbx = pbx;
    ap2->iters = NITER;
    ap2->done_flag = &flags[0];
    pthread_create(&tid2, NULL, random_reg_unreg_thread, ap2);

    // Wait for the increment/decrement to complete, then release the thread counter.
    pthread_join(tid2, NULL);
    ret = pbx_unregister(pbx, tu);
    assert_success(ret, "pbx_register");

    // Get the result from the waiting thread, to see if it returned prematurely.
    pthread_join(tid1, NULL);

    // Assert that the flag was set when the wait was finished.
    cr_assert(ap1->ret, "Premature return from pbx_shutdown");
}

Test(pbx_suite, many_threads_random_reg_unreg, .timeout = 30) {
#ifdef NO_PBX
    cr_assert_fail("PBX was not implemented");
#endif
    PBX *pbx = pbx_init();
    assert_not_null(pbx, "pbx_init");

    // Spawn threads to run random increment/decrement.
    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++) {
        struct reg_unreg_args *ap = calloc(1, sizeof(struct reg_unreg_args));
        ap->pbx = pbx;
        ap->iters = NITER;
        pthread_create(&tid[i], NULL, random_reg_unreg_thread, ap);
    }

    // Wait for all threads to finish.
    for(int i = 0; i < NTHREAD; i++)
        pthread_join(tid[i], NULL);

    // Call shutdown -- should not time out.
    pbx_shutdown(pbx);
    cr_assert(1);
}
#endif

Test(pbx_suite, many_threads_random_reg_unreg_premature, .timeout = 25) {
#ifdef NO_PBX
    cr_assert_fail("PBX was not implemented");
#endif
    int ret;
    PBX *pbx = pbx_init();
    assert_not_null(pbx, "pbx_init");

    // Register a client to temporarily prevent an empty situation.
    int fd = fileno(tmpfile());
    TU *tu = tu_init(fd);
    ret = pbx_register(pbx, tu, fd);
    assert_success(ret, "pbx_register");

    volatile int flags[NTHREAD] = { 0 };

    // Spawn a thread to shutdown and then check the flag.
    pthread_t tid1;
    struct shutdown_args *ap1 = calloc(1, sizeof(struct shutdown_args));
    ap1->pbx = pbx;
    ap1->flags = flags;
    ap1->nflags = NTHREAD;
    pthread_create(&tid1, NULL, shutdown_thread, ap1);

    // Spawn threads to run random increment/decrement.
    pthread_t tid[NTHREAD];
    for(int i = 0; i < NTHREAD; i++) {
        struct reg_unreg_args *ap = calloc(1, sizeof(struct reg_unreg_args));
        ap->pbx = pbx;
        ap->iters = NITER;
        ap->done_flag = &flags[i];
        pthread_create(&tid[i], NULL, random_reg_unreg_thread, ap);
    }

    // Wait for all threads to finish.
    for(int i = 0; i < NTHREAD; i++)
        pthread_join(tid[i], NULL);
    ret = pbx_unregister(pbx, tu);
    assert_success(ret, "pbx_unregister");

    // Get the result from the waiting thread, to see if it returned prematurely.
    pthread_join(tid1, NULL);

    // Assert that the flags were all set when the wait was finished.
    cr_assert(ap1->ret, "Premature return from pbx_shutdown");
}

Test(pbx_suite, dial_two_threads, .timeout = 5) {
#ifdef NO_PBX
    cr_assert_fail("PBX was not implemented");
#endif
    PBX *pbx = pbx_init();
    assert_not_null(pbx, "pbx_init");

    int fd = fileno(tmpfile());
    TU *tu = tu_init(fd);
    int fd1 = fileno(tmpfile());
    TU *tu1 = tu_init(fd1);

    pthread_t tid;
    struct dial_args *ap = calloc(1, sizeof(struct dial_args));
    ap->pbx = pbx;
    ap->tu = tu;
    ap->peer = fd1;
    pthread_create(&tid, NULL, dial_thread, ap);

    pthread_t tid1;
    struct dial_args *ap1 = calloc(1, sizeof(struct dial_args));
    ap1->pbx = pbx;
    ap1->tu = tu1;
    ap1->peer = fd;
    pthread_create(&tid1, NULL, dial_thread, ap1);

    pthread_join(tid, NULL);
    pthread_join(tid1, NULL);
}
