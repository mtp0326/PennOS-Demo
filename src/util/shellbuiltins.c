#include "shellbuiltins.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "errno.h"
#include "pennos.h"
#include "sys_call.h"
#include "unistd.h"


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


void* b_man(void* arg) {
    char* output = "cat\nsleep\nbusy\necho\nls\ntouch\nmv\ncp\nrm\nchmod\nps\nkill\nzombify\norphanify\nnice\nnice_pid\nman\nbg\nfg\njobs\nlogout\n";
    ssize_t result = s_write(STDOUT_FILENO, output, strlen(output));
    if (result == -1) {
        u_perror("Failed to write to STDOUT");
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

void* b_orphan_child(void* arg) {
  // Please sir,
  // I want some more
  while (1)
    ;
  return NULL;
}

void* b_orphanify(void* arg) {
  s_spawn(b_orphan_child, arg, STDIN_FILENO, STDOUT_FILENO);
  return NULL;
}

void* b_zombie_child(void* arg) {
  // MMMMM Brains...!
  return NULL;
}

void* b_zombify(void* arg) {
  s_spawn(b_zombie_child, arg, STDIN_FILENO, STDOUT_FILENO);
  while (1)
    ;
  return NULL;
}

void* b_logout(void* arg) {
   done = true; 
    return NULL;
}

// FAT LEVEL SHELL FUNCTIONS

void* b_ls(void* arg) {
  s_ls(NULL);
  s_exit();
  return NULL;
}
