/*
 * "PBX" server module.
 * Manages interaction with a client telephone unit (TU).
 */
#include "debug.h"
#include "pbx.h"
#include "server.h"
#include "utils.h"

/*
 * Internal helper methods declared here
 */
static int pstrlen(char *str);
static int pstrcmp(char *str_buf, char *str_cmd);
static int parse_command(char *buffer, int *offset_p, TU_COMMAND *cmd);
static int read_client_message(int connfd, char *buffer, size_t buffer_size);

/*
 * Thread function for the thread that handles interaction with a client TU.
 * This is called after a network connection has been made via the main server
 * thread and a new thread has been created to handle the connection.
 */
void *pbx_client_service(void *arg) {
    // Retrieve the fd and free the pointer 
    int connfd = *(int *)arg;
    free(arg);

    info("service thread (tid %ld) created for client on fd '%d'", pthread_self(), connfd);

    // Detach the thread
    int status;
    if ((status = pthread_detach(pthread_self())) != 0) {
        error("pthread_detach returned %d with error for (tid %ld)!", status, pthread_self());
        close(connfd);
        return NULL;
    }

    // Initialize new TU with connfd and register the TU with PBX module under extension number
    TU *tu = tu_init(connfd);
    if (!tu || pbx_register(pbx, tu, connfd) == -1) {
        error("error in initializing or regisstering TU!");
        if (tu) tu_unref(tu, "exiting client service thread due to pbx_register fail");
        close(connfd);
        return NULL;
    }

    // Buffer to store the incoming message
    // TODO: check if this static buffer is an issue
    char buffer[1024];
    int offset = 0;
    ssize_t bytes_read;
    TU_COMMAND cmd;

    // Service loop
    success("service loop started for TU '%d' on (tid %ld)", connfd, pthread_self()); 
    while((bytes_read = read_client_message(connfd, buffer, sizeof(buffer))) > 0) {
        // if (bytes_read <= 0) {
        //     break;
        // }
        debug("received message from client on TU (ext %d): %s", tu_extension(tu), buffer);

        // Parse the command from the buffer
        if (parse_command(buffer, &offset, &cmd) == -1) {
            warn("unknown or malformed command from client on TU (ext %d): %s", tu_extension(tu), buffer);
            continue;
        }

        // Handle the command
        switch (cmd) {
            case TU_PICKUP_CMD:
                debug("pickup command received from client on TU (ext %d)", tu_extension(tu));
                if (tu_pickup(tu) == -1) error("error handling pickup command!");
                break;

            case TU_HANGUP_CMD:
                debug("hangup command received from client on TU (ext %d)", tu_extension(tu));
                if (tu_hangup(tu) == -1) error("error handling hangup command!");
                break;

            case TU_DIAL_CMD:
                debug("dial command received from client on TU (ext %d)", tu_extension(tu));
                int target_ext = atoi(buffer + offset);
                if (pbx_dial(pbx, tu, target_ext) == -1) error("error handling dial command!");
                break;

            case TU_CHAT_CMD:
                debug("chat command received from client on TU (ext %d)", tu_extension(tu));
                if (tu_chat(tu, buffer + offset) == -1) error("error handling chat command!");
                break;

            default:
                error("unhandled command: %s", buffer);
                break;
        }
    }

    pbx_unregister(pbx, tu);
    close(connfd);
    info("closing service thread (tid %ld) for client on fd '%d'...", pthread_self(), connfd);
    return NULL;
}

static int read_client_message(int connfd, char *buffer, size_t buffer_size) {
    ssize_t bytes_read = 0;

    while (1) {
        char c;
        ssize_t n = read(connfd, &c, 1);

        // Error handling
        if (n <= 0) {
            if (n == 0) {
                debug("client on fd '%d' disconnected", connfd);
            } else {
                error("error reading from client on fd '%d'!", connfd);
            }
            return n;
        }

        buffer[bytes_read++] = c;

        // Check for end-of-line sequence
        if (bytes_read >= 2 &&
            buffer[bytes_read - 2] == '\r' &&
            buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 2] = '\0';
            break;
        }

        // Prevent buffer overflow
        if (bytes_read >= buffer_size) {
            error("message from client on fd '%d' too long for buffer!", connfd);
            return -1;
        }
    }
    return bytes_read;
}

static int pstrlen(char *str) {
    int length = 0;
    while (str[length] != '\0' && str[length] != ' ') {
        length++;
    }
    if (str[length] == ' ') length++;
    return length;
}

static int pstrcmp(char *str_buf, char *str_cmd) {
    while (*str_cmd != '\0') {
        if (*str_buf != *str_cmd) return -1;
        str_buf++;
        str_cmd++;
    }
    if (*str_buf != '\0' && *str_buf != ' ') return -1;
    return 0;
}

static int parse_command(char *buffer, int *offset_p, TU_COMMAND *cmd) {
    int num_cmd = 4;
    for (int i = 0; i < num_cmd; i++) {
        if (pstrcmp(buffer, tu_command_names[i]) == 0) {
            *cmd = i;
            *offset_p = pstrlen(buffer);
            return 0;
        }
    }
    return -1;
}