#ifndef GLOBALS_H
#define GLOBALS_H

#include "clinkedlist.h"
#include "kernel.h"
#include "prioritylist.h"

/**
 * A global array of pointers to the process lists.
 * Each priority level can be accessed via processes[priority].
 * Processes enter this list after creation or via s_kill after receiving
 * P_SIGCONT when stopped.
 */
extern CircularList* processes[3];

/**
 * A global pointer to the process list of blocked processes.
 * Processes enter this list via s_waitpid() or s_sleep().
 */
extern CircularList* blocked;

/**
 * A global pointer to the process list of stopped processes.
 * Processes enter this list via s_kill() after receiving a P_SIGTERM signal.
 */
extern CircularList* stopped;

/**
 * A global pointer to the process list of zombied/terminated processes.
 * These processes enter this list via s_exit() or s_kill(), with the P_SIGTERM
 * signal.
 */
extern CircularList* zombied;

/**
 * A global pointer to the process list of background processes. The processes
 * enter this list when
 */
extern CircularList* bg_list;

/**
 * This is the currently scheduled process. It can be accessed by any method to
 * easily access the current method.
 *
 */
extern pcb_t* current;

/**
 * This the next pid to be used, by k_proc_create(), to ensure that PIDs are
 * not duplicated. This may be rewritten later to reuse/reallocate old processes
 * that have been exited/terminated. I have no strong desire to do so, but do so
 * if you wish.
 *
 */
extern pid_t next_pid;

/**
 * This is to keep track of the job numbers from processes that are in the
 * background or stopped.
 *
 */
extern uint64_t job_id;

/**
 * This is the int representing the file descriptor of the log file, to be used
 * for writing purposes for the logging of events.
 *
 */
extern int logfiledescriptor;

/**
 * This is an int representing the current tick of pennos, to be used for
 * logging purposes.
 *
 */
extern unsigned int tick;

#endif