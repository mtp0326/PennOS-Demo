#include "sys_call.h"
#include <unistd.h>
#include "stdio.h"

// Helper function to duplicate argv for the child process
char** duplicate_argv(char* argv[]) {
  if (argv == NULL) {
    return NULL;
  }

  // Count the number of arguments
  int argc;
  for (argc = 0; argv[argc] != NULL; argc++)
    ;

  // Allocate space for the argument vector, including NULL terminator
  char** new_argv = (char**)malloc((argc + 1) * sizeof(char*));
  if (new_argv == NULL) {
    return NULL;  // Allocation failed
  }

  // Copy each argument
  for (int i = 0; i < argc; i++) {
    new_argv[i] = strdup(argv[i]);  // strdup allocates and copies a string
    if (new_argv[i] == NULL) {
      // Cleanup in case of failure
      while (--i >= 0)
        free(new_argv[i]);
      free(new_argv);
      return NULL;
    }
  }

  // NULL terminate the array
  new_argv[argc] = NULL;

  return new_argv;
}

void free_argv(char* argv[]) {
  if (argv == NULL) {
    return;  // Nothing to free
  }

  // Free each argument string
  for (int i = 0; argv[i] != NULL; i++) {
    free(argv[i]);
  }

  // Free the argument vector itself
  free(argv);
}

pid_t s_spawn(void* (*func)(void*), char* argv[], int fd0, int fd1) {
  pcb_t* child = k_proc_create(current);
  if (child == NULL) {
    return -1;
  }

  char** child_argv = duplicate_argv(argv);
  if (child_argv == NULL) {
    k_proc_cleanup(child);
    return -1;
  }

  fd_bitmap_set(child->open_fds, fd0);
  fd_bitmap_set(child->open_fds, fd1);

  struct child_process_arg {
    char** argv;

  }* arg = malloc(sizeof(struct child_process_arg));

  if (arg == NULL) {
    k_proc_cleanup(child);
    free(child_argv);
    return -1;
  }
  arg->argv = child_argv;

  add_process(processes[1], child);  // 1 is default priority
  if (spthread_create(&child->handle, NULL, func, child_argv) != 0) {
    k_proc_cleanup(child);
    free_argv(child_argv);
    free(arg);
    return -1;
  }

  child->processname =
      (char*)malloc(sizeof(char) * (strlen(child_argv[0]) + 1));
  strcpy(child->processname, child_argv[0]);

  s_write_log(CREATE, child, -1);

  return (child->pid);
}

pid_t s_spawn_nice(void* (*func)(void*),
                   char* argv[],
                   int fd0,
                   int fd1,
                   unsigned int priority) {
  pcb_t* child = k_proc_create(current);
  if (child == NULL) {
    return -1;
  }

  char** child_argv = duplicate_argv(argv);
  if (child_argv == NULL) {
    k_proc_cleanup(child);
    return -1;
  }

  child->input_fd = fd0;
  child->output_fd = fd1;

  fd_bitmap_set(child->open_fds, fd0);
  fd_bitmap_set(child->open_fds, fd1);

  struct child_process_arg {
    char** argv;

  }* arg = malloc(sizeof(struct child_process_arg));

  if (arg == NULL) {
    k_proc_cleanup(child);
    free(child_argv);
    return -1;
  }
  arg->argv = child_argv;
  child->priority = (priority == -1 ? 1 : priority);

  add_process(processes[(priority == -1 ? 1 : priority)], child);
  if (spthread_create(&child->handle, NULL, func, child_argv) != 0) {
    k_proc_cleanup(child);
    free_argv(child_argv);
    free(arg);
    return -1;
  }

  child->processname =
      (char*)malloc(sizeof(char) * (strlen(child_argv[0]) + 1));
  strcpy(child->processname, child_argv[0]);

  s_write_log(CREATE, child, -1);
  if (priority != -1) {
    s_write_log(NICE, child, 1);
  }
  return (child->pid);
}

pid_t s_waitpid(pid_t pid, int* wstatus, bool nohang) {
  pcb_t* child_pcb;
  if (find_process(processes[0], pid) != NULL) {
    child_pcb = find_process(processes[0], pid);
  } else if (find_process(processes[1], pid) != NULL) {
    child_pcb = find_process(processes[1], pid);
  } else if (find_process(processes[2], pid) != NULL) {
    child_pcb = find_process(processes[2], pid);
  } else {
    return -1;
  }

  // if nohang, return immediately
  if (nohang) {
    if (child_pcb->state == ZOMBIED) {
      *wstatus = 0;
      return pid;
    } else {
      return -1;
    }
  }

  // if child already changed state
  if (child_pcb->statechanged) {
    if (child_pcb->state == ZOMBIED) {
      if (child_pcb->term_signal != -1) {
        *wstatus = STATUS_SIGNALED;
      } else {
        *wstatus = STATUS_EXITED;
      }
    } else if (child_pcb->state == STOPPED) {
      *wstatus = STATUS_STOPPED;
    } else {
      // became blocked? edstem #730 will define spec
    }
    return pid;
  }

  s_write_log(BLOCK, current, -1);

  // else, block self until state change
  current->state = BLOCKED;
  current->waiting_for_change = true;
  current->waiting_on_pid = child_pcb->pid;
  remove_process(processes[current->priority],
                 current->pid);   // remove process from running processes
  add_process(blocked, current);  // add process to list of blocked processes

  spthread_suspend_self();

  child_pcb->statechanged = false;
  if (child_pcb->state == ZOMBIED) {
    if (child_pcb->term_signal != -1) {
      *wstatus = STATUS_SIGNALED;
    } else {
      *wstatus = STATUS_EXITED;
    }
  } else if (child_pcb->state == STOPPED) {
    *wstatus = STATUS_STOPPED;
  } else {
    // became blocked? edstem #730 will define spec
  }

  s_write_log(WAIT, child_pcb, -1);

  return pid;
}

int s_kill(pid_t pid, int signal) {
  pcb_t* process = s_find_process(pid);
  if (process == NULL) {
    // SET ERRNO: PROCESS NOT FOUND
    return -1;
  }

  switch (signal) {
    case P_SIGSTOP:
      s_move_process(stopped, pid);
      process->state = STOPPED;
      process->statechanged = true;
      s_write_log(STOP, process, -1);
      break;
    case P_SIGCONT:
      if (!(process->state == STOPPED)) {
        // SET ERRNO? Ignore?
        return -1;
      }
      process->state = RUNNING;
      process->statechanged = true;
      s_move_process(processes[process->priority], pid);
      s_write_log(CONTINUE, process, -1);
      break;
    case P_SIGTER:
      process->state = ZOMBIED;
      process->statechanged = true;
      process->term_signal = P_SIGTER;
      process->exit_status = 0;
      s_move_process(zombied, pid);
      s_write_log(SIGNAL, process, -1);
      break;
    default:
      // ERRNO: invalid SIGNAL?
      return -1;
      break;
  }
  return 0;
}

void s_exit(void) {
  remove_process(processes[current->priority], current->pid);
  add_process(zombied, current);
  current->state = ZOMBIED;
  current->statechanged = true;

  s_write_log(EXIT, current, -1);

  // TODO: need to kill all children
}

int s_nice(pid_t pid, int priority) {
  pcb_t* pcb = s_find_process(pid);
  if (pcb == NULL) {
    // ERRNO: could not find process
    return -1;
  }
  pcb->priority = priority;
  if (pcb->state == RUNNING) {
    s_move_process(processes[priority], pid);
  }

  return 0;
}

void s_sleep(unsigned int ticks) {
  if (ticks <= 0) {
    return;
  }

  current->ticks_to_wait = ticks;

  current->state = BLOCKED;
  remove_process(processes[current->priority], current->pid);
  add_process(blocked, current);
  spthread_suspend_self();
  s_exit();
  return;
}

int s_spawn_and_wait(void* (*func)(void*),
                     char* argv[],
                     int fd0,
                     int fd1,
                     bool nohang,
                     unsigned int priority) {
  pid_t child = s_spawn_nice(func, argv, fd0, fd1, priority);
  int wstatus = 0;
  s_waitpid(child, &wstatus, nohang);
  if (!nohang) {
    pcb_t* child_pcb = s_find_process(child);
    s_remove_process(child);
    k_proc_cleanup(child_pcb);
    return 0;
  } else {
    return 0;
  }
}

pcb_t* s_find_process(pid_t pid) {
  pcb_t* ret = NULL;
  find_process(zombied, pid);
  find_process(blocked, pid);
  find_process(stopped, pid);
  find_process(processes[0], pid);
  find_process(processes[1], pid);
  find_process(processes[2], pid);

  if (ret == NULL) {
    // SET ERRNO
    return NULL;
  } else {
    return ret;
  }
}

int s_remove_process(pid_t pid) {
  pcb_t* proc = s_find_process(pid);
  if (proc == NULL) {
    // SET ERRNO
    return -1;
  }
  switch (proc->state) {
    case BLOCKED:
      remove_process(blocked, pid);
      return 0;
    case STOPPED:
      remove_process(stopped, pid);
      return 0;
    case ZOMBIED:
      remove_process(zombied, pid);
      return 0;
    case RUNNING:
      remove_process(processes[proc->priority], pid);
      return 0;
    default:
      // SET ERRNO
      return -1;
  }
}

void* s_function_from_string(char* program) {
  // replace as you create/define more function
  if (strcmp(program, "cat") == 0) {
    return NULL;
  } else if (strcmp(program, "sleep") == 0) {
    return b_sleep;
  } else if (strcmp(program, "busy") == 0) {
    return NULL;
  } else if (strcmp(program, "echo") == 0) {
    return NULL;
  } else if (strcmp(program, "ls") == 0) {
    return NULL;
  } else if (strcmp(program, "touch") == 0) {
    return NULL;
  } else if (strcmp(program, "mv") == 0) {
    return NULL;
  } else if (strcmp(program, "cp") == 0) {
    return NULL;
  } else if (strcmp(program, "rm") == 0) {
    return NULL;
  } else if (strcmp(program, "chmod") == 0) {
    return NULL;
  } else if (strcmp(program, "ps") == 0) {
    return NULL;
  } else if (strcmp(program, "kill") == 0) {
    return b_kill;
  } else if (strcmp(program, "zombify") == 0) {
    return NULL;
  } else if (strcmp(program, "orphanify") == 0) {
    return NULL;
  } else {
    return NULL;
  }
}

int s_write_log(log_message_t logtype, pcb_t* proc, unsigned int old_nice) {
  char buf[100];
  switch (logtype) {
    case SCHEDULE:
      sprintf(buf, "[%4u]\tSCHEDULE \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case CREATE:
      sprintf(buf, "[%4u]\tCREATE   \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case SIGNAL:
      sprintf(buf, "[%4u]\tSIGNALED \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case EXIT:
      sprintf(buf, "[%4u]\tEXITED   \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case ZOMBIE:
      sprintf(buf, "[%4u]\tZOMBIE   \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case ORPHAN:
      sprintf(buf, "[%4u]\tORPHAN   \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case WAIT:
      sprintf(buf, "[%4u]\tWAITED   \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case NICE:
      sprintf(buf, "[%4u]\tNICE     \t%4u\t%4u\t%u\t%s\n", tick, proc->pid,
              old_nice, proc->priority, proc->processname);
      break;
    case BLOCK:
      sprintf(buf, "[%4u]\tBLOCKED  \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case UNBLOCK:
      sprintf(buf, "[%4u]\tUNBLOCKED\t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case STOP:
      sprintf(buf, "[%4u]\tSTOPPED  \t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    case CONTINUE:
      sprintf(buf, "[%4u]\tCONTINUED\t%4u\t%4u\t\t%s\n", tick, proc->pid,
              proc->priority, proc->processname);
      break;
    default:
      // SET ERRNO, INVALID LOGTYPE
      return -1;
  }
  if (write(logfiledescriptor, buf, strlen(buf)) == -1) {
    // SET ERRNO, WRITE TO LOGFILE FAILED
    return -1;
  } else {
    return 0;  // exit successfully
  }
}

int s_move_process(CircularList* destination, pid_t pid) {
  pcb_t* pcb = s_find_process(pid);
  if (pcb == NULL) {
    // SET ERRNO?
    return -1;
  }

  switch (pcb->state) {
    case STOPPED:
      remove_process(stopped, pid);
      break;
    case BLOCKED:
      remove_process(blocked, pid);
      break;
    case ZOMBIED:
      remove_process(zombied, pid);
      break;
    case RUNNING:
      remove_process(processes[pcb->priority], pid);
      break;
    default:
      // SET ERRNO? INVALID?
      return -1;
  }
  add_process(destination, pcb);
  return 0;
}

int s_open(const char* fname, int mode) {
  int fd = k_open(fname, mode);

  if (fd == -1) {
    perror("error: s_open: k_open error");
    return -1;
  }
  fd_bitmap_set(current->open_fds, fd);
  return fd;
}

ssize_t s_read(int fd, int n, char* buf) {
  ssize_t ret = k_read(fd, n, buf);
  if (ret == -1) {
    perror("error: s_read: k_read error");
    return -1;
  }
  return ret;
}

ssize_t s_write(int fd, const char* str, int n) {
  ssize_t ret = k_write(fd, str, n);
  if (ret == -1) {
    perror("error: s_write: k_write error");
    return -1;
  }
  return ret;
}

int s_close(int fd) {
  k_close(fd);
  fd_bitmap_clear(current->open_fds, fd);
  return 0;
}

int s_unlink(const char* fname) {
  return k_unlink(fname);
}

off_t s_lseek(int fd, int offset, int whence) {
  return k_lseek(fd, offset, whence);
}

void s_ls(const char* filename) {
  k_ls(filename);
}

char* s_read_all(const char* filename, int* read_num) {
  return k_read_all(filename, read_num);
}

char* s_get_fname_from_fd(int fd) {
  return k_get_fname_from_fd(fd);
}