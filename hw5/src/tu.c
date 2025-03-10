/*
 * TU: simulates a "telephone unit", which interfaces a client with the PBX.
 */
#include "pbx.h"
#include "debug.h"
#include "utils.h"

/*
 * TU structure holds extension, connection fd, state and reference count of TU along with mutex lock
 */
typedef struct tu {
    int ext;
    int connfd;
    TU_STATE state;
    TU *peer_tu;
    pthread_mutex_t lock;
    int ref_count;
} TU;

static void notify_state(TU *tu);

/*
 * Initialize a TU
 *
 * @param fd  The file descriptor of the underlying network connection.
 * @return  The TU, newly initialized and in the TU_ON_HOOK state, if initialization
 * was successful, otherwise NULL.
 */
TU *tu_init(int fd) {
    if (fd < 0) {
        error("invalid connection fd (%d) passed!", fd);
        return NULL;
    }
    // Allocate memory for TU
    TU *tu = malloc(sizeof(TU));
    if (!tu) {
        error("malloc failed with error: %s", strerror(errno));
        return NULL;
    }

    // Initialize fields
    tu->ext = -1;
    tu->connfd = fd;
    tu->state = TU_ON_HOOK;
    tu->ref_count = 0;
    tu->peer_tu = NULL;

    // Initialize mutex
    int status;
    if ((status = pthread_mutex_init(&tu->lock, NULL)) != 0) {
        error("pthread_mutex_init failed and returned status %d", status);
        free(tu);
        return NULL;
    }

    success("TU initialized successfully");
    return tu;
}

/*
 * Increment the reference count on a TU.
 *
 * @param tu  The TU whose reference count is to be incremented
 * @param reason  A string describing the reason why the count is being incremented
 * (for debugging purposes).
 */
void tu_ref(TU *tu, char *reason) {
    if (!tu) {
        error("null TU pointer passed!");
        return;
    }
    if (!reason) {
        error("null reason pointer passed!");
        return;
    }

    pthread_mutex_lock(&tu->lock);
    tu->ref_count++;
    debug("TU (ext %d) ref count inc to %d: %s", tu->ext, tu->ref_count, reason);
    pthread_mutex_unlock(&tu->lock);
}

/*
 * Decrement the reference count on a TU, freeing it if the count becomes 0.
 *
 * @param tu  The TU whose reference count is to be decremented
 * @param reason  A string describing the reason why the count is being decremented
 * (for debugging purposes).
 */
void tu_unref(TU *tu, char *reason) {
    if (!tu) {
        error("null TU pointer passed!");
        return;
    }
    if (!reason) {
        warn("null reason pointer passed!");
    }

    pthread_mutex_lock(&tu->lock);
    tu->ref_count--;
    debug("TU (ext %d) ref count dec to %d: %s", tu->ext, tu->ref_count, reason ? reason : "null");

    if (tu->ref_count == 0) {
        pthread_mutex_unlock(&tu->lock);
        pthread_mutex_destroy(&tu->lock);
        debug("TU (ext %d) freed", tu->ext);
        free(tu);
        return;
    }
    pthread_mutex_unlock(&tu->lock);
}

/*
 * Get the file descriptor for the network connection underlying a TU.
 * This file descriptor should only be used by a server to read input from
 * the connection.  Output to the connection must only be performed within
 * the PBX functions.
 *
 * @param tu
 * @return the underlying file descriptor, if any, otherwise -1.
 */
int tu_fileno(TU *tu) {
    if (!tu) {
        error("null TU pointer passed!");
        return -1;
    }
    return tu->connfd;
}

/*
 * Get the extension number for a TU.
 * This extension number is assigned by the PBX when a TU is registered
 * and it is used to identify a particular TU in calls to tu_dial().
 * The value returned might be the same as the value returned by tu_fileno(),
 * but is not necessarily so.
 *
 * @param tu
 * @return the extension number, if any, otherwise -1.
 */
int tu_extension(TU *tu) {
    if (!tu) {
        error("null TU pointer passed!");
        return -1;
    }
    return tu->ext;
}

/*
 * Set the extension number for a TU.
 * A notification is set to the client of the TU.
 * This function should be called at most once one any particular TU.
 *
 * @param tu  The TU whose extension is being set.
 */
int tu_set_extension(TU *tu, int ext) {
    if (!tu || ext < 0) {
        error("invalid arguments passed!");
        return -1;
    }

    pthread_mutex_lock(&tu->lock);
    if (tu->ext != -1) {
        error("tried setting ext %d to TU (ext %d)", ext, tu->ext);
        pthread_mutex_unlock(&tu->lock);
        return -1;
    }

    tu->ext = ext;
    success("ext %d set to TU", ext);
    notify_state(tu);
    pthread_mutex_unlock(&tu->lock);

    return 0;
}

/*
 * Initiate a call from a specified originating TU to a specified target TU.
 *   If the originating TU is not in the TU_DIAL_TONE state, then there is no effect.
 *   If the target TU is the same as the originating TU, then the TU transitions
 *     to the TU_BUSY_SIGNAL state.
 *   If the target TU already has a peer, or the target TU is not in the TU_ON_HOOK
 *     state, then the originating TU transitions to the TU_BUSY_SIGNAL state.
 *   Otherwise, the originating TU and the target TU are recorded as peers of each other
 *     (this causes the reference count of each of them to be incremented),
 *     the target TU transitions to the TU_RINGING state, and the originating TU
 *     transitions to the TU_RING_BACK state.
 *
 * In all cases, a notification of the resulting state of the originating TU is sent to
 * to the associated network client.  If the target TU has changed state, then its client
 * is also notified of its new state.
 *
 * If the caller of this function was unable to determine a target TU to be called,
 * it will pass NULL as the target TU.  In this case, the originating TU will transition
 * to the TU_ERROR state if it was in the TU_DIAL_TONE state, and there will be no
 * effect otherwise.  This situation is handled here, rather than in the caller,
 * because here we have knowledge of the current TU state and we do not want to introduce
 * the possibility of transitions to a TU_ERROR state from arbitrary other states,
 * especially in states where there could be a peer TU that would have to be dealt with.
 *
 * @param tu  The originating TU.
 * @param target  The target TU, or NULL if the caller of this function was unable to
 * identify a TU to be dialed.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
int tu_dial(TU *tu, TU *target) {
    if (!tu) {
        error("null pointer passed!");
        return 0;
    }

    pthread_mutex_lock(&tu->lock);

    // Ensure the originating TU is in the TU_DIAL_TONE state
    if (tu->state != TU_DIAL_TONE) {
        notify_state(tu);
        pthread_mutex_unlock(&tu->lock);
        return 0;
    }

    // Handle invalid target case
    if (!target) {
        tu->state = TU_ERROR;
        notify_state(tu);
        pthread_mutex_unlock(&tu->lock);
        return -1;
    }

    // Handle case where the originating TU is trying to call itself
    if (tu == target) {
        tu->state = TU_BUSY_SIGNAL;
        notify_state(tu);
        pthread_mutex_unlock(&tu->lock);
        return 0;
    }

    pthread_mutex_unlock(&target->lock);

    if (target->peer_tu || target->state != TU_ON_HOOK) {
        // Target TU is busy or not available
        tu->state = TU_BUSY_SIGNAL;
        notify_state(tu);
        pthread_mutex_unlock(&target->lock);
        pthread_mutex_unlock(&tu->lock);
        return 0;
    }

    // Establish peers
    tu->peer_tu = target;
    tu->ref_count++;
    debug("TU (ext %d) ref count inc to %d: tu_dial", tu->ext, tu->ref_count);
    target->peer_tu = tu;
    target->ref_count++;
    debug("TU (ext %d) ref count inc to %d: tu_dial", target->ext, target->ref_count);

    // Transition states
    info("TU (ext %d) state change to TU_RING_BACK from maybe TU_ON_HOOK", tu->ext);
    tu->state = TU_RING_BACK;
    info("TU (ext %d) state change to TU_RINGING from maybe TU_ON_HOOK", target->ext);
    target->state = TU_RINGING;

    notify_state(target);
    notify_state(tu);

    pthread_mutex_unlock(&target->lock);
    pthread_mutex_unlock(&tu->lock);

    return 0;
}

/*
 * Take a TU receiver off-hook (i.e. pick up the handset).
 *   If the TU is in neither the TU_ON_HOOK state nor the TU_RINGING state,
 *     then there is no effect.
 *   If the TU is in the TU_ON_HOOK state, it goes to the TU_DIAL_TONE state.
 *   If the TU was in the TU_RINGING state, it goes to the TU_CONNECTED state,
 *     reflecting an answered call.  In this case, the calling TU simultaneously
 *     also transitions to the TU_CONNECTED state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The TU that is to be picked up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
int tu_pickup(TU *tu) {
    // TODO: think why tu can go to error state 
    if (!tu) {
        error("null pointer passed!");
        return -1;
    }

    pthread_mutex_lock(&tu->lock);

    // Case 1: TU is 'on hook'
    if (tu->state == TU_ON_HOOK) {
        tu->state = TU_DIAL_TONE;
        info("TU (ext %d) state change to TU_DIAL_TONE from maybe TU_ON_HOOK", tu->ext);
        notify_state(tu);
    }
    // Case 2: TU is 'ringing'
    else if (tu->state == TU_RINGING) {
        TU *peer = tu->peer_tu;
        if (!peer) {
            error("peer_tu is NULL in RINGING state!");
            tu->state = TU_ERROR;
            notify_state(tu);
            pthread_mutex_unlock(&tu->lock);
            return -1;
        }

        pthread_mutex_lock(&peer->lock);

        // Tranisition states
        tu->state = TU_CONNECTED;
        info("TU (ext %d) state change to TU_CONNECTED from maybe TU_RINGING", tu->ext);
        peer->state = TU_CONNECTED;
        info("TU (ext %d) state change to TU_CONNECTED from maybe DNK", peer->ext);

        // Notify both TUs 
        notify_state(tu);
        notify_state(peer);

        pthread_mutex_unlock(&peer->lock);
    }
    // Case 3: Invalid state
    else {
        notify_state(tu);
    }

    pthread_mutex_unlock(&tu->lock);
    return 0;
}

/*
 * Hang up a TU (i.e. replace the handset on the switchhook).
 *
 *   If the TU is in the TU_CONNECTED or TU_RINGING state, then it goes to the
 *     TU_ON_HOOK state.  In addition, in this case the peer TU (the one to which
 *     the call is currently connected) simultaneously transitions to the TU_DIAL_TONE
 *     state.
 *   If the TU was in the TU_RING_BACK state, then it goes to the TU_ON_HOOK state.
 *     In addition, in this case the calling TU (which is in the TU_RINGING state)
 *     simultaneously transitions to the TU_ON_HOOK state.
 *   If the TU was in the TU_DIAL_TONE, TU_BUSY_SIGNAL, or TU_ERROR state,
 *     then it goes to the TU_ON_HOOK state.
 *
 * In all cases, a notification of the resulting state of the specified TU is sent to
 * to the associated network client.  If a peer TU has changed state, then its client
 * is also notified of its new state.
 *
 * @param tu  The tu that is to be hung up.
 * @return 0 if successful, -1 if any error occurs that results in the originating
 * TU transitioning to the TU_ERROR state. 
 */
int tu_hangup(TU *tu) {
    if (!tu) {
        error("null pointer passed!");
        return -1;
    }

    pthread_mutex_lock(&tu->lock);

    // Handle cases with a valid peer TU
    if (tu->state == TU_CONNECTED || tu->state == TU_RINGING || tu->state == TU_RING_BACK) {
        TU *peer = tu->peer_tu;

        if (!peer) {
            error("peer_tu is NULL in state requiring a peer!");
            tu->state = TU_ERROR;
            notify_state(tu);
            pthread_mutex_unlock(&tu->lock);
            return -1;
        }

        pthread_mutex_lock(&peer->lock);

        // Transition states
        // Case 1: TU is not calling
        if (tu->state == TU_CONNECTED || tu->state == TU_RINGING) {
            tu->state = TU_ON_HOOK;
            info("TU (ext %d) state change to TU_ON_HOOK from maybe DNK", tu->ext);
            peer->state = TU_DIAL_TONE;
            info("TU (ext %d) state change to TU_DIAL_TONE from maybe DNK", peer->ext);
        }
        // Case 2: TU is calling
        else if (tu->state == TU_RING_BACK) {
            tu->state = TU_ON_HOOK;
            peer->state = TU_ON_HOOK;
        }
        
        // Disconnect the peers
        tu->peer_tu = NULL;
        peer->peer_tu = NULL;

        tu->ref_count--;
        debug("TU (ext %d) ref count dec to %d: tu_hangup", tu->ext, tu->ref_count);
        peer->ref_count--;
        debug("TU (ext %d) ref count dec to %d: tu_hangup", peer->ext, peer->ref_count);

        notify_state(peer);
        pthread_mutex_unlock(&peer->lock);
    }
    // Self contained states
    else if (tu->state == TU_DIAL_TONE || tu->state == TU_BUSY_SIGNAL || tu->state == TU_ERROR) {
        tu->state = TU_ON_HOOK;
    }

    notify_state(tu);
    pthread_mutex_unlock(&tu->lock);

    return 0;
}

/*
 * "Chat" over a connection.
 *
 * If the state of the TU is not TU_CONNECTED, then nothing is sent and -1 is returned.
 * Otherwise, the specified message is sent via the network connection to the peer TU.
 * In all cases, the states of the TUs are left unchanged and a notification containing
 * the current state is sent to the TU sending the chat.
 *
 * @param tu  The tu sending the chat.
 * @param msg  The message to be sent.
 * @return 0  If the chat was successfully sent, -1 if there is no call in progress
 * or some other error occurs.
 */
int tu_chat(TU *tu, char *msg) {
    if (!tu) {
        error("null TU pointer passed!");
        return -1;
    }
    if (!msg) {
        error("null msg pointer passed!");
        return -1;
    }

    pthread_mutex_lock(&tu->lock);
    // Invalid state to chat
    if (tu->state != TU_CONNECTED) {
        notify_state(tu);
        pthread_mutex_unlock(&tu->lock);
        return -1;
    }

    // Retrieve and validate the peer TU
    TU *peer = tu->peer_tu;
    if (!peer) {
        error("Peer TU is NULL in TU_CONNECTED state.");
        notify_state(tu);
        pthread_mutex_unlock(&tu->lock);
        return -1;
    }

    pthread_mutex_lock(&peer->lock);
    int ret_val = 0;

    // Prepare and send the chat message
    char fmsg[5 + strlen(msg) + 2 + 1];
    int written_bytes = snprintf(fmsg, sizeof(fmsg), "CHAT %s%s", msg, EOL);
    if (written_bytes < 0 || written_bytes >= sizeof(fmsg)) {
        error("message formatting failed!");
        ret_val = -1;
    }
    else if (write(peer->connfd, fmsg, strlen(fmsg)) == -1) {
        error("write failed with error: %s", strerror(errno));
        ret_val = -1;
    }

    pthread_mutex_unlock(&peer->lock);
    notify_state(tu);
    pthread_mutex_unlock(&tu->lock);
    
    return ret_val;
}

/*
 * Helper function to notify client about TU state
 * MUST BE ACCESSED ONLY BY TU WHOSE MUTEX IS LOCKED
 */
static void notify_state(TU *tu) {
    if (!tu) {
        error("null pointer passed!");
        return;
    }
    
    // Retrieve fd, state and ext
    int fd = tu->connfd;
    if (fd == -1) {
        error("invalid connection fd!");
        return;
    }
    int state = tu->state;
    int ext = tu->ext;

    // Write to connection fd notifying the state of TU
    char msg[256];
    int written_bytes = 0;

    if (state == TU_ON_HOOK) {
        written_bytes = snprintf(msg, sizeof(msg), "%s %d%s", tu_state_names[state], ext, EOL);
    } else {
        written_bytes = snprintf(msg, sizeof(msg), "%s%s", tu_state_names[state], EOL);
    }
    if (written_bytes < 0 || written_bytes >= sizeof(msg)) {
        error("msg formatting failed!");
        return;
    }

    if (write(fd, msg, strlen(msg)) == -1) {
        error("write failed with error: %s", strerror(errno));
        return;
    }
}