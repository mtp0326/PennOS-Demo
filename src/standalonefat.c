#include "pennfat.h"

int main(int argc, char* argv[]) {
  // TODO: register signal handlers
  if (signal(SIGINT, int_handler) == SIG_ERR) {
    perror("error: can't handle sigint\n");
    exit(EXIT_FAILURE);
  }

  // create fd_table
  initialize_global_fd_table();

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
        fprintf(stderr, "block size: %d\n", block_size);
      } else if (strcmp(args[0], "mount") == 0) {
        mount(args[1]);
        k_open("f1", 2);
        k_open("f1", 1);
        k_open("f1", 0);
        fprintf(stderr, "fd table: %s\n", global_fd_table[3].fname);
        fprintf(stderr, "mode: %d\n", global_fd_table[3].mode);
        fprintf(stderr, "fd table: %s\n", global_fd_table[4].fname);
        fprintf(stderr, "mode: %d\n", global_fd_table[4].mode);
        fprintf(stderr, "fd table: %s\n", global_fd_table[5].fname);
        fprintf(stderr, "mode: %d\n", global_fd_table[5].mode);
      } else if (strcmp(args[0], "unmount") == 0) {
        unmount();
      } else if (strcmp(args[0], "write") == 0) {
        char* overview =
            "In this assignment you will implement PennOS, your own UNIX-like "
            "operating system. PennOS is designed around subsystems that model "
            "those of standard UNIX. This will include programming a basic "
            "priority scheduler, FAT file system, and user shell "
            "interactions. hahahahahaha";
        k_write(3, overview, strlen(overview) + 1);

      } else {
        fprintf(stderr, "pennfat: command not found: %s\n", args[0]);
      }
      free_parsed_command(parsed);
    }
    free(cmd);
  }
  return EXIT_SUCCESS;
}
