#include "pbx.h"
#include "server.h"
#include "debug.h"
#include "utils.h"

static void terminate(int status);
void sighup_handler(int signum);

/*
 * "PBX" telephone exchange simulation.
 *
 * Usage: pbx <port>
 */
int main(int argc, char* argv[]){

    // Parse command-line arguments for '-p <port>'
    int port = 0;
    if (argc < 3 || strcmp(argv[1], "-p") != 0) {
        error("usage: %s -p <port>", argv[0]);
        exit(EXIT_FAILURE);
    }
    port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        error("invalid port number: %d", port);
        exit(EXIT_FAILURE);
    }

    // Set up SIGHUP handler
    struct sigaction sa;
    sa.sa_handler = sighup_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        error("sigaction failed while installing SIGHUP handler with error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Perform required initialization of the PBX module.
    debug("initializing PBX...");
    pbx = pbx_init();

    int listenfd = open_listenfd(port);
    if (listenfd < 0) {
        error("open_listenfd failed with error: %s", strerror(errno));
        terminate(EXIT_FAILURE);
    }

    while (1) {
        struct sockaddr_storage clientaddr;
        socklen_t clientlen = sizeof(clientaddr);

        // TODO: resuse address
        int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        if (connfd < 0) {
            error("accept failed with error: %s", strerror(errno));
            continue;
        }
        info("accepted connection '%d'", connfd);

        // Spawn a thread to handle the client connection
        pthread_t tid;
        int *connfd_ptr = malloc(sizeof(int));
        if (!connfd_ptr) {
            error("malloc failed with error: %s", strerror(errno));
            close(connfd);
            continue;
        }
        *connfd_ptr = connfd;

        if (pthread_create(&tid, NULL, (void *(*)(void *))pbx_client_service, connfd_ptr)) {
            error("pthread_create failed with error: %s", strerror(errno));
            free(connfd_ptr);
            close(connfd);
        }
    }

    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    debug("shutting down PBX...");
    pbx_shutdown(pbx);
    debug("PBX server terminating");
    exit(status);
}


// Signal handler for SIGHUP
void sighup_handler(int signum) {
    debug("terminating server due to SIGHUP...");
    terminate(EXIT_SUCCESS);
}