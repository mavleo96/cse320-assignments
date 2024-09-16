#include "global.h"
#include "debug.h"
#include "string_functions.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

#define MAGIC_BYTE_1 0x0C
#define MAGIC_BYTE_2 0x0D
#define MAGIC_BYTE_3 0xED


#define START_OF_TRANSMISSION 0
#define END_OF_TRANSMISSION 1
#define START_OF_DIRECTORY 2
#define END_OF_DIRECTORY 3
#define DIRECTORY_ENTRY 4
#define FILE_DATA 5

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 *
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in global.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 *
 * IMPORTANT: You MAY NOT use floating point arithmetic or declare
 * any "float" or "double" variables.  IF YOU VIOLATE THIS RESTRICTION,
 * YOU WILL GET A ZERO!
 */

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

/*
 * @brief Deserialize directory contents into an existing directory.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory.  It reads (from the standard input) a sequence of DIRECTORY_ENTRY
 * records bracketed by a START_OF_DIRECTORY and END_OF_DIRECTORY record at the
 * same depth and it recreates the entries, leaving the deserialized files and
 * directories within the directory named by path_buf.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * each of the records processed.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including depth fields in the records read that do not match the
 * expected value, the records to be processed to not being with START_OF_DIRECTORY
 * or end with END_OF_DIRECTORY, or an I/O error occurs either while reading
 * the records from the standard input or in creating deserialized files and
 * directories.
 */
int deserialize_directory(int depth) {
    // To be implemented.
    abort();
}

/*
 * @brief Deserialize the contents of a single file.
 * @details  This function assumes that path_buf contains the name of a file
 * to be deserialized.  The file must not already exist, unless the ``clobber''
 * bit is set in the global_options variable.  It reads (from the standard input)
 * a single FILE_DATA record containing the file content and it recreates the file
 * from the content.
 *
 * @param depth  The value of the depth field that is expected to be found in
 * the FILE_DATA record.
 * @return 0 in case of success, -1 in case of an error.  A variety of errors
 * can occur, including a depth field in the FILE_DATA record that does not match
 * the expected value, the record read is not a FILE_DATA record, the file to
 * be created already exists, or an I/O error occurs either while reading
 * the FILE_DATA record from the standard input or while re-creating the
 * deserialized file.
 */
int deserialize_file(int depth) {
    // To be implemented.
    abort();
}

/*
 * @brief  Serialize the contents of a directory as a sequence of records written
 * to the standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * directory to be serialized.  It serializes the contents of that directory as a
 * sequence of records that begins with a START_OF_DIRECTORY record, ends with an
 * END_OF_DIRECTORY record, and with the intervening records all of type DIRECTORY_ENTRY.
 *
 * @param depth  The value of the depth field that is expected to occur in the
 * START_OF_DIRECTORY, DIRECTORY_ENTRY, and END_OF_DIRECTORY records processed.
 * Note that this depth pertains only to the "top-level" records in the sequence:
 * DIRECTORY_ENTRY records may be recursively followed by similar sequence of
 * records describing sub-directories at a greater depth.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open files, failure to traverse directories, and I/O errors
 * that occur while reading file content and writing to standard output.
 */
int traverse_dir(const char *path, int depth);

int stream_data(int type, int depth, int size) {
    // Emit magic bytes
    putchar(MAGIC_BYTE_1);
    putchar(MAGIC_BYTE_2);
    putchar(MAGIC_BYTE_3);

    // Emit record type
    putchar(type);

    // Emit depth unsigned 32-bit integer in big-endian format  
    for (int i = 3; i >= 0; i--) {
        putchar((depth >> (i * 8)) & 0xFF);
    }
    
    // Emit size unsigned 32-bit integer in big-endian format  
    for (int i = 7; i >= 0; i--) {
        putchar((size >> (i * 8)) & 0xFF);
    }
    return 0;
}

int serialize_directory(int depth) {

    debug("START DIRECTORY, DEPTH: %d, SIZE: TBD, PATH: %s", depth + 1, path_buf);
    stream_data(START_OF_DIRECTORY, depth + 1, 16);
    traverse_dir(path_buf, depth);
    debug("END DIRECTORY, DEPTH: %d, SIZE: TBD, PATH: %s", depth + 1, path_buf);
    stream_data(END_OF_DIRECTORY, depth + 1, 16);

    // TODO: account for failure cases
    return 0;
}

/*
 * @brief  Serialize the contents of a file as a single record written to the
 * standard output.
 * @details  This function assumes that path_buf contains the name of an existing
 * file to be serialized.  It serializes the contents of that file as a single
 * FILE_DATA record emitted to the standard output.
 *
 * @param depth  The value to be used in the depth field of the FILE_DATA record.
 * @param size  The number of bytes of data in the file to be serialized.
 * @return 0 in case of success, -1 otherwise.  A variety of errors can occur,
 * including failure to open the file, too many or not enough data bytes read
 * from the file, and I/O errors reading the file data or writing to standard output.
 */
int serialize_file(int depth, off_t size) {

    debug("FILE DATA; DEPTH: %d, PATH: %s", depth, path_buf);
    stream_data(FILE_DATA, depth, 0);
    // FILE *f = fopen(path_buf, "w");
    // fclose(f);

    // TODO: account for failure cases
    return 0;
}

/**
 * @brief Serializes a tree of files and directories, writes
 * serialized data to standard output.
 * @details This function assumes path_buf has been initialized with the pathname
 * of a directory whose contents are to be serialized.  It traverses the tree of
 * files and directories contained in this directory (not including the directory
 * itself) and it emits on the standard output a sequence of bytes from which the
 * tree can be reconstructed.  Options that modify the behavior are obtained from
 * the global_options variable.
 *
 * @return 0 if serialization completes without error, -1 if an error occurs.
 */
int serialize() {

    // TODO: need to check if there is a better way to manage depth updates
    uint32_t depth = 0;

    debug("START TRANSMISSION, DEPTH: %d, SIZE: TBD", depth);
    stream_data(START_OF_TRANSMISSION, 0, 16);

    traverse_dir(path_buf, depth);
    
    debug("END TRANSMISSION, DEPTH: %d, SIZE: TBD", depth);
    stream_data(END_OF_TRANSMISSION, 0, 16);

    // TODO: account for failure cases
    return 0;
}

int traverse_dir(const char *path, int depth) {
    DIR *dir = opendir(path);
    depth += 1;
    if (dir == NULL) {
        return -1;
    }
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (string_compare(de->d_name, ".") + string_compare(de->d_name, "..") > -2) {
            continue;
        }
        if (de->d_type == DT_DIR) {
            path_push(de->d_name);
            debug("DIRECTORY ENTRY, DEPTH: %d, SIZE: TBD, PATH: %s\n", depth, path_buf);
            stream_data(DIRECTORY_ENTRY, depth, 0);
            serialize_directory(depth);
            path_pop();
        } else if (de->d_type == DT_REG) {
            path_push(de->d_name);
            stream_data(DIRECTORY_ENTRY, depth, 0);
            serialize_file(depth, 0);
            path_pop();
        } else {
            // TODO: account for more failure cases
            return -1;
        }
    }
    depth -= 1;
    closedir(dir);
    return 0;
}

/**
 * @brief Reads serialized data from the standard input and reconstructs from it
 * a tree of files and directories.
 * @details  This function assumes path_buf has been initialized with the pathname
 * of a directory into which a tree of files and directories is to be placed.
 * If the directory does not already exist, it is created.  The function then reads
 * from from the standard input a sequence of bytes that represent a serialized tree
 * of files and directories in the format written by serialize() and it reconstructs
 * the tree within the specified directory.  Options that modify the behavior are
 * obtained from the global_options variable.
 *
 * @return 0 if deserialization completes without error, -1 if an error occurs.
 */
int deserialize() {
    // To be implemented.
    abort();
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv) {
    int validation_status = 0;    // Default to valid arguments
    int check_args = 1;           // Validation to start from 1st argument
    char mode = '\0';             // Mode to store the operation s|d

    // Insufficient arguments case
    if (argc == 1) {
        validation_status = -1;
    }

    // Loop through the command-line arguments
    while (argc - check_args > 0) {
        // Check for positional argument (must be -h|-s|-d)
        if (check_args == 1) {
            if (string_compare(*(argv + check_args), "-h") == 0) {
                global_options |= (1 << 0);
                break;
            } else if (string_compare(*(argv + check_args), "-s") == 0) {
                mode = 's';
                global_options |= (1 << 1);
            } else if (string_compare(*(argv + check_args), "-d") == 0) {
                mode = 'd';
                global_options |= (1 << 2);
            } else {
                validation_status = -1;
                break;
            }
            check_args += 1;
            continue;
        }

        // Check for -p flag (DIR follows -p flag)
        if (string_compare(*(argv + check_args), "-p") == 0) {
            if (argc - check_args - 1 == 0) {
                validation_status = -1;
                break;
            }
            check_args += 2;
            continue;
        }

        // Check for -c flag (valid only for mode 'd')
        if (string_compare(*(argv + check_args), "-c") == 0) {
            if (mode == 'd') {
                global_options |= (1 << 3);
                check_args += 1;
                continue;
            } else {
                validation_status = -1;
                break;
            }
        }
        // Break for invalid argument
        validation_status = -1;
        break;
    }
    return validation_status;
}
