#include "kernel.h"

pcb_t* k_proc_create(pcb_t* parent) {
  // allocate memory for child pcb
  pcb_t* child = (pcb_t*)malloc(sizeof(pcb_t));
  if (child == NULL) {
    return NULL;
  }
  child->pid = 0;             // code here to generate pids
  child->ppid = parent->pid;  // inherit
  child->priority = 1;        // default value
  child->state = STOPPED;
  child->child_pids = dynamic_pid_array_create(4);
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

  return parent;
}

void k_proc_cleanup(pcb_t* proc) {
  return;
}
