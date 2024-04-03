#include "pennos.h"
#include "util/array.h"
#include "util/clinkedlist.h"
#include "util/kernel.h"
#include "util/prioritylist.h"

static pthread_mutex_t done_lock;

static bool done = false;

static const int centisecond = 10000;  // 10 milliseconds

CircularList* priorityzero;
CircularList* priorityone;
CircularList* prioritytwo;

pcb_t* current;

PList* priority;

static void* inc(void* arg) {
  int thread_num = *(int*)arg;
  free(arg);
  for (int i = 0;; i++) {
    dprintf(STDERR_FILENO, "%*cThread %d: i = %d\n", thread_num * 20, ' ',
            thread_num, i);
    usleep(centisecond);
  }
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
    if (priority->head->priority == 0) {
      priorityzero->head = priorityzero->head->next;
    } else if (priority->head->priority == 1) {
      priorityone->head = priorityone->head->next;
    } else {
      prioritytwo->head = prioritytwo->head->next;
    }
    priority->head = priority->head->next;

    if (priority->head->priority == 0) {
      current = priorityzero->head->process;
      curr_thread = priorityzero->head->process->handle;
    } else if (priority->head->priority == 1) {
      current = priorityone->head->process;
      curr_thread = priorityone->head->process->handle;
    } else {
      current = prioritytwo->head->process;
      curr_thread = prioritytwo->head->process->handle;
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

int main(int argc, char** argv) {
  // create the circular linked lists
  priorityzero = init_list();
  priorityone = init_list();
  prioritytwo = init_list();
  priority = init_priority();
  pthread_mutex_init(&done_lock, NULL);
  spthread_t temp;

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

  for (int i = 0; i < 2; i++) {
    int* arg = malloc(sizeof(int));
    *arg = 0;
    spthread_create(&temp, NULL, inc, arg);
    pcb_t* ahh = malloc(sizeof(pcb_t));
    ahh->handle = temp;
    add_process(priorityzero, ahh);
  }
  for (int i = 2; i < 4; i++) {
    int* arg = malloc(sizeof(int));
    *arg = 1;
    spthread_create(&temp, NULL, inc, arg);
    pcb_t* ahh = malloc(sizeof(pcb_t));
    ahh->handle = temp;
    add_process(priorityone, ahh);
  }
  for (int i = 4; i < 6; i++) {
    int* arg = malloc(sizeof(int));
    *arg = 2;
    spthread_create(&temp, NULL, inc, arg);
    pcb_t* ahh = malloc(sizeof(pcb_t));
    ahh->handle = temp;
    add_process(prioritytwo, ahh);
  }

  scheduler();

  // cleanup
  while (priorityzero->size != 0) {
    cancel_and_join(priorityzero->head->process->handle);
    remove_process(priorityzero, priorityzero->head->process->pid);
  }

  pthread_mutex_destroy(&done_lock);

  return EXIT_SUCCESS;
}
