The kernel side of PennOS consists of three main aspects. They are described in more details on their own subpages below. 
1. [**The Kernel**](system.md): The kernel refers to the collection of system calls, as well as the overall datastructures and control mechanisms used by the scheduler. 
2. [**The Scheduler**](scheduler.md): The scheduler is the main function that is in charge of deciding which process to schedule/run based on priorities, blocking and unblocking processes, and idling.
3. [**The Shell**](shell.md): The shell is simply a priority zero process that is instantiated at start, and continuously checks for user input to spawn new processes, or modify existing ones.

