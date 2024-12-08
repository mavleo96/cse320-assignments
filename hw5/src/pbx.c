/*
 * PBX: simulates a Private Branch Exchange.
 */
#include <stdlib.h>

#include "pbx.h"
#include "debug.h"
#include "utils.h"

/*
 * PBX structure holds an array of TU pointers and a mutex lock for synchronization
 */
typedef struct pbx {
    TU **tu_array;
    pthread_mutex_t lock;
} PBX;

/*
 * Initialize a new PBX.
 *
 * @return the newly initialized PBX, or NULL if initialization fails.
 */
PBX *pbx_init() {
    // Allocate memory for the PBX structure
    PBX *pbx = malloc(sizeof(PBX));
    if (!pbx) {
        error("malloc failed with error: %s", strerror(errno));
        return NULL;
    }

    // Allocate memory for the TU pointer array
    pbx->tu_array = malloc(sizeof(TU *) * PBX_MAX_EXTENSIONS);
    if (!pbx->tu_array) {
        error("malloc failed with error: %s", strerror(errno));
        free(pbx);
        return NULL;
    }
    memset(pbx->tu_array, 0, sizeof(TU *) * PBX_MAX_EXTENSIONS);

    // Initialize the mutex
    int status;
    if ((status = pthread_mutex_init(&pbx->lock, NULL)) != 0) {
        error("pthread_mutex_init failed and returned status %d", status);
        free(pbx->tu_array);
        free(pbx);
        return NULL;
    }

    success("PBX initialized successfully");
    return pbx;
}

/*
 * Shut down a pbx, shutting down all network connections, waiting for all server
 * threads to terminate, and freeing all associated resources.
 * If there are any registered extensions, the associated network connections are
 * shut down, which will cause the server threads to terminate.
 * Once all the server threads have terminated, any remaining resources associated
 * with the PBX are freed.  The PBX object itself is freed, and should not be used again.
 *
 * @param pbx  The PBX to be shut down.
 */
void pbx_shutdown(PBX *pbx) {
    if (!pbx) {
        error("null PBX pointer passed!");
        return;
    }

    // Signal all threads to terminate by shutting down each TU's connection
    pthread_mutex_lock(&pbx->lock);
    //-------- CRITICAL SECTION --------//
    for (int i = 0; i < PBX_MAX_EXTENSIONS; i++) {
        if (pbx->tu_array[i]) {
            TU *tu = pbx->tu_array[i];
            info("shutting down TU (ext %d)", tu_extension(tu));
            pbx->tu_array[i] = NULL;
            tu_hangup(tu);
            tu_unref(tu, "pbx_shutdown");
        }
    }
    //-------- CRITICAL SECTION --------//
    pthread_mutex_unlock(&pbx->lock);

    // Free the resources and destroy mutex
    int status;
    if ((status = pthread_mutex_destroy(&pbx->lock)) != 0) {
        error("pthread_mutex_destroy failed and returned status %d", status);
    }
    free(pbx->tu_array);
    free(pbx);

    success("PBX shut down successfully");
}

/*
 * Register a telephone unit with a PBX at a specified extension number.
 * This amounts to "plugging a telephone unit into the PBX".
 * The TU is initialized to the TU_ON_HOOK state.
 * The reference count of the TU is increased and the PBX retains this reference
 *for as long as the TU remains registered.
 * A notification of the assigned extension number is sent to the underlying network
 * client.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU to be registered.
 * @param ext  The extension number on which the TU is to be registered.
 * @return 0 if registration succeeds, otherwise -1.
 */
int pbx_register(PBX *pbx, TU *tu, int ext) {
    if (!pbx || !tu || ext < 0 || ext >= PBX_MAX_EXTENSIONS) {
        error("invalid parameters passed!");
        return -1;
    }

    // Register TU in pbx tu_array
    pthread_mutex_lock(&pbx->lock);
    //-------- CRTICAL SECTION --------//
    if (pbx->tu_array[ext] != NULL) {
        error("ext %d is already in use!", ext);
        pthread_mutex_unlock(&pbx->lock);
        return -1;
    }
    pbx->tu_array[ext] = tu;
    //-------- CRTICAL SECTION --------//
    pthread_mutex_unlock(&pbx->lock);
    
    // Initialize the TU state and increment its reference count
    tu_set_extension(tu, ext);
    tu_ref(tu, "pbx_register");

    success("registered TU (ext %d) on PBX", ext);
    return 0;
}

/*
 * Unregister a TU from a PBX.
 * This amounts to "unplugging a telephone unit from the PBX".
 * The TU is disassociated from its extension number.
 * Then a hangup operation is performed on the TU to cancel any
 * call that might be in progress.
 * Finally, the reference held by the PBX to the TU is released.
 *
 * @param pbx  The PBX.
 * @param tu  The TU to be unregistered.
 * @return 0 if unregistration succeeds, otherwise -1.
 */
int pbx_unregister(PBX *pbx, TU *tu) {
    if (!pbx || !tu) {
        error("invalid parameters passed!");
        return -1;
    }

    // Locate the TU's extension number
    int ext = tu_extension(tu);
    if (ext < 0 || ext >= PBX_MAX_EXTENSIONS) {
        error("error in retrieving ext!");
        return -1;
    }

    // Unregister TU from PBX
    pthread_mutex_lock(&pbx->lock);
    //-------- CRTICAL SECTION --------//
    if (pbx->tu_array[ext] != tu) {
        error("cannot find TU in PBX!");
        pthread_mutex_unlock(&pbx->lock);
        return -1;
    }
    pbx->tu_array[ext] = NULL;
    //-------- CRTICAL SECTION --------//
    pthread_mutex_unlock(&pbx->lock);

    // Perform a hangup operation to terminate any ongoing call
    if (tu_hangup(tu) == -1) {
        error("failed to hang up TU (ext %d)!", ext);
    }

    // Release the reference to the TU
    tu_unref(tu, "pbx_unregister");

    success("unregistered TU (ext %d) on PBX", ext);
    return 0;
}

/*
 * Use the PBX to initiate a call from a specified TU to a specified extension.
 *
 * @param pbx  The PBX registry.
 * @param tu  The TU that is initiating the call.
 * @param ext  The extension number to be called.
 * @return 0 if dialing succeeds, otherwise -1.
 */
int pbx_dial(PBX *pbx, TU *tu, int ext) {
    if (!pbx || !tu || ext < 0 || ext >= PBX_MAX_EXTENSIONS) {
        error("invalid parameters passed!");
        return -1;
    }

    TU *target;

    // Locate the target TU
    pthread_mutex_lock(&pbx->lock);
    debug("pbx_dial routing TU (ext %d) to TU (ext %d)", tu_extension(tu), ext);
    //-------- CRTICAL SECTION --------//
    target = pbx->tu_array[ext];
    //-------- CRTICAL SECTION --------//
    pthread_mutex_unlock(&pbx->lock);

    // Perform a tu_dial operation
    if (tu_dial(tu, target) == -1) {
        error("failed to dial TU (ext %d)!", ext);
        return -1;
    }

    success("successfully dialed TU (ext %d)", ext);
    return 0;
}
