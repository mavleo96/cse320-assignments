/*
 * "PBX" server module.
 * Manages interaction with a client telephone unit (TU).
 */
#include "debug.h"
#include "pbx.h"
#include "server.h"
#include "utils.h"

// TODO: change these to static methods
int fstrlen(char *str);
int fstrcmp(char *str1, char *str2);
int parse_command(char *buffer, TU_COMMAND *cmd);
int read_client_message(int connfd, char *buffer, size_t buffer_size);

/*
 * Thread function for the thread that handles interaction with a client TU.
 * This is called after a network connection has been made via the main server
 * thread and a new thread has been created to handle the connection.
 */
void *pbx_client_service(void *arg) {
    // Retrieve the fd and free the pointer 
    int connfd = *(int *)arg;
    free(arg);

    info("client service thread created for tu '%d' on (tid %ld)", connfd, pthread_self());

    // Detach the thread
    int status;
    if ((status = pthread_detach(pthread_self())) != 0) {
        error("pthread_detach returned %d with error for (tid %ld)!", status, pthread_self());
        close(connfd);
        return NULL;
    }

    // Initialize new TU with connfd
    TU *tu = tu_init(connfd);
    if (!tu) {
        error("error in initializing TU!");
    }

    // Register the TU with PBX module under extension number
    if (pbx_register(pbx, tu, connfd) == -1) {
        error("error registering TU with PBX!");
        close(connfd);
        return NULL;
    }

    // Buffer to store the incoming message
    // TODO: check if this static buffer is an issue
    char buffer[1024];
    ssize_t bytes_read;
    TU_COMMAND cmd;

    // Service loop
    while (1) {
        success("client service loop started for tu '%d' on (tid %ld)", connfd, pthread_self()); 

        // Read message from the client
        bytes_read = read_client_message(connfd, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            break;
        }
        debug("received message: %s", buffer);

        // Parse the command from the buffer
        if (parse_command(buffer, &cmd) == -1) {
            error("unknown command received: %s", buffer);
            continue;
        }

        // Handle the command
        switch (cmd) {
            case TU_PICKUP_CMD:
                debug("pickup command received");
                if (tu_pickup(tu) == -1) {
                    error("error handling pickup command!");
                }
                break;

            case TU_HANGUP_CMD:
                debug("hangup command received");
                if (tu_hangup(tu) == -1) {
                    error("error handling hangup command!");
                }
                break;

            case TU_DIAL_CMD:
                debug("dial command received");
                int target_ext = atoi(buffer + fstrlen(buffer) + 1);
                if (pbx_dial(pbx, tu, target_ext) == -1) {
                    error("error handling dial command!");
                }
                break;

            case TU_CHAT_CMD:
                debug("chat command received");
                if (tu_chat(tu, buffer + fstrlen(buffer) + 1) == -1) {
                    error("error handling chat command.");
                }
                break;

            default:
                // TODO: change this server will silently ignore syntactically incorrect commands,
                // such as "dial" without an extension number.  If commands are issued when the
                // TU at the server side is in an inappropriate state (for example, if "dial 5"
                // is sent when the TU is "on hook"), then a response will be sent by the server
                // that just repeats the state of the TU, which does not change.
                error("unknown command received: %s", buffer);
                break;
        }
    }

    pbx_unregister(pbx, tu);
    return NULL;
}

int read_client_message(int connfd, char *buffer, size_t buffer_size) {
    ssize_t bytes_read = 0;
    while (1) {
        char c;
        ssize_t n = read(connfd, &c, 1);
        if (n <= 0) {
            if (n == 0) {
                debug("client disconnected for connection %d", connfd);
            } else {
                error("error reading from client for connection %d", connfd);
            }
            return 0;
        }
        buffer[bytes_read++] = c;

        // Null-terminate the string before the "\r\n"
        if (bytes_read > 1 && buffer[bytes_read - 2] == '\r' && buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 2] = '\0'; // Remove "\r\n"
            break;
        }

        // Prevent buffer overflow
        if (bytes_read >= buffer_size) {
            error("Message too long for buffer");
            return -1; // Buffer overflow
        }
    }
    return bytes_read;
}

int fstrlen(char *str) {
    int length = 0;
    while (*str != '\0' && *str != ' ') {
        length++;
        str++;
    }
    return length;
}

int fstrcmp(char *str1, char *str2) {
    // TODO: edge case pending
    while ((*str1 != '\0' || *str1 != ' ') && *str2 != '\0') {
        if (*str1 != *str2) return *str1 - *str2;
        str1++;
        str2++;
    }
    return 0;
}

int parse_command(char *buffer, TU_COMMAND *cmd) {
    // TODO: remove 4 hard coding here
    for (int i = 0; i < 4; i++) {
        if (fstrcmp(buffer, tu_command_names[i]) == 0) {
            *cmd = i;
            return 0;
        }
    }
    return -1;
}