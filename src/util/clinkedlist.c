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
  list->size = 0;
  return list;
}

// Adds a new process to the circular linked list
void add_process(CircularList* list, pcb_t* process) {
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
  } else {
    // Insert the new node at the end of the list
    Node* current = list->head;
    while (current->next != list->head) {
      current = current->next;
    }
    current->next = newNode;
    newNode->next = list->head;  // Complete the circle
  }
  list->size++;
}

// Removes a process from the circular linked list by its PID
bool remove_process(CircularList* list, pid_t pid) {
  if (list == NULL || list->head == NULL) {
    return false;  // List is empty or not initialized
  }

  Node *current = list->head, *prev = NULL;
  do {
    if (current->process->pid == pid) {
      if (prev == NULL) {                   // Removing the head
        if (current->next == list->head) {  // Only one element
          list->head = NULL;
        } else {
          prev = list->head;
          while (prev->next != list->head)
            prev = prev->next;
          list->head = current->next;
          prev->next = list->head;
        }
      } else {  // Removing non-head
        prev->next = current->next;
        if (current == list->head) {
          list->head = current->next;
        }
      }
      free(current);
      list->size--;
      return true;
    }
    prev = current;
    current = current->next;
  } while (current != list->head);

  return false;  // Process not found
}

// Finds a process in the circular linked list by its PID
pcb_t* find_process(CircularList* list, pid_t pid) {
  if (list == NULL || list->head == NULL) {
    return NULL;  // List is empty or not initialized
  }

  Node* current = list->head;
  do {
    if (current->process->pid == pid) {
      return current->process;
    }
    current = current->next;
  } while (current != list->head);

  return NULL;  // Process not found
}
