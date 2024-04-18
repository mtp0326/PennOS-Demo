#include "pennfat.h"
#include <unistd.h>

void prompt() {
  // display the prompt to the user
  ssize_t prompt_res =
      k_write(STDERR_FILENO, PROMPT_PENN_FAT, strlen(PROMPT_PENN_FAT));
  // error catching for write

  if (prompt_res < 0) {
    perror("error: prompting");
    exit(EXIT_FAILURE);
  }
}

void read_command(char** cmds) {
  char* cmd_temp;
  cmd_temp = calloc(MAX_LEN, sizeof(char));
  if (cmd_temp == NULL) {
    free(cmd_temp);
    exit(EXIT_FAILURE);
  }
  // read in the user input
  ssize_t read_res = k_read(STDIN_FILENO, MAX_LEN, cmd_temp);
  // error catching for read
  if (read_res < 0) {
    perror("error: reading input");
    exit(EXIT_FAILURE);
  }
  if (read_res == 0) {  // EOF (CTRL-D)
    free(cmd_temp);
    exit(EXIT_SUCCESS);
  }
  *cmds = cmd_temp;
}

void int_handler(int signo) {
  // fprintf(stderr, "entered int handler");
  if (signo == SIGINT) {
    ssize_t new_line = write(STDERR_FILENO, "\n", strlen("\n"));
    if (new_line < 0) {
      perror("error: printing new line upon CTRL+C");
      exit(EXIT_FAILURE);
    }
  }
  prompt();
}

void initialize_global_fd_table() {
  // create an array of file_descriptor_st
  global_fd_table = calloc(1, sizeof(struct file_descriptor_st) * MAX_FD_NUM);

  if (global_fd_table == NULL) {
    perror("Memory allocation failed\n");
    return;
  }

  // stdin
  struct file_descriptor_st* std_in =
      create_file_descriptor(0, "stdin", F_READ, 0);
  global_fd_table[0] = *std_in;

  // stdout
  struct file_descriptor_st* std_out =
      create_file_descriptor(1, "stdout", F_WRITE, 0);

  global_fd_table[1] = *std_out;

  // stderr
  struct file_descriptor_st* std_err =
      create_file_descriptor(2, "stdout", F_WRITE, 0);

  global_fd_table[2] = *std_err;
}

void mkfs(const char* fs_name, int blocks_in_fat, int block_size_config) {
  // error handling if there is a currently mounted fs
  if (fat != NULL) {
    perror("unexpected command");
    // exit(EXIT_FAILURE);
  }
  // call helper to get FAT size
  block_size = get_block_size(block_size_config);
  if (block_size == -1) {
    return;
  }
  fat_size = get_fat_size(block_size, blocks_in_fat);

  num_fat_entries = get_num_fat_entries(block_size, blocks_in_fat);
  data_size = get_data_size(block_size, num_fat_entries);

  // declared global
  fs_fd = open(fs_name, O_RDWR | O_CREAT, 0666);

  if (blocks_in_fat == 32 && block_size_config == 4) {  // handle case of maxfs
    data_size -= block_size;
  }
  if (ftruncate(fs_fd, fat_size + data_size) != 0) {
    perror("mkfs: truncate error");
    // exit(EXIT_FAILURE);
  }

  // move back to front
  if (lseek(fs_fd, 0, SEEK_SET) == -1) {
    perror("lseek error");
    // exit(EXIT_FAILURE);
  }

  // write metadata to fs (FAT[0])
  uint16_t metadata = (blocks_in_fat << 8) | block_size_config;
  if (write(fs_fd, &metadata, 2) != 2) {
    perror("write error");
    // exit(EXIT_FAILURE);
  }

  // fs root dir (FAT[1])
  uint16_t root = 0xFFFF;
  if (write(fs_fd, &root, 2) != 2) {
    perror("write error");
    // exit(EXIT_FAILURE);
  }
  // close(fs_fd);
}

int mount(const char* fs_name) {
  if (fat != NULL) {
    perror("error: exists an already mounted file system");
    return -1;
    // exit(EXIT_FAILURE);
  }
  fs_fd = open(fs_name, O_RDWR);
  if (fs_fd == -1) {
    perror("fs_fd open error");
    return -1;
    // exit(EXIT_FAILURE);
  }
  // get blocks_in_fat and block_size_config from meta data
  int num_blocks = 0;
  int block_config = 0;

  unsigned char buffer[1];
  if (read(fs_fd, buffer, 1) != 1) {
    perror("mount: read error");
    return -1;
    // exit(EXIT_FAILURE);
  }
  block_config = buffer[0];

  unsigned char buffer2[1];
  if (read(fs_fd, buffer2, 1) != 1) {
    perror("mount: read error");
    return -1;
    // exit(EXIT_FAILURE);
  }
  num_blocks = buffer2[0];

  // set externs
  block_size = get_block_size(block_config);
  fat_size = block_size * num_blocks;
  num_fat_entries = get_num_fat_entries(block_size, num_blocks);
  data_size = get_data_size(block_size, num_fat_entries);

  // need to make a helper to re-create fd table

  // mmap FAT into memory
  fat = mmap(NULL, fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
  if (fat == MAP_FAILED) {
    perror("FAT mmap error");
    return -1;
    // exit(EXIT_FAILURE);
  }
  // fprintf(stderr, "here: %x\n", fat[0]);
  return 0;
}

int unmount() {
  // error handling if no currently mounted fs
  if (fat == NULL) {
    perror("error: no file system mounted");
    return -1;
    // exit(EXIT_FAILURE);
  }

  msync(fat, fat_size, MS_SYNC);

  // munmap(2) to unmount
  if (munmap(fat, fat_size) == -1) {
    perror("error: munmap failed");
    return -1;
    // exit(EXIT_FAILURE);
  }
  // close(fs_fd);
  fat = NULL;
  return 0;
}

// helper function that gets the block size
int get_block_size(int block_size_config) {
  int block_size = 0;
  switch (block_size_config) {
    case 0:
      block_size = 256;
      break;
    case 1:
      block_size = 512;
      break;
    case 2:
      block_size = 1024;
      break;
    case 3:
      block_size = 2048;
      break;
    case 4:
      block_size = 4096;
      break;
    default: {
      perror("invalid block size config");
      return -1;
      // exit(EXIT_FAILURE);
    }
  }
  return block_size;
}

// helper function that gets FAT table size
int get_fat_size(int block_size, int blocks_in_fat) {
  return block_size * blocks_in_fat;
}

// helper function that gets number of possible FAT entries
int get_num_fat_entries(int block_size, int blocks_in_fat) {
  return get_fat_size(block_size, blocks_in_fat) / 2;
}

// helper function that gets DATA region size
int get_data_size(int block_size, int num_fat_entries) {
  return block_size * (num_fat_entries - 1);
}

// helper function that gets total offset from beginning of FAT
int get_offset_size(int block_num, int offset) {
  int block_offset = block_size * (block_num - 1);
  int total_offset = fat_size + block_offset + offset;
  return total_offset;
}

void touch(char** args) {
  int i = 1;
  while (args[i] != NULL) {
    if (does_file_exist2(args[i]) != -1) {
      k_update_timestamp(args[i]);
    } else {
      int fd = k_open(args[i], F_WRITE);
      k_close(fd);
    }

    i += 1;
  }
}

void rm(char** args) {
  int i = 1;
  while (args[i] != NULL) {
    k_unlink(args[i]);
    i += 1;
  }
}

void mv(char** args) {
  k_rename(args[1], args[2]);
}

void chmod(char** args) {
  k_change_mode(args[1], args[2]);
}

void cat_file_wa(char** args) {
  int i = 1;
  bool file_flag = false;
  bool name_flag = false;
  bool w = false;
  char* filename;
  while (args[i] != NULL) {
    if (name_flag) {
      filename = args[i];
      name_flag = false;
    }
    if (strcmp(args[i], "-w") == 0) {
      file_flag = true;
      name_flag = true;
      w = true;
    } else if (strcmp(args[i], "-a") == 0) {
      file_flag = true;
      name_flag = true;
    }
    i += 1;
  }
  i = 1;
  if (!file_flag) {
    while (args[i] != NULL) {
      int read_num;
      char* contents = k_read_all(args[i], &read_num);
      if (contents == NULL) {
        free(contents);
        perror("cat error");
        return;
      }
      k_write(STDOUT_FILENO, contents, read_num);
      // fprintf(stdout, "%s\n", contents);
      free(contents);
      i += 1;
    }
  } else {
    int fd;
    if (w) {
      fd = k_open(filename, F_WRITE);
    } else {
      fd = k_open(filename, F_APPEND);
    }
    while (args[i] != NULL && (strcmp(args[i], "-a") != 0) &&
           (strcmp(args[i], "-w") != 0)) {
      int read_num;
      char* contents = k_read_all(args[i], &read_num);
      if (contents == NULL) {
        free(contents);
        k_close(fd);
        perror("cat error");
        return;
      }
      k_write(fd, (char*)contents, read_num);
      free(contents);
      i += 1;
    }
    k_close(fd);
  }
}

void cat_w(char* output) {
  int fd = k_open(output, F_WRITE);
  int BUF_SIZE = 4096;
  ssize_t bytesRead;      // Number of bytes read
  char buffer[BUF_SIZE];  // Buffer to store terminal input
  while ((bytesRead = k_read(STDIN_FILENO, BUF_SIZE, buffer)) > 0) {
    k_write(fd, buffer, bytesRead);
  }

  // Check for read error
  if (bytesRead == -1) {
    perror("Error reading from terminal");
    k_close(fd);
    return;
  }

  k_close(fd);
}

void cat_a(char* output) {
  int fd = k_open(output, F_APPEND);
  int BUF_SIZE = 4096;
  ssize_t bytesRead;      // Number of bytes read
  char buffer[BUF_SIZE];  // Buffer to store terminal input
  while ((bytesRead = k_read(STDIN_FILENO, BUF_SIZE, buffer)) > 0) {
    k_write(fd, buffer, bytesRead);
  }

  // Check for read error
  if (bytesRead == -1) {
    perror("Error reading from terminal");
    k_close(fd);
    return;
  }

  k_close(fd);
}

void ls() {
  k_ls(NULL);
}

void cp_within_fat(char* source, char* dest) {
  // source file must exist

  if (does_file_exist2(source) == -1) {
    perror("error: source does not exist");
    return;
  }

  int dest_fd = k_open(dest, F_WRITE);
  int source_fd = k_open(source, F_READ);

  int read_num;

  char* contents = k_read_all(source, &read_num);

  k_write(dest_fd, contents, read_num);

  k_close(dest_fd);
  k_close(source_fd);
}

void cp_to_host(char* source, char* host_dest) {
  if (does_file_exist2(source) == -1) {
    perror("error: source does not exist");
    return;
  }
  int source_fd = k_open(source, F_READ);

  int read_num;

  char* contents = k_read_all(source, &read_num);

  int host_fd = open(host_dest, O_RDWR | O_CREAT | O_TRUNC, 0777);

  if (write(host_fd, contents, read_num) == -1) {
    perror("error: write to host file failed\n");
  }

  close(host_fd);
  k_close(source_fd);
}

void cp_from_host(char* host_source, char* dest) {
  int host_fd = open(host_source, O_RDWR, 0777);

  if (host_fd == -1) {
    perror("error: host source does not exist or is invalid\n");
    return;
  }

  char buffer[1];
  int dest_fd = k_open(dest, F_WRITE);

  while (read(host_fd, buffer, 1) > 0) {
    k_write(dest_fd, buffer, 1);
  }

  close(host_fd);
  k_close(dest_fd);
}