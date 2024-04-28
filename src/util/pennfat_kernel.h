#ifndef PENNFAT_KERNEL_H
#define PENNFAT_KERNEL_H

#include <stdint.h>
#include <sys/types.h>  //needed for ssize_t, if we use ints, can remove
#include "../pennfat.h"
#include "spthread.h"

/************************************************
 *  PENNFAT MACRO DEFINITION
 ***********************************************/

/**
 * @brief open the file for reading only
 */

#define F_READ 0

/**
 * @brief writing and reading, truncates if the file exists, or creates it if it
 * does not exist. Only one instance of a file can be opened in F_WRITE mode;
 * error if attempted to open a file in F_WRITE mode more than once
 */
#define F_WRITE 1

/**
 * @brief open the file for reading and writing but does not truncate the file
 * if exists; additionally, the file pointer references the end of the file.
 */
#define F_APPEND 2

/**
 * @brief Size of the global_fd_table.
 */
#define MAX_FD_NUM 1024

/**
 * @brief Error number for when there does not exist a file with the given file
 * name.
 */
#define FILE_NOT_FOUND -1

/**
 * @brief Error number for when the file name doesn't follow the POSIX standard.
 */
#define INVALID_FILE_NAME -2

/**
 * @brief Error number for when trying to open more than one file descriptor in
 * F_WRITE / F_APPEND mode.
 */
#define MULTIPLE_F_WRITE -3

/**
 * @brief Error number for when trying to use the file descriptor in an invalid
 * way such as writing to F_READ file descriptor.
 */
#define WRONG_PERMISSION -4

/**
 * @brief Error number for when C level system function fails.
 */
#define SYSTEM_ERROR -5

/**
 * @brief Error number for when trying to access or use a deleted file.
 */
#define FILE_DELETED -6

/**
 * @brief Error number for when trying to access or use a invalid file
 * descriptor.
 */
#define INVALID_FILE_DESCRIPTOR -7

/**
 * @brief Error number for when delete a file that is used by some other
 * processes.
 */
#define FILE_IN_USE -8

/**
 * @brief Error number for when the parameter given to the function is invalid.
 */
#define INVALID_PARAMETERS -9

/**
 * @brief Error number for when the filesystem is not mounted but tries to
 * access or use the file system.
 */
#define FS_NOT_MOUNTED -10

/**
* @brief Error number for when the resulting file mode/permission is invalid
*/
#define INVALID_CHMOD -11

/**
 * @enum Whence
 * @brief Defines how the offset will be calculated when using the k_lseek
 * method. For more detail, refer to lseek(2).
 */
enum Whence { F_SEEK_SET, F_SEEK_CUR, F_SEEK_END };

/** @brief PennFAT filesystem that has been mounted to memory using the
 * mmap(2).*/
extern uint16_t* fat;

/** @brief Kernel level global file descriptor table that stores all file
 * descriptor that has been created through out the program's runtime.*/
extern struct file_descriptor_st** global_fd_table;

/** @brief File descriptor number (host system level) for the filesystem that
 * has been mounted to the program.*/
extern int fs_fd;

/** @brief Block size of the currently mounted filesystem that is defined during
 * the mkfs process.*/
extern int block_size;

/** @brief FAT region size of the currently mounted filesystem.*/
extern int fat_size;

/** @brief Calculated value of the total number of FAT entries within the
 * currently mounted filesystem.*/
extern int num_fat_entries;

/** @brief Calculated data region size of the currently mounted filesystem*/
extern int data_size;

/**
 * @struct directory_entries
 * @brief This structure stores all required information about the directory
 * entries that are stored in the root directory.
 */
struct directory_entries {
  char name[32]; /** @brief 32-byte null-terminated file name. name[0] also
serves as a special marker 0: end of directory 1: deleted entry; the file is
also deleted 2: deleted entry; the file is still being used  */

  uint32_t size; /** @brief 4-byte number of bytes in file*/

  uint16_t firstBlock; /** @brief 2-byte number indicating the first block
                          number of the file (undefined if size is zero)*/

  uint8_t type; /** @brief 1-byte number for the type of the file, which will be
one of the following: 0: unknown 1: a regular file 2: a directory file*/

  uint8_t perm; /** @brief file permissions, which will be one of the following:
0: none 2: write only 4: read only 5: read and executable (shell scripts) 6:
read and write 7: read, write, and executable*/

  time_t mtime; /** @brief creation/modification time as returned by time(2) in
                   Linux*/

  uint8_t reserved[16]; /** @brief unused buffer*/
};

/**
 * @struct file_descriptor_st
 * @brief This structure stores all required information about the file
 * descriptor.
 */
struct file_descriptor_st {
  int fd; /** @brief File descriptor number. This is also used as the index for
             the global_fd_table*/
  char* fname; /** @brief File name.*/
  int mode; /** @brief Either F_WRITE, F_READ, F_OPEN. Refer to k_open for more
               details*/
  int offset;  /** @brief Offset from the start of the file.*/
  int ref_cnt; /** @brief Number of processes that has this file descriptor
                  open*/
};

/**
 * @brief Creates a new file_descriptor_st struct, initialized with the values
 * provided by the parameters. For more information of the parameters, refer to
 * struct file_descriptor_st
 *
 * @param fd File descriptor number.
 * @param fname Name of the file.
 * @param mode Either F_WRITE, F_READ, F_APPEND.
 * @param offset Offset to the start of the file.
 *
 * @return A newly created file_descriptor_st struct. NULL on memory allocation
 * error.
 */
struct file_descriptor_st* create_file_descriptor(int fd,
                                                  char* fname,
                                                  int mode,
                                                  int offset);

/**
 * @brief Creates a new directory_entries struct, initialized with the values
 * provided by the parameters. For more information of the parameters, refer to
 * struct directory_entries.
 *
 * @param name Name of the file.
 * @param size Size of the current file.
 * @param firstBlock First FAT block number.
 * @param type Type of the file.
 * @param perm Permission of the file.
 * @param mtime Last modified time.
 *
 * @return A newly created directory_entries struct. NULL on memory allocation
 * error.
 */
struct directory_entries* create_directory_entry(const char* name,
                                                 uint32_t size,
                                                 uint16_t firstBlock,
                                                 uint8_t type,
                                                 uint8_t perm,
                                                 time_t mtime);

/**
 * @brief lseek the file system's offset to the start of the root directory.
 *
 */
void lseek_to_root_directory();

/**
 * @brief Extend the fat region of the given file (marked by the \p start_index
 * ) by one block.
 *
 * @param start_index Start fat index for the given file.
 * @param empty_fat_index The first empty index of the current FAT table. Should
 * be calculated using get_first_empty_fat_index().
 *
 */
void extend_fat(int start_index, int empty_fat_index);

/**
 * @brief Finds and returns the first empty fat index marked as 0x0000.
 *
 * @return first empty fat index.
 */
int get_first_empty_fat_index();

/**
 * @brief Change the offset to the fs_fd to the first open directory entry.
 *
 * @param found
 */
void move_to_open_de(bool found);

/**
 * @brief helper that traverses root directory block by block to check if fname
 * file exists return: the directory entry struct with name fname (NULL if not
 * found) also moves fs_fd to the end of the root directory
 *
 * @param fname Name of the file that we want to check.
 */
struct directory_entries* does_file_exist(const char* fname);

/**
 * @brief Helper function that given a files name, it outputs the offset to the
 * directory entry or negative number if the file isn't found
 *
 * @param fname Name of the file that we want to check.
 */
off_t does_file_exist2(const char* fname);

/**
 * @brief Returns the number of currently open in the global_fd_table with the
 * \p name as the fname.
 *
 * @param name Name of the file that we want to check.
 */
int k_count_fd_num(const char* name);

/**
 * @brief Return the file descriptor struct for the given file descriptor
 * number.
 *
 * @param fd File descriptor number.
 */
struct file_descriptor_st* get_file_descriptor(int fd);

/************************************************
 *  PENNFAT KERNEL LEVEL FUNCTIONS
 ***********************************************/

/**
 * @brief Open file name \p fname with the mode \p mode, and return a file
 * descriptor to that file.
 *
 * This function opens a file specified by the file name \p fname in the
 * mode specified by \p mode and returns a file descriptor associated with
 * the open file that can be used for subsequent file operations.
 *
 * @param fname The name of the file to open. See POSIX standard for allowed
 * names.
 * @param mode The mode with which to open the file. This should specify the
 * access mode (e.g., read, write) and other flags as defined by the
 * operating system. Allowed modes are: write (`F_WRITE`), read (`F_READ`),
 * and append
 * (`F_APPEND`).
 *
 *
 *
 * @return int A non-negative file descriptor on success, or -1 on error and
 * \p errno set.
 *
 * @note The \p mode parameter may only be `F_WRITE`, `F_READ`, or
 * `F_APPEND`. Note that despite their names, write and append support both
 * reading and writing. `F_APPEND`'s file pointer will point to the end of
 * the file rather than the beginning. Both `F_WRITE` and `F_APPEND` will
 * create the named file if it does not already exist.
 *
 * @sa
 * https://www.ibm.com/docs/en/zos/3.1.0?topic=locales-posix-portable-file-name-character-set
 * @details Possible values of \p errno are:
 *              - `EACCES` : // need to fill these in, will expand as
 * further progress
 *              - `ENAMETOOLONG` :
 *
 */
int k_open(const char* fname, int mode);

/**
 * @brief Read n bytes from the file referenced by fd. On return, k_read returns
 * the number of bytes read, 0 if EOF is reached, or a negative number on error.
 *
 * @param fd File descriptor number we are reading from
 * @param n Number of bytes we are reading from \p fd
 * @param buf Buffer where we store the read value
 *
 * @return ssize_t the number of bytes read, 0 if EOF is reached, or a negative
 * number on error
 */
ssize_t k_read(int fd, int n, char* buf);

/**
 * @brief Write n bytes of the string referenced by str to the file fd and
 * increment the file pointer by n. On return, k_write returns the number of
 * bytes written, or a negative value on error.
 *
 * @param fd File descriptor number we are reading to
 * @param str Provided string we want to write to \p fd
 * @param n Number of bytes we are writing
 *
 * @return ssize_t number of bytes written, or a negative value on error.
 */
ssize_t k_write(int fd, const char* str, int n);

/**
 * @brief Close the file fd and return 0 on success, or a negative value on
 * failure.
 *
 * @param fd File descriptor number that needs to be closed
 * @return 0 on success, or a negative value on failure.
 */
int k_close(int fd);

/**
 * @brief Remove the file by freeing the FAT table and zeroing out previously
 * existing data.
 *
 * @param fname Name of the file we want to remove.
 *
 * @return 1 on success. Negative value of failure.
 */
int k_unlink(const char* fname);

/**
 * @brief Reposition the file pointer for fd to the offset relative to whence.
 * Refer to lseek(2) for how whence interacts with the file offset. If the newly
 * calculated offset is creater than the current size of the file, the file
 * expands to match that offset with the newly allocated space filled with 0s.
 *
 * @param fd File descriptor number
 * @param offset Offset value
 * @param whence F_SEEK_SET, F_SEEK_CUR, and F_SEEK_END. Follows the lseek(2)
 * whence mode.
 *
 * @return off_t Newly calculated offset for \p fd
 */
off_t k_lseek(int fd, int offset, int whence);

/**
 * @brief List the file filename in the current directory. If filename is NULL,
 * list all files in the current directory.
 *
 * Similar to posix ls.
 *
 * @param filename Optional parameter. If specified, ls data for the specified
 * file is printed
 * @param fd The file descriptor you want to write the result of k_ls to
 *
 * @return 1 on success, negative value on failure
 */
int k_ls(const char* filename, int fd);

/**
 * @brief Rename \p source to \p dest
 *
 * @param source Source file name.
 * @param dest Destination file name.
 *
 * @return 1 on success. Negative number on failure.
 */
int k_rename(const char* source, const char* dest);

/**
 * @brief Change the timestamp of the file to the current time
 *
 * @param source Source file name.
 *
 * @return 1 on success. Negative number on failure.
 */
int k_update_timestamp(const char* source);

/**
 * @brief Change file mode bits
 *
 * The operator + causes the selected file mode bits to be added to the existing
 * file mode bits of each file; - causes them to be removed.
 *
 * @param change String that determines how the bits are modified.
 * @param filename Name of the file.
 *
 * @return 1 on success. Negative number on failure.
 */
int k_change_mode(const char* change, const char* filename);

/**
 * @brief Reads all contents from the file with the file name \p filename.
 * Outputs the contents as well as update \p read_num to the number of bytes
 * read.
 *
 * @param filename Name of the file we want to read from.
 * @param read_num Pointer to an integer variable that will store the number of
 * bytes read.
 *
 * @return All contents of \p filename in char* format.
 */
char* k_read_all(const char* filename, int* read_num);

/**
 * @brief Checks whether the filename follows the POSIX standard.
 *
 * @param name Filename.
 *
 * @return True if valid. False otherwise.
 */
bool is_file_name_valid(char* name);

/**
 * @brief Returns the filename for the given file descriptor number.
 *
 * @param fd The file descriptor number.
 *
 * @return The file name of the \p fd . NULL is \p fd is invalid.
 */
char* k_get_fname_from_fd(int fd);

/**
 * @brief Copies the contents from \p source to \p dest. Both \p source and \p
 * dest must be files within the PENNFAT system.
 *
 * \p source must exist. If \p dest does not exist, it will be newly created.
 *
 * @param source File name of source. Must be a PennFAT file.
 * @param dest File name of dest. Must be a PennFAT file.
 *
 * @return 1 on success. Negative number on failure.
 */
int k_cp_within_fat(char* source, char* dest);

/**
 * @brief Copies the contents from \p source to \p host_dest \p source must be a
 * file within the PENNFAT system. \p host_dest is a host system file.
 *
 * \p source must exist. If \p host_dest does not exist, it will be newly
 * created.
 *
 * @param source File name of source. Must be a PennFAT file.
 * @param host_dest File name of dest. Must be a host system file.
 *
 * @return 1 on success. Negative number on failure.
 */
int k_cp_to_host(char* source, char* host_dest);

/**
 * @brief Copies the contents from \p host_source to \p dest. \p dest must be a
 * file within the PENNFAT system. \p host_source is a host system file.
 *
 * \p host_source must exist. If \p dest does not exist, it will be newly
 * created.
 *
 * @param host_source File name of source. Must be a host system file.
 * @param dest File name of dest. Must be a PennFAT file.
 *
 * @return 1 on success. Negative number on failure.
 */
int k_cp_from_host(char* host_source, char* dest);

#endif