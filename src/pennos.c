#include "pennos.h"
#include "util/kernel.h"
#include "util/prioritylist.h"

static pthread_mutex_t done_lock;

static bool done = false;

static const int centisecond = 10000;  // 10 milliseconds

PList* priority;

static void* inc(void* arg) {
  char** argv = (char**)arg;

  // Assuming the second argument is what you want
  int thread_num = atoi(argv[1]);

  for (int i = 0;; i++) {
    dprintf(STDERR_FILENO, "%*cThread %d: i = %d\n", thread_num * 20, ' ',
            thread_num, i);
    usleep(centisecond);
  }

  // Free the arguments here as before
  for (int i = 0; argv[i] != NULL; i++) {
    free(argv[i]);
  }
  free(argv);

  return NULL;
}

static void alarm_handler(int signum) {}

void scheduler(void) {
  sigset_t suspend_set;
  sigfillset(&suspend_set);
  sigdelset(&suspend_set, SIGALRM);

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
    processes[priority->head->priority]->head =
        processes[priority->head->priority]->head->next;
    priority->head = priority->head->next;

    if (priority->head->priority == 0) {
      current = processes[0]->head->process;
      curr_thread = processes[0]->head->process->handle;
    } else if (priority->head->priority == 1) {
      current = processes[1]->head->process;
      curr_thread = processes[1]->head->process->handle;
    } else {
      current = processes[2]->head->process;
      curr_thread = processes[2]->head->process->handle;
    }
    spthread_continue(curr_thread);
    sigsuspend(&suspend_set);
    spthread_suspend(curr_thread);
    pthread_mutex_lock(&done_lock);
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

  for (int i = 0; i < 2; i++) {
    char** argv = malloc(3 * sizeof(char*));  // Space for 3 pointers
    argv[0] = strdup("inc");  // strdup allocates new memory for the string
    argv[1] = malloc(2);  // Enough for a single digit and the null terminator
    sprintf(argv[1], "%d", 3);  // Set thread number, divides by 2 to mimic
                                // your original grouping
    argv[2] = NULL;             // Terminate the array
    s_spawn(inc, argv, STDIN_FILENO, STDOUT_FILENO);
  }
  for (int i = 0; i < 6; i++) {
    // Dynamically allocate argv for each thread
    char** argv = malloc(3 * sizeof(char*));  // Space for 3 pointers
    argv[0] = strdup("inc");  // strdup allocates new memory for the string
    argv[1] = malloc(2);  // Enough for a single digit and the null terminator
    sprintf(argv[1], "%d", i / 2);  // Set thread number, divides by 2 to mimic
                                    // your original grouping
    argv[2] = NULL;                 // Terminate the array

    s_spawn(inc, argv, STDIN_FILENO, STDOUT_FILENO);
  }

  scheduler();

  // cleanup
  while (processes[0]->size != 0) {
    cancel_and_join(processes[0]->head->process->handle);
    remove_process(processes[0], processes[0]->head->process->pid);
  }

  pthread_mutex_destroy(&done_lock);

  return EXIT_SUCCESS;
}
