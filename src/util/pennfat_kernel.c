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
  char* fname_copy = strdup(fname);

  // F_WRITES
  if (mode == 0) {
    if (does_file_exist(fname) != NULL) {
    } else {
      // create the "file": add directory entry in root directory
      // int empty_fat_index = get_first_empty_fat_index();
      fprintf(stderr, "hereeeee\n");
    }
  } else if (mode == 1) {                  // F_READ
    if (does_file_exist(fname) != NULL) {  // open file: add it to fd table
      struct file_descriptor_st* opened_file =
          create_file_descriptor(curr_fd, fname_copy, 4, 0);
      global_fd_table[curr_fd] = *opened_file;
    } else {
      perror("k_open: f_read: file does not exist");
    }
  } else if (mode == 2) {  // F_APPEND
  }
  return curr_fd;
}

// helper that traverses root directory block by block to check if fname file
// exists
struct directory_entries* does_file_exist(const char* fname) {
  lseek_to_root_directory();  // seek to root directory
  // following links in the fat, for each block: get the offset, then check each
  // directory name
  int curr = 1;
  int num_directories_per_block =
      block_size / 64;  // each directory entry fixed size of 64 bytes
  while (1) {
    if (curr != 0xFFFF) {  // check if we are at last block
      int offset = get_offset_size(curr, 0);
      lseek(fs_fd, offset, SEEK_SET);
      for (int i = 0; i < num_directories_per_block;
           i++) {  // check each directory in block
        struct directory_entries* temp =
            malloc(sizeof(struct directory_entries));
        read(fs_fd, temp, sizeof(struct directory_entries));
        if (strcmp(temp->name, fname) == 0) {
          return temp;
        } else if (i == num_directories_per_block - 1) {
          break;
        }
        lseek(fs_fd, 64,
              SEEK_SET);  // move to the next directory entry in block
      }
    } else {
      // last block case (still need to check, guaranteed to either return true
      // or false)
      for (int i = 0; i < num_directories_per_block;
           i++) {  // check each directory in block
        struct directory_entries* temp =
            malloc(sizeof(struct directory_entries));
        read(fs_fd, temp, sizeof(struct directory_entries));
        if (strcmp(temp->name, fname) == 0) {
          return temp;
        } else if (i == num_directories_per_block - 1) {
          return NULL;
        }
        lseek(fs_fd, 64,
              SEEK_SET);  // move to the next directory entry in block
      }
    }
    curr = fat[curr];  // move to next block in fat link
  }
  return NULL;
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

struct file_descriptor_st* get_file_descriptor(int fd) {
  return (global_fd_table + fd);
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

ssize_t k_read(int fd, int n, char* buf) {
  // check if fd is a valid open file descriptor
  struct file_descriptor_st* curr = get_file_descriptor(fd);

  if (curr == NULL) {
    return -1;
  }

  int mode = curr->mode;
  char* fname = curr->fname;
  int offset = curr->offset;

  // check if we can read the file or not
  if (mode == 0 || mode == 2) {
    return -1;
  }

  // go to root directory to find the first block of the file
  lseek_to_root_directory();

  // find the first block here

  // go to the first block of the reading file + offset

  // start reading

  // while EOF (can be known using the size variable in directory entry) or read
  // all n bytes need to update the offset accordingly

  // read
}

ssize_t k_write(int fd, const char* str, int n);

int k_close(int fd) {
  struct file_descriptor_st* curr = get_file_descriptor(fd);

  // fd is not a valid open file descriptor
  if (curr == NULL) {
    return -1;
  }

  // close it by freeing it and turning it to null
  free(curr);
  curr = NULL;

  return 0;
}

int k_unlink(const char* fname);

off_t k_lseek(int fd, int offset, int whence);

void k_ls(const char* filename);