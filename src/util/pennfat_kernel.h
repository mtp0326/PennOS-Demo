#ifndef PENNFAT_KERNEL_H
#define PENNFAT_KERNEL_H

#include <stdint.h>
#include <sys/types.h>  //needed for ssize_t, if we use ints, can remove
#include "../pennfat.h"
#include "spthread.h"

#define F_READ 0
#define F_WRITE 1
#define F_APPEND 2

#define MAX_FD_NUM 1024

enum Whence { F_SEEK_SET, F_SEEK_CUR, F_SEEK_END };

extern uint16_t* fat;
extern struct file_descriptor_st* global_fd_table;
extern int fs_fd;
extern int block_size;
extern int fat_size;
extern int num_fat_entries;
extern int data_size;

struct directory_entries {
  char name[32];
  uint32_t size;
  uint16_t firstBlock;
  uint8_t type;
  uint8_t perm;
  time_t mtime;
  uint8_t reserved[16];
};

struct file_descriptor_st {
  int fd;
  char* fname;
  int mode;
  int offset;
  int ref_cnt;
};

// helper functions
struct file_descriptor_st* create_file_descriptor(int fd,
                                                  char* fname,
                                                  int mode,
                                                  int offset);
struct directory_entries* create_directory_entry(const char* name,
                                                 uint32_t size,
                                                 uint16_t firstBlock,
                                                 uint8_t type,
                                                 uint8_t perm,
                                                 time_t mtime);
void lseek_to_root_directory();
void extend_fat(int start_index, int empty_fat_index);
int get_first_empty_fat_index();
void move_to_open_de(bool found);
struct directory_entries* does_file_exist(const char* fname);
off_t does_file_exist2(const char* fname);
int k_count_fd_num(const char* name);
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
 */
void k_ls(const char* filename);

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