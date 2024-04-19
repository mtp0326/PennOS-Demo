#ifndef KERNEL_H
#define KERNEL_H

#include <sys/types.h>  //needed for ssize_t, if we use ints, can remove
#include "array.h"
#include "bitmap.h"
#include "clinkedlist.h"
#include "globals.h"
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
 * @brief This is the STOP signal definition to be used by s_kill(). Running
 * processes (ONLY) that receive the P_SIGSTOP signal will become stopped and
 * have their state and process list adjusted accordingly. Note that
 * statechanged will NOT be changed, as this state transition does NOT cause
 * s_waitpid() to return/unblock.
 */

#define P_SIGSTOP 0

/**
 * @brief This is the CONTINUE signal definition to be used by s_kill(). Stopped
 * processes (ONLY) that receive the P_SIGCONT signal will become running and
 * have their state and process list adjusted accordingly. Note that
 * statechanged will NOT be changed, as this state transition does NOT cause
 * s_waitpid() to return/unblock.
 */
#define P_SIGCONT 1

/**
 * @brief This is the TERMINATE signal definition to be used by s_kill(). Any
 * process that receives the P_SIGTER signal will become zombied and
 * have their state and process list adjusted accordingly. Note that
 * statechanged WILL be changed, as this state transition DOES cause
 * s_waitpid() to return/unblock.
 */
#define P_SIGTER 2

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

  int input_fd; /** @brief The input i/o that his process reads data from.*/

  int output_fd; /** @brief The out i/o that his process writes data to*/

  bool statechanged; /** @brief This contains a bool that keeps track of whether
                        or not the process state has changed.*/

  int exit_status; /** @brief Exit status of process, 0 if exited , -1 if
                      not exited*/
  int term_signal; /** @brief Signal number that caused process to terminate, -1
                      if not terminated */
  bool waiting_for_change; /** @brief Bool describing whether or not the process
                              is currently waiting on a process.*/
  pid_t waiting_on_pid;    /** @brief PID of the child the process is currently
                              waiting on, or -1 if none */
  unsigned int ticks_to_wait; /** @brief Ticks remaining to wait, used only for
                                 s_sleep calls */

  char* processname; /** @brief Name of process, to be used for logging and ps*/
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
