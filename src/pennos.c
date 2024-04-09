#include "pennos.h"
#include <signal.h>
#include "fcntl.h"
#include "parser.h"
#include "pennfat.h"
#include "unistd.h"
#include "util/kernel.h"
#include "util/prioritylist.h"

static pthread_mutex_t done_lock;

static const int centisecond = 10000;  // 10 milliseconds

PList* priority;

// will be used for nice later maybe
// void* function_from_string(char* program) {
//   if (strcmp(program, "cat") == 0) {
//     return cat;
//   } else if (strcmp(program, "sleep") == 0) {
//     return sleep;
//   } else if (strcmp(program, "busy") == 0) {
//     return busy;
//   } else if (strcmp(program, "echo") == 0) {
//     return echo;
//   } else if (strcmp(program, "ls") == 0) {
//     return ls;
//   } else if (strcmp(program, "touch") == 0) {
//     return touch;
//   } else if (strcmp(program, "mv") == 0) {
//     return mv;
//   } else if (strcmp(program, "cp") == 0) {
//     return cp;
//   } else if (strcmp(program, "rm") == 0) {
//     return rm;
//   } else if (strcmp(program, "chmod") == 0) {
//     return chmod;
//   } else if (strcmp(program, "ps") == 0) {
//     return ps;
//   } else if (strcmp(program, "kill") == 0) {
//     return kill;
//   } else if (strcmp(program, "zombify") == 0) {
//     return zombify;
//   } else if (strcmp(program, "orphanify") == 0) {
//     return orphanify;
//   } else {
//     return NULL;
//   }
// }

static void* shell(void* arg) {
  while (1) {
    prompt();
    char* cmd;
    read_command(&cmd);
    if (cmd[0] != '\n') {
      // parse command
      struct parsed_command* parsed;
      if (parse_command(cmd, &parsed) != 0) {
        free(parsed);
        exit(EXIT_FAILURE);
      }

      char** args = parsed->commands[0];
      // Unfortunate canonical way to switch based on string in C.
      // Shell built-ins that are implemented using user or system calls only.
      if (strcmp(args[0], "cat") == 0) {
        // TODO: Call your implemented cat() function
      } else if (strcmp(args[0], "sleep") == 0) {
        // TODO: Call your implemented sleep() function
      } else if (strcmp(args[0], "busy") == 0) {
        // TODO: Call your implemented busy() function
      } else if (strcmp(args[0], "echo") == 0) {
        // TODO: Call your implemented echo() function
      } else if (strcmp(args[0], "ls") == 0) {
        // TODO: Call your implemented ls() function
      } else if (strcmp(args[0], "touch") == 0) {
        // TODO: Call your implemented touch() function
      } else if (strcmp(args[0], "mv") == 0) {
        // TODO: Call your implemented mv() function
      } else if (strcmp(args[0], "cp") == 0) {
        // TODO: Call your implemented cp() function
      } else if (strcmp(args[0], "rm") == 0) {
        // TODO: Call your implemented rm() function
      } else if (strcmp(args[0], "chmod") == 0) {
        // TODO: Call your implemented chmod() function
      } else if (strcmp(args[0], "ps") == 0) {
        // TODO: Call your implemented ps() function
      } else if (strcmp(args[0], "kill") == 0) {
        // TODO: Call your implemented kill() function
      } else if (strcmp(args[0], "zombify") == 0) {
        // TODO: Call your implemented zombify() function
      } else if (strcmp(args[0], "orphanify") == 0) {
        // TODO: Call your implemented orphanify() function
      } else if (strcmp(args[0], "nice") == 0) {
        // TODO;
      } else if (strcmp(args[0], "nice_pid") == 0) {
        // TODO: Call your implemented nice_pid() function
      } else if (strcmp(args[0], "man") == 0) {
        // TODO: Call your implemented man() function
      } else if (strcmp(args[0], "bg") == 0) {
        // TODO: Call your implemented bg() function
      } else if (strcmp(args[0], "fg") == 0) {
        // TODO: Call your implemented fg() function
      } else if (strcmp(args[0], "jobs") == 0) {
        // TODO: Call your implemented jobs() function
      } else if (strcmp(args[0], "logout") == 0) {
        b_logout(NULL);
      } else {
        fprintf(stderr, "pennos: command not found: %s\n", args[0]);
      }
      free(parsed);
    }
    free(cmd);
  }
  return EXIT_SUCCESS;
}

static void alarm_handler(int signum) {}

void scheduler(char* logfile) {
  unsigned int tick = 0;

  char buf[100];

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

  pthread_mutex_lock(&done_lock);
  while (!done) {
    pthread_mutex_unlock(&done_lock);
    // need to add logic in case no processes of a given priority level for
    // future
    int allBlockedOrEmpty = 1;

    // Check if all queues are empty or all processes are blocked.
    for (int i = 0; i < 3; i++) {
      if (processes[i]->size > 0) {
        allBlockedOrEmpty = 0;
        break;
      }
    }

    if (allBlockedOrEmpty) {
      // All processes are blocked or queues are empty, so idle.
      // sigsuspend will atomically unblock signals and put the process to
      // sleep.
      sigsuspend(&idle_set);
    } else {
      while (processes[priority->head->priority]->size == 0) {
        priority->head = priority->head->next;
      }

      CircularList* current_priority = processes[priority->head->priority];
      current = current_priority->head->process;
      curr_thread = current->handle;

      spthread_continue(curr_thread);
      sigsuspend(&suspend_set);
      spthread_suspend(curr_thread);
      tick++;
      sprintf(buf, "[%4u]\t CREATE\n", tick);
      write(file, buf, strlen(buf));
      current_priority->head = current_priority->head->next;
      pthread_mutex_lock(&done_lock);
    }
  }
  close(file);
  pthread_mutex_unlock(&done_lock);
}

void cancel_and_join(spthread_t thread) {
  spthread_cancel(thread);
  spthread_continue(thread);
  spthread_join(thread, NULL);
}

#include "pennfat.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "pennos: filesystem not specified");
    return -1;
  } else if (argc > 3) {
    fprintf(stderr, "pennos: too many arguments");
    return -1;
  }

  char* log = "log.txt";
  if (argc == 3) {
    log = argv[2];
  }

  processes[0] = init_list();
  processes[1] = init_list();
  processes[2] = init_list();
  // create the circular linked lists

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

  pcb_t* place = malloc(sizeof(pcb_t));
  place->priority = 0;
  place->pid = 0;
  place->child_pids = dynamic_pid_array_create(4);
  current = place;
  char** arg = malloc(2 * sizeof(char*));  // Space for 3 pointers
  arg[0] = strdup("shell");  // strdup allocates new memory for the string
  arg[1] = NULL;             // Terminate the array

  s_spawn(shell, arg, STDIN_FILENO, STDOUT_FILENO);

  scheduler(log);

  // cleanup
  while (processes[0]->size != 0) {
    cancel_and_join(processes[0]->head->process->handle);
    remove_process(processes[0], processes[0]->head->process->pid);
  }

  pthread_mutex_destroy(&done_lock);

  return EXIT_SUCCESS;
}
