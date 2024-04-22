#ifndef PENNFAT_H
#define PENNFAT_H

#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "parser.h"
#include "util/pennfat_kernel.h"
#ifndef PROMPT_PENN_FAT
#define PROMPT_PENN_FAT "penn-fat> "
#endif
#ifndef PROMPT_SHELL
#define PROMPT_SHELL "$ "
#endif
#define MAX_LEN 4096
#endif

/**
 * @brief Creates a "filesytem" (file on host device) with name \p fs_name
 * with each of the \p blocks_in_fat blocks of size specified via
 * \p block_size_config .
 *
 * @param fs_name Name of the file system to be created
 * @param blocks_in_fat Number of blocks in the fat
 * @param block_size_config Configuration specifying size of each block in fat
 *
 *
 */
void mkfs(const char* fs_name, int blocks_in_fat, int block_size_config);

/**
 * @brief Mounts the file system specified by \p fs_name via mmap(2)
 *
 * @param fs_name Name of the file system to be mounted
 *
 * @return Returns 0 if successful and -1 if error
 *
 */
int mount(const char* fs_name);

/**
 * @brief Unmounts the currently mounted filesystem
 *
 * @return Returns 0 if successful and -1 if error
 *
 */
int unmount();

/**
 * @brief Helper function to display the prompt to the user
 *
 * @param shell true if shell prompt, false otherwise
 *
 */
void prompt(bool shell);

/**
 * @brief Helper function that reads user input and handles CTRL-D
 *
 */
void read_command();

/**
 * @brief Helper function that handles CTRL-Z
 *
 * @param signo Signal for int handler
 *
 */
void int_handler(int signo);

/**
 * @brief Converts config number to actual blocks size in bytes
 *
 * @param block_size_config The block size config number
 *
 * @return integer representing block size in bytes
 *
 */
int get_block_size(int block_size_config);

/**
 * @brief Computes fat size via block_size and number of blocks in fat
 *
 * @param block_size the size in bytes of each block
 * @param blocks_in_fat number of blocks in fat
 *
 * @return integer representing size of fat in bytes
 *
 */
int get_fat_size(int block_size, int blocks_in_fat);

/**
 * @brief Computes number of fat entries
 *
 * @param block_size the size in bytes of each block
 * @param blocks_in_fat number of blocks in fat
 *
 * @return integer representing number of fat entries
 *
 */
int get_num_fat_entries(int block_size, int blocks_in_fat);

/**
 * @brief Computes size of data region in bytes
 *
 * @param block_size the size in bytes of each block
 * @param num_fat_entries number of fat entries
 *
 * @return integer representing data size in bytes
 *
 */
int get_data_size(int block_size, int num_fat_entries);

/**
 * @brief Gets offset size from start of filesystem to \p block_num with \p
 * offset
 *
 * @param block_num the block number
 * @param offset offset from start if \p block_num
 *
 * @return total offset size in bytes
 *
 */
int get_offset_size(int block_num, int offset);

/**
 * @brief Initializes global fd table with stdin, stdout, and stderr
 *
 */
void initialize_global_fd_table();

/**
 * @brief Implements touch function via k_open.
 * Opens all files specified in user command
 *
 * @param args user command
 *
 */
void touch(char** args);

/**
 * @brief Implements rm function via k_unlink.
 * Removes files specific by user. Errors if file to be removed doesn't exist.
 *
 * @param args user command
 *
 */
void rm(char** args);

/**
 * @brief Implements mv function via k_rename.
 * For command mv f1 f2, f1 is renamed to f2
 * f2 is removed if it already exists
 *
 * @param args user command
 *
 */
void mv(char** args);

/**
 * @brief Implements chmod function via k_change_mode.
 * Changes mode (file permission) of file directory entry
 * Errors if resulting file permission is invalid.
 *
 * @param args user command
 *
 */
void chmod(char** args);

/**
 * @brief Implements cat functions
 * E.g. cat f1 -w f2 writes contents of f1 into f2, cat f1 f2
 * prints contents of f1 and f2 into stdout, cat f1 -a f2, appends
 * contents of f1 into f2
 *
 * @param args user command
 *
 */
void cat_file_wa(char** args);

/**
 * @brief Implements standard ls function
 *
 */
void ls();

/**
 * @brief Implements cp function of copying within the fat by calling
 * k_cp_within_fat
 * Creates \p dest file if it doesn't exist, \p source must exist
 *
 * @param source source file to copy from
 * @param dest destination file to copy into
 *
 * @return -1 on error, 0 if successful
 *
 */
int cp_within_fat(char* source, char* dest);

/**
 * @brief Implements cp function of copying from fat to host by calling
 * k_cp_to_host
 * Creates \p host_dest if it doesn't exist, \p source must exist
 *
 * @param source source file to copy from in fat
 * @param host_dest destination file to copy into in host filesystem
 *
 * @return -1 on error, 1 if successful
 *
 */
int cp_to_host(char* source, char* host_dest);

/**
 * @brief Implements cp function of copying from the host to fat by calling
 * k_cp_from_host
 * Creates \p dest file if it doesn't exist, \p host_source nust exist
 *
 * @param host_source source file to copy from host
 * @param dest destination file to copy into in fat
 *
 * @return -1 on error, 1 if successful
 *
 */
int cp_from_host(char* host_source, char* dest);

/**
 * @brief Implements cat function that reads from terminal and writes
 * to \p output file. Creates \p output file if it doesn't exist.
 *
 * @param output File to be written to
 */
void cat_w(char* output);

/**
 * @brief Implements cat function that reads from terminal and appends
 * to \p output file. Creates \p output file if it doesn't exist.
 *
 * @param output File to be appended to
 */
void cat_a(char* output);
