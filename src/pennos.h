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

#endif
