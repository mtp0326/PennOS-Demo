#ifndef PENNOS_H
#define PENNOS_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif

#define _XOPEN_SOURCE 700

#ifndef MAX_LEN
#define MAX_LEN 4096
#endif

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "penn-parser.h"
#include "util/globals.h"
#include "util/kernel.h"
#include "util/sys_call.h"

#endif
