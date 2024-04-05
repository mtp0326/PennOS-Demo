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
  char* fname_copy2 = strdup(fname);
  int empty_fat_index = get_first_empty_fat_index();
  struct file_descriptor_st* opened_file;
  struct directory_entries* new_de;

  // F_WRITE
  if (mode == 0) {
    struct directory_entries* dir_entry = does_file_exist(fname);
    if (dir_entry != NULL) {  // file already exists (truncate)
      if (dir_entry->perm == 6) {
        fd_counter--;
        perror(
            "k_open: F_WRITE error: attempted to open a file in F_WRITE mode "
            "more than once");
        exit(EXIT_FAILURE);
      } else {
        fprintf(stderr, "dir entry name: %s\n", dir_entry->name);
        fprintf(stderr, "dir entry first block: %d\n", dir_entry->firstBlock);
        // add to global fd table
        opened_file = create_file_descriptor(curr_fd, fname_copy, 0, 0);
        global_fd_table[curr_fd] = *opened_file;  // update fd table
        // truncate
        int start_fat_index = dir_entry->firstBlock;
        int curr = start_fat_index;
        while (fat[curr] != 0xFFFF) {
          int next = fat[curr];
          fat[curr] = 0x0000;
          curr = next;
        }
        fat[curr] = 0x0000;
        fat[start_fat_index] = 0xFFFF;
      }
    } else {  // file doesn't exist
      // create the "file": add directory entry in root directory
      fprintf(stderr, "hereeeee\n");
      fat[empty_fat_index] = 0xFFFF;
      opened_file = create_file_descriptor(curr_fd, fname_copy, 0, 0);
      global_fd_table[curr_fd] = *opened_file;  // update fd table
      new_de = create_directory_entry(fname_copy2, 0, empty_fat_index, 1, 6,
                                      time(NULL));
      // fs_fd should already be at next open location in root directory
      // (lseeked in does_file_exist())
      if (write(fs_fd, new_de, sizeof(struct directory_entries)) !=
          sizeof(struct directory_entries)) {
        fd_counter--;
        perror("k_open: write error");
        exit(EXIT_FAILURE);
      }
    }
  } else if (mode == 1) {                  // F_READ
    if (does_file_exist(fname) != NULL) {  // open file: add it to fd table
      opened_file = create_file_descriptor(curr_fd, fname_copy, 1, 0);
      global_fd_table[curr_fd] = *opened_file;
    } else {
      fd_counter--;
      perror("k_open: f_read: file does not exist");
    }
  } else if (mode == 2) {  // F_APPEND
  }
  return curr_fd;
}

// // helper that determines if a file is already opened in F_WRITE mode
// bool file_already_write() {
//   return false;
// }

// helper that traverses root directory block by block to check if fname file
// exists return: the directory entry struct with name fname (NULL if not found)
// also moves fs_fd to the end of the root directory
struct directory_entries* does_file_exist(const char* fname) {
  bool found = false;
  struct directory_entries* temp;
  lseek_to_root_directory();  // seek to root directory
  off_t current_offset = lseek(fs_fd, 0, SEEK_CUR);
  fprintf(stderr, "offset: %ld\n", current_offset);
  // following links in the fat, for each block: get the offset, then check each
  // directory name
  int curr = 1;
  int num_directories_per_block =
      block_size / 64;  // each directory entry fixed size of 64 bytes
  while (1) {
    if (fat[curr] != 0xFFFF) {  // check if we are at last block
      fprintf(stderr, "0xffffffff\n");
      for (int i = 0; i < num_directories_per_block;
           i++) {  // check each directory in block
        if (!found) {
          temp = malloc(sizeof(struct directory_entries));
          read(fs_fd, temp, sizeof(struct directory_entries));
          if (strcmp(temp->name, fname) == 0) {
            found = true;
          } else if (i == num_directories_per_block - 1) {
            break;
          }
        }
        lseek(fs_fd, 64,
              SEEK_CUR);  // move to the next directory entry in block
      }
    } else {
      // last block case (still need to check, guaranteed to either return true
      // or false)
      for (int i = 0; i < num_directories_per_block;
           i++) {  // check each directory in block
        if (!found) {
          temp = malloc(sizeof(struct directory_entries));
          off_t current_offset3 = lseek(fs_fd, 0, SEEK_CUR);
          fprintf(stderr, "offset3: %ld\n", current_offset3);
          read(fs_fd, temp, sizeof(struct directory_entries));
          if (strcmp(temp->name, fname) == 0) {
            fprintf(stderr, "name: %s\n", temp->name);
            found = true;
          } else if (i == num_directories_per_block - 1) {
            found = false;
          }
          lseek(fs_fd, -(sizeof(struct directory_entries)), SEEK_CUR);
        }
        lseek(fs_fd, -(sizeof(struct directory_entries)), SEEK_CUR);
        // check if we are at the end of root directory (marked with name[0] =
        // 0)
        off_t current_offset4 = lseek(fs_fd, 0, SEEK_CUR);
        fprintf(stderr, "offset4: %ld\n", current_offset4);
        unsigned char buffer[1];
        if (read(fs_fd, buffer, 1) != 1) {
          perror("does file exist: read error");
          exit(EXIT_FAILURE);
        }
        lseek(fs_fd, -1, SEEK_CUR);
        fprintf(stderr, "here1!\n");
        if (buffer[0] == 0) {
          fprintf(stderr, "here!\n");
          break;
        } else {
          if (i ==
              num_directories_per_block - 1) {  // at last directory entry of
                                                // last block and still occupied
            int next_fat_block = get_first_empty_fat_index();
            int offset1 = get_offset_size(next_fat_block, 0);
            lseek(fs_fd, offset1, SEEK_SET);  // position fs_fd at new block for
                                              // extended root directory
            // update fat
            fat[curr] = next_fat_block;
            fat[next_fat_block] = 0xFFFF;
            break;
          }
          fprintf(stderr, "hereeeee!\n");
          lseek(fs_fd, 64,
                SEEK_CUR);  // move to the next directory entry in block
        }
      }
      break;
    }
    curr = fat[curr];  // move to next block in fat link
    int offset = get_offset_size(curr, 0);
    lseek(fs_fd, offset, SEEK_SET);
  }
  off_t current_offset2 = lseek(fs_fd, 0, SEEK_CUR);
  fprintf(stderr, "offset2: %ld\n", current_offset2);
  if (found) {
    return temp;
  } else {
    free(temp);
    return NULL;
  }
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
  fprintf(stderr, "fat size: %d\n", fat_size);
  fprintf(stderr, "fsfd: %d\n", fs_fd);
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

struct directory_entries* create_directory_entry(const char* name,
                                                 uint32_t size,
                                                 uint16_t firstBlock,
                                                 uint8_t type,
                                                 uint8_t perm,
                                                 time_t mtime) {
  struct directory_entries* new_de = malloc(sizeof(struct directory_entries));

  if (new_de == NULL) {
    fprintf(stderr, "Memory allocation failed\n");
    return NULL;
  }

  strncpy(new_de->name, name, 32);
  new_de->name[31] = '\0';
  new_de->size = size;
  new_de->firstBlock = firstBlock;
  new_de->type = type;
  new_de->perm = perm;
  new_de->mtime = mtime;

  return new_de;
}

ssize_t k_read(int fd, int n, char* buf);

ssize_t k_write(int fd, const char* str, int n) {
  // 0 for READ/WRITE, 1 for READ, and 2 for WRITE, 3 for APPEND
  fprintf(stderr, "check1\n");
  struct file_descriptor_st* curr = get_file_descriptor(fd);
  fprintf(stderr, "check2\n");
  // fd is not a valid open file descriptor
  if (curr == NULL) {
    return -1;
  }

  int mode = curr->mode;
  int offset = curr->offset;
  char* fname = curr->fname;

  // READ_ONLY (F_READ)
  if (mode == 1) {
    return -1;
  }

  struct directory_entries* curr_de = does_file_exist(fname);
  fprintf(stderr, "check3\n");
  uint8_t perm = curr_de->perm;
  uint16_t firstBlock = curr_de->firstBlock;

  // file permission is read only
  if (perm == 4) {
    return -1;
  }

  // this is opened with F_WRITE
  if (mode == 0) {
    // we need to lseek to where we want to write
    uint16_t curr_block = firstBlock;
    // move to the correct block
    while (offset > block_size && curr_block != 0xFFFF) {
      curr_block = fat[curr_block];
      offset -= block_size;
    }

    // we have the correct block number and the offset
    // we should lseek to this offset

    lseek(fs_fd, fat_size + block_size * (curr_block) + offset, SEEK_SET);

    write(fs_fd, str, n);

    return n;
  }
  return -1;
}

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

off_t k_lseek(int fd, int offset, int whence) {
  struct file_descriptor_st* curr_fd = get_file_descriptor(fd);

  if (curr_fd == NULL) {
    return -1;
  }

  struct directory_entries* curr_de = does_file_exist(curr_fd->fname);

  int curr_size = curr_de->size;

  if (curr_de == NULL) {
    return -1;
  }

  if (whence == F_SEEK_SET) {
    // easy part: set the offset
    curr_fd->offset = offset;

    // we have to expand the file
    // if offset is larger than current file size
    if (offset > curr_size) {
      const char* str = calloc(sizeof(uint8_t), offset - curr_size);
      ssize_t num_written = k_write(fd, str, offset - curr_size);
      if (num_written < 0) {
        return -1;
      }
    }

    // resulting offset location
    return curr_fd->offset;
  }

  if (whence == F_SEEK_CUR) {
    // set the offset
    // we start from the current location plus offset bytes
    curr_fd->offset += offset;

    // if the changed offset is larger
    if (curr_fd->offset > curr_size) {
      const char* str = calloc(sizeof(uint8_t), curr_fd->offset - curr_size);
      ssize_t num_written = k_write(fd, str, curr_fd->offset - curr_size);
      if (num_written < 0) {
        return -1;
      }
    }

    return curr_fd->offset;
  }

  if (whence == F_SEEK_END) {
    // set the offset
    curr_fd->offset = curr_size + offset;

    // in this case we will always write to the file
    const char* str = calloc(sizeof(uint8_t), offset);
    ssize_t num_written = k_write(fd, str, offset);
    if (num_written < 0) {
      return -1;
    }

    return curr_fd->offset;
  }

  // invalid whence
  return -1;
}

void k_ls(const char* filename);