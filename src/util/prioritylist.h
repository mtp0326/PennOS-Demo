#ifndef PLIST_H
#define PLIST_H

#include <stdbool.h>

/**
 * @struct Node
 * @brief This structure represents a node in the circular linked list.
 */
typedef struct PNode {
  unsigned int
      priority : 2;   /**< Pointer to the process control block (pcb_t). */
  struct PNode* next; /**< Pointer to the next node in the list. */
} PNode;

/**
 * @struct CircularList
 * @brief This structure represents a circular linked list for managing
 * processes.
 */
typedef struct {
  PNode* head;       /**< Pointer to the head (first node) of the list. */
  unsigned int size; /**< Number of nodes in the list. */
} PList;

/**
 * Initializes a circular linked list.
 * @return CircularList* Pointer to the newly initialized list.
 */
PList* init_priority(void);

/**
 * Adds a new process to the circular linked list.
 * @param list Pointer to the circular linked list.
 * @param process Pointer to the process control block (pcb_t) to add.
 */
void add_priority(PList* list, unsigned int priority);

/**
 * Removes a process from the circular linked list by its PID.
 * @param list Pointer to the circular linked list.
 * @param pid PID of the process to remove.
 * @return bool true if the process was successfully removed, false otherwise.
 */
bool remove_priority(PList* list, unsigned int priority);

/**
 * Frees all nodes and their associated processes in a circular linked list,
 * then frees the list itself.
 * 
 * @param list Pointer to the circular linked list to free.
 */
void free_plist(PList* list);

#endif  // SCHEDULER_LIST_H
