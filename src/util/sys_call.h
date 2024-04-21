#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdbool.h>
#include <string.h>
#include "error.h"
#include "globals.h"
#include "kernel.h"
#include "pennfat_kernel.h"
#include "shellbuiltins.h"

#define STATUS_EXITED 0x00
#define STATUS_STOPPED 0x01
#define STATUS_SIGNALED 0x02

#define P_WIFEXITED(status) (((status) & 0xFF) == STATUS_EXITED)
#define P_WIFSTOPPED(status) (((status) & 0xFF) == STATUS_STOPPED)
#define P_WIFSIGNALED(status) (((status) & 0xFF) == STATUS_SIGNALED)

/**
 * @enum log_message_t
 * @brief This is an enum used to specify which log message should be added for
 * \ref s_write_log.
 *
 */
typedef enum {
  SCHEDULE,
  CREATE,
  EXIT,
  SIGNAL,
  ZOMBIE,
  ORPHAN,
  WAIT,
  NICE,
  BLOCK,
  UNBLOCK,
  STOP,
  CONTINUE
} log_message_t;

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

/**
 * @brief Create a child process that executes the function `func`, with a
 * specified priority. This is an exact copy of \ref s_spawn except that the
 * priority of the created process can be specificed at creation.
 *
 * @param func Function to be executed by the child process.
 * @param argv Null-terminated array of args, including the command name as
 * argv[0].
 * @param fd0 Input file descriptor.
 * @param fd1 Output file descriptor.
 * @return pid_t The process ID of the created child process.
 */
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
 *
 * This will set the process state to zombied, adjust its state within the
 * scheduler structures, and kill all child proceseses. (not done).
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
 *
 * @return int Returns 0 on success, -1 on failure and sets errno.
 */
int s_sleep(unsigned int ticks);

/***** CUSTOM SYSCALLS FOR SCHEDULER*/

/**
 * @brief Spawns and waits for a process, combining s_spawn and s_waitpid.
 *
 * This generalizes the control loop of spawning a process, waiting on it via
 * s_waitpid (depending on whether it was a background process) and then
 * cleaning it up, into one function call. Used to call most shell functions
 * (b_functions).
 *
 * @param func The function to be executed.
 * @param argv The argument to that function that is passed into the \ref
 * s_spawn call.
 * @param fd0 Input file descriptor.
 * @param fd1 Output file descriptor.
 * @param nohang Whether or not to wait on the process or immediately proceed.
 * @return int Returns 0 on success, -1 on failure and sets errno.
 */
int s_spawn_and_wait(void* (*func)(void*),
                     char* argv[],
                     int fd0,
                     int fd1,
                     bool nohang,
                     unsigned int priority);

/**
 * @brief Finds a process in any state.
 *
 * @param pid
 * @return pcb_t*
 */
pcb_t* s_find_process(pid_t pid);

/**
 * @brief Removes a process in any state.
 *
 * @param pid
 * @return int
 */
int s_remove_process(pid_t pid);

void* s_function_from_string(char* program);

int s_write_log(log_message_t logtype, pcb_t* proc, unsigned int old_nice);

int s_move_process(CircularList* destination, pid_t pid);

/***** CUSTOM SYSCALLS FOR SCHEDULER*/

/**
 * @brief Spawns and waits for a process
 *
 * @param func
 * @param argv
 * @param fd0
 * @param fd1
 * @param nohang
 * @return int
 */
int s_spawn_and_wait(void* (*func)(void*),
                     char* argv[],
                     int fd0,
                     int fd1,
                     bool nohang,
                     unsigned int priority);

/**
 * @brief Checks status of background processes with waitpid(nohang)
 *
 * @param proc
 * @return int
 */
int s_bg_wait(pcb_t* proc);

/**
 * @brief Finds a process in any state.
 *
 * @param pid
 * @return pcb_t*
 */
pcb_t* s_find_process(pid_t pid);

/**
 * @brief Removes a process in any state.
 *
 * @param pid
 * @return int
 */
int s_remove_process(pid_t pid);

void* s_function_from_string(char* program);

int s_write_log(log_message_t logtype, pcb_t* proc, unsigned int old_nice);

int s_move_process(CircularList* destination, pid_t pid);

int s_print_process(CircularList* list);

int s_print_jobs(CircularList* list);

/*== system call functions for interacting with PennOS filesystem ==*/
/**
 * @brief open a file name fname with the mode mode and return a file
descriptor. The allowed modes are as follows:

F_WRITE - writing and reading, truncates if the file exists, or creates it if it
does not exist. Only one instance of a file can be opened in F_WRITE mode; error
if PennOS attempts to open a file in F_WRITE mode more than once
F_READ - open the file for reading only, return an error if the file does not
exist F_APPEND - open the file for reading and writing but does not truncate the
file if exists; additionally, the file pointer references the end of the file.

s_open returns a file descriptor on success and a negative value on error. This
open will initially be done at the kernel level, using a more intricate and
already-implemented kernel level function. If the kernel level function succeeds
and returns a fd, the user level function should also somehow keep track that
such file descriptor is managed by the calling process. This can be done in
multiple ways.

 *
 * @param fname
 * @param mode
 * @return int
 */
int s_open(const char* fname, int mode);

/**
 * @brief read n bytes from the file referenced by fd. On return, s_read returns
 * the number of bytes read, 0 if EOF is reached, or a negative number on error.
 * A kernel level read should occur to perform the actual functionality, but its
 * important to remember that the process’ file descriptor may need to be
 * updated as the position of the file pointer changes.
 *
 * @param fd
 * @param n
 * @param buf
 * @return ssize_t
 */
ssize_t s_read(int fd, int n, char* buf);

/**
 * @brief write n bytes of the string referenced by str to the file fd and
 * increment the file pointer by n. On return, s_write returns the number of
 * bytes written, or a negative value on error. Note that this writes bytes not
 * chars, these can be anything, even '\0' A kernel level write should occur to
 * perform the actual functionality, but its important to remember that the
 * process’ file descriptor may need to be updated as the position of the file
 * pointer changes.
 *
 * @param fd
 * @param str
 * @param n
 * @return ssize_t
 */
ssize_t s_write(int fd, const char* str, int n);

/**
 * @brief close the file fd and return 0 on success, or a negative value on
 * failure. A kernel level close should occur, and on success the local process’
 * file descriptor table should be cleaned up appropriately.
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
 * @brief reposition the file pointer for fd to the offset relative to whence.
 * You must also implement the constants F_SEEK_SET, F_SEEK_CUR, and F_SEEK_END,
 * which reference similar file whences as their similarly named counterparts in
 * lseek(2). A kernel level lseek should occur, and necessary changes to the
 * calling process’ file descriptor table will be necessary.
 *
 * @param fd
 * @param offset
 * @param whence
 */
off_t s_lseek(int fd, int offset, int whence);

/**
 * @brief List the file filename in the current directory. If filename is NULL,
 * list all files in the current directory. Before EC implementations, this
 * should be very simple and could literally be a call a similar k_function.
 *
 * @param filename
 */
void s_ls(const char* filename);

char* s_read_all(const char* filename, int* read_num);

char* s_get_fname_from_fd(int fd);

int s_update_timestamp(const char* source);

off_t s_does_file_exist2(const char* fname);

int s_rename(const char* source, const char* dest);

int s_change_mode(const char* change, const char* filename);

int s_cp_within_fat(char* source, char* dest);

int s_cp_to_host(char* source, char* host_dest);

int s_cp_from_host(char* host_source, char* dest);

#endif
