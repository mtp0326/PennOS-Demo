#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdbool.h>
#include <string.h>
#include "globals.h"
#include "kernel.h"

/*== system call functions for interacting with PennOS process creation==*/
/**
 * @brief Create a child process that executes the function `func`.
 * The child will retain some attributes of the parent.
 *
 * @param func Function to be executed by the child process.
 * @param argv Null-terminated array of args, including the command name as
 * argv[0].
 * @param fd0 Input file descriptor.
 * @param fd1 Output file descriptor.
 * @return pid_t The process ID of the created child process.
 * // need to define error output?
 */
pid_t s_spawn(void* (*func)(void*), char* argv[], int fd0, int fd1);

pid_t s_spawn_nice(void* (*func)(void*),
                   char* argv[],
                   int fd0,
                   int fd1,
                   unsigned int priority);

/**
 * @brief Wait on a child of the calling process, until it changes state.
 * If `nohang` is true, this will not block the calling process and return
 * immediately.
 *
 * @param pid Process ID of the child to wait for.
 * @param wstatus Pointer to an integer variable where the status will be
 * stored.
 * @param nohang If true, return immediately if no child has exited.
 * @return pid_t The process ID of the child which has changed state on success,
 * -1 on error.
 */
pid_t s_waitpid(pid_t pid, int* wstatus, bool nohang);

/**
 * @brief Send a signal to a particular process.
 *
 * @param pid Process ID of the target proces.
 * @param signal Signal number to be sent.
 * @return 0 on success, -1 on error.
 */
int s_kill(pid_t pid, int signal);

/**
 * @brief Unconditionally exit the calling process.
 */
void s_exit(void);

/*============================ system calls for interacting with the scheduler
 * ===================================*/

/**
 * @brief Set the priority of the specified thread.
 *
 * @param pid Process ID of the target thread.
 * @param priority The new priorty value of the thread (0, 1, or 2)
 * @return 0 on success, -1 on failure.
 */
int s_nice(pid_t pid, int priority);

/**
 * @brief Suspends execution of the calling proces for a specified number of
 * clock ticks.
 *
 * This function is analogous to `sleep(3)` in Linux, with the behavior that the
 * system clock continues to tick even if the call is interrupted. The sleep can
 * be interrupted by a P_SIGTERM signal, after which the function will return
 * prematurely.
 *
 * @param ticks Duration of the sleep in system clock ticks. Must be greater
 * than 0.
 */
void s_sleep(unsigned int ticks);

/*== system call functions for interacting with PennOS filesystem ==*/
/**
 * @brief
 *
 * @param fname
 * @param mode
 * @return int
 */
int s_open(const char* fname, int mode);

/**
 * @brief
 *
 * @param fd
 * @param n
 * @param buf
 * @return ssize_t
 */
ssize_t s_read(int fd, int n, char* buf);

/**
 * @brief
 *
 * @param fd
 * @param str
 * @param n
 * @return ssize_t
 */
ssize_t s_write(int fd, const char* str, int n);

/**
 * @brief
 *
 * @param fd
 * @return int
 */
int s_close(int fd);

/**
 * @brief
 *
 * @param fname
 * @return int
 */
int s_unlink(const char* fname);

/**
 * @brief Construct a new s lseek object
 *
 * @param fd
 * @param offset
 * @param whence
 */
off_t s_lseek(int fd, int offset, int whence);

/**
 * @brief Construct a new s ls object
 *
 * @param filename
 */
int s_ls(const char* filename);

#endif