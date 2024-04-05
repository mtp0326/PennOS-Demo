#ifndef KERNEL_H
#define KERNEL_H

#include <sys/types.h>  //needed for ssize_t, if we use ints, can remove
#include "array.h"
#include "bitmap.h"
#include "clinkedlist.h"
#include "spthread.h"
#include "stdlib.h"

/**
 * @enum process_state_t
 * @brief Defines the possible states of a process in the system.
 *
 * This enumeration lists all the possible states that a process could
 * be in at any given time. It is used within the pcb_t structure to track
 * the current state of each process.
 */
typedef enum {
  RUNNING, /**< Process is currently executing.
                A process enters the RUNNING state when the scheduler
                selects it for execution, typically from the READY state. */

  STOPPED, /**< Process is not executing, but can be resumed. A process
                should only become STOPPED if signaled by s_kill, recieving
              the P_SIGSTOP signal.
  */

  BLOCKED, /**< Process is not executing, waiting for an event to occur.
                A process should only be blocked if it made a call to either
                s_waitpid or s_sleep.
                 */

  ZOMBIED /**< Process has finished execution but awaits resource cleanup.
               A process enters the ZOMBIED state after it has finished
               its execution and is waiting for the parent process to read
               its exit status. If the parent process ever exits prior to
               reading exit status, this process should immediately cleaned
             up.
           */
} process_state_t;

/**
 * @struct pcb_t
 * @brief This structure stores all required information about a running
 * process.
 */
typedef struct pcb_t {
  spthread_t handle; /** @brief This stores a handle to the spthread.  */
  pid_t pid;         /** @brief This stores the PID of the process. */
  pid_t ppid;        /** @brief This stores the PPID of the process. */
  DynamicPIDArray* child_pids; /** @brief This stores a pointer to a
                                  dynamically sized array of child pid_t's. */
  unsigned int priority : 2; /** @brief This the priority level of the process.
                            (0, 1, or 2). */
  process_state_t
      state; /** @brief This is an enum storing the process's current state.*/
  FD_Bitmap* open_fds; /** @brief This stores a bitmap containg all open file
                          descriptors.*/
} pcb_t;

/**
 * @brief Create a new child process, inheriting applicable properties from the
 * parent.
 * @param parent This is a pointer to the process control block of the parent,
 * from which it inherits.
 * @return Reference to the child PCB.
 */
pcb_t* k_proc_create(pcb_t* parent);

/**
 * @brief Clean up a terminated/finished thread's resources.
 * This may include freeing the PCB, handling children, etc.
 * @param proc This is a pointer to the process control block of a process
 *              that has terminated.
 */
void k_proc_cleanup(pcb_t* proc);

#endif