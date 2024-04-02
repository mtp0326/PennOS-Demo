#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "penn-parser.h"
#include "pennfat.h"
#include "util/pennfat_kernel.h"

#define MAX_LEN 4096
#define MAX_FD_NUM 1024

// function d =eclarations for special routines
static void mkfs(const char* fs_name, int blocks_in_fat, int block_size_config);
static int mount(const char* fs_name);
static int unmount();

// global variables for the currently mounted fs
int size = 0;
int fs_fd;
uint16_t* fat = NULL;

// global file descriptor table
// indexed by the fd
struct file_descriptor_st* global_fd_table = NULL;

void prompt() {
  // display the prompt to the user
  ssize_t prompt_res = write(STDERR_FILENO, PROMPT, strlen(PROMPT));
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
  ssize_t read_res = read(STDIN_FILENO, cmd_temp, MAX_LEN);
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

static void mkfs(const char* fs_name,
                 int blocks_in_fat,
                 int block_size_config) {
  // error handling if there is a currently mounted fs
  if (fat != NULL) {
    perror("unexpected command");
    exit(EXIT_FAILURE);
  }
  // call helper to get FAT size
  int block_size = get_block_size(block_size_config);
  int fat_size = get_fat_size(block_size, blocks_in_fat);

  int num_fat_entries = get_num_fat_entries(block_size, blocks_in_fat);
  int data_size = get_data_size(block_size, num_fat_entries);

  // declared global
  fs_fd = open(fs_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

  if (ftruncate(fs_fd, fat_size + data_size) != 0) {
    perror("mkfs: truncate error");
    exit(EXIT_FAILURE);
  }

  // move back to front
  if (lseek(fs_fd, 0, SEEK_SET) == -1) {
    perror("lseek error");
    exit(EXIT_FAILURE);
  }

  // write metadata to fs (FAT[0])
  uint16_t metadata = (blocks_in_fat << 8) | block_size_config;
  if (write(fs_fd, &metadata, 2) != 2) {
    perror("write error");
    exit(EXIT_FAILURE);
  }

  // fs root dir (FAT[1])
  uint16_t root = 0xFFFF;
  if (write(fs_fd, &root, 2) != 2) {
    perror("write error");
    exit(EXIT_FAILURE);
  }
}

static int mount(const char* fs_name) {
  if (fat != NULL) {
    perror("unexpected command");
    exit(EXIT_FAILURE);
  }
  int fs_fd = open(fs_name, O_RDWR);
  if (fs_fd == -1) {
    perror("fs_fd open error");
    exit(EXIT_FAILURE);
  }
  // get blocks_in_fat and block_size_config from meta data
  int num_blocks = 0;
  int block_config = 0;

  unsigned char buffer[1];
  block_config = read(fs_fd, buffer, 1);

  unsigned char buffer2[1];
  num_blocks = read(fs_fd, buffer2, 1);

  // call helper to get FAT size
  int block_size = get_block_size(block_config);
  int fat_size = block_size * num_blocks;
  size = fat_size;

  // mmap FAT into memory
  fat = mmap(NULL, fat_size, PROT_READ | PROT_WRITE, MAP_SHARED, fs_fd, 0);
  if (fat == MAP_FAILED) {
    perror("FAT mmap error");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "here: %x\n", fat[0]);
  return 0;
}

static int unmount() {
  // error handling if no currently mounted fs
  if (fat == NULL) {
    perror("unexpected command");
    exit(EXIT_FAILURE);
  }

  // munmap(2) to unmount
  if (munmap(fat, size) == -1) {
    perror("munmap failed");
    exit(EXIT_FAILURE);
  }
  fat = NULL;
  size = 0;
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
      exit(EXIT_FAILURE);
    }
  }
  return block_size;
}

int get_fat_size(int block_size, int blocks_in_fat) {
  return block_size * blocks_in_fat;
}

int get_num_fat_entries(int block_size, int blocks_in_fat) {
  return get_fat_size(block_size, blocks_in_fat) / 2;
}

int get_data_size(int block_size, int num_fat_entries) {
  return block_size * (num_fat_entries - 1);
}

int get_offset_size(int block_size,
                    int blocks_in_fat,
                    int block_num,
                    int offset) {
  int fat_size = get_fat_size(block_size, blocks_in_fat);
  int block_offset = block_size * block_num;
  int total_offset = fat_size + block_offset + offset;

  return total_offset;
}

int main(int argc, char* argv[]) {
  // TODO: register signal handlers
  if (signal(SIGINT, int_handler) == SIG_ERR) {
    perror("error: can't handle sigint\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    prompt();
    char* cmd;
    read_command(&cmd);
    if (cmd[0] != '\n') {
      // for strtol
      char* ptr;
      const int base = 10;
      // parse command
      struct parsed_command* parsed;
      if (parse_command(cmd, &parsed) != 0) {
        free_parsed_command(parsed);
        exit(EXIT_FAILURE);
      }

      char** args = parsed->commands[0];
      // touch, mv, rm, cat, cp, chmod, ls (implement using k functions)
      if (strcmp(args[0], "ls") == 0) {
        // TODO: Call your implemented ls() function
      } else if (strcmp(args[0], "touch") == 0) {
        // TODO: Call your implemented touch() function
      } else if (strcmp(args[0], "cat") == 0) {
        // TODO: Call your implemented cat() function
      } else if (strcmp(args[0], "chmod") == 0) {
        // TODO: Call your implemented chmod() function
      } else if (strcmp(args[0], "mkfs") == 0) {
        int blocks_in_fat = strtol(args[2], &ptr, base);
        int block_size_config = strtol(args[3], &ptr, base);
        mkfs(args[1], blocks_in_fat, block_size_config);
      } else if (strcmp(args[0], "mount") == 0) {
        mount(args[1]);
        k_open(args[1], 0);
      } else if (strcmp(args[0], "unmount") == 0) {
        unmount();
      } else {
        fprintf(stderr, "pennfat: command not found: %s\n", args[0]);
      }
      free_parsed_command(parsed);
    }
    free(cmd);
  }
  return EXIT_SUCCESS;
}
