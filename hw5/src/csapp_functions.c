#include "utils.h"

/*
 * open_listenfd - Open and return a listening socket on port
 * Returns:
 *  - A listening socket descriptor on success
 *  - -1 on error (and sets errno appropriately)
 */
int open_listenfd(int port) {
    int listenfd;
    int optval = 1;
    struct sockaddr_in serveraddr;

    // Step 1: Create a socket descriptor
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // Step 2: Eliminate "Address already in use" error with setsockopt
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int)) < 0) {
        perror("setsockopt error");
        close(listenfd);
        return -1;
    }

    // Step 3: Bind the socket to the given port
    memset(&serveraddr, 0, sizeof(serveraddr));  // Clear the struct
    serveraddr.sin_family = AF_INET;            // Use IPv4
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections on any IP
    serveraddr.sin_port = htons((unsigned short)port);

    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind error");
        close(listenfd);
        return -1;
    }

    // Step 4: Listen for incoming connections
    if (listen(listenfd, 1024) < 0) { // 1024 is the backlog queue size
        perror("listen error");
        close(listenfd);
        return -1;
    }

    return listenfd;
}
