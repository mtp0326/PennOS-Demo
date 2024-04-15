#include <errno.h>
#include "pennfat.h"

int main(int argc, char* argv[]) {
  // TODO: register signal handlers
  if (signal(SIGINT, int_handler) == SIG_ERR) {
    perror("error: can't handle sigint\n");
    exit(EXIT_FAILURE);
  }
  bool mounted = false;
  bool mounted = false;
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
        free(parsed);
        exit(EXIT_FAILURE);
      }
      char** args = parsed->commands[0];
      // touch, mv, rm, cat, cp, chmod, ls (implement using k functions)
      if (strcmp(args[0], "touch") == 0) {
        if (!mounted) {
          perror("error: needs to mount fs");
        } else {
          touch(args);
          if (!mounted) {
            perror("error: needs to mount fs");
          } else {
            touch(args);
          }
        }
        else if (strcmp(args[0], "mv") == 0) {
          if (!mounted) {
            perror("error: needs to mount fs");
          } else {
            mv(args);
          }
          if (!mounted) {
            perror("error: needs to mount fs");
          } else {
            mv(args);
          }
        }
        else if (strcmp(args[0], "rm") == 0) {
          if (!mounted) {
            perror("error: needs to mount fs");
          } else {
            rm(args);
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              rm(args);
            }
          }
          else if (strcmp(args[0], "cat") == 0) {
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              if (strcmp(args[1], "-w") == 0) {
                cat_w(args[2]);
              } else if (strcmp(args[1], "-a") == 0) {
                cat_a(args[2]);
              } else {
                cat_file_wa(args);
              }
            }
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              if (strcmp(args[1], "-w") == 0) {
                cat_w(args[2]);
              } else if (strcmp(args[1], "-a") == 0) {
                cat_a(args[2]);
              } else {
                cat_file_wa(args);
              }
            }
          }
          else if (strcmp(args[0], "cp") == 0) {
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              if (strcmp(args[1], "-h") == 0) {
                // cp -h SOURCE DEST
                cp_from_host(args[2], args[3]);
              } else {
                if (strcmp(args[2], "-h") == 0) {
                  // cp SOURCE -h DEST
                  cp_to_host(args[1], args[3]);
                } else {
                  // cp SOURCE DEST
                  cp_within_fat(args[1], args[2]);
                }
              }
            }
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              if (strcmp(args[1], "-h") == 0) {
                // cp -h SOURCE DEST
                cp_from_host(args[2], args[3]);
              } else {
                if (strcmp(args[2], "-h") == 0) {
                  // cp SOURCE -h DEST
                  cp_to_host(args[1], args[3]);
                } else {
                  // cp SOURCE DEST
                  cp_within_fat(args[1], args[2]);
                }
              }
            }
          }
          else if (strcmp(args[0], "chmod") == 0) {
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              chmod(args);
            }
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              chmod(args);
            }
          }
          else if (strcmp(args[0], "ls") == 0) {
            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              ls();
            }

            if (!mounted) {
              perror("error: needs to mount fs");
            } else {
              ls();
            }
          }

          // other test stuff

          else if (strcmp(args[0], "mkfs") == 0) {
            if (args[1] == NULL || args[2] == NULL || args[3] == NULL) {
              perror("error: invalid arguments for mkfs");
              continue;
            }
            errno = 0;
            int blocks_in_fat = strtol(args[2], &ptr, base);
            if (errno == ERANGE) {
              perror("error: invalid arguments for number of blocks in fat");
              continue;
            }
            errno = 0;
            if (blocks_in_fat < 1 || blocks_in_fat > 32) {
              perror("error: number of blocks in fat must be between 1~32");
              continue;
            }
            int block_size_config = strtol(args[3], &ptr, base);
            if (errno == ERANGE) {
              perror(
                  "error: invalid arguments for number of block size "
                  "configuration");
              continue;
            }
            mkfs(args[1], blocks_in_fat, block_size_config);
            // fprintf(stderr, "block size: %d\n", block_size);
            // fprintf(stderr, "block size: %d\n", block_size);
          }
          else if (strcmp(args[0], "mount") == 0) {
            if (args[1] == NULL) {
              perror("error: mount requires an argument");
              continue;
            }
            if (mount(args[1]) != 0) {
              perror("mount error");
            } else {
              mounted = true;
            }
            // fprintf(stderr, "fd table: %s\n", global_fd_table[3].fname);
            // fprintf(stderr, "mode: %d\n", global_fd_table[3].mode);
          }
          else if (strcmp(args[0], "unmount") == 0) {
            if (!mounted) {
              perror("error: unexpected command");
              continue;
            }
            if (unmount() != 0) {
              perror("unmount error");
            } else {
              mounted = false;
            }
          }

          else if (strcmp(args[0], "write") == 0) {
            int fd = strtol(args[1], &ptr, base);
            k_write(fd, args[2], strlen(args[2]));
          }
          else if (strcmp(args[0], "open") == 0) {
            int mode = strtol(args[2], &ptr, base);
            k_open(args[1], mode);
          }
          else if (strcmp(args[0], "read") == 0) {
            int fd = strtol(args[1], &ptr, base);
            char buffer[1000];
            buffer[999] = '\0';
            k_read(fd, 1000, buffer);
            fprintf(stderr, "READ OUTPUT: %s\n", buffer);
          }
          else if (strcmp(args[0], "kunlink") == 0) {
            k_unlink(args[1]);
          }
          else if (strcmp(args[0], "klseek") == 0) {
            int fd = strtol(args[1], &ptr, base);
            int offset = strtol(args[2], &ptr, base);
            int whence = strtol(args[3], &ptr, base);
            k_lseek(fd, offset, whence);
          }

          else {
            fprintf(stderr, "pennfat: command not found: %s\n", args[0]);
          }
          free(parsed);
        }
        free(cmd);
      }
      unmount();
      return EXIT_SUCCESS;
    }
