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
    // Check if name is longer than PATH_MAX; exit if yes
    int name_length = string_length(name);
    if (name_length + 1 > sizeof(path_buf)) {
        error("Provided path argument of length %d is longer than size of path_buf %ld", name_length + 1, sizeof(path_buf));
        return -1;
    }

    // Pop trailing '/' character
    if (*(name + name_length - 1) == '/') name_length--;

    // Copy name into path_buf
    for (int i = 0; i < name_length; i++) {
        *(path_buf + i) = *(name + i);
    }
    *(path_buf + name_length) = '\0';

    // Initialize path_length
    path_length = name_length;

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
    // Check if name is valid; '/' should not exist in name
    int name_length = string_length(name);
    for (int i = 0; i < name_length; i++) {
        if (*(name + i) == '/') {
            error("String to be appended contains '/' character");
            return -1;
        }
    }

    // Check if name is valid; final path_length should not be longer than PATH_MAX
    int count = 0;
    if (path_length + 1 + name_length + 1 > sizeof(path_buf)) { // path_buf + '/' + name + '\0'
        error("Updated path_buf of length %d will be longer than size of path_bug %ld", path_length + name_length + 2 , sizeof(path_buf));
        return -1;
    }

    // Update path_buf
    *(path_buf + path_length) = '/';
    for (int i = 0; i < name_length + 1; i++) {
        *(path_buf + path_length + 1 + i) = *(name + i);
    }

    // Update path_length
    path_length += name_length + 1;

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
    // Check if path_buf is empty
    if (string_length(path_buf) == 0) {
        error("Nothing to pop; path_buf is empty");
        return -1;
    }

    // Pop characters after last occurance of '/'
    int count = 0;
    while (*(path_buf + path_length - count) != '/') {
        *(path_buf + path_length - count) = '\0';
        count += 1;
    }
    *(path_buf + path_length - count) = '\0';

    // Update path_length
    path_length -= count;

    return 0;
}