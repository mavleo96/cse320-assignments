#include "global.h"
#include "debug.h"
#include "string_functions.h"
#include "path_functions.h"
#include "helper_functions.h"

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

#define STD_RECORD_SIZE 16
#define STD_METADATA_SIZE 12

#define OPTION_CLOBBER (1 << 3)

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
 * @brief Read record header and update pointers as per parsed data
 *
 * @param type Pointer to type to be updated
 * @param depth Pointer to depth to be updated
 * @param size Pointer to size to be updated
 *
 * @return 0 in case of success else -1
 */
int read_record_header(int *type, int *depth, int *size) {
    // Validate magic bytes
    if (getchar() != MAGIC_BYTE_1 || getchar() != MAGIC_BYTE_2 || getchar() != MAGIC_BYTE_3) {
        error("Magic bytes were not found in beginning of the record");
        return -1;
    }

    // Read type
    *type = getchar();

    // Read depth
    for (int i = 3; i >= 0; i--) {
        *depth = (*depth << 8);
        *depth += getchar();
    }

    // Read size
    for (int i = 7; i >= 0; i--) {
        *size = (*size << 8);
        *size += getchar();
    }

    // Validate type
    if ((*type < 0) || (*type > 5)) {
        error("Record has invalid type of %d", *type);
        return -1;
    }

    // Validate size based on the type
    if ((*type <= END_OF_DIRECTORY && *size != STD_RECORD_SIZE) || 
        (*type == DIRECTORY_ENTRY && *size < STD_RECORD_SIZE + STD_METADATA_SIZE) ||
        (*type == FILE_DATA && *size < STD_RECORD_SIZE)) {
        error("Record has invalid size of %d for type %d", *size, *type);
        debug("FAILURE OCCURED");
        return -1;
    }
    return 0;
}

/*
 * @brief Read metadata in the record and update pointers as per parsed data
 *
 * @param mode Pointer to type to be updated
 * @param size Pointer to size to be updated
 */
void read_metadata(int *mode, int *size) {
    // Read type and permissions
    for (int i = 3; i >= 0; i--) {
        *mode = (*mode << 8);
        *mode += getchar();
    }

    // Read size of file
    for (int i = 7; i >= 0; i--) {
        *size = (*size << 8);
        *size += getchar();
    }
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
    int status;

    int record_type;
    int record_depth;
    int record_size;

    int metadata_mode;
    int metadata_size;

    // Read start directory record; return -1 in case of error
    status = read_record_header(&record_type, &record_depth, &record_size);
    if (status == -1) return -1;

    // Validate if start transmission record entry matches expectation 
    if ((record_type == START_OF_DIRECTORY) &&
        (record_depth == depth) &&
        (record_size == STD_RECORD_SIZE)) {
        debug("START_OF_DIRECTORY, DEPTH: %d, SIZE: %d, PATH: %s", record_depth, record_size, path_buf);
    } else {
        error("Error in start directory record");
        error("Expected type %d but got %d", START_OF_DIRECTORY, record_type);
        error("Expected depth %d but got %d", depth, record_depth);
        error("Expected size %d but got %d", STD_RECORD_SIZE, record_size);
        return -1;
    }

    // Deserialize until end of directory is reached
    while (1) {
        // Read directory entry record; return -1 in case of error
        status = read_record_header(&record_type, &record_depth, &record_size);
        if (status == -1) return -1;

        if (record_type == END_OF_DIRECTORY) {
            break;
        }

        // Validate if directory entry record entry matches expectation 
        if ((record_type == DIRECTORY_ENTRY) &&
            (record_depth == depth)) {
            debug("DIRECTORY_ENTRY, DEPTH: %d, SIZE: %d, *PATH: %s", record_depth, record_size, path_buf);
        } else {
            error("Error in directory entry record");
            error("Expected type %d but got %d", DIRECTORY_ENTRY, record_type);
            error("Expected size %d but got %d", STD_RECORD_SIZE, record_size);
            return -1;
        }

        // Read metadata
        read_metadata(&metadata_mode, &metadata_size);

        // Store file name in name_buf
        int name_size = record_size - STD_RECORD_SIZE - STD_METADATA_SIZE;
        for (int i = 0; i < name_size; i++) {
            *(name_buf + i) = getchar();
        }
        *(name_buf + name_size + 1) = '\0';
        
        // Update path_buf
        if (path_push(name_buf) == -1) return -1;
        debug("UPDATED PATH: %s", path_buf);
        string_clear(name_buf);

        // Deserialize next record basis file type
        if (S_ISDIR(metadata_mode)) {
            mkdir(path_buf, 0700);                            // Create directory
            status = deserialize_directory(depth + 1);        // Deserialize directory
            if (status == -1) return -1;                      // Exit if status is -1
            chmod(path_buf, metadata_mode & 0777);            // Set permissions
        } else if (S_ISREG(metadata_mode)) {
            status = deserialize_file(depth);                 // Deserialize file
            if (status == -1) return -1;                      // Exit if status is -1
            chmod(path_buf, metadata_mode & 0777);            // Set permissions
        } else {
            error("Could not deserialize file: %s", path_buf);
            error("This file type is currently not supported in deserialization");
            return -1;
        }
        // Update path_buf
        if (path_pop() == -1) return -1;
    }

    // Validate if end directory record entry matches expectation 
    if ((record_type == END_OF_DIRECTORY) &&
        (record_depth == depth) &&
        (record_size == STD_RECORD_SIZE)) {
        debug("END_OF_DIRECTORY, DEPTH: %d, SIZE: %d, PATH: %s", record_depth, record_size, path_buf);
    } else {
        error("Error in end directory record");
        error("Expected type %d but got %d", END_OF_DIRECTORY, record_type);
        error("Expected depth %d but got %d", depth, record_depth);
        error("Expected size %d but got %d", STD_RECORD_SIZE, record_size);
        return -1;
    }

    return 0;
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
    int status;
    int clobber = (global_options & OPTION_CLOBBER) ? 1 : 0;

    int record_type;
    int record_depth;
    int record_size;

    // Read file data record; return -1 in case of error
    status = read_record_header(&record_type, &record_depth, &record_size);
    if (status == -1) return -1;

    // Validate if file data record entry matches expectation 
    if ((record_type == FILE_DATA) &&
        (record_depth == depth)) {
        debug("FILE_DATA, DEPTH: %d, SIZE: %d, PATH: %s", record_depth, record_size, path_buf);
    } else {
        error("Error in file data record");
        error("Expected type %d but got %d", FILE_DATA, record_type);
        error("Expected depth %d but got %d", depth, record_depth);
        return -1;
    }

    // Check if file already exists and return -1 if clobber flag is not set
    if (!clobber && file_exists(path_buf)) {
        error("File %s exists already", path_buf);
        return -1;
    }

    // Create a file and write content
    int file_size = record_size - STD_RECORD_SIZE;
    FILE *f = fopen(path_buf, "w");
    char ch;
    for (int i = 0; i < file_size; i++) {
        ch = getchar();
        fputc(ch, f);
    }
    fclose(f);

    return 0;
}

/*
 * @brief Stream data to stdout
 *
 * @param type Type of record to be emitted
 * @param depth Depth of the given record
 * @param size Size of record to be emitted
 */
void stream_data(int type, int depth, off_t size) {
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
}

/*
 * @brief Stream metadata to stdout
 *
 * @param mode Type/permission of the directory entry
 * @param depth Depth of the given directory entry record
 */
void stream_metadata(mode_t mode, off_t size) {
    // Emit type/permission unsigned 32-bit integer in big-endian format
    for (int i = 3; i >= 0; i--) {
        putchar((mode >> (i * 8)) & 0xFF);
    }
    
    // Emit size unsigned 32-bit integer in big-endian format 
    for (int i = 7; i >= 0; i--) {
        putchar((size >> (i * 8)) & 0xFF);
    }
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
int serialize_directory(int depth) {
    int status;

    // Emit start of directory record
    stream_data(START_OF_DIRECTORY, depth, STD_RECORD_SIZE);
    debug("START_OF_DIRECTORY, DEPTH: %d, SIZE: %d, PATH: %s", depth, STD_RECORD_SIZE, path_buf);

    // Open directory to be traversed
    DIR *dir = opendir(path_buf);
    if (dir == NULL) return -1;

    // Traverse directory inside a while loop
    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        // Skip "." & ".." directories
        if (string_compare(de->d_name, ".") + string_compare(de->d_name, "..") > -2) {
            continue;
        }

        // Update path_buf to given cursor de; return -1 if error
        if (path_push(de->d_name) == -1) return -1;

        // Emit directory entry record header
        int entry_name_size = string_length(de->d_name);
        off_t record_size = STD_RECORD_SIZE + STD_METADATA_SIZE + entry_name_size;
        stream_data(DIRECTORY_ENTRY, depth, record_size);
        debug("DIRECTORY_ENTRY, DEPTH: %d, SIZE: %ld, PATH: %s", depth, record_size, path_buf);

        // Emit metadata
        struct stat stat_buf;
        stat(path_buf, &stat_buf);
        stream_metadata(stat_buf.st_mode, stat_buf.st_size);

        // Emit file/directory name
        for (int i = 0; i < entry_name_size; i++) {
            putchar(*(de->d_name + i));
        }

        // Emit next record basis file type
        if (S_ISDIR(stat_buf.st_mode)) {
            status = serialize_directory(depth + 1);
            if (status == -1) return -1;
        } else if (S_ISREG(stat_buf.st_mode)) {
            status = serialize_file(depth, stat_buf.st_size);
            if (status == -1) return -1;
        } else {
            error("Could not serialize file: %s\nThis file type is currently not supported in serialization", path_buf);
            return -1;
        }
        // Update path_buf to end of directory
        if (path_pop() == -1) return -1;
    }

    // Close directory
    closedir(dir);

    // Emit end of directory record
    stream_data(END_OF_DIRECTORY, depth, STD_RECORD_SIZE);
    debug("END_OF_DIRECTORY, DEPTH: %d, SIZE: %d, PATH: %s", depth, STD_RECORD_SIZE, path_buf);

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
    // Emit file data record header
    off_t record_size = size + STD_RECORD_SIZE;
    stream_data(FILE_DATA, depth, record_size);
    debug("FILE DATA; DEPTH: %d, SIZE: %ld, PATH: %s", depth, record_size, path_buf);

    // Open file initialized in path_buf; return -1 if error in opening file
    FILE *f = fopen(path_buf, "r");
    if (f == NULL) {
        error("Error in opening file %s", path_buf);
        return -1;
    }

    // Emit file data
    long int count = 0;
    char ch;
    while ((ch = fgetc(f)) != EOF) {
        putchar(ch);
        count += 1;
    }

    // Close file
    fclose(f);

    // Check if correct number of bytes were emitted 
    if (count != size) {
        error("Error in transmitting file %s\nShould have emitted %ld byte(s) but emitted %ld byte(s)", path_buf, size, count);
        return -1;
    }

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
    int depth = 0;
    int status;

    // Emit start transmission record
    stream_data(START_OF_TRANSMISSION, depth, STD_RECORD_SIZE);
    debug("START_OF_TRANSMISSION, DEPTH: %d, SIZE: %d, PATH: %s", depth, STD_RECORD_SIZE, path_buf);

    // Call serialize_directory
    status = serialize_directory(depth + 1);
    if (status == -1) return -1; // Exit if status is -1

    // Emit end transmission record
    stream_data(END_OF_TRANSMISSION, depth, STD_RECORD_SIZE);
    debug("END_OF_TRANSMISSION, DEPTH: %d, SIZE: %d, PATH: %s", depth, STD_RECORD_SIZE, path_buf);

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
    int depth = 0;
    int status;

    int record_type;
    int record_depth;
    int record_size;

    // Read start transmission record; return -1 in case of error
    status = read_record_header(&record_type, &record_depth, &record_size);
    if (status == -1) return -1;

    // Validate if start transmission record entry matches expectation 
    if ((record_type == START_OF_TRANSMISSION) &&
        (record_depth == depth) &&
        (record_size == STD_RECORD_SIZE)) {
        debug("START_OF_TRANSMISSION, DEPTH: %d, SIZE: %d, PATH: %s", record_depth, record_size, path_buf);
    } else {
        error("Error in start transmission record");
        error("Expected type %d but got %d", START_OF_TRANSMISSION, record_type);
        error("Expected depth %d but got %d", depth, record_depth);
        error("Expected size %d but got %d", STD_RECORD_SIZE, record_size);
        return -1;
    }

    // Call deserialize_directory
    status = deserialize_directory(record_depth + 1);
    if (status == -1) return -1; // Exit if status is -1

    // Read end transmission record; return -1 in case of error
    status = read_record_header(&record_type, &record_depth, &record_size);
    if (status == -1) return -1;

    // Validate if end transmission record entry matches expectation 
    if ((record_type == END_OF_TRANSMISSION) &&
        (record_depth == depth) &&
        (record_size == STD_RECORD_SIZE)) {
        debug("END_OF_TRANSMISSION, DEPTH: %d, SIZE: %d, PATH: %s", record_depth, record_size, path_buf);
    } else {
        error("Error in end transmission record");
        error("Expected type %d but got %d", START_OF_TRANSMISSION, record_type);
        error("Expected depth %d but got %d", depth, record_depth);
        error("Expected size %d but got %d", STD_RECORD_SIZE, record_size);
        return -1;
    }

    // Check if EOF is reached on stdin
    if (getchar() != EOF) {
        clear_input_buffer();
        error("Input did not end after end transmission record");
        return -1;
    }
    
    return 0;
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
    debug("Entering validargs...");
    debug("Arguments to validargs -> argc: %d, %p", argc, argv);

    int validation_status = 0;    // Default to valid arguments
    int check_args = 1;           // Validation to start from 1st argument
    char mode = '\0';             // Mode to store the operation s|d

    // Insufficient arguments case
    if (argc == 1) {
        validation_status = -1;
        error("Insufficient arguments");
    }

    // Loop through the command-line arguments
    while (argc - check_args > 0) {
        debug("Checking argment %d: %s", check_args, *(argv + check_args));
        // Check for positional argument (must be -h|-s|-d)
        if (check_args == 1) {
            if (string_compare(*(argv + 1), "-h") == 0) {
                global_options |= (1 << 0);
                debug("Detected -h flag");
                break;
            } else if (string_compare(*(argv + 1), "-s") == 0) {
                mode = 's';
                global_options |= (1 << 1);
                debug("Detected -s flag");
            } else if (string_compare(*(argv + 1), "-d") == 0) {
                mode = 'd';
                global_options |= (1 << 2);
                debug("Detected -d flag");
            } else {
                validation_status = -1;
                error("Detected invalid positional argument: %s", *(argv + 1));
                break;
            }
            check_args += 1;
            continue;
        }

        // Check for -p flag (DIR follows -p flag)
        if (string_compare(*(argv + check_args), "-p") == 0) {
            if (argc - check_args - 1 == 0) {
                validation_status = -1;
                error("No input passed for -p argument");
                break;
            }
            debug("Detcted -p flag with arg: %s", *(argv + check_args + 1));
            check_args += 2;
            continue;
        }

        // Check for -c flag (valid only for mode 'd')
        if (string_compare(*(argv + check_args), "-c") == 0) {
            if (mode == 'd') {
                global_options |= (1 << 3);
                check_args += 1;
                debug("Detected -c flag for deserialization");
                continue;
            } else {
                validation_status = -1;
                error("Detected -c flag for invalid mode");
                break;
            }
        }

        // Break for invalid argument
        validation_status = -1;
        error("Detected invalid argument: %s", *(argv + check_args));
        break;
    }
    return validation_status;
}
