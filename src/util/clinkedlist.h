#ifndef SCHEDULER_LIST_H
#define SCHEDULER_LIST_H

#include <stdbool.h>
#include <sys/types.h>

typedef struct pcb_t pcb_t;

/**
 * @struct Node
 * @brief This structure represents a node in the circular linked list.
 */
typedef struct Node {
  pcb_t* process;    /**< Pointer to the process control block (pcb_t). */
  struct Node* next; /**< Pointer to the next node in the list. */
} Node;

/**
 * @struct CircularList
 * @brief This structure represents a circular linked list for managing
 * processes.
 */
typedef struct {
  Node* head;        /**< Pointer to the head (first node) of the list. */
  Node* tail;        /**< Pointer to the tail (last node) of the list. */
  unsigned int size; /**< Number of nodes in the list. */
} CircularList;

/**
 * Initializes a circular linked list.
 * @return CircularList* Pointer to the newly initialized list.
 */
CircularList* init_list(void);

/**
 * Adds a new process to the circular linked list.
 * @param list Pointer to the circular linked list.
 * @param process Pointer to the process control block (pcb_t) to add.
 */
int add_process(CircularList* list, pcb_t* process);

/**
 * Adds a new process to the front of circular linked list.
 * @param list Pointer to the circular linked list.
 * @param process Pointer to the process control block (pcb_t) to add.
 */
void add_process_front(CircularList* list, pcb_t* process);

/**
 * Removes a process from the circular linked list by its PID.
 * @param list Pointer to the circular linked list.
 * @param pid PID of the process to remove.
 * @return bool true if the process was successfully removed, false otherwise.
 */
bool remove_process(CircularList* list, pid_t pid);

/**
 * Finds a process in the circular linked list by its PID.
 * @param list Pointer to the circular linked list.
 * @param pid PID of the process to find.
 * @return pcb_t* Pointer to the found process control block, or NULL if not
 * found.
 */
pcb_t* find_process(CircularList* list, pid_t pid);

/**
 * Finds a process in the circular linked list by its Job ID.
 * @param list Pointer to the circular linked list.
 * @param index Job Id specified by user.
 * @return pcb_t* Pointer to the found process control block, or NULL if not
 * found.
 */
pcb_t* find_process_job_id(CircularList* list, int index);

#endif  // SCHEDULER_LIST_H
