#include "pennos.h"
#include <signal.h>
#include "fcntl.h"
#include "parser.h"
#include "pennfat.h"
#include "sys/time.h"
#include "unistd.h"
#include "util/kernel.h"
#include "util/prioritylist.h"

static const int centisecond = 10000;  // 10 milliseconds

PList* priority;

static void* shell(void* arg) {
  while (1) {
    prompt(true);
    char* cmd;

    read_command(&cmd);

    b_background_poll(NULL);

    if (cmd[0] != '\n') {
      // parse command
      struct parsed_command* parsed;
      if (parse_command(cmd, &parsed) != 0) {
        free(parsed);

        exit(EXIT_FAILURE);
      }

      char** args = parsed->commands[0];
      // Shell built-ins that are implemented using user or system calls only.
      if (strcmp(args[0], "cat") == 0) {
        // this is when files arg was NOT provided
        int input_fd = -1;
        int output_fd = -1;
        // create the correct input
        if (parsed->stdin_file == NULL) {
          input_fd = STDIN_FILENO;
        } else {
          input_fd = s_open(parsed->stdin_file, F_READ);
        }

        if (input_fd == -1) {
          perror("No such file or directory\n");
          continue;
        }

        if (parsed->stdout_file == NULL) {
          output_fd = STDOUT_FILENO;
        } else {
          if (parsed->is_file_append) {
            output_fd = s_open(parsed->stdout_file, F_APPEND);
          } else {
            output_fd = s_open(parsed->stdout_file, F_WRITE);
          }
        }

        s_spawn_and_wait(b_cat, args, input_fd, output_fd,
                         parsed->is_background, -1);

        if (input_fd != STDIN_FILENO) {
          s_close(input_fd);
        }

        if (output_fd != STDOUT_FILENO) {
          s_close(output_fd);
        }

      } else if (strcmp(args[0], "sleep") == 0) {
        s_spawn_and_wait(b_sleep, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "busy") == 0) {
        b_busy(NULL);
      } else if (strcmp(args[0], "echo") == 0) {
        // echo should ignore any input redirection
        // but it should write to the redirected output file
        if (parsed->stdout_file == NULL) {
          // we want to print to stdout
          s_spawn_and_wait(b_echo, args, STDIN_FILENO, STDOUT_FILENO,
                           parsed->is_background, -1);
        } else {
          if (parsed->is_file_append) {
            int fd = s_open(parsed->stdout_file, F_APPEND);
            s_spawn_and_wait(b_echo, args, STDIN_FILENO, fd,
                             parsed->is_background, -1);
            s_close(fd);
          } else {
            int fd = s_open(parsed->stdout_file, F_WRITE);
            s_spawn_and_wait(b_echo, args, STDIN_FILENO, fd,
                             parsed->is_background, -1);
            s_close(fd);
          }
        }

      } else if (strcmp(args[0], "ls") == 0) {
        s_spawn_and_wait(b_ls, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "touch") == 0) {
        s_spawn_and_wait(b_touch, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "mv") == 0) {
        s_spawn_and_wait(b_mv, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "cp") == 0) {
        s_spawn_and_wait(b_cp, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "rm") == 0) {
        s_spawn_and_wait(b_rm, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "chmod") == 0) {
        s_spawn_and_wait(b_chmod, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "ps") == 0) {
        s_spawn_and_wait(b_ps, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "kill") == 0) {
        s_spawn_and_wait(b_kill, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "zombify") == 0) {
        // TODO: Call your implemented zombify() function
      } else if (strcmp(args[0], "orphanify") == 0) {
        // TODO: Call your implemented orphanify() function
      } else if (strcmp(args[0], "nice") == 0) {
        b_nice(cmd);
      } else if (strcmp(args[0], "nice_pid") == 0) {
        b_nice_pid(args);
      } else if (strcmp(args[0], "man") == 0) {
        s_spawn_and_wait(b_man, args, STDIN_FILENO, STDOUT_FILENO,
                         parsed->is_background, -1);
      } else if (strcmp(args[0], "bg") == 0) {
        b_bg(args);
      } else if (strcmp(args[0], "fg") == 0) {
        b_fg(args);
      } else if (strcmp(args[0], "jobs") == 0) {
        b_jobs(NULL);
      } else if (strcmp(args[0], "logout") == 0) {
        b_logout(NULL);
        break;
      } else if (strcmp(args[0], "clear") == 0) {
        b_clear(NULL);
      } else {
        fprintf(stderr, "pennos: command not found: %s\n", args[0]);
        // REPLACE WITH PERROR
      }
      free(parsed);
    }
    free(cmd);
  }

  return EXIT_SUCCESS;
}

static void alarm_handler(int signum) {
  if (signum == SIGINT) {
    char* newline = "\n";
    s_write(STDOUT_FILENO, newline, strlen(newline));
    s_exit();
  } else if (signum == SIGTSTP) {
    s_kill(current->pid, P_SIGSTOP);
  }
}

void scheduler(char* logfile) {
  sigset_t suspend_set;
  sigfillset(&suspend_set);
  sigdelset(&suspend_set, SIGALRM);

  sigset_t idle_set;
  sigemptyset(&idle_set);

  // just to make sure that
  // sigalrm doesn't terminate the process
  struct sigaction act = (struct sigaction){
      .sa_handler = alarm_handler,
      .sa_mask = suspend_set,
      .sa_flags = SA_RESTART,
  };
  sigaction(SIGALRM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTSTP, &act, NULL);

  // make sure SIGALRM is unblocked
  sigset_t alarm_set;
  sigemptyset(&alarm_set);
  sigaddset(&alarm_set, SIGALRM);
  pthread_sigmask(SIG_UNBLOCK, &alarm_set, NULL);

  struct itimerval it;
  it.it_interval = (struct timeval){.tv_usec = centisecond * 10};
  it.it_value = it.it_interval;
  setitimer(ITIMER_REAL, &it, NULL);

  spthread_t curr_thread;
  // locks to check the global value done
  int file = open(logfile, O_CREAT | O_TRUNC | O_RDWR, 0666);
  logfiledescriptor = file;
  pthread_mutex_lock(&done_lock);

  pcb_t* place = malloc(sizeof(pcb_t));
  place->priority = 0;
  place->pid = 0;
  place->child_pids = dynamic_pid_array_create(4);
  current = place;
  char** arg = malloc(2 * sizeof(char*));  // Space for 3 pointers
  arg[0] = strdup("shell");  // strdup allocates new memory for the string
  arg[1] = NULL;             // Terminate the array

  // spawn in the shell process at priority 0
  s_spawn_nice(shell, arg, STDIN_FILENO, STDOUT_FILENO, 0);

  // main loop
  while (!done) {
    // unlock done lock so that other processes can lock it if they are not
    // reenttrant or are executing a non-reentrant part of code
    pthread_mutex_unlock(&done_lock);

    // iterates over all blocked processes
    for (unsigned int i = 0; i < blocked->size; i++) {
      // fprintf(stdout, "num blocked: %d\n", blocked->size);
      pcb_t* block = blocked->head->process;
      // checks if process is executing s_sleep, and decrements ticks to wait
      if (block->ticks_to_wait > 0) {
        // if last decrement, move process to zombied.
        if (block->ticks_to_wait == 1) {
          block->state = ZOMBIED;
          block->statechanged = true;
          remove_process(blocked, block->pid);
          add_process(zombied, block);
          s_write_log(ZOMBIE, block, -1);
        }

        block->ticks_to_wait--;
        if (blocked->head != NULL) {
          blocked->head = blocked->head->next;
        }
        continue;
      }
      pcb_t* child_pcb;
      if (block->waiting_on_pid !=
          -1) {  // if this process is currently s_waitpiding on a child
        if (find_process(processes[0], block->waiting_on_pid) != NULL) {
          child_pcb = find_process(processes[0], block->waiting_on_pid);
        } else if (find_process(processes[1], block->waiting_on_pid) != NULL) {
          child_pcb = find_process(processes[1], block->waiting_on_pid);
        } else if (find_process(processes[2], block->waiting_on_pid) != NULL) {
          child_pcb = find_process(processes[2], block->waiting_on_pid);
        } else if (find_process(blocked, block->waiting_on_pid) != NULL) {
          child_pcb = find_process(blocked, block->waiting_on_pid);
        } else if (find_process(stopped, block->waiting_on_pid) != NULL) {
          child_pcb = find_process(stopped, block->waiting_on_pid);
        } else if (find_process(zombied, block->waiting_on_pid) != NULL) {
          child_pcb = find_process(zombied, block->waiting_on_pid);
        } else {
          blocked->head = blocked->head->next;
          continue;
        }
      }

      if (child_pcb->statechanged) {
        block->state = RUNNING;
        remove_process(blocked, block->pid);
        add_process(processes[block->priority], block);

        s_write_log(UNBLOCK, block, -1);
      }
      if (blocked->size != 0) {
        blocked->head = blocked->head->next;
      } else {
        blocked->head = NULL;
      }
    }

    bool noRunningProcesses = true;
    // Check if all queues are empty or all processes are blocked.
    for (int i = 0; i < 3; i++) {
      if (processes[i]->size > 0) {
        noRunningProcesses = false;
        break;
      }
    }
    if (noRunningProcesses) {  // remove and false later, debugging AHHHHHHH
      // All processes are blocked or queues are empty, so idle.
      // sigsuspend will atomically unblock signals and put the process to
      // sleep.
      sigsuspend(&suspend_set);

    } else {
      if (((processes[0]->size + processes[1]->size + processes[2]->size) -
           processes[priority->head->priority]->size) != 0) {
        priority->head = priority->head->next;
        while (processes[priority->head->priority]->size == 0) {
          priority->head = priority->head->next;
        }
      }
      CircularList* current_priority = processes[priority->head->priority];
      current = current_priority->head->process;
      curr_thread = current->handle;

      // LOGGING OF SCHEDULE // make into helper later
      s_write_log(SCHEDULE, current, -1);

      //
      spthread_continue(curr_thread);
      sigsuspend(&suspend_set);
      spthread_suspend(curr_thread);
      tick++;

      // move head to next
      if (current_priority->size != 0) {
        current_priority->head = current_priority->head->next;
      }
      pthread_mutex_lock(&done_lock);
    }
  }
  close(file);
  pthread_mutex_unlock(&done_lock);
  return;
}

void cancel_and_join(spthread_t thread) {
  spthread_cancel(thread);
  spthread_continue(thread);
  spthread_join(thread, NULL);
}

#include "pennfat.h"

int main(int argc, char** argv) {
  errno = -1;
  if (argc < 2) {
    fprintf(stderr, "pennos: filesystem not specified");  // REPLACE WITH PERROR
    return -1;
  } else if (argc > 3) {
    fprintf(stderr, "pennos: too many arguments");  // REPLACE WITH PERROR
    return -1;
  }

  char* log = "log/log.txt";
  if (argc == 3) {
    log = argv[2];
  }

  // mount the file system
  if (mount(argv[1]) == -1) {
    perror("Mount error");
    return -1;
  }

  // create the global file descriptor table
  initialize_global_fd_table();

  processes[0] = init_list();
  processes[1] = init_list();
  processes[2] = init_list();
  // create the circular linked lists

  stopped = init_list();
  blocked = init_list();
  zombied = init_list();

  bg_list = init_list();

  priority = init_priority();
  pthread_mutex_init(&done_lock, NULL);
  // spthread_t temp;

  // 0201 0210 1020 1010 210
  add_priority(priority, 0);
  add_priority(priority, 2);
  add_priority(priority, 0);
  add_priority(priority, 1);
  add_priority(priority, 0);
  add_priority(priority, 2);
  add_priority(priority, 1);
  add_priority(priority, 0);
  add_priority(priority, 1);
  add_priority(priority, 0);
  add_priority(priority, 2);
  add_priority(priority, 0);
  add_priority(priority, 1);
  add_priority(priority, 0);
  add_priority(priority, 1);
  add_priority(priority, 0);
  add_priority(priority, 2);
  add_priority(priority, 1);
  add_priority(priority, 0);

  scheduler(log);
  fprintf(stderr, "AHH");
  pthread_mutex_destroy(&done_lock);

  return EXIT_SUCCESS;
}
