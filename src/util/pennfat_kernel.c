#include "pennfat_kernel.h"

uint16_t* fat = NULL;
struct file_descriptor_st* global_fd_table = NULL;

/*ALL THE EXTERN VARIABLES ARE INITIALIZED HERE*/
int fs_fd = -1;

int block_size = 0;
int fat_size = 0;
int num_fat_entries = 0;
int data_size = 0;

/*END OF EXTERN VARIABLES*/

// fd_counter starts at three since 0, 1, 2 is reserved
// the invariant is that the current fd_counter is the valid next value
// we increment it afterwards
int fd_counter = 3;

int k_open(const char* fname, int mode) {
  int curr_fd = fd_counter;
  fd_counter++;

  // F_WRITES
  if (mode == 0) {
    fat = NULL;

    // F_READ
  } else if (mode == 1) {
    // unmounted
    if (fs_fd == -1) {
      return -1;
    }

    // first check whether the file exists
    // -> need to read through file directory
    lseek_to_root_directory();

    // we want to read the struct directory_entries from the data region
    // until we either find the same file name or reach the end of the
    // root_directory we know that there exists at maximum, block_size /
    // sizeof(struct directory_entries) entries per block we need to move to the
    // next block when we read the end of the block

    // F_APPEND
  } else if (mode == 2) {
  }
  return curr_fd;
}

int get_first_empty_fat_index() {
  int i = 0;
  // until we find 0x0000
  while (fat[i] != 0x0000) {
    i++;
  }

  return i;
}

void lseek_to_root_directory() {
  lseek(fs_fd, fat_size, SEEK_SET);
}

struct file_descriptor_st* create_file_descriptor(int fd,
                                                  char* fname,
                                                  int mode,
                                                  int offset) {
  struct file_descriptor_st* new_fd = malloc(sizeof(struct file_descriptor_st));

  if (new_fd == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  new_fd->fd = fd;
  new_fd->fname = strdup(fname);
  new_fd->mode = mode;      // 1 for F_WRITE, but prob doesn't matter
  new_fd->offset = offset;  // init as 0
  new_fd->ref_cnt = 1;

  return new_fd;
}

ssize_t k_read(int fd, int n, char* buf);

ssize_t k_write(int fd, const char* str, int n);

int k_close(int fd) {
  struct file_descriptor_st* curr = (global_fd_table + fd);

  // fd is not a valid open file descriptor
  if (curr == NULL) {
    return -1;
  }

  // close it by turning it to null
  free(curr);
  curr = NULL;

  return 0;
}

int k_unlink(const char* fname);

off_t k_lseek(int fd, int offset, int whence);

void k_ls(const char* filename);