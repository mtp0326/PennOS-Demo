#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "pennfat.h"
#include "util/pennfat_kernel.h"

int k_open(const char* fname, int mode) {
  // F_WRITE
  if (mode == 0) {
    fat = NULL;
  } else if (mode == 1) {
    // F_APPEND
  } else if (mode == 2) {
  }
  return 0;
}

ssize_t k_read(int fd, int n, char* buf);

ssize_t k_write(int fd, const char* str, int n);

int k_close(int fd);

int k_unlink(const char* fname);

off_t k_lseek(int fd, int offset, int whence);

void k_ls(const char* filename);