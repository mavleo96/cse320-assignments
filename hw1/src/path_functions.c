#include "global.h"
#include "debug.h"
#include "string_functions.h"
#include "path_functions.h"

/*
 * @brief  Initialize path_buf to a specified base path.
 * @details  This function copies its null-terminated argument string into
 * path_buf, including its terminating null byte.
 * The function fails if the argument string, including the terminating
 * null byte, is longer than the size of path_buf.  The path_length variable 
 * is set to the length of the string in path_buf, not including the terminating
 * null byte.
 *
 * @param  Pathname to be copied into path_buf.
 * @return 0 on success, -1 in case of error
 */
int path_init(char *name) {
    // TODO: Add comments and format the function better
    int count = 0;
    if (string_length(name) + 1 > sizeof(path_buf)) {
        error("Provided path argument of length %d is longer than size of path_buf %ld", string_length(name), sizeof(path_buf));
        return -1;
    }
    // TODO: Need to handle the trailing / issue
    while (*(name + count) != '\0') {
        *(path_buf + count) = *(name + count);
        count += 1;
    }
    *(path_buf + count + 1) = '\0';
    path_length = string_length(name);

    return 0;
}

/*
 * @brief  Append an additional component to the end of the pathname in path_buf.
 * @details  This function assumes that path_buf has been initialized to a valid
 * string.  It appends to the existing string the path separator character '/',
 * followed by the string given as argument, including its terminating null byte.
 * The length of the new string, including the terminating null byte, must be
 * no more than the size of path_buf.  The variable path_length is updated to
 * remain consistent with the length of the string in path_buf.
 * 
 * @param  The string to be appended to the path in path_buf.  The string must
 * not contain any occurrences of the path separator character '/'.
 * @return 0 in case of success, -1 otherwise.
 */
int path_push(char *name) {
    // TODO: Add comments and format the function better
    int count = 0;
    if (path_length + string_length(name) + 1 + 1 > sizeof(path_buf)) {
        return -1;
    }
    *(path_buf + path_length) = '/';
    while (*(name + count) != '\0') {
        *(path_buf + path_length + 1 + count) = *(name + count);
        count += 1;
    }
    path_length += string_length(name) + 1;

    return 0;
}

/*
 * @brief  Remove the last component from the end of the pathname.
 * @details  This function assumes that path_buf contains a non-empty string.
 * It removes the suffix of this string that starts at the last occurrence
 * of the path separator character '/'.  If there is no such occurrence,
 * then the entire string is removed, leaving an empty string in path_buf.
 * The variable path_length is updated to remain consistent with the length
 * of the string in path_buf.  The function fails if path_buf is originally
 * empty, so that there is no path component to be removed.
 *
 * @return 0 in case of success, -1 otherwise.
 */
int path_pop() {
    // TODO: Add comments and format the function better
    int count = 0;
    while (*(path_buf + path_length - count) != '/') {
        *(path_buf + path_length - count) = '\0';
        count += 1;
    }
    *(path_buf + path_length - count) = '\0';
    path_length -= count;
    // TODO: Empty path_buf case to be handled
    return 0;
}