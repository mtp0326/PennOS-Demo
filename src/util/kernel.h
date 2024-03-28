#include "spthread.h"
#include <sys/types.h> //needed for ssize_t, if we use ints, can remove
/**
 * @enum process_state_t
 * @brief Defines the possible states of a process in the system.
 *
 * This enumeration lists all the possible states that a process could
 * be in at any given time. It is used within the pcb_t structure to track the
 * current state of each process.
 */
typedef enum {
    RUNNING,  /**< Process is currently executing. 
                   A process enters the RUNNING state when the scheduler
                   selects it for execution, typically from the READY state. */

    STOPPED,  /**< Process is not executing, but can be resumed. A process
                   should only become STOPPED if signaled by s_kill, recieving the 
                   P_SIGSTOP signal.
     */

    BLOCKED,  /**< Process is not executing, waiting for an event to occur.
                   A process should only be blocked if it made a call to either
                   s_waitpid or s_sleep. 
                    */

    ZOMBIED   /**< Process has finished execution but awaits resource cleanup.
                   A process enters the ZOMBIED state after it has finished
                   its execution and is waiting for the parent process to read
                   its exit status. If the parent process ever exits prior to 
                   reading exit status, this process should immediately cleaned up.  */
} process_state_t;

 

/**
 * @struct pcb_t 
 * @brief This structure stores all required information about a running process.
 */
typedef struct pcb_t {
    spthread_t handle; /**< @brief This stores a handle to the spthread.  */
    pid_t pid; /**< @brief This stores the PID of the process. */
    pid_t ppid; /**< @brief This stores the PPID of the process. */
    // need to store child pids
    int priority; /**< @brief This the priority level of the process. (0, 1, or 2) */
    // need to store all open file descriptors
} pcb_t;


/**
 * @brief Create a new child process, inheriting applicable properties from the parent.
 * @param parent This is a pointer to the process control block of the parent, from which it inherits. 
 * @return Reference to the child PCB.
 */
pcb_t* k_proc_create(pcb_t *parent);

/**
 * @brief Clean up a terminated/finished thread's resources.
 * This may include freeing the PCB, handling children, etc.
 * @param proc This is a pointer to the process control block of a process
*              that has terminated.
 */
void k_proc_cleanup(pcb_t *proc);






/************************************************
 *  PENNFAT KERNEL LEVEL FUNCTIONS
 ***********************************************/



/**
 * @brief Open file name \p fname with the mode \p mode, and return a file descriptor
 *        to that file.
 * 
 * This function opens a file specified by the file name \p fname in the mode specified
 * by \p mode and returns a file descriptor associated with the open file that can be
 * used for subsequent file operations.
 * 
 * @param fname The name of the file to open. See POSIX standard for allowed names.
 * @param mode The mode with which to open the file. This should specify the access
 *        mode (e.g., read, write) and other flags as defined by the operating system.
 *        Allowed modes are: write (`F_WRITE`), read (`F_READ`), and append (`F_APPEND`). 
 *        
 *        
 * 
 * @return int A non-negative file descriptor on success, or -1 on error and \p errno set.
 * 
 * @note The \p mode parameter may only be `F_WRITE`, `F_READ`, or `F_APPEND`. Note that despite
 *       their names, write and append support both reading and writing. `F_APPEND`'s file pointer
 *       will point to the end of the file rather than the beginning. Both `F_WRITE` and `F_APPEND` 
 *       will create the named file if it does not already exist.
 * 
 * @sa https://www.ibm.com/docs/en/zos/3.1.0?topic=locales-posix-portable-file-name-character-set
 * @details Possible values of \p errno are: 
 *              - `EACCES` : // need to fill these in, will expand as further progress
 *              - `ENAMETOOLONG` : 
 *
 */
int k_open(const char *fname, int mode);


/**
 * @brief 
 * 
 * @param fd 
 * @param n 
 * @param buf 
 * @return ssize_t 
 */
ssize_t k_read(int fd, int n, char *buf);



ssize_t k_write(int fd, const char *str, int n);


int k_close(int fd);

int k_unlink(const char *fname);

off_t k_lseek(int fd, int offset, int whence);

void k_ls(const char *filename);



