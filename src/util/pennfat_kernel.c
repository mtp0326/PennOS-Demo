#include "pennfat_kernel.h"

int k_open(const char* fname, int mode) {
  // F_WRITE
  if (mode == 0) {
    // F_READ
  } else if (mode == 1) {
    // F_APPEND
  } else if (mode == 2) {
  }
};

ssize_t k_read(int fd, int n, char* buf);

ssize_t k_write(int fd, const char* str, int n);

int k_close(int fd);

int k_unlink(const char* fname);

off_t k_lseek(int fd, int offset, int whence);

void k_ls(const char* filename);