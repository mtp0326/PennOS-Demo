#include "unistd.h"
#include "errno.h"
#include "stdio.h"
#include <string.h>


#ifndef ERROR
#define ERROR
/**
 * This is an integer that can be set by various functions to denote the kind of
 * error that occurred. This is similar to that defined by errno.h, except that
 * it is a custom definition.
 *
 */
extern int errno;

// KERNEL LEVEL ERRORS

// S_SPAWN ERRORS
#define EPCBCREATE 0
#define ENOARGS 1
#define EADDPROC 2
#define ETHREADCREATE 3
#define EBITMAP 4


// S_SLEEP ERRORS
#define EINVARG 5

// S_CP_FROM_HOST ERRORS


// FAT LEVEL ERRORS
/**
 * @brief This is an analogue of the perror(3) function. This allows the shell
 * or other function to pass in a message describing what error occured, which
 * the function then concatenates to the default error message based on your
 * errno value. The filesystem and kernel will both set errno and return -1 on
 * system calls if they fail, after which u_perror() can be called.
 *
 * @param message This is the message that will be concatenated to the default
 * error message.
 */
void u_perror(char* message);

#endif
