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

  char buf[100];
  sprintf(buf, "[%4u]\tCREATE\t%4u\t%4u\t%s\n", tick, child->pid,
          child->priority, child_argv[0]);
  write(logfiledescriptor, buf, strlen(buf));

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
  child->priority = priority;

  add_process(processes[priority], child);
  if (spthread_create(&child->handle, NULL, func, child_argv) != 0) {
    k_proc_cleanup(child);
    free_argv(child_argv);
    free(arg);
    return -1;
  }

  child->processname =
      (char*)malloc(sizeof(char) * (strlen(child_argv[0]) + 1));
  strcpy(child->processname, child_argv[0]);

  char buf[100];
  sprintf(buf, "[%4u]\tCREATE\t%4u\t%4u\t%s\n", tick, child->pid,
          child->priority, child_argv[0]);
  write(logfiledescriptor, buf, strlen(buf));

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

  char buf[100];
  sprintf(buf, "[%4u]\tWAITED\t%4u\t%4u\t%s\n", tick, child_pcb->pid,
          child_pcb->priority, child_pcb->processname);
  write(logfiledescriptor, buf, strlen(buf));

  return pid;
}

int s_kill(pid_t pid, int signal);

void s_exit(void) {
  current->state = ZOMBIED;
  current->statechanged = true;
}

int s_nice(pid_t pid, int priority) {
  pcb_t* pcb;
  if (find_process(processes[0], pid) != NULL) {
    pcb = find_process(processes[0], pid);
  } else if (find_process(processes[1], pid) != NULL) {
    pcb = find_process(processes[1], pid);
  } else if (find_process(processes[2], pid) != NULL) {
    pcb = find_process(processes[2], pid);
  } else {
    return -1;
  }

  remove_process(processes[pcb->priority], pid);
  add_process(processes[priority], pcb);

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