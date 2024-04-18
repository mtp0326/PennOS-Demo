#include "shellbuiltins.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "errno.h"
#include "pennos.h"
#include "sys_call.h"

void* b_sleep(void* arg) {
  char** argv = (char**)arg;
  if (argv[1] == NULL) {
    // code later to add errno and write to the terminal (using pennfat writes),
    // but not today :(
    return NULL;
  }
  int seconds = atoi(argv[1]);  // NOTE: use strtol in later implementation
  // here, as atoi does not define errors
  // (differentiate on 0), will need for errno
  unsigned int ticks = seconds * 10;

  s_sleep(ticks);
  return NULL;
}

void* b_kill(void* arg) {
  char** argv = (char**)arg;
  int signal;
  int i = 1;
  if (strlen(argv[1]) >= 4) {
    signal = P_SIGTER;
  } else {
    if (strcmp(argv[1], "-term") == 0) {
      signal = P_SIGTER;
    } else if (strcmp(argv[1], "-stop") == 0) {
      signal = P_SIGSTOP;

    } else if (strcmp(argv[1], "-cont") == 0) {
      signal = P_SIGCONT;

    } else {
      return NULL;
    }
    i = 2;
  }
  for (; argv[i] != NULL; i++) {
    s_kill(atoi(argv[i]), signal);
  }

  s_exit();
  return NULL;
}

void* b_nice(void* arg) {
  struct parsed_command* parsed = NULL;
  parse_command(arg, &parsed);
  char** args = parsed->commands[0];
  void* (*func)(void*) = s_function_from_string(args[2]);
  unsigned priority = atoi(args[1]);  // USE STROL AND ERRNO
  s_spawn_and_wait(func, &args[2], STDIN_FILENO, STDOUT_FILENO,
                   parsed->is_background, priority);
  return NULL;
}

void* b_nice_pid(void* arg) {
  char** argv = (char**)arg;
  s_nice(atoi(argv[2]), atoi(argv[1]));

  return NULL;
}

void* b_logout(void* arg) {
  done = true;
  return NULL;
}

void* b_ls(void* arg) {
  s_ls(NULL);
  s_exit();
  return NULL;
}

void* b_echo(void* arg) {
  char** argv = (char**)arg;

  int i = 1;
  while (argv[i] != NULL) {
    s_write(current->output_fd, argv[i], strlen(argv[i]));
    s_write(current->output_fd, " ", 1);
    i++;
  }

  s_exit();
  return NULL;
}

void* b_cat(void* arg) {
  char** argv = (char**)arg;
  // int read_num;

  // files arg is not provided
  if (argv[1] == NULL) {
    if ((current->input_fd) == STDIN_FILENO) {
      char contents[4096];
      s_read(STDIN_FILENO, 4096, contents);
      s_write(current->output_fd, contents, strlen(contents));
    } else {
      int read_num;
      char* fname = s_get_fname_from_fd(current->input_fd);
      char* contents = s_read_all(fname, &read_num);
      s_write(current->output_fd, contents, strlen(contents));
    }
  } else {
    // we always ignore the input_fd here

    int i = 1;
    while (argv[i] != NULL) {
      int read_num;
      char* contents = s_read_all(argv[i], &read_num);
      if (contents == NULL) {
        perror("No such file or directory\n");
        i++;
        continue;
      }
      s_write(current->output_fd, contents, strlen(contents));
      i++;
    }
  }

  s_exit();
  return NULL;
}