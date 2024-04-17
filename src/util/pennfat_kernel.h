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
 * @brief
 *
 * @param fd A
 * @param n
 * @param buf
 * @return ssize_t
 */
ssize_t k_read(int fd, int n, char* buf);

/**
 * @brief
 *
 * @param fd
 * @param str
 * @param n
 * @return ssize_t
 */
ssize_t k_write(int fd, const char* str, int n);

/**
 * @brief
 *
 * @param fd
 * @return int
 */
int k_close(int fd);

/**
 * @brief
 *
 * @param fname
 * @return int
 */
int k_unlink(const char* fname);

/**
 * @brief
 *
 * @param fd
 * @param offset
 * @param whence
 * @return off_t
 */
off_t k_lseek(int fd, int offset, int whence);

/**
 * @brief
 *
 * @param filename
 */
void k_ls(const char* filename);

void k_rename(const char* source, const char* dest);

void k_update_timestamp(const char* source);

void k_change_mode(const char* change, const char* filename);

char* k_read_all(const char* filename, int* read_num);

bool is_file_name_valid(char* name);

#endif