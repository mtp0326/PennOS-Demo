For the system/kernel of PennOS, the most important notion is that of the process control block (PCB).

This can be seen in more detail here: \ref pcb_t.

The main idea of the process control block it represents a process, or thread, that can be in several states. Processes can be running, stopped, blocked, or zombied. 

The transitions between processes are mediated via signals sent via \ref s_kill, \ref s_exit, and \ref s_sleep.

The kernel maintains circular linked lists \ref CircularList's of processes in each state. There are 6 in total, one for each state: \ref ZOMBIED, \ref STOPPED, \ref BLOCKED, and 3 for \ref RUNNING, one for each priority level.

As a rule, user level functions should not need to and should not mediate process state transitions, as these will be handled by the scheduler or by system calls.