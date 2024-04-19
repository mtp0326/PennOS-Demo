#include "shellbuiltins.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "errno.h"
#include "pennos.h"
#include "sys_call.h"

void* b_background_poll(void* arg) {
  if (bg_list == NULL || bg_list->head == NULL) {
    return NULL;
  }

  Node* node = bg_list->head;
  int status = 0;
  do {
    pcb_t* proc = node->process;
    s_waitpid(proc->pid, &status, true);

    if (P_WIFSIGNALED(status)) {
      fprintf(stdout, "[%ld]\t %4u SIGNALED\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);
      // remove_process(bg_list, proc->pid);
      /// might have to change processor manually
      /// do we know with waitpid the processors changed???
    } else if (P_WIFSTOPPED(status)) {
      fprintf(stdout, "[%ld]\t %4u STOPPED\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);
      // remove_process(bg_list, proc->pid);
    } else if (P_WIFEXITED(status)) {
      fprintf(stdout, "[%ld]\t %4u DONE\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);
      // remove_process(bg_list, proc->pid);
    }
    node = node->next;
  } while (node != bg_list->head);

  return NULL;
}

void* b_sleep(void* arg) {
  char** argv = (char**)arg;
  if (argv[1] == NULL) {
    // code later to add errno and write to the terminal (using pennfat writes),
    // but not today :(
    return NULL;
  }
  int seconds = atoi(argv[1]);  // NOTE: use strtol in later implementation
  // here, as atoi does not define errors
  // (differentiate on 0), will need for errno
  unsigned int ticks = seconds * 10;

  s_sleep(ticks);
  return NULL;
}

void* b_kill(void* arg) {
  char** argv = (char**)arg;
  int signal;
  int i = 1;
  if (strlen(argv[1]) >= 4) {
    signal = P_SIGTER;
  } else {
    if (strcmp(argv[1], "-term") == 0) {
      signal = P_SIGTER;
    } else if (strcmp(argv[1], "-stop") == 0) {
      signal = P_SIGSTOP;

    } else if (strcmp(argv[1], "-cont") == 0) {
      signal = P_SIGCONT;

    } else {
      return NULL;
    }
    i = 2;
  }
  for (; argv[i] != NULL; i++) {
    s_kill(atoi(argv[i]), signal);
  }

  s_exit();
  return NULL;
}

void* b_nice(void* arg) {
  struct parsed_command* parsed = NULL;
  parse_command(arg, &parsed);
  char** args = parsed->commands[0];
  void* (*func)(void*) = s_function_from_string(args[2]);
  unsigned priority = atoi(args[1]);  // USE STROL AND ERRNO
  s_spawn_and_wait(func, &args[2], STDIN_FILENO, STDOUT_FILENO, arg,
                   parsed->is_background, priority);
  return NULL;
}

void* b_nice_pid(void* arg) {
  char** argv = (char**)arg;
  s_nice(atoi(argv[2]), atoi(argv[1]));

  return NULL;
}

void* b_ps(void* arg) {
  // displaying PID, PPID, priority, status, and command name.
  /// not sure if order has to change
  fprintf(stdout, "  PID\tPPID\tPRIORITY    STATUS\tCMD_NAME\n");

  for (int i = 0; i < 3; i++) {
    s_print_process(processes[i]);
  }
  s_print_process(blocked);
  s_print_process(stopped);
  s_print_process(zombied);

  return NULL;
}

void* b_jobs(void* arg) {
  s_print_jobs(bg_list);
  s_print_jobs(stopped);

  return NULL;
}

void* b_fg(void* arg) {
  char** argv = (char**)arg;
  pcb_t* proc;

  if (argv[1] != NULL) {
    // job id is specified
    long index = (long)atoi(argv[1]);

    proc = find_process_job_id(stopped, index);

    if (proc != NULL) {
      fprintf(stdout, "[%ld]\t %4u CONTINUED\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);
      if (s_kill(proc->pid, SIGCONT) < 0) {
        // error
        fprintf(stdout, "SIGCONT failed to send\n");
        return NULL;
      }

      /// TODO: immediate send to tcprescp

      //   remove_process(stopped, proc->pid);
      //   // add to IMMEDIATE front of processes
      //   add_process_front(processes[proc->priority], proc);
      return NULL;
    }

    proc = find_process_job_id(bg_list, index);

    if (proc != NULL) {
      fprintf(stdout, "[%ld]\t %4u RUNNING\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);

      /// TODO: immediate send to tcprescp

      // remove_process(bg_list, proc->pid);
      // // add to IMMEDIATE front of processes
      // add_process_front(processes[proc->priority], proc);

      return NULL;
    }
    /// error: PID with specified number does not exist

    return NULL;
  }

  if (stopped != NULL && stopped->tail != NULL) {
    proc = stopped->tail->process;
    fprintf(stdout, "[%ld]\t %4u CONTINUED\t%s\n", proc->job_num, proc->pid,
            proc->cmd_name);
    if (s_kill(proc->pid, SIGCONT) < 0) {
      // error
      fprintf(stdout, "SIGCONT failed to send\n");
      return NULL;
    }

    /// TODO: immediate send to tcprescp

    // remove_process(stopped, proc->pid);
    // // add to IMMEDIATE front of processes
    // add_process_front(processes[proc->priority], proc);
    return NULL;
  }

  if (bg_list != NULL && bg_list->tail != NULL) {
    proc = bg_list->tail->process;
    fprintf(stdout, "[%ld]\t %4u RUNNING\t%s\n", proc->job_num, proc->pid,
            proc->cmd_name);

    /// TODO: immediate send to tcprescp

    // remove_process(bg_list, proc->pid);
    // // add to IMMEDIATE front of processes
    // add_process_front(processes[proc->priority], proc);

    return NULL;
  }

  // error: to stopped or background job exist
  fprintf(stdout, "Job does not exist\n");
  return NULL;
}

void* b_bg(void* arg) {
  char** argv = (char**)arg;
  pcb_t* proc;

  if (argv[1] != NULL) {
    // pid is specified
    long index = (long)atoi(argv[1]);

    proc = find_process_job_id(stopped, index);

    if (proc != NULL) {
      proc = stopped->head->process;
      if (s_kill(proc->pid, SIGCONT) < 0) {
        // error
        fprintf(stdout, "SIGCONT failed to send\n");
        return NULL;
      }

      add_process(bg_list, proc);
      fprintf(stdout, "[%ld]\t %4u CONTINUED\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);

      // proc->state = RUNNING;
      // /// proc->statechanged = true;
      // /// proc->is_background = true;
      // remove_process(stopped, proc->pid);
      // add_process(bg_list, proc);

      return NULL;
    }
    /// error: PID with specified number does not exist

    return NULL;
  }

  if (stopped != NULL && stopped->head != NULL) {
    proc = stopped->tail->process;
    if (s_kill(proc->pid, SIGCONT) < 0) {
      // error
      fprintf(stdout, "SIGCONT failed to send\n");
      return NULL;
    }

    add_process(bg_list, proc);
    fprintf(stdout, "[%ld]\t %4u CONTINUED\t%s\n", proc->job_num, proc->pid,
            proc->cmd_name);

    // proc->state = RUNNING;
    // /// proc->statechanged = true;
    // /// proc->statechanged = true;
    // remove_process(stopped, proc->pid);
    return NULL;
  }
  /// error: there are no stopped jobs
  return NULL;
}

void* b_logout(void* arg) {
  done = true;
  return NULL;
}
