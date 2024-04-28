#include "shellbuiltins.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "errno.h"
#include "pennos.h"
#include "sys_call.h"
#include "unistd.h"

void* b_background_poll(void* arg) {
  // b_ps(NULL);
  if (bg_list == NULL || bg_list->head == NULL) {
    return NULL;
  }

  Node* node = bg_list->head;
  int bgsize = bg_list->size;
  for (int i = 0; i < bgsize; i++) {
    pcb_t* proc = node->process;
    if (proc->is_bg) {
      s_bg_wait(proc);
    }
    node = node->next;
  }

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

void* b_busy(void* arg) {
  s_busy();
  s_exit();
  return NULL;
}

void* b_kill(void* arg) {
  char** argv = (char**)arg;
  int signal;
  int i = 1;
  if (strlen(argv[1]) <= 4) {
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

  s_exit();
  return NULL;
}

void* b_jobs(void* arg) {
  s_print_jobs();

  return NULL;
}

void* b_fg(void* arg) {
  char** argv = (char**)arg;
  pcb_t* proc;

  if (argv[1] != NULL) {
    // job id is specified
    int index = atoi(argv[1]);

    proc = find_process_job_id(stopped, index);

    if (proc != NULL) {
      fprintf(stdout, "[%d]  + %4u Continued\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);
      if (proc->initial_state != RUNNING) {
        s_resume_block(proc->pid);
      } else {
        if (s_kill(proc->pid, P_SIGCONT) < 0) {
          fprintf(stderr, "SIGCONT failed to send\n");
          return NULL;
        }
      }
      s_fg(proc);
      return NULL;
    }

    proc = find_process_job_id(bg_list, index);

    if (proc != NULL) {
      fprintf(stdout, "[%d]  + %4u Running\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);
      s_fg(proc);
      return NULL;
    }
    fprintf(stderr, "PID with specified number does not exist\n");

    return NULL;
  }

  proc = find_jobs_proc(stopped);
  if (proc != NULL) {
    fprintf(stdout, "[%d]  + %4u Continued\t%s\n", proc->job_num, proc->pid,
            proc->cmd_name);
    /// TODO:there could be more but wrote for sleep for now
    if (proc->initial_state != RUNNING) {
      s_resume_block(proc->pid);
    } else {
      if (s_kill(proc->pid, P_SIGCONT) < 0) {
        // error
        fprintf(stderr, "SIGCONT failed to send\n");
        return NULL;
      }
    }

    s_fg(proc);
    return NULL;
  }

  proc = find_jobs_proc(bg_list);
  if (proc != NULL) {
    fprintf(stdout, "[%d]  + %4u Running\t%s\n", proc->job_num, proc->pid,
            proc->cmd_name);
    s_fg(proc);
    return NULL;
  }
  fprintf(stderr, "Job does not exist\n");
  return NULL;
}

void* b_bg(void* arg) {
  char** argv = (char**)arg;
  pcb_t* proc;

  if (argv[1] != NULL) {
    // pid is specified
    int index = atoi(argv[1]);

    proc = find_process_job_id(stopped, index);

    if (proc != NULL) {
      proc = stopped->head->process;
      if (proc->initial_state != RUNNING) {
        s_resume_block(proc->pid);
      } else {
        if (s_kill(proc->pid, P_SIGCONT) < 0) {
          fprintf(stderr, "SIGCONT failed to send\n");
          return NULL;
        }
      }

      proc->is_bg = true;
      add_process(bg_list, proc);
      fprintf(stdout, "[%d]  + %4u Continued\t%s\n", proc->job_num, proc->pid,
              proc->cmd_name);
      return NULL;
    }
    fprintf(stderr, "PID with specified number does not exist\n");
    return NULL;
  }

  proc = find_jobs_proc(stopped);
  if (proc != NULL) {
    proc = stopped->tail->process;
    if (proc->initial_state != RUNNING) {
      s_resume_block(proc->pid);
    } else {
      if (s_kill(proc->pid, P_SIGCONT) < 0) {
        fprintf(stderr, "SIGCONT failed to send\n");
        return NULL;
      }
    }

    proc->is_bg = true;
    add_process(bg_list, proc);
    fprintf(stdout, "[%d]  + %4u Continued\t%s\n", proc->job_num, proc->pid,
            proc->cmd_name);
    return NULL;
  }
  fprintf(stderr, "there are no stopped jobs\n");

  return NULL;
}

void* b_man(void* arg) {
  char* output;
  char** args = (char**)arg;
  if (args[1] == NULL) {
    output =
        "cat\nsleep\nbusy\necho\nls\ntouch\nmv\ncp\nrm\nchmod\nps\nkill\nzombif"
        "y"
        "\norphanify\nnice\nnice_pid\nman\nbg\nfg\njobs\nlogout\n";
  } else {
    if (strcmp(args[1], "cat") == 0) {
      output =
          "The cat utility reads files sequentially, writing them to "
          "the "
          "standard output.  The file operands are processed in command-line "
          "order.\n";
    } else if (strcmp(args[1], "sleep") == 0) {
      output =
          "The sleep command suspends execution for a minimum of "
          "seconds.\n";
    } else if (strcmp(args[1], "busy") == 0) {
      output = "Make shell wait indefinitely until a signal is given.\n";
    } else if (strcmp(args[1], "echo") == 0) {
      output =
          "The echo utility writes any specified operands, separated "
          "by "
          "single "
          "blank characters and followed by a newline character, to "
          "the standard output.\n";
    } else if (strcmp(args[1], "ls") == 0) {
      output =
          "For each operand that names a file of a type other than "
          "directory, ls displays its name as well as any requested, "
          "associated "
          "information.\n";
    } else if (strcmp(args[1], "touch") == 0) {
      output =
          "The touch utility sets the modification and access times "
          "of "
          "files.  If any file does not exist, it is created with default "
          "permissions.\n";
    } else if (strcmp(args[1], "mv") == 0) {
      output =
          "In its first form, the mv utility renames the file named by "
          "the "
          "source operand to the destination path named by the target operand. "
          " "
          "This form is assumed when the last operand does not name an already "
          "existing directory.\n";
    } else if (strcmp(args[1], "cp") == 0) {
      output =
          "Cp utility copies the contents of the source_file to the "
          "target_file.\n";
    } else if (strcmp(args[1], "rm") == 0) {
      output =
          "The rm utility attempts to remove the non-directory type "
          "files "
          "specified on the command line.\n";
    } else if (strcmp(args[1], "chmod") == 0) {
      output =
          "The chmod utility modifies the file mode bits of the "
          "listed "
          "files as specified by the mode operand.\n";
    } else if (strcmp(args[1], "ps") == 0) {
      output =
          "The ps utility displays a header line, followed by lines "
          "containing information about all of your processes that have "
          "controlling terminals.\n";
    } else if (strcmp(args[1], "kill") == 0) {
      output =
          "The kill utility sends a signal to the processes specified "
          "by "
          "the pid operands.\n";
    } else if (strcmp(args[1], "zombify") == 0) {
      output = "zombify\n\nCreates a processor that has a zombie child.\n";
    } else if (strcmp(args[1], "orphanify") == 0) {
      output = "Creates a processor killed that has a child running.\n";
    } else if (strcmp(args[1], "nice") == 0) {
      output = "The nice utility assigns priority to a specific thread\n";
    } else if (strcmp(args[1], "nice_pid") == 0) {
      output =
          "The nice utility assigns priority to a specific "
          "processor "
          "for a specified pid\n";
    } else if (strcmp(args[1], "man") == 0) {
      output =
          "The man utility finds and displays online manual "
          "documentation "
          "pages.\n";
    } else if (strcmp(args[1], "bg") == 0) {
      output =
          "The bg utility finds processors that are currently stopped "
          "and "
          "resumes the one with the highest job id in the background. bg can "
          "also find processors with specified pid.\n";
    } else if (strcmp(args[1], "fg") == 0) {
      output =
          "The fg utility finds processors in order of stopped and "
          "background list, then in the highest job id. Then it resumes the "
          "processor and gives it terminal control. fg can also find "
          "processors "
          "with specified pid\n";
    } else if (strcmp(args[1], "jobs") == 0) {
      output =
          "The jobs command lists processors that are currently "
          "stopped "
          "or in background.\n";
    } else if (strcmp(args[1], "logout") == 0) {
      output = "logout\n\nExits the shell and shutsdown PennOS..\n";
    } else {
      output = "No manual entry.\n";
    }
  }
  ssize_t result = s_write(STDOUT_FILENO, output, strlen(output));
  if (result == -1) {
    u_perror("Failed to write to STDOUT");
  }

  s_exit();
  return NULL;
}

void* b_nice(void* arg) {
  struct parsed_command* parsed = NULL;
  parse_command(arg, &parsed);
  char** args = parsed->commands[0];

  if (args[1] == NULL || args[2] == NULL) {
    /// error
    return NULL;
  }

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
  char* args[2] = {"orphan_child", NULL};
  s_spawn(b_orphan_child, args, STDIN_FILENO, STDOUT_FILENO);
  s_exit();
  return NULL;
}

void* b_zombie_child(void* arg) {
  // MMMMM Brains...!
  s_zombie(current->pid);
  return NULL;
}

void* b_zombify(void* arg) {
  char* args[2] = {"zombie_child", NULL};
  s_spawn(b_zombie_child, args, STDIN_FILENO, STDOUT_FILENO);
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
  s_ls(NULL, current->output_fd);

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
  s_write(current->output_fd, "\n", 1);

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
