#ifndef PENNFAT_KERNEL_H
#define PENNFAT_KERNEL_H

#include <stdint.h>
#include <sys/types.h>  //needed for ssize_t, if we use ints, can remove
#include "../pennfat.h"
#include "spthread.h"

extern uint16_t* fat;

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
};

/************************************************
 *  PENNFAT KERNEL LEVEL FUNCTIONS
 ***********************************************/

/**
 * @brief Open file name \p fname with the mode \p mode, and return a file
 * descriptor to that file.
 *
 * This function opens a file specified by the file name \p fname in the mode
 * specified by \p mode and returns a file descriptor associated with the open
 * file that can be used for subsequent file operations.
 *
 * @param fname The name of the file to open. See POSIX standard for allowed
 * names.
 * @param mode The mode with which to open the file. This should specify the
 * access mode (e.g., read, write) and other flags as defined by the operating
 * system. Allowed modes are: write (`F_WRITE`), read (`F_READ`), and append
 * (`F_APPEND`).
 *
 *
 *
 * @return int A non-negative file descriptor on success, or -1 on error and \p
 * errno set.
 *
 * @note The \p mode parameter may only be `F_WRITE`, `F_READ`, or `F_APPEND`.
 * Note that despite their names, write and append support both reading and
 * writing. `F_APPEND`'s file pointer will point to the end of the file rather
 * than the beginning. Both `F_WRITE` and `F_APPEND` will create the named file
 * if it does not already exist.
 *
 * @sa
 * https://www.ibm.com/docs/en/zos/3.1.0?topic=locales-posix-portable-file-name-character-set
 * @details Possible values of \p errno are:
 *              - `EACCES` : // need to fill these in, will expand as further
 * progress
 *              - `ENAMETOOLONG` :
 *
 */
int k_open(const char* fname, int mode);

/**
 * @brief
 *
 * @param fd
 * @param n
 * @param buf
 * @return ssize_t
 */
ssize_t k_read(int fd, int n, char* buf);

ssize_t k_write(int fd, const char* str, int n);

int k_close(int fd);

int k_unlink(const char* fname);

off_t k_lseek(int fd, int offset, int whence);

void k_ls(const char* filename);

#endif