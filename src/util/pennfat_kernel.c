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
  struct directory_entries* dir_entry = does_file_exist(fname);
  // F_WRITE
  if (mode == 0) {
    if (dir_entry != NULL) {  // file already exists (truncate)
      for (int i = 0; i < curr_fd - 1; i++) {
        if (global_fd_table[i].mode == WRITE &&
            (strcmp(global_fd_table[i].fname, fname) == 0)) {
          fd_counter--;
          perror(
              "k_open: F_WRITE error: attempted to open a file in F_WRITE mode "
              "more than once");
          exit(EXIT_FAILURE);
        }
      }
      if (dir_entry->perm == 4) {
        perror("k_open: F_WRITE: attempting to open file that is read only");
        exit(EXIT_FAILURE);
      }
      fprintf(stderr, "dir entry name: %s\n", dir_entry->name);
      fprintf(stderr, "dir entry first block: %d\n", dir_entry->firstBlock);
      // add to global fd table
      opened_file = create_file_descriptor(curr_fd, fname_copy, READ_WRITE, 0);
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
      // set directory entry size to 0 since truncated and write to fs_fd to
      // update
      off_t dir_entry_offset =
          does_file_exist2(fname);               // should be lseeked there
      lseek(fs_fd, dir_entry_offset, SEEK_SET);  // get to found directory entry
      struct directory_entries* updated_de =
          create_directory_entry(dir_entry->name, 0, dir_entry->firstBlock,
                                 dir_entry->type, dir_entry->perm, time(NULL));

      write(fs_fd, updated_de, 64);
    } else {  // file doesn't exist
      // create the "file": add directory entry in root directory
      fprintf(stderr, "file doesn't exist: hereeeee\n");
      int efi = get_first_empty_fat_index();  // in case needed to allocate new
                                              // block for root dir
      fat[efi] = 0xFFFF;
      opened_file = create_file_descriptor(curr_fd, fname_copy, READ_WRITE, 0);
      global_fd_table[curr_fd] = *opened_file;  // update fd table
      new_de = create_directory_entry(fname_copy2, 0, efi, 1, 6, time(NULL));
      // fs_fd should already be at next open location in root directory
      // (lseeked in does_file_exist())
      if (write(fs_fd, new_de, sizeof(struct directory_entries)) !=
          sizeof(struct directory_entries)) {
        fd_counter--;
        perror("k_open: write error");
        exit(EXIT_FAILURE);
      }
    }
  } else if (mode == 1) {     // F_READ
    if (dir_entry != NULL) {  // open file: add it to fd table
      if (dir_entry->perm == 2) {
        perror("k_open: F_READ: attempting to open file that is write only");
        exit(EXIT_FAILURE);
      }
      opened_file = create_file_descriptor(curr_fd, fname_copy, READ, 0);
      global_fd_table[curr_fd] = *opened_file;
    } else {
      fd_counter--;
      perror("k_open: f_read: file does not exist");
    }
  } else if (mode == 2) {     // F_APPEND
    if (dir_entry != NULL) {  // file exists, add to fd table in APPEND mode
      // file offset is at end of the file
      opened_file =
          create_file_descriptor(curr_fd, fname_copy, APPEND, dir_entry->size);
      global_fd_table[curr_fd] = *opened_file;
    } else {  // file doesn't exist, create it in root dir with read/write perm,
              // add to fd table in APPEND mode
      fat[empty_fat_index] = 0xFFFF;
      opened_file = create_file_descriptor(curr_fd, fname_copy, APPEND, 0);
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
  }
  return curr_fd;
}

// helper that traverses root directory block by block to check if fname file
// exists return: the directory entry struct with name fname (NULL if not found)
// also moves fs_fd to the end of the root directory
struct directory_entries* does_file_exist(const char* fname) {
  bool found = false;
  struct directory_entries* temp;
  lseek_to_root_directory();  // seek to root directory
  lseek(fs_fd, 0, SEEK_CUR);
  // fprintf(stderr, "offset: %ld\n", current_offset);
  // following links in the fat, for each block: get the offset, then check each
  // directory name
  int curr = 1;
  int num_directories_per_block =
      block_size / 64;  // each directory entry fixed size of 64 bytes
  while (1) {
    if (fat[curr] != 0xFFFF) {  // check if we are at last block
      // fprintf(stderr, "0xffffffff\n");
      for (int i = 0; i < num_directories_per_block;
           i++) {  // check each directory in block
        if (!found) {
          temp = malloc(sizeof(struct directory_entries));
          read(fs_fd, temp, sizeof(struct directory_entries));
          // fprintf(stderr, "temp name: %s\n", temp->name);
          if (strcmp(temp->name, fname) == 0) {
            found = true;
          } else if (i == num_directories_per_block - 1) {
            break;
          }
          lseek(fs_fd, -(sizeof(struct directory_entries)), SEEK_CUR);
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
          lseek(fs_fd, 0, SEEK_CUR);
          // fprintf(stderr, "offset3: %ld\n", current_offset3);
          read(fs_fd, temp, sizeof(struct directory_entries));
          fprintf(stderr, "temp name LAST BLCOK: %s\n", temp->name);
          if (strcmp(temp->name, fname) == 0) {
            fprintf(stderr, "name: %s\n", temp->name);
            found = true;
          } else if (i == num_directories_per_block - 1) {
            found = false;
          }
          lseek(fs_fd, -(sizeof(struct directory_entries)), SEEK_CUR);
        }
        // check if we are at the end of root directory (marked with name[0] =
        // 0)
        lseek(fs_fd, 0, SEEK_CUR);
        // fprintf(stderr, "offset4: %ld\n", current_offset4);
        unsigned char buffer[1];
        if (read(fs_fd, buffer, 1) != 1) {
          perror("does file exist: read error");
          exit(EXIT_FAILURE);
        }
        lseek(fs_fd, -1, SEEK_CUR);
        fprintf(stderr, "here1!\n");
        if (buffer[0] == 0 ||
            buffer[0] == 1) {  // look for first open or deleted entry
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
          // fprintf(stderr, "hereeeee!\n");
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
  lseek(fs_fd, 0, SEEK_CUR);
  // fprintf(stderr, "offset2: %ld\n", current_offset2);
  if (found) {
    return temp;
  } else {
    free(temp);
    return NULL;
  }
}

/*Helper function that given a files name, it outputs the offset to the
 * directory entry or -1 if the file isn't found
 */
off_t does_file_exist2(const char* fname) {
  bool found = false;
  struct directory_entries* temp;
  lseek_to_root_directory();  // seek to root directory
  // it always starts at 1
  int curr_root_block = 1;
  int num_directories_per_block = block_size / 64;
  int read_cnt = 0;
  while (1) {
    // if we found the file we can exit immediately
    if (found) {
      break;
    }

    // we need to move to the next block of the root directory
    if (read_cnt >= num_directories_per_block) {
      // this is the end!
      if (fat[curr_root_block] == 0xFFFF) {
        found = false;
        break;
      }

      curr_root_block = fat[curr_root_block];

      // to the start of that block!
      lseek(fs_fd, get_offset_size(curr_root_block, 0), SEEK_SET);

      read_cnt = 0;
    }

    // we should be at the correct offset for fs_fd to read the next directory
    // entry
    temp = malloc(sizeof(struct directory_entries));
    read(fs_fd, temp, sizeof(struct directory_entries));
    if (strcmp(temp->name, fname) == 0) {
      found = true;
    }

    read_cnt += 1;
  }

  if (found) {
    // since we read it, we want to go back by 64 bytes for the start
    return lseek(fs_fd, -sizeof(struct directory_entries), SEEK_CUR);
  }

  return -1;
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
  // fprintf(stderr, "fat size: %d\n", fat_size);
  // fprintf(stderr, "fsfd: %d\n", fs_fd);
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

ssize_t k_read(int fd, int n, char* buf) {
  // returns number of bytes on success, 0 if EOF is reached, -1 on error
  struct file_descriptor_st* curr = get_file_descriptor(fd);
  // fd is not a valid open file descriptor
  if (curr == NULL) {
    perror("k_read error: could not find file descriptor");
    return -1;
  }

  int mode = curr->mode;
  int offset = curr->offset;
  char* fname = curr->fname;

  if (fname[0] == 1) {
    perror("k_read error: attempting to read into a deleted file");
    return -1;
  }

  // WRITE_ONLY (F_WRITE)
  if (mode == WRITE) {
    return -1;
  }
  // find the directory entry from the fd name
  struct directory_entries* curr_de = does_file_exist(fname);
  uint8_t perm = curr_de->perm;
  uint16_t firstBlock = curr_de->firstBlock;
  uint32_t size = curr_de->size;
  if (curr_de == NULL) {
    perror("k_read error: could not find directory entry");
    return -1;
  }

  // file permission is write only
  if (perm == 2) {
    return -1;
  }

  if (offset >= size) {  // already starting at EOF condition
    return 0;
  }

  int total_bytes_read = 0;  // counter for total bytes to see if we hit EOF
  int bytes_left = n;
  int current_offset = offset;

  // move to where we want to read
  uint16_t curr_block = firstBlock;
  int offset_copy = offset;
  // move to the correct block
  while (offset_copy > block_size && curr_block != 0xFFFF) {
    curr_block = fat[curr_block];
    offset_copy -= block_size;
  }
  int total_offset = fat_size + block_size * (curr_block - 1) + offset_copy;

  lseek(fs_fd, total_offset, SEEK_SET);

  while (1) {
    // finished reading
    if (bytes_left <= 0) {
      break;
    }

    if (offset + total_bytes_read > size) {
      return 0;
    }

    // we need to move to the next block of the file
    if (current_offset >= block_size) {
      int next_block = fat[curr_block];
      curr_block = next_block;
      fprintf(stderr, "entered\n");

      // move the offset so that we can write immediately
      lseek(fs_fd, fat_size + block_size * (curr_block - 1), SEEK_SET);

      // reset
      current_offset = 0;
    }
    char buffer[1];
    read(fs_fd, buffer, 1);
    buf[total_bytes_read] = buffer[0];

    bytes_left -= 1;
    total_bytes_read += 1;
    current_offset += 1;
  }

  return total_bytes_read;
}

void extend_fat(int start_index, int empty_fat_index) {
  while (fat[start_index] != 0xFFFF) {
    start_index = fat[start_index];
  }

  fat[start_index] = empty_fat_index;
  fat[empty_fat_index] = 0xFFFF;
}

ssize_t k_write(int fd, const char* str, int n) {
  // 0 for READ/WRITE, 1 for READ, and 2 for WRITE, 3 for APPEND
  struct file_descriptor_st* curr = get_file_descriptor(fd);
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

  if (curr_de == NULL) {
    fprintf(stderr, "k_write: the file doesn't exist?\n");
  }

  if (curr_de->name[0] == 1 || curr_de->name[1] == 2) {
    fprintf(stderr,
            "k_write: trying to write into a file that has been deleted\n");
    return -1;
  }

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

    // we need to make sure when writing, we didn't fill up the current block

    // we have the correct block number and the offset
    // we should lseek to this offset
    lseek(fs_fd, fat_size + block_size * (curr_block - 1) + offset, SEEK_SET);

    int bytes_left = n;
    int bytes_written = 0;
    int current_offset = offset;

    // we need to make sure when writing, we didn't fill up the current block
    while (1) {
      // finished writing all data from str
      if (bytes_left <= 0) {
        break;
      }

      // we need to create a new block for this file
      if (current_offset >= block_size) {
        int empty_fat_index = get_first_empty_fat_index();

        // extend the fat to the empty_fat_index
        extend_fat(firstBlock, empty_fat_index);

        // move the offset so that we can write immediately
        lseek(fs_fd, fat_size + block_size * (empty_fat_index - 1), SEEK_SET);

        // reset
        current_offset = 0;
      }

      write(fs_fd, str + bytes_written, 1);

      bytes_left -= 1;
      bytes_written += 1;
      current_offset += 1;
    }

    // we need to actually write into the file descriptor here
    // create a new file directory
    struct directory_entries* updated_de = create_directory_entry(
        curr_de->name, curr_de->size + bytes_written, curr_de->firstBlock,
        curr_de->type, curr_de->perm, time(NULL));

    off_t de_offset = does_file_exist2(fname);

    lseek(fs_fd, de_offset, SEEK_SET);

    write(fs_fd, updated_de, 64);

    // modify the file descriptor accordingly
    curr->offset += bytes_written;

    // output how much we wrote into the file
    return bytes_written;
  }  // end of mode == 0 (F_WRITE)

  // F_APPEND
  if (mode == APPEND) {
    // we need to move the offset to the eof if this is in F_APPEND mode
    // the offset in the fd doesn't matter here
    uint32_t append_offset = curr_de->size;

    // we need to lseek to where we want to write
    uint16_t curr_block = firstBlock;
    // move to the correct block
    while (append_offset > block_size && curr_block != 0xFFFF) {
      curr_block = fat[curr_block];
      append_offset -= block_size;
    }

    // set to the end of the file
    lseek(fs_fd, fat_size + block_size * (curr_block - 1) + append_offset,
          SEEK_SET);

    int bytes_left = n;
    int bytes_written = 0;
    int current_offset = append_offset;

    // we need to make sure when writing, we didn't fill up the current block
    while (1) {
      // finished writing all data from str
      if (bytes_left <= 0) {
        break;
      }

      // we need to create a new block for this file
      if (current_offset >= block_size) {
        int empty_fat_index = get_first_empty_fat_index();

        // extend the fat to the empty_fat_index
        extend_fat(firstBlock, empty_fat_index);

        // move the offset so that we can write immediately
        lseek(fs_fd, fat_size + block_size * (empty_fat_index - 1), SEEK_SET);

        // reset
        current_offset = 0;
      }

      write(fs_fd, str + bytes_written, 1);

      bytes_left -= 1;
      bytes_written += 1;
      current_offset += 1;
    }

    // we need to actually write into the file descriptor here
    // create a new file directory
    struct directory_entries* updated_de = create_directory_entry(
        curr_de->name, curr_de->size + bytes_written, curr_de->firstBlock,
        curr_de->type, curr_de->perm, time(NULL));

    off_t de_offset = does_file_exist2(fname);

    lseek(fs_fd, de_offset, SEEK_SET);

    write(fs_fd, updated_de, 64);

    // modify the file descriptor accordingly
    curr->offset += bytes_written;

    // output how much we wrote into the file
    return bytes_written;
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

void generate_permission(uint8_t perm, char** permissions) {
  if (perm == 0) {
    *permissions = "----";
  } else if (perm == 2) {
    *permissions = "--w-";
  } else if (perm == 4) {
    *permissions = "-r--";
  } else if (perm == 5) {
    *permissions = "-r-x";
  } else if (perm == 6) {
    *permissions = "-rw-";
  } else if (perm == 7) {
    *permissions = "-rwx";
  }
}

char* formatTime(time_t t) {
  static char buffer[30];  // Static buffer to hold formatted string
  struct tm* tm_info;

  // Convert time_t to a tm struct
  tm_info = localtime(&t);

  // Format the time into the buffer
  strftime(buffer, 30, "%b %d %H:%M:%S %Y", tm_info);

  return buffer;
}

void k_ls(const char* filename) {
  struct directory_entries* temp = malloc(sizeof(struct directory_entries));
  if (filename == NULL) {
    fprintf(stderr, "k_ls: the param is NULL here\n");

    // we need to list everything in the current directory
    lseek_to_root_directory();
    int curr_root_block = 1;
    int num_directories_per_block = block_size / 64;
    int read_cnt = 0;

    while (1) {
      // moving to the next root directory block
      if (read_cnt >= num_directories_per_block) {
        // this is the end!
        if (fat[curr_root_block] == 0xFFFF) {
          break;
        }

        curr_root_block = fat[curr_root_block];

        // to the start of that block!
        lseek(fs_fd, get_offset_size(curr_root_block, 0), SEEK_SET);

        read_cnt = 0;
      }

      read(fs_fd, temp, sizeof(struct directory_entries));

      // end of the directory
      if (temp->name[0] == 0) {
        break;
      }

      // deleted files we don't want!
      if (temp->name[0] == 1 || temp->name[0] == 2) {
        continue;
      }

      char* permissions = "";
      generate_permission(temp->perm, &permissions);

      // first_block permissions file_size last_touched_timestamp file_name
      fprintf(stderr, "%u %s %d %s %s\n", temp->firstBlock, permissions,
              temp->size, formatTime(temp->mtime), temp->name);

      read_cnt += 1;
    }
    return;
  }  // end of if

  // here we have a specific file we want to ls
  fprintf(stderr, "k_ls: the param is %s here\n", filename);

  does_file_exist2(filename);

  read(fs_fd, temp, sizeof(struct directory_entries));

  if (temp->name[0] == 0) {
    return;
  }

  // deleted files we don't want!
  if (temp->name[0] == 1 || temp->name[0] == 2) {
    return;
  }

  char* permissions = "";
  generate_permission(temp->perm, &permissions);

  // first_block permissions file_size last_touched_timestamp file_name
  fprintf(stderr, "%u %s %d %s %s\n", temp->firstBlock, permissions, temp->size,
          formatTime(temp->mtime), temp->name);

  return;
}