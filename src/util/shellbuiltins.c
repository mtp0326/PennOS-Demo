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

void* b_logout(void* arg) {
  done = true;
  return NULL;
}
