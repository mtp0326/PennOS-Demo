#include "shellbuiltins.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "errno.h"
#include "pennos.h"
#include "sys_call.h"
#include "unistd.h"

void* b_background_poll(void* arg) {
  if (bg_list == NULL || bg_list->head == NULL) {
    return NULL;
  }

  Node* node = bg_list->head;
  int bgsize = bg_list->size;
  for (int i = 0; i < bgsize; i++) {
    pcb_t* proc = node->process;
    s_bg_wait(proc);
    node = node->next;
  }
  // do {
  //   pcb_t* proc = node->process;
  //   s_bg_wait(proc);
  //   if (bg_list->size == 0) {
  //     return NULL;
  //   }
  //   // if (P_WIFSIGNALED(status)) {
  //   //   fprintf(stdout, "[%ld]\t %4u SIGNALED\t%s\n", proc->job_num,
  //   proc->pid,
  //   //           proc->processname);
  //   //   // remove_process(bg_list, proc->pid);
  //   //   /// might have to change processor manually
  //   //   /// do we know with waitpid the processors changed???
  //   // } else if (P_WIFSTOPPED(status)) {
  //   //   fprintf(stdout, "[%ld]\t %4u STOPPED\t%s\n", proc->job_num,
  //   proc->pid,
  //   //           proc->processname);
  //   //   // remove_process(bg_list, proc->pid);
  //   // } else if (P_WIFEXITED(status)) {

  //   // remove_process(bg_list, proc->pid);
  //   // }
  //   node = node->next;
  // } while (node != bg_list->head);

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

<<<<<<< HEAD
void* b_nice(void* arg) {
  struct parsed_command* parsed = NULL;
  parse_command(arg, &parsed);
  char** args = parsed->commands[0];
  void* (*func)(void*) = s_function_from_string(args[2]);
  unsigned priority = atoi(args[1]);  // USE STROL AND ERRNO
  s_spawn_and_wait(func, &args[2], STDIN_FILENO, STDOUT_FILENO,
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
  fprintf(stdout, "  PID\t PPID\t  PRI\tSTAT\tCMD\n");

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
              proc->processname);
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
              proc->processname);

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
            proc->processname);
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
            proc->processname);

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
              proc->processname);

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
            proc->processname);

    // proc->state = RUNNING;
    // /// proc->statechanged = true;
    // /// proc->statechanged = true;
    // remove_process(stopped, proc->pid);
    return NULL;
  }
  /// error: there are no stopped jobs
=======
void* b_man(void* arg) {
  char* output =
      "cat\nsleep\nbusy\necho\nls\ntouch\nmv\ncp\nrm\nchmod\nps\nkill\nzombify"
      "\norphanify\nnice\nnice_pid\nman\nbg\nfg\njobs\nlogout\n";
  ssize_t result = s_write(STDOUT_FILENO, output, strlen(output));
  if (result == -1) {
    u_perror("Failed to write to STDOUT");
  }
  s_exit();
>>>>>>> main
  return NULL;
}

void* b_nice(void* arg) {
  struct parsed_command* parsed = NULL;
  parse_command(arg, &parsed);
  char** args = parsed->commands[0];
  void* (*func)(void*) = s_function_from_string(args[2]);
  unsigned priority = atoi(args[1]);  // USE STROL AND ERRNO
  s_spawn_and_wait(func, &args[2], STDIN_FILENO, STDOUT_FILENO,
                   parsed->is_background, priority);
  return NULL;
}

void* b_nice_pid(void* arg) {
  char** argv = (char**)arg;
  s_nice(atoi(argv[2]), atoi(argv[1]));

  return NULL;
}

void* b_orphan_child(void* arg) {
  // Please sir,
  // I want some more
  while (1)
    ;
  return NULL;
}

void* b_orphanify(void* arg) {
  s_spawn(b_orphan_child, arg, STDIN_FILENO, STDOUT_FILENO);
  return NULL;
}

void* b_zombie_child(void* arg) {
  // MMMMM Brains...!
  return NULL;
}

void* b_zombify(void* arg) {
  s_spawn(b_zombie_child, arg, STDIN_FILENO, STDOUT_FILENO);
  while (1)
    ;
  return NULL;
}

void* b_logout(void* arg) {
    pthread_mutex_lock(&done_lock);
    done = true;
    pthread_mutex_unlock(&done_lock);
    s_exit();
    return NULL;
}

void* b_clear(void* arg) {
    char* clear = "\033c";
    s_write(STDOUT_FILENO, clear, strlen(clear));

    return NULL;
}


// FAT LEVEL SHELL FUNCTIONS

void* b_ls(void* arg) {
  s_ls(NULL);
  s_exit();
  return NULL;
}

void* b_echo(void* arg) {
  char** argv = (char**)arg;

  int i = 1;
  while (argv[i] != NULL) {
    s_write(current->output_fd, argv[i], strlen(argv[i]));
    s_write(current->output_fd, " ", 1);
    i++;
  }

  s_exit();
  return NULL;
}

void* b_cat(void* arg) {
  char** argv = (char**)arg;
  // int read_num;

  // files arg is not provided
  if (argv[1] == NULL) {
    if ((current->input_fd) == STDIN_FILENO) {
      char contents[4096];
      s_read(STDIN_FILENO, 4096, contents);
      s_write(current->output_fd, contents, strlen(contents));
    } else {
      int read_num;
      char* fname = s_get_fname_from_fd(current->input_fd);
      char* contents = s_read_all(fname, &read_num);
      s_write(current->output_fd, contents, strlen(contents));
    }
  } else {
    // we always ignore the input_fd here

    int i = 1;
    while (argv[i] != NULL) {
      int read_num;
      char* contents = s_read_all(argv[i], &read_num);
      if (contents == NULL) {
        perror("No such file or directory\n");
        i++;
        continue;
      }
      s_write(current->output_fd, contents, strlen(contents));
      i++;
    }
  }

  s_exit();
  return NULL;
}

void* b_touch(void* arg) {
  char** args = (char**)arg;
  int i = 1;
  while (args[i] != NULL) {
    if (s_does_file_exist2(args[i]) != -1) {
      s_update_timestamp(args[i]);
    } else {
      int fd = s_open(args[i], F_WRITE);
      s_close(fd);
    }
    i += 1;
  }

  s_exit();
  return NULL;
}

void* b_mv(void* arg) {
  char** args = (char**)arg;
  s_rename(args[1], args[2]);
  s_exit();
  return NULL;
}

void* b_rm(void* arg) {
  char** args = (char**)arg;
  int i = 1;
  while (args[i] != NULL) {
    s_unlink(args[i]);
    i += 1;
  }
  s_exit();
  return NULL;
}

void* b_chmod(void* arg) {
  char** args = (char**)arg;
  s_change_mode(args[1], args[2]);
  s_exit();
  return NULL;
}

void* b_cp(void* arg) {
  char** args = (char**)arg;
  if (strcmp(args[1], "-h") == 0) {
    // cp -h SOURCE DEST
    s_cp_from_host(args[2], args[3]);
  } else {
    if (strcmp(args[2], "-h") == 0) {
      // cp SOURCE -h DEST
      s_cp_to_host(args[1], args[3]);
    } else {
      // cp SOURCE DEST
      s_cp_within_fat(args[1], args[2]);
    }
  }
  s_exit();
  return NULL;
}
