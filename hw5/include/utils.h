#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <pthread.h>
#include <signal.h>
#include <netdb.h>

int open_listenfd(int port);