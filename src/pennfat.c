#include "pennfat.h"

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

void mkfs(const char* fs_name, int blocks_in_fat, int block_size_config) {
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

int mount(const char* fs_name) {
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

int unmount() {
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

int get_first_empty_fat_index() {
  int i = 0;
  // until we find 0x0000
  while (fat[i] != 0x0000) {
    i++;
  }

  return i;
}
