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
#include <sys/stat.h>
#include <fcntl.h>
#include <criterion/criterion.h>
#include <pthread.h>
#include <semaphore.h>
#include "tu.h"
#include "__grading_helpers.h"

/* Number of threads we create in multithreaded tests. */
#define NTHREAD (100)

/* Number of iterations we use in several tests. */
#define NITER (50)

static ssize_t read_file(FILE *in, char **msgp) {
    ssize_t max = 100;
    char *msg, *mp;
    int c;
    if((msg = malloc(max)) == NULL) {
        perror("read_file");
        return -1;
    }
    mp = msg;
    while((c = fgetc(in)) != EOF) {
        ssize_t n = mp - msg;
        if(n == max-1) {
            max *= 2;
            if((msg = realloc(msg, max)) == NULL) {
                perror("read_file");
                return -1;
            }
            mp = msg + n;
        }
        *mp++ = c;
    }
    *mp = 0;
    *msgp = msg;
    return mp - msg;
}

Test(TU_suite, tu_init, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif

    TU *tu = tu_init(10);

    assert_not_null((void *)tu, "tu_init");
}

static void *TU_ref_stress_thread(void *arg) 
{
    TU *tu = (TU *) arg;
    tu_ref(tu, "test ref");
    return (void *) tu;
}

static void *TU_unref_thread(void *arg)
{
    TU *tu = (TU *) arg;
    tu_unref(tu, "test unref");
    return (void *) tu;
}

/* tu_ref stress test make sure there are no crashes and deadlocks */
Test(TU_suite, tu_ref_stress, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif

    pthread_t tid[NTHREAD];
    pthread_t tid_unref[NTHREAD];

    TU *tu = tu_init(10);

    for(int i = 0; i < NTHREAD/2; i++)  
    {
        tu_ref(tu, "ref");
    }

    /* ref threads */
    for(int i = 0; i < NTHREAD/2; i++) 
    {
        pthread_create(&tid[i], NULL, TU_ref_stress_thread, tu);
    }
    /* unref thread */ 
    for(int i = 0; i < NTHREAD/2; i++) 
    {
        pthread_create(&tid_unref[i], NULL, TU_unref_thread, tu);
    }

    for(int i = 0; i < NTHREAD/2; i++)
	pthread_join(tid[i], NULL);

    for(int i = 0; i < NTHREAD/2; i++)
        pthread_join(tid_unref[i], NULL);

    for(int i = 0; i < NTHREAD/2; i++)  
    {
        tu_unref(tu, "unref");
    }
}

/* call tu_init with an fd adn verify it by calling tu_fileno */
Test(TU_suite, tu_fileno, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif

    TU *tu = tu_init(9);
    int fd = tu_fileno(tu);
    assert_values_equal(fd, 9, "tu_fileno");
}

/* call tu_set_extension and verify it by calling tu_extension */
Test(TU_suite, tu_extension, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif

    TU *tu = tu_init(1);
    redirect_stdout();
    tu_set_extension(tu, 400);
    int ext = tu_extension(tu);
    assert_values_equal(ext, 400, "tu_extension");
}

/* call tu_pickup and check the returned message */
Test(TU_suite, tu_pickup_basic, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    FILE *fp = tmpfile();

    TU *tu = tu_init(fileno(fp));
    tu_set_extension(tu, 400);
    int ret = tu_pickup(tu);
    assert_values_equal(ret, 0, "tu_pickup");

    rewind(fp);
    char *msg = NULL;
    ret = read_file(fp,&msg);
    assert_not_fail(ret, "receive_message");
    assert_alpha_string_matches(msg, "ON HOOK 400\r\nDIAL TONE\r\n", "TU message");

    close(fileno(fp));
    free(msg);
}

/* call tu_pickup twice */
Test(TU_suite, tu_pickup_repeat, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    FILE *fp = tmpfile();

    TU *tu = tu_init(fileno(fp));
    tu_set_extension(tu, 400);
    int ret = tu_pickup(tu);
    ret = tu_pickup(tu); //the second time
    assert_values_equal(ret, 0, "tu_pickup second time");
}

/* dial a valid TU */
Test(TU_suite, tu_dial_basic, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    FILE *fp1 = tmpfile();
    TU *tu1 = tu_init(fileno(fp1));
    tu_set_extension(tu1, 400);
    int ret = tu_pickup(tu1);
    assert_values_equal(ret, 0, "TU1 pickup");

    FILE *fp2 = tmpfile();
    TU *tu2 = tu_init(fileno(fp2));
    tu_set_extension(tu2, 500);

    ret = tu_dial(tu1,tu2);
    assert_values_equal(ret, 0, "TU1 dial");

    ret = tu_pickup(tu2);
    assert_values_equal(ret, 0, "TU2 pickup");
    
    ret = tu_hangup(tu1);
    assert_values_equal(ret, 0, "TU2 pickup");

    ret = tu_hangup(tu2);
    assert_values_equal(ret, 0, "TU2 pickup");

    rewind(fp1);
    rewind(fp2);
    char *msg = NULL;
    ret = read_file(fp1,&msg);
    assert_not_fail(ret, "receive_message");
    assert_alpha_string_matches(msg, "ON HOOK 400\r\nDIAL TONE\r\nRING BACK\r\nCONNECTED 500\r\nON HOOK 400\r\n", "TU1 message");

    free(msg);
    msg = NULL;
    ret = read_file(fp2,&msg);
    assert_not_fail(ret, "receive_message");
    assert_alpha_string_matches(msg, "ON HOOK 500\r\nRINGING\r\nCONNECTED 400\r\nDIAL TONE\r\nON HOOK 500\r\n", "TU2 message");

    close(fileno(fp1));
    close(fileno(fp2));
    free(msg);
}

/* self dial */
Test(TU_suite, tu_self_dial, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    FILE *fp1 = tmpfile();
    TU *tu1 = tu_init(fileno(fp1));
    tu_set_extension(tu1, 400);
    int ret = tu_pickup(tu1);
    assert_values_equal(ret, 0, "TU1 pickup");

    ret = tu_dial(tu1,tu1);
    assert_values_equal(ret, 0, "TU1 dial");

    rewind(fp1);
    char *msg = NULL;
    ret = read_file(fp1,&msg);
    assert_not_fail(ret, "receive_message");
    assert_alpha_string_matches(msg, "ON HOOK 400\r\nDIAL TONE\r\nBUSY SIGNAL\r\n", "TU1 message");

    close(fileno(fp1));
    free(msg);
}

/* dial a TU  that is not ON HOOK */
Test(TU_suite, tu_dial_off_hook, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    FILE *fp1 = tmpfile();
    TU *tu1 = tu_init(fileno(fp1));
    tu_set_extension(tu1, 400);
    int ret = tu_pickup(tu1);
    assert_values_equal(ret, 0, "TU1 pickup");

    FILE *fp2 = tmpfile();
    TU *tu2 = tu_init(fileno(fp2));
    tu_set_extension(tu2, 500);
    ret = tu_pickup(tu2);

    ret = tu_dial(tu1,tu2);
    assert_values_equal(ret, 0, "TU1 dial");

    rewind(fp1);
    char *msg = NULL;
    ret = read_file(fp1,&msg);
    assert_not_fail(ret, "receive_message");
    assert_alpha_string_matches(msg, "ON HOOK 400\r\nDIAL TONE\r\nBUSY SIGNAL\r\n", "TU1 message");

    close(fileno(fp1));
    close(fileno(fp2));
    free(msg);
}

typedef struct tupair
{
    TU *tu;
    TU *peer;
}tupair;

static void *TU_pair_dial(void *arg)
{
    tupair *tp = arg;
    int i;

    for (i = 0; i < NITER; i++)
    {
        tu_pickup(tp->tu);
        tu_dial(tp->tu, tp->peer);
        tu_hangup(tp->tu);
    }
    return NULL;
}

/* TU1 dials TU2, TU2 dials TU1 at the same time. There should not be any deadlock*/
Test(TU_suite, tu_dial_deadlock, .timeout=10)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    pthread_t tid1, tid2;

    FILE *fp1 = tmpfile();
    TU *tu1 = tu_init(fileno(fp1));
    tu_set_extension(tu1, 400);

    FILE *fp2 = tmpfile();
    TU *tu2 = tu_init(fileno(fp2));
    tu_set_extension(tu2, 500);

    tupair tp1 = {tu1, tu2};
    tupair tp2 = {tu2, tu1};

    pthread_create(&tid1, NULL, TU_pair_dial, &tp1);
    pthread_create(&tid2, NULL, TU_pair_dial, &tp2);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
}

/* establish a connection and call tu_chat */
Test(TU_suite, tu_chat_basic, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    FILE *fp1 = tmpfile();
    TU *tu1 = tu_init(fileno(fp1));
    tu_set_extension(tu1, 400);
    int ret = tu_pickup(tu1);
    assert_values_equal(ret, 0, "TU1 pickup");

    FILE *fp2 = tmpfile();
    TU *tu2 = tu_init(fileno(fp2));
    tu_set_extension(tu2, 500);

    ret = tu_dial(tu1,tu2);
    assert_values_equal(ret, 0, "TU1 dial");

    ret = tu_pickup(tu2);
    assert_values_equal(ret, 0, "TU2 pickup");

    ret = tu_chat(tu1, "Hello 500");
    assert_values_equal(ret, 0, "TU1 chat");
    ret = tu_chat(tu2, "Hello 400");
    assert_values_equal(ret, 0, "TU2 chat");

    rewind(fp1);
    rewind(fp2);
    char *msg = NULL;
    ret = read_file(fp1,&msg);
    assert_not_fail(ret, "receive_message");
    assert_alpha_string_matches(msg, "ON HOOK 400\r\nDIAL TONE\r\nRING BACK\r\nCONNECTED 500\r\nCONNECTED 500\r\nCHAT Hello 400\r\n", "TU1 message");

    free(msg);
    msg = NULL;
    ret = read_file(fp2,&msg);
    assert_not_fail(ret, "receive_message");
    assert_alpha_string_matches(msg, "ON HOOK 500\r\nRINGING\r\nCONNECTED 400\r\nCHAT Hello 500\r\nCONNECTED 400\r\n", "TU2 message");

    close(fileno(fp1));
    close(fileno(fp2));
    free(msg);
}
/* call tu_chat without a peer */
Test(TU_suite, tu_chat_invalid, .timeout=2)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    FILE *fp1 = tmpfile();
    TU *tu1 = tu_init(fileno(fp1));
    tu_set_extension(tu1, 400);
    int ret = tu_pickup(tu1);
    assert_values_equal(ret, 0, "TU1 pickup");
    ret = tu_chat(tu1, "Invalid connection");
    assert_values_equal(ret, -1, "TU chat");
}

#if 0
static void *TU_chat_1(void *arg)
{
    printf("TU1 thread\n");
    tupair *tp = arg;
    tu_pickup(tp->tu);

    tu_dial(tp->tu,tp->peer);
    sem_post(&wait_sem);

    //sem_wait(&wait_sem);
    printf("TU1 woken");
    return NULL;
}
static void *TU_chat_2(void *arg)
{
    printf("TU2 thread\n");
    sem_wait(&wait_sem);
    TU *tu2 = arg;
    printf("TU2 woken");
    tu_pickup(tu2);
    tu_chat(tu2, "Hello 400");
    sem_post(&wait_sem);
    return NULL;
}


/* invoke tu_chat across two different threads */
Test(TU_suite, TU_chat, .timeout=10)
{
#ifdef NO_TU
    cr_assert_fail("TU module was not implemented");
#endif
    pthread_t tid1, tid2;

    FILE *fp1 = tmpfile();
    TU *tu1 = tu_init(fileno(fp1));
    tu_set_extension(tu1, 400);

    FILE *fp2 = tmpfile();
    TU *tu2 = tu_init(fileno(fp2));
    tu_set_extension(tu2, 500);
    printf("creating threads\n");

    tupair tp1 = {tu1, tu2};
    sem_init(&wait_sem, 0, 0);

    pthread_create(&tid1, NULL, TU_chat_1, &tp1);
    pthread_create(&tid2, NULL, TU_chat_2, &tu2);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
}
#endif
