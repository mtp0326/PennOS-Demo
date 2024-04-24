#include "kernel.h"
#include "stdio.h"

pcb_t* k_proc_create(pcb_t* parent)
{
  // allocate memory for child pcb
  pcb_t* child = (pcb_t*)malloc(sizeof(pcb_t));
  if (child == NULL) {
    return NULL;
  }
  child->pid = next_pid++;   // code here to generate pids
  child->ppid = parent->pid; // inherit
  child->priority = 1;       // default value // AHHH CHANGE TO 1 after test
  child->state = RUNNING;
  child->initial_state = RUNNING;
  child->child_pids = dynamic_pid_array_create(4);
  child->statechanged = false;
  child->waiting_for_change = false;
  child->term_signal = -1;
  child->waiting_on_pid = 0;
  child->ticks_to_wait = 0;
  child->exit_status = -1;
  child->job_num = -1;
  child->is_bg = false;
  if (child->child_pids == NULL) {
    free(child);
    return NULL;
  }

  child->open_fds = (FD_Bitmap*)malloc(sizeof(FD_Bitmap));
  if (child->open_fds == NULL) {
    dynamic_pid_array_destroy(child->child_pids);
    free(child);
    return NULL;
  }
  fd_bitmap_initialize(child->open_fds);
  dynamic_pid_array_add(parent->child_pids, child->pid);

  return child;
}

void k_proc_cleanup(pcb_t* proc)
{
  pcb_t* parent = s_find_process(proc->ppid);
  if (parent != NULL) {
    dynamic_pid_array_remove(parent->child_pids, proc->pid);
  }

  dynamic_pid_array_destroy(proc->child_pids);
  free(proc->open_fds);
  free(proc->processname);

  if (proc->argv) {
    for (int i = 0; proc->argv[i] != NULL; i++) {
      free(proc->argv[i]);
    }
    free(proc->argv);
  }

  cancel_and_join(proc->handle);
  return;
}
