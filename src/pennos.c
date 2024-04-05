#include "pennos.h"
#include "pennfat.h"
#include "util/kernel.h"
#include "util/prioritylist.h"

static pthread_mutex_t done_lock;

static bool done = false;

static const int centisecond = 10000;  // 10 milliseconds

PList* priority;

static void* shell(void* arg) {
  while (1) {
    prompt();
    char* cmd;
    read_command(&cmd);
    // s_spawn(cmd, &cmd, STDIN_FILENO, STDOUT_FILENO);
  }
  return NULL;
}

static void alarm_handler(int signum) {}

void scheduler(void) {
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

      current_priority->head = current_priority->head->next;
      pthread_mutex_lock(&done_lock);
    }
  }
  pthread_mutex_unlock(&done_lock);
}

void cancel_and_join(spthread_t thread) {
  spthread_cancel(thread);
  spthread_continue(thread);
  spthread_join(thread, NULL);
}

#include "pennfat.h"

int main(int argc, char** argv) {
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

  scheduler();

  // cleanup
  while (processes[0]->size != 0) {
    cancel_and_join(processes[0]->head->process->handle);
    remove_process(processes[0], processes[0]->head->process->pid);
  }

  pthread_mutex_destroy(&done_lock);

  return EXIT_SUCCESS;
}
