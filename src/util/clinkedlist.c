#include "clinkedlist.h"
#include <stdlib.h>  // for malloc, free
#include "kernel.h"

// Initializes a circular linked list
CircularList* init_list(void) {
  CircularList* list = (CircularList*)malloc(sizeof(CircularList));
  if (list == NULL) {
    return NULL;  // Failed to allocate memory for the list
  }
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
  return list;
}

// Adds a new process to the circular linked list
int add_process(CircularList* list, pcb_t* process) {
  if (list == NULL || process == NULL) {
    return -1;  // Invalid parameters
  }

  Node* newNode = (Node*)malloc(sizeof(Node));
  if (newNode == NULL) {
    return -1;  // Failed to allocate memory for the new node
  }
  newNode->process = process;

  if (list->head == NULL) {
    // If the list is empty, initialize it with the new node
    newNode->next = newNode;  // Points to itself, making it circular
    list->head = newNode;
  } else {
    // Insert the new node at the end of the list
    Node* currentnode = list->head;
    while (currentnode->next != list->head) {
      currentnode = currentnode->next;
    }
    currentnode->next = newNode;
    newNode->next = list->head;  // Complete the circle
  }
  list->tail = newNode;  // the recently added newNode will be the last(tail)
  list->size++;
  return 0;
}

// Adds a new process to the circular linked list
void add_process_front(CircularList* list, pcb_t* process) {
  if (list == NULL || process == NULL) {
    return;  // Invalid parameters
  }

  Node* newNode = (Node*)malloc(sizeof(Node));
  if (newNode == NULL) {
    return;  // Failed to allocate memory for the new node
  }
  newNode->process = process;

  if (list->head == NULL) {
    // If the list is empty, initialize it with the new node
    newNode->next = newNode;  // Points to itself, making it circular
    list->head = newNode;
    list->tail = newNode;
  } else {
    // Insert the new node at the end of the list
    Node* currentnode = list->head;
    while (currentnode->next != list->head) {
      currentnode = currentnode->next;
    }
    list->tail = currentnode;  // the node before the head becomes the new tail
    currentnode->next = newNode;
    newNode->next = list->head;  // Complete the circle
    list->head = newNode;        // move head to the last node
  }
  list->size++;
  return;
}

// Removes a process from the circular linked list by its PID
bool remove_process(CircularList* list, pid_t pid) {
  if (list == NULL || list->head == NULL) {
    return false;  // List is empty or not initialized
  }

  Node *currentnode = list->head, *prev = NULL;
  do {
    if (currentnode->process->pid == pid) {
      if (prev == NULL) {                       // Removing the head
        if (currentnode->next == list->head) {  // Only one element
          list->head = NULL;
          list->tail = NULL;
        } else {
          prev = list->head;
          while (prev->next != list->head)
            prev = prev->next;
          list->head = currentnode->next;
          prev->next = list->head;
        }
      } else if (currentnode ==
                 list->tail) {  // Removing the tail ///not sure this works
        prev->next = list->head;
        list->tail = prev;
      } else {  // Removing non-head and non-tail
        prev->next = currentnode->next;
        if (currentnode == list->head) {
          list->head = currentnode->next;
        }
      }
      free(currentnode);
      list->size--;
      return true;
    }
    prev = currentnode;
    currentnode = currentnode->next;
  } while (currentnode != list->head);

  return false;  // Process not found
}

// Finds a process in the circular linked list by its PID
pcb_t* find_process(CircularList* list, pid_t pid) {
  if (list == NULL || list->head == NULL) {
    return NULL;  // List is empty or not initialized
  }

  Node* currentnode = list->head;
  do {
    if (currentnode->process->pid == pid) {
      return currentnode->process;
    }
    currentnode = currentnode->next;
  } while (currentnode != list->head);

  return NULL;  // Process not found
}

// Finds a process in the circular linked list by its PID
pcb_t* find_process_job_id(CircularList* list, u_int64_t index) {
  if (list == NULL || list->head == NULL) {
    return NULL;  // List is empty or not initialized
  }

  Node* currentnode = list->head;
  do {
    if (currentnode->process->job_num == index) {
      return currentnode->process;
    }
    currentnode = currentnode->next;
  } while (currentnode != list->head);

  return NULL;  // Process not found
}