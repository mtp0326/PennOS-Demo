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

// get the total length of the command line
int get_arg_size(char* argv[]) {
  int i = 0;
  int size = 0;
  while (argv[i] != NULL) {
    size += strlen(argv[i]) + 1;
    i++;
  }
  return size + 1;
}

pid_t s_spawn(void* (*func)(void*), char* argv[], int fd0, int fd1) {
  pcb_t* child = k_proc_create(current);
  if (child == NULL) {
    errno = EPCBCREATE;
    return -1;
  }

  char** child_argv = duplicate_argv(argv);
  if (child_argv == NULL) {
    k_proc_cleanup(child);
    errno = ENOARGS;
    return -1;
  }

  if (!fd_bitmap_set(child->open_fds, fd0)) {
    errno = EBITMAP;
  }

  if (!fd_bitmap_set(child->open_fds, fd1)) {
    errno = EBITMAP;
  }

  struct child_process_arg {
    char** argv;

  }* arg = malloc(sizeof(struct child_process_arg));

  if (arg == NULL) {
    k_proc_cleanup(child);
    free(child_argv);
    errno = ENOARGS;
    return -1;
  }
  arg->argv = child_argv;
  fg_proc = child;

  if (add_process(processes[1], child) == -1) {
    errno = EADDPROC;
    return -1;
  }

  if (spthread_create(&child->handle, NULL, func, child_argv) != 0) {
    k_proc_cleanup(child);
    free_argv(child_argv);
    free(arg);
    errno = ETHREADCREATE;
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
    errno = EPCBCREATE;
    return -1;
  }

  char** child_argv = duplicate_argv(argv);
  if (child_argv == NULL) {
    errno = ENOARGS;
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
    errno = ENOARGS;
    k_proc_cleanup(child);
    free(child_argv);
    return -1;
  }
  arg->argv = child_argv;
  child->priority = (priority == -1 ? 1 : priority);
  fg_proc = child;

  if (add_process(processes[(priority == -1 ? 1 : priority)], child) == -1) {
    errno = EADDPROC;
    k_proc_cleanup(child);
    free_argv(child_argv);
    free(arg);
    return -1;
  }
  if (spthread_create(&child->handle, NULL, func, child_argv) != 0) {
    errno = ETHREADCREATE;
    k_proc_cleanup(child);
    free_argv(child_argv);
    free(arg);
    return -1;
  }
  free(arg);
  child->processname =
      (char*)malloc(sizeof(char) * (strlen(child_argv[0]) + 1));
  strcpy(child->processname, child_argv[0]);

  int arg_size = get_arg_size(argv);
  child->cmd_name = (char*)malloc(sizeof(char) * arg_size);

  int i = 0;
  while (argv[i] != NULL) {
    strcat(child->cmd_name, argv[i]);
    strcat(child->cmd_name, " ");
    i++;
  }
  child->cmd_name[arg_size] = '\0';

  s_write_log(CREATE, child, -1);
  if (priority != -1) {
    s_write_log(NICE, child, 1);
  }
  return (child->pid);
}

pid_t s_waitpid(pid_t pid, int* wstatus, bool nohang) {
  // fprintf(stdout, "s_waitpid called by pid: %d, to wait on pid: %d\n",
  // current->pid, pid); fprintf(stdout, "process caller was: %s \n",
  // current->processname);
  pcb_t* child_pcb;
  // child_pcb = s_find_process(pid);

  if (pid > 0) {
    child_pcb = s_find_process(pid);
    if (child_pcb == NULL) {
      errno = ENOPROC;
      return -1;
    }
  } else {
    bool give_up = true;
    DynamicPIDArray* pid_array = current->child_pids;
    for (size_t i = 0; i < pid_array->used; i++) {
      pid_t current_pid = pid_array->array[i];
      child_pcb = s_find_process(current_pid);
      // is there at least one child process left to wait on?
      if (child_pcb == NULL) {
        continue;
      }
      if ((child_pcb->state == RUNNING) || (child_pcb->state == BLOCKED) ||
          (child_pcb->statechanged)) {
        give_up = false;
      }
      if (child_pcb->statechanged) {
        break;
      }
    }
    if (give_up == true) {
      return -1;  // no children to wait on
    }
  }

  // if nohang, return immediately
  if (nohang) {
    if (child_pcb->statechanged) {
      if (child_pcb->state == ZOMBIED) {
        if (wstatus != NULL) {
          *wstatus = 0;
        }
        child_pcb->statechanged = false;
        return child_pcb->pid;
      }
    }
    return 0;
  }

  // if child already changed state
  if (child_pcb->statechanged && (wstatus != NULL)) {
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
  current->waiting_for_change = true;
  current->waiting_on_pid = pid;
  s_move_process(blocked, current->pid);
  current->state = BLOCKED;
  spthread_suspend_self();

  child_pcb->statechanged = false;
  if ((child_pcb->state == ZOMBIED) && (wstatus != NULL)) {
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

  return child_pcb->pid;
}

int s_resume_block(pid_t pid) {
  pcb_t* process = s_find_process(pid);
  if (process == NULL) {
    errno = ENOPROC;
    return -1;
  }

  if (s_move_process(blocked, pid) == -1) {
    return -1;
  }
  process->state = BLOCKED;
  // process->statechanged = true;
  if (process->job_num == -1) {
    process->job_num = job_id;
    job_id++;
  }
  s_write_log(BLOCKED, process, -1);
  char message[40];
  s_write(STDOUT_FILENO, message, strlen(message));

  return 0;
}

int s_kill(pid_t pid, int signal) {
  pcb_t* process = s_find_process(pid);
  if (process == NULL) {
    errno = ENOPROC;
    return -1;
  }

  switch (signal) {
    case P_SIGSTOP:
      if (s_move_process(stopped, pid) == -1) {
        return -1;
      }
      process->state = STOPPED;
      process->statechanged = true;
      process->is_bg = false;
      if (process->job_num == -1) {
        process->job_num = job_id;
        job_id++;
      }
      s_write_log(STOP, process, -1);
      char message[40];
      sprintf(message, "\n[%d] + Stopped %s\n", process->job_num,
              process->cmd_name);
      s_write(STDOUT_FILENO, message, strlen(message));
      break;
    case P_SIGCONT:
      if (!(process->state == STOPPED)) {
        errno = ENOTSTOP;
        return -1;
      }
      if (s_move_process(processes[process->priority], pid) == -1) {
        return -1;
      }
      process->state = RUNNING;
      process->statechanged = true;
      s_write_log(CONTINUE, process, -1);
      break;
    case P_SIGTER:
      process->term_signal = P_SIGTER;
      process->exit_status = 0;

      s_write_log(SIGNAL, process, -1);
      s_zombie(pid);
      break;
    default:
      errno = EINVALIDSIG;
      return -1;
  }
  return 0;
}

void s_reap_all_child(pcb_t* parent) {
  if (parent == NULL || parent->child_pids == NULL ||
      parent->child_pids->array) {
    return;
  }

  DynamicPIDArray* child_array = parent->child_pids;
  if (child_array == NULL) {
    return;
  }
  int c_size = child_array->size;
  for (int i = 0; i < c_size; i++) {
    pcb_t* child_proc = s_find_process(child_array->array[i]);
    s_reap_all_child(child_proc);
    if (child_proc != NULL) {
      s_remove_process(child_array->array[i]);
      k_proc_cleanup(child_proc);
    }
  }
}

void s_exit(void) {
  s_write_log(EXIT, current, -1);
  s_zombie(current->pid);
}

int s_zombie(pid_t pid) {
  pcb_t* proc = s_find_process(pid);
  if (proc == NULL) {
    errno = ENOPROC;
    return -1;
  }
  if (s_move_process(zombied, pid) == -1) {
    return -1;
  }
  proc->state = ZOMBIED;
  proc->statechanged = true;
  s_write_log(ZOMBIE, proc, -1);
  DynamicPIDArray* pid_array = proc->child_pids;
  for (size_t i = 0; i < pid_array->used; i++) {
    pid_t current_pid = pid_array->array[i];
    pcb_t* child_proc = s_find_process(current_pid);
    if (child_proc == NULL) {
      continue;
    }
    s_write_log(ORPHAN, child_proc, -1);
    child_proc->ppid = 0;
  }
  s_reap_all_child(proc);
  return 0;
}

int s_nice(pid_t pid, int priority) {
  pcb_t* pcb = s_find_process(pid);
  if (pcb == NULL) {
    errno = ENOPROC;
    return -1;
  }
  pcb->priority = priority;
  if (pcb->state == RUNNING) {
    if (s_move_process(processes[priority], pid) == -1) {
      return -1;
    }
  }

  return 0;
}

int s_sleep(unsigned int ticks) {
  if (ticks <= 0) {
    errno = EINVARG;
    return -1;
  }

  current->ticks_to_wait = ticks;
  current->initial_state = BLOCKED;
  while (current->ticks_to_wait >= 1) {
    current->state = BLOCKED;
    if (!remove_process(processes[current->priority], current->pid)) {
      errno = EREMOVEPROC;
      return -1;
    }
    if (add_process(blocked, current) == -1) {
      errno = EADDPROC;
      return -1;
    }
    spthread_suspend_self();
  }
  return 0;
}

int s_busy(void) {
  while (1)
    ;
  s_exit();
  return 0;
}

int s_spawn_and_wait(void* (*func)(void*),
                     char* argv[],
                     int fd0,
                     int fd1,
                     bool nohang,
                     unsigned int priority) {
  pid_t child = s_spawn_nice(func, argv, fd0, fd1, priority);

  if (nohang) {
    pcb_t* child_proc = s_find_process(child);
    if (child_proc == NULL) {
      errno = ENOPROC;
      return -1;
    }

    child_proc->job_num = job_id;
    job_id++;
    child_proc->is_bg = true;

    if (add_process(bg_list, child_proc) == -1) {
      errno = EADDPROC;
      return -1;
    }
    char message[25];
    sprintf(message, "[%d] %4u\n", child_proc->job_num, child_proc->pid);
    if (s_write(STDOUT_FILENO, message, strlen(message)) == -1) {
      errno = EINVALIDSTDOUT;
    }
  }
  int wstatus = 0;
  s_waitpid(child, &wstatus, nohang);
  if (!nohang && (wstatus == STATUS_EXITED || wstatus == STATUS_SIGNALED)) {
    pcb_t* child_pcb = s_find_process(child);
    if (child_pcb == NULL) {
      errno = ENOPROC;
      return -1;
    }

    s_reap_all_child(child_pcb);
    s_remove_process(child);
    k_proc_cleanup(child_pcb);
  }
  return 0;
}

int s_fg(pcb_t* proc) {
  proc->is_bg = false;
  fg_proc = proc;

  int wstatus = 0;
  s_waitpid(proc->pid, &wstatus, proc->is_bg);

  if (!proc->is_bg &&
      (wstatus == STATUS_EXITED || wstatus == STATUS_SIGNALED)) {
    s_reap_all_child(proc);
    s_remove_process(proc->pid);
    k_proc_cleanup(proc);
  }
  return 0;
}

int s_bg_wait(pcb_t* proc) {
  int status = 0;
  if (proc->state == ZOMBIED) {
    char message[50];
    sprintf(message, "[%d]\t %4u DONE\t%s\n", proc->job_num, proc->pid,
            proc->processname);
    if (s_write(STDOUT_FILENO, message, strlen(message)) == -1) {
      errno = EINVALIDSTDOUT;
      return -1;
    }

    proc->is_bg = false;
    s_remove_process(proc->pid);

    return 0;
  }
  pid_t wpid = s_waitpid(proc->pid, &status, true);
  if (wpid == proc->pid) {
    char message[50];
    sprintf(message, "[%d]\t %4u DONE\t%s\n", proc->job_num, proc->pid,
            proc->processname);
    if (s_write(STDOUT_FILENO, message, strlen(message)) == -1) {
      errno = EINVALIDSTDOUT;
      return -1;
    }

    proc->is_bg = false;
    s_remove_process(proc->pid);
  }

  return 0;
}

pcb_t* s_find_process(pid_t pid) {
  pcb_t* ret = NULL;

  ret = find_process(zombied, pid);
  if (ret != NULL) {
    return ret;
  }
  ret = find_process(blocked, pid);
  if (ret != NULL) {
    return ret;
  }
  ret = find_process(stopped, pid);
  if (ret != NULL) {
    return ret;
  }
  ret = find_process(processes[0], pid);
  if (ret != NULL) {
    return ret;
  }
  ret = find_process(processes[1], pid);
  if (ret != NULL) {
    return ret;
  }
  ret = find_process(processes[2], pid);
  if (ret != NULL) {
    return ret;
  }
  return NULL;
}

pcb_t* find_jobs_proc(CircularList* list) {
  if (list == NULL || list->head == NULL) {
    return NULL;
  }

  int max_job_num = -1;
  pid_t max_pid = -1;

  Node* current_node = list->head;
  pcb_t* proc;
  do {
    proc = current_node->process;

    if (proc->job_num > max_job_num &&
        (proc->state == STOPPED || proc->is_bg)) {
      max_job_num = proc->job_num;
      max_pid = proc->pid;
    }

    current_node = current_node->next;
  } while (current_node != list->head);

  if (max_job_num != -1 && max_pid != -1) {
    return find_process(list, max_pid);
  }

  return NULL;
}

int s_remove_process(pid_t pid) {
  pcb_t* proc = s_find_process(pid);
  if (proc == NULL) {
    errno = ENOPROC;
    return -1;
  }
  switch (proc->state) {
    case BLOCKED:
      if (!remove_process(blocked, pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      return 0;
    case STOPPED:
      if (!remove_process(stopped, pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      return 0;
    case ZOMBIED:
      if (!remove_process(zombied, pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      return 0;
    case RUNNING:
      if (!remove_process(processes[proc->priority], pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      return 0;
    default:
      errno = EPCBSTATE;
      return -1;
  }
}

void* s_function_from_string(char* program) {
  // replace as you create/define more function
  if (strcmp(program, "cat") == 0) {
    return b_cat;
  } else if (strcmp(program, "sleep") == 0) {
    return b_sleep;
  } else if (strcmp(program, "busy") == 0) {
    return b_busy;
  } else if (strcmp(program, "echo") == 0) {
    return b_echo;
  } else if (strcmp(program, "ls") == 0) {
    return b_ls;
  } else if (strcmp(program, "touch") == 0) {
    return b_touch;
  } else if (strcmp(program, "mv") == 0) {
    return b_mv;
  } else if (strcmp(program, "cp") == 0) {
    return b_cp;
  } else if (strcmp(program, "rm") == 0) {
    return b_rm;
  } else if (strcmp(program, "chmod") == 0) {
    return b_chmod;
  } else if (strcmp(program, "ps") == 0) {
    return b_ps;
  } else if (strcmp(program, "kill") == 0) {
    return b_kill;
  } else if (strcmp(program, "zombify") == 0) {
    return b_zombify;
  } else if (strcmp(program, "orphanify") == 0) {
    return b_orphanify;
  } else {
    errno = EINVALIDCMD;
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
      errno = EINVALIDLOG;
      return -1;
  }
  if (write(logfiledescriptor, buf, strlen(buf)) == -1) {
    errno = EINVALIDLOGWRITE;
    return -1;
  } else {
    return 0;  // exit successfully
  }
}

int s_move_process(CircularList* destination, pid_t pid) {
  pcb_t* pcb = s_find_process(pid);
  if (pcb == NULL) {
    errno = ENOPROC;
    return -1;
  }

  switch (pcb->state) {
    case STOPPED:
      if (!remove_process(stopped, pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      break;
    case BLOCKED:
      if (!remove_process(blocked, pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      break;
    case ZOMBIED:
      if (!remove_process(zombied, pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      break;
    case RUNNING:
      if (!remove_process(processes[pcb->priority], pid)) {
        errno = EREMOVEPROC;
        return -1;
      }
      break;
    default:
      errno = EPCBSTATE;
      return -1;
  }
  if (add_process(destination, pcb) == -1) {
    errno = EADDPROC;
    return -1;
  }
  return 0;
}

int s_print_process(CircularList* list) {
  if (list == NULL) {
    errno = ELISTNULL;
    return -1;
  }
  if (list->head == NULL) {
    return 0;
  }

  Node* current_node = list->head;
  pcb_t* proc;

  do {
    proc = current_node->process;

    char message[50];
    switch (proc->state) {
      case RUNNING:
        sprintf(message, "%4u\t%4u\t%4u\t R\t%s\n", proc->pid, proc->ppid,
                proc->priority, proc->processname);
        break;
      case BLOCKED:
        sprintf(message, "%4u\t%4u\t%4u\t B\t%s\n", proc->pid, proc->ppid,
                proc->priority, proc->processname);
        break;
      case STOPPED:
        sprintf(message, "%4u\t%4u\t%4u\t S\t%s\n", proc->pid, proc->ppid,
                proc->priority, proc->processname);
        break;
      case ZOMBIED:
        sprintf(message, "%4u\t%4u\t%4u\t Z\t%s\n", proc->pid, proc->ppid,
                proc->priority, proc->processname);
        break;
    }
    ssize_t result = s_write(STDOUT_FILENO, message, strlen(message));
    if (result == -1) {
      errno = EINVALIDSTDOUT;
      return -1;
    }

    current_node = current_node->next;
  } while (current_node != list->head);

  return 0;
}

int s_print_jobs(void) {
  if (bg_list != NULL && bg_list->head != NULL) {
    Node* current_node = bg_list->head;
    pcb_t* proc;
    do {
      proc = current_node->process;

      if (proc->is_bg) {
        char message[40];
        sprintf(message, "[%d] Running %s\n", proc->job_num, proc->cmd_name);
        ssize_t result = s_write(STDOUT_FILENO, message, strlen(message));
        if (result == -1) {
          errno = EINVALIDSTDOUT;
          return -1;
        }
      }

      current_node = current_node->next;
    } while (current_node != bg_list->head);
  }

  if (stopped != NULL && stopped->head != NULL) {
    Node* current_node = stopped->head;
    pcb_t* proc;
    do {
      proc = current_node->process;

      char message[40];
      sprintf(message, "[%d] Stopped %s\n", proc->job_num, proc->cmd_name);
      ssize_t result = s_write(STDOUT_FILENO, message, strlen(message));
      if (result == -1) {
        errno = EINVALIDSTDOUT;
        return -1;
      }

      current_node = current_node->next;
    } while (current_node != stopped->head);
  }

  return 0;
}

int file_errno_helper(int ret) {
  if (ret == INVALID_FILE_NAME) {
    errno = EBADFILENAME;
    return -1;
  } else if (ret == MULTIPLE_F_WRITE) {
    errno = EMULTWRITE;
    return -1;
  } else if (ret == WRONG_PERMISSION) {
    errno = EWRONGPERM;
    return -1;
  } else if (ret == SYSTEM_ERROR) {
    errno = ESYSERR;
    return -1;
  } else if (ret == FILE_NOT_FOUND) {
    errno = ENOFILE;
    return -1;
  } else if (ret == FILE_DELETED) {
    errno = EFILEDEL;
    return -1;
  } else if (ret == INVALID_FILE_DESCRIPTOR) {
    errno = EINVALIDFD;
    return -1;
  } else if (ret == INVALID_PARAMETERS) {
    errno = EINVALIDPARAMETER;
    return -1;
  } else if (ret == FILE_IN_USE) {
    errno = EUSEDFILE;
    return -1;
  } else if (ret == INVALID_CHMOD) {
    errno = EINVALIDCHMOD;
    return -1;
  } else if (ret == SOURCE_FILE_NO_READ_PERM) {
    errno = ENOREADPERM;
    return -1;
  } else if (ret == DEST_FILE_NO_WRITE_PERM) {
    errno = ENOWRITEPERM;
    return -1;
  }
  return ret;
}

int s_open(const char* fname, int mode) {
  int fd = k_open(fname, mode);

  if (file_errno_helper(fd) == -1) {
    return -1;
  }

  fd_bitmap_set(current->open_fds, fd);
  return fd;
}

ssize_t s_read(int fd, int n, char* buf) {
  ssize_t ret = k_read(fd, n, buf);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

ssize_t s_write(int fd, const char* str, int n) {
  ssize_t ret = k_write(fd, str, n);
  if (file_errno_helper(ret) == -1) {
    return -1;
  }
  return ret;
}

int s_close(int fd) {
  int ret = k_close(fd);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  fd_bitmap_clear(current->open_fds, fd);
  return 0;
}

int s_unlink(const char* fname) {
  int ret = k_unlink(fname);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

off_t s_lseek(int fd, int offset, int whence) {
  int ret = k_lseek(fd, offset, whence);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }
  return ret;
}

int s_ls(const char* filename, int fd) {
  int ret = k_ls(filename, fd);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

char* s_read_all(const char* filename, int* read_num) {
  char* ret = k_read_all(filename, read_num);

  if (ret == NULL) {
    errno = EREADERROR;
    return NULL;
  }

  return ret;
}

char* s_get_fname_from_fd(int fd) {
  char* ret = k_get_fname_from_fd(fd);

  if (ret == NULL) {
    errno = EINVALIDFD;
    return NULL;
  }

  return ret;
}

int s_update_timestamp(const char* source) {
  int ret = k_update_timestamp(source);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

off_t s_does_file_exist2(const char* fname) {
  int ret = does_file_exist2(fname);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

int s_rename(const char* source, const char* dest) {
  int ret = k_rename(source, dest);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

int s_change_mode(const char* change, const char* filename) {
  int ret = k_change_mode(change, filename);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

int s_cp_within_fat(char* source, char* dest) {
  int ret = k_cp_within_fat(source, dest);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

int s_cp_to_host(char* source, char* host_dest) {
  int ret = k_cp_to_host(source, host_dest);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}

int s_cp_from_host(char* host_source, char* dest) {
  int ret = k_cp_from_host(host_source, dest);

  if (file_errno_helper(ret) == -1) {
    return -1;
  }

  return ret;
}
