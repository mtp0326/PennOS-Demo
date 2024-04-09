#ifndef PENNOS_H
#define PENNOS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#undef PROMPT

#ifndef PROMPT
#define PROMPT "penn-os> "
#endif

#define _XOPEN_SOURCE 700

#ifndef MAX_LEN
#define MAX_LEN 4096
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "pennfat.h"
#include "util/globals.h"
#include "util/kernel.h"
#include "util/shellbuiltins.h"
#include "util/sys_call.h"

static bool done = false;
void* function_from_string(char* program);

#endif
